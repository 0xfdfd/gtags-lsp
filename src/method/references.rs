use tower_lsp::lsp_types::*;

pub async fn do_references(
    backend: &crate::TagsLspBackend,
    params: ReferenceParams,
) -> tower_lsp::jsonrpc::Result<Option<Vec<Location>>> {
    let rt = backend.rt.lock().await;

    let doc_uri = &params.text_document_position.text_document.uri;
    let doc_pos = &params.text_document_position.position;
    let cwd = crate::method::find_belong_workspace_folder(&rt.workspace_folders, doc_uri)?;

    let loc_list = search_for_references(&cwd.uri, doc_uri, doc_pos).await?;
    return Ok(Some(loc_list));
}

/// Search for references
///
/// # Arguments
///
/// + `cwd`: Workspace folder
/// + `file`: The file url that contains source symbol.
/// + `pos`: Source symbol position.
async fn search_for_references(
    cwd: &Url,
    file: &Url,
    pos: &Position,
) -> Result<Vec<Location>, tower_lsp::jsonrpc::Error> {
    // Location list for return.
    let mut loc_list = Vec::new();

    // Get symbol.
    let symbol = crate::method::get_symbol_by_position(file, pos).await?;

    let args = vec!["-r", "-s", "-x", symbol.as_str()];
    let lines = crate::method::execute_and_split_lines("global", cwd, &args).await?;

    for line in lines {
        let (_, line_number, file_path, _) = crate::method::parse_cxref(&line)?;
        let file_path = crate::method::join_workspace_path(cwd, &file_path)?;

        let location =
            crate::method::find_symbol_in_line(&file_path, line_number - 1, &symbol).await?;
        loc_list.push(location);
    }

    return Ok(loc_list);
}
