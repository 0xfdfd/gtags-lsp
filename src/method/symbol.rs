use regex::Regex;
use tower_lsp::lsp_types::*;

pub async fn do_symbol(
    backend: &crate::TagsLspBackend,
    params: WorkspaceSymbolParams,
) -> tower_lsp::jsonrpc::Result<Option<Vec<SymbolInformation>>> {
    let rt = backend.rt.lock().await;
    let low_precision = rt.config.low_precision;
    let workspace_folders = rt.workspace_folders.clone();

    let mut query = params.query.clone();
    if query == "" {
        query = String::from(".");
    }

    // Get Partial Result Token
    let partial_result_token = match params.partial_result_params.partial_result_token {
        Some(v) => v,
        None => return do_symbol_sync(&workspace_folders, &query, low_precision).await,
    };

    // Partial Result Progress
    let client = backend.client.clone();
    tokio::task::spawn(async move {
        let _ = do_symbol_async(
            client,
            &workspace_folders,
            &query,
            &partial_result_token,
            low_precision,
        )
        .await;
    });

    return Ok(None);
}

lazy_static::lazy_static! {
    static ref RE_MACRO: Regex = Regex::new(r"#\s*define").unwrap();
}

async fn do_symbol_sync(
    workspace_folders: &Vec<WorkspaceFolder>,
    query: &String,
    low_precision: bool,
) -> tower_lsp::jsonrpc::Result<Option<Vec<SymbolInformation>>> {
    let mut symbol_list = Vec::<SymbolInformation>::new();

    // No need to return too much of symbols.
    const RECORD_LIMIT: usize = 99;

    for ele in workspace_folders {
        let cwd = &ele.uri;
        let args = vec!["-d", "-x", query.as_str()];
        let lines = crate::method::execute_and_split_lines("global", cwd, &args).await?;

        for line in lines {
            let (symbol_name, line_number, file_path, rest_string) =
                crate::method::parse_cxref(&line)?;
            let file_path = crate::method::join_workspace_path(cwd, &file_path)?;
            let loc = crate::method::find_symbol_in_line(
                &file_path,
                line_number - 1,
                &symbol_name,
                low_precision,
            )
            .await?;

            let info = create_symbol_information(symbol_name, loc, &rest_string);
            symbol_list.push(info);

            if symbol_list.len() >= RECORD_LIMIT {
                break;
            }
        }
    }

    return Ok(Some(symbol_list));
}

async fn do_symbol_async(
    client: tower_lsp::Client,
    workspace_folders: &Vec<WorkspaceFolder>,
    query: &String,
    partial_result_token: &NumberOrString,
    low_precision: bool,
) -> Result<(), tower_lsp::jsonrpc::Error> {
    let mut symbol_list = Vec::<SymbolInformation>::new();

    for ele in workspace_folders {
        let cwd = &ele.uri;
        let args = vec!["-d", "-x", query.as_str()];
        let lines = crate::method::execute_and_split_lines("global", cwd, &args).await?;

        for line in lines {
            let (symbol_name, line_number, file_path, rest_string) =
                crate::method::parse_cxref(&line)?;
            let file_path = crate::method::join_workspace_path(cwd, &file_path)?;
            let loc = crate::method::find_symbol_in_line(
                &file_path,
                line_number - 1,
                &symbol_name,
                low_precision,
            )
            .await?;

            let info = create_symbol_information(symbol_name, loc, &rest_string);
            symbol_list.push(info);

            if symbol_list.len() >= 16 {
                send_partial_result(&client, partial_result_token, &symbol_list).await;
                symbol_list.clear();
            }
        }
    }

    send_partial_result(&client, partial_result_token, &symbol_list).await;

    return Ok(());
}

enum TagsLspProgress {}

#[derive(Debug, PartialEq, serde::Deserialize, serde::Serialize, Clone)]
#[serde(rename_all = "camelCase")]
struct TagsLspProgressParams {
    token: ProgressToken,
    value: TagsLspProgressParamsValue,
}

#[derive(Debug, PartialEq, serde::Deserialize, serde::Serialize, Clone)]
#[serde(untagged)]
enum TagsLspProgressParamsValue {
    WorkDone(WorkDoneProgress),
    Symbol(Option<Vec<SymbolInformation>>),
}

impl notification::Notification for TagsLspProgress {
    type Params = TagsLspProgressParams;
    const METHOD: &'static str = "$/progress";
}

async fn send_partial_result(
    client: &tower_lsp::Client,
    token: &NumberOrString,
    symbol_list: &Vec<SymbolInformation>,
) {
    let symbol_list: Option<Vec<SymbolInformation>> = if symbol_list.len() == 0 {
        None
    } else {
        Some(symbol_list.clone())
    };

    client
        .send_notification::<TagsLspProgress>(TagsLspProgressParams {
            token: token.clone(),
            value: TagsLspProgressParamsValue::Symbol(symbol_list),
        })
        .await;
}

fn check_symbol_kind(context: &str) -> SymbolKind {
    match RE_MACRO.find(context) {
        Some(_) => return SymbolKind::NULL,
        None => return SymbolKind::FUNCTION,
    };
}

#[allow(deprecated)]
fn create_symbol_information(
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
