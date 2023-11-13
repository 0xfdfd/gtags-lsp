use tokio::io::{AsyncBufReadExt, AsyncReadExt};
use tower_lsp::lsp_types::*;

pub async fn goto_definition(
    backend: &crate::TagsLspBackend,
    params: GotoDefinitionParams,
) -> tower_lsp::jsonrpc::Result<Option<GotoDefinitionResponse>> {
    let rt = backend.rt.lock().await;

    let file_uri = &params.text_document_position_params.text_document.uri;
    let file_position = &params.text_document_position_params.position;
    let cwd = find_matching_workspace_folder(&rt.workspace_folders, file_uri)?;

    let loc_list = searc_for_definition(&cwd.uri, file_uri, file_position).await?;
    return Ok(Some(GotoDefinitionResponse::Array(loc_list)));
}

/// Find workspace folder that contains this file.
///
/// # Arguments
///
/// + `workspace_folders`: Workspace folder list.
/// + `uri`: File uri
fn find_matching_workspace_folder(
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

/// Split string into lines, return the Vec of content copy.
///
/// # Arguments
///
/// + `content`: String content.
fn split_string_by_lines(content: &String) -> Vec<String> {
    let ret = content.lines().map(|line| line.to_string()).collect();
    return ret;
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

/// Get symbol from file position.
///
/// # Arguments
///
/// + `path`: File path.
/// + `line_no`: line number, starting from 0.
/// + `column_no`: column_no, starting from 0.
async fn get_symbol(
    path: &str,
    line_no: u32,
    column_no: u32,
) -> Result<String, tower_lsp::jsonrpc::Error> {
    let line_no = line_no as usize;
    let column_no = column_no as usize;

    let lines = match get_file_as_lines(path).await {
        Ok(v) => v,
        Err(e) => return Err(e),
    };
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
async fn find_symbol(
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

/// Search for symbol definition.
///
/// # Arguments
///
/// + `cwd`: Workspace folder
/// + `path`: File path
/// + `pos`: Symbol position
async fn searc_for_definition(
    cwd: &Url,
    file: &Url,
    pos: &Position,
) -> Result<Vec<Location>, tower_lsp::jsonrpc::Error> {
    // Get file path.
    let path = file.path();
    let cwd_path = cwd.path();
    // Location list for return.
    let mut loc_list = Vec::new();

    // Get symbol from position.
    let symbol = get_symbol(path, pos.line, pos.character).await?;

    // Prepare to execute global.
    let mut cmd = tokio::process::Command::new("global");
    cmd.current_dir(cwd_path)
        .arg("-dt")
        .arg(symbol.as_str())
        .stdout(std::process::Stdio::piped());

    // Execute command.
    let mut child = match cmd.spawn() {
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
            });
        }
    };

    // Take child stdout.
    let stdout = match child.stdout.take() {
        Some(v) => v,
        None => {
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("child did not have a handle to stdout"),
                data: None,
            })
        }
    };

    let mut reader = tokio::io::BufReader::new(stdout).lines();

    tokio::spawn(async move {
        let _status = child
            .wait()
            .await
            .expect("child process encountered an error");
    });

    loop {
        let line = match reader.next_line().await {
            Ok(v) => v,
            Err(_) => break,
        };

        let line = match line {
            Some(v) => v,
            None => break,
        };

        let words: Vec<&str> = line.split_whitespace().collect();
        let path = words[1];
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
                        "error": path,
                    })),
                })
            }
        };
        let line: u32 = words[2].parse().expect("Not a valid u32");

        let location = find_symbol(&path, line - 1, &symbol).await?;

        loc_list.push(location);
    }

    return Ok(loc_list);
}
