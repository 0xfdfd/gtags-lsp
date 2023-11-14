pub mod definition;
pub mod initialize;
pub mod initialized;
pub mod references;

use tokio::io::AsyncReadExt;
use tower_lsp::lsp_types::*;

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
    let path = path.path();
    let line_no = pos.line as usize;
    let column_no = pos.character as usize;

    let lines = get_file_as_lines(path).await?;
    let line = &lines[line_no];

    let re = match regex::Regex::new("[_0-9a-zA-Z]+") {
        Ok(v) => v,
        Err(e) => {
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("Compile regex failed"),
                data: Some(serde_json::json!({
                    "error": e.to_string(),
                })),
            })
        }
    };
    let mut word = String::new();

    for mat in re.find_iter(line.as_str()) {
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

/// Find symbol in specific file line.
///
/// # Arguments
///
/// + `path`: File path.
/// + `line_no`: Line number, starting from 0.
/// + `symbol`: The symbol to find.
pub async fn find_symbol_in_line(
    path: &Url,
    line_no: u32,
    symbol: &str,
) -> Result<Location, tower_lsp::jsonrpc::Error> {
    let line_no = line_no as usize;

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
    cwd: &str,
    args: &Vec<&str>,
) -> Result<Vec<String>, tower_lsp::jsonrpc::Error> {
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
/// let (symbol_name, line_number, file_path, rest_string) = parse_cxref(cxref)?;
/// ```
pub fn parse_cxref(data: &str) -> Result<(String, u32, String, String), tower_lsp::jsonrpc::Error> {
    let re = match regex::Regex::new(r"([^\s]+)\s+(\d+)\s+([^\s]+)\s+([^\s].*)") {
        Ok(v) => v,
        Err(e) => {
            tracing::error!("Compile regex failed.");
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("Compile regex failed"),
                data: Some(serde_json::json!({
                    "error": e.to_string(),
                })),
            });
        }
    };

    let caps = match re.captures(data) {
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

/// Read content of file and return it as lines.
///
/// # Arguments
///
/// + `path`: File path.
async fn get_file_as_lines(path: &str) -> Result<Vec<String>, tower_lsp::jsonrpc::Error> {
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
