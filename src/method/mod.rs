pub mod completion;
pub mod definition;
pub mod did_change;
pub mod did_close;
pub mod did_open;
pub mod document_symbol;
pub mod implementation;
pub mod initialize;
pub mod initialized;
pub mod references;
pub mod symbol;
pub mod type_definition;

use regex::Regex;
use tokio::io::AsyncReadExt;
use tower_lsp::lsp_types::*;

lazy_static::lazy_static! {

    /// Pattern for match `cxref` string.
    static ref RE_CXREF: Regex = Regex::new(r"([^\s]+)\s+(\d+)\s+([^\s]+)\s+([^\s].*)").unwrap();

    /// Pattern for matching symbol.
    static ref RE_SYMBOL: Regex = Regex::new(r"[_0-9a-zA-Z]+").unwrap();

    /// Pattern for matching C macro.
    static ref RE_MACRO: Regex = Regex::new(r"#\s*define").unwrap();
}

/// Find workspace folder that contains this file.
///
/// # Arguments
///
/// + `workspace_folders`: Workspace folder list.
/// + `uri`: File uri
pub fn find_belong_workspace_folder(
    workspace_folders: &Vec<WorkspaceFolder>,
    uri: &Url,
) -> Result<WorkspaceFolder, tower_lsp::jsonrpc::Error> {
    for folder in workspace_folders {
        let workspace_folder_path = folder.uri.path();
        let file_path = uri.path();

        if file_path.starts_with(workspace_folder_path) {
            return Ok(folder.clone());
        }
    }

    return Err(tower_lsp::jsonrpc::Error {
        code: tower_lsp::jsonrpc::ErrorCode::InvalidParams,
        message: std::borrow::Cow::Borrowed("cannot find matching workspace folder"),
        data: Some(serde_json::json!({
            "workspace_folders": workspace_folders,
            "uri": uri.to_string(),
        })),
    });
}

pub fn get_symbol_by_pos_from_dat(
    data: &String,
    pos: &Position,
) -> Result<String, tower_lsp::jsonrpc::Error> {
    let line_no = pos.line as usize;
    let column_no = pos.character as usize;

    let lines = split_string_by_lines(data);
    let line = &lines[line_no];

    let mut word = String::new();
    for mat in RE_SYMBOL.find_iter(line.as_str()) {
        if mat.start() <= column_no && mat.end() >= column_no {
            word = mat.as_str().to_string();
            break;
        }
    }

    if word.is_empty() {
        return Err(tower_lsp::jsonrpc::Error {
            code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                crate::LspErrorCode::RequestFailed.code(),
            ),
            message: std::borrow::Cow::Borrowed("No word found at the specified position"),
            data: None,
        });
    }

    return Ok(word);
}

/// Get symbol from file by position.
///
/// # Arguments
///
/// + `path`: File path.
/// + `pos`: Symbol position.
pub async fn get_symbol_by_position(
    path: &Url,
    pos: &Position,
) -> Result<String, tower_lsp::jsonrpc::Error> {
    let content = read_file_content(path.path()).await?;
    return get_symbol_by_pos_from_dat(&content, pos);
}

/// Find symbol in specific file line.
///
/// # Arguments
///
/// + `path`: File path.
/// + `line_no`: Line number, starting from 0.
/// + `symbol`: The symbol to find.
/// + `low_precision`: Do not find precise position, just point to begin of line.
pub async fn find_symbol_in_line(
    path: &Url,
    line_no: u32,
    symbol: &str,
    low_precision: bool,
) -> Result<Location, tower_lsp::jsonrpc::Error> {
    let line_no = line_no as usize;

    if low_precision {
        let start = Position::new(line_no as u32, 0);
        let end = Position::new(line_no as u32, 0);
        let range = Range::new(start, end);
        return Ok(Location::new(path.clone(), range));
    }

    let lines = match get_file_as_lines(path.path()).await {
        Ok(v) => v,
        Err(e) => return Err(e),
    };

    let line = &lines[line_no];

    let start_off = match line.find(symbol) {
        Some(pos) => Position::new(line_no as u32, pos as u32),
        None => {
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("symbol not found in line"),
                data: None,
            })
        }
    };

    let end_off = start_off.character as usize + symbol.len();
    let end_off = Position::new(line_no as u32, end_off as u32);
    let ret_range = Range::new(start_off, end_off);

    return Ok(Location::new(path.clone(), ret_range));
}

/// Execute command and get output as lines.
///
/// # Arguments
///
/// + `program`: The program to execute.
/// + `cwd`: The current working directory.
/// + `args`: Arguments list.
pub async fn execute_and_split_lines(
    program: &str,
    cwd: &Url,
    args: &Vec<&str>,
) -> Result<Vec<String>, tower_lsp::jsonrpc::Error> {
    let cwd = cwd.path();
    let output = tokio::process::Command::new(program)
        .current_dir(cwd)
        .args(args)
        .output();

    let output = match output.await {
        Ok(v) => v,
        Err(e) => {
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("execute `global` failed"),
                data: Some(serde_json::json!({
                    "error": e.to_string(),
                })),
            })
        }
    };

    let content = match String::from_utf8(output.stdout) {
        Ok(v) => v,
        Err(e) => {
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("execute `global` failed"),
                data: Some(serde_json::json!({
                    "error": e.to_string(),
                })),
            })
        }
    };

    return Ok(split_string_by_lines(&content));
}

/// Parser cxref string.
///
/// A `cxref` string looks like:
///
/// ```txt
/// main              10 src/main.c  main (argc, argv) {
/// ```
///
/// # Arguments
///
/// + `data`: The `cxref` string to parse.
///
/// # Examples
///
/// ```rust
/// let cxref = "main              10 src/main.c  main (argc, argv) {";
/// let (symbol, lineno, path, xref) = parse_cxref(cxref)?;
/// ```
pub fn parse_cxref(data: &str) -> Result<(String, u32, String, String), tower_lsp::jsonrpc::Error> {
    let caps = match RE_CXREF.captures(data) {
        Some(v) => v,
        None => {
            tracing::error!("Capture group failed. cxref:{}", data);
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("Capture group failed"),
                data: None,
            });
        }
    };

    let line_number: u32 = match caps[2].parse() {
        Ok(v) => v,
        Err(e) => {
            tracing::error!(
                "Not a valid u32. caps[1]:{}, caps[2]:{}, caps[3]:{}, caps[4]:{}",
                caps[1].to_string(),
                caps[2].to_string(),
                caps[3].to_string(),
                caps[4].to_string()
            );
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("Not a valid u32"),
                data: Some(serde_json::json!({
                    "error": e.to_string(),
                })),
            });
        }
    };

    return Ok((
        caps[1].to_string(),
        line_number,
        caps[3].to_string(),
        caps[4].to_string(),
    ));
}

/// Join workspace folder Url and file path.
///
/// # Arguments
///
/// + `cwd`: Workspace folder Url,
/// + `path`: File path in that folder.
pub fn join_workspace_path(cwd: &Url, path: &str) -> Result<Url, tower_lsp::jsonrpc::Error> {
    let path = format!("{}/{}", cwd.path(), path);

    let path = match Url::from_file_path(&path) {
        Ok(v) => v,
        Err(_) => {
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("Path is not absolute"),
                data: Some(serde_json::json! ({
                    "cwd": cwd.path(),
                    "path": path,
                })),
            })
        }
    };

    return Ok(path);
}

#[allow(deprecated)]
pub fn create_symbol_information(
    symbol_name: String,
    loc: Location,
    rest_string: &str,
) -> SymbolInformation {
    return SymbolInformation {
        name: symbol_name,
        kind: check_symbol_kind(&rest_string),
        tags: None,
        deprecated: None,
        location: loc,
        container_name: None,
    };
}

fn check_symbol_kind(context: &str) -> SymbolKind {
    match RE_MACRO.find(context) {
        Some(_) => return SymbolKind::NULL,
        None => return SymbolKind::FUNCTION,
    };
}

async fn read_file_content(path: &str) -> Result<String, tower_lsp::jsonrpc::Error> {
    // Open file.
    let mut file = match tokio::fs::File::open(path).await {
        Ok(v) => v,
        Err(e) => {
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("open file failed"),
                data: Some(serde_json::json!({
                    "error": e.to_string(),
                })),
            });
        }
    };

    // Read content.
    let mut content = String::new();
    match file.read_to_string(&mut content).await {
        Ok(_) => (),
        Err(e) => {
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("cannot read file"),
                data: Some(serde_json::json!({
                    "error": e.to_string(),
                })),
            })
        }
    }

    return Ok(content);
}

/// Read content of file and return it as lines.
///
/// # Arguments
///
/// + `path`: File path.
async fn get_file_as_lines(path: &str) -> Result<Vec<String>, tower_lsp::jsonrpc::Error> {
    let content = read_file_content(path).await?;
    return Ok(split_string_by_lines(&content));
}

/// Split string into lines, return the Vec of content copy.
///
/// # Arguments
///
/// + `content`: String content.
fn split_string_by_lines(content: &String) -> Vec<String> {
    let ret = content.lines().map(|line| line.to_string()).collect();
    return ret;
}
