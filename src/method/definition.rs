use tower_lsp::lsp_types::*;

pub async fn goto_definition(
    backend: &crate::TagsLspBackend,
    params: GotoDefinitionParams,
) -> tower_lsp::jsonrpc::Result<Option<GotoDefinitionResponse>> {
    let rt = backend.rt.lock().await;

    let file_uri = &params.text_document_position_params.text_document.uri;
    let file_position = &params.text_document_position_params.position;
    let cwd = crate::method::find_belong_workspace_folder(&rt.workspace_folders, file_uri)?;

    let loc_list = searc_for_definition(&cwd.uri, file_uri, file_position).await?;
    return Ok(Some(GotoDefinitionResponse::Array(loc_list)));
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
    // Location list for return.
    let mut loc_list = Vec::new();

    // Get symbol from position.
    let symbol = crate::method::get_symbol_by_position(file, pos).await?;

    // Execute command.
    let args = vec!["-d", "-x", symbol.as_str()];
    let lines = crate::method::execute_and_split_lines("global", cwd, &args).await?;

    for line in lines {
        let words: Vec<&str> = line.split_whitespace().collect();
        let path = crate::method::join_workspace_path(cwd, words[2])?;
        let line: u32 = words[1].parse().expect("Not a valid u32");
        let location = crate::method::find_symbol_in_line(&path, line - 1, &symbol).await?;

        loc_list.push(location);
    }

    return Ok(loc_list);
}
