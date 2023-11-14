use tower_lsp::lsp_types::*;

pub async fn do_document_symbol(
    backend: &crate::TagsLspBackend,
    params: DocumentSymbolParams,
) -> tower_lsp::jsonrpc::Result<Option<DocumentSymbolResponse>> {
    let mut loc_list = Vec::<SymbolInformation>::new();

    let rt = backend.rt.lock().await;
    let low_precision = rt.config.low_precision;

    let doc_uri = &params.text_document.uri;
    let cwd = crate::method::find_belong_workspace_folder(&rt.workspace_folders, doc_uri)?;

    let args = vec!["-x", "-f", doc_uri.path()];
    let lines = crate::method::execute_and_split_lines("global", &cwd.uri, &args).await?;

    for line in lines {
        let (symbol_name, line_number, file_path, rest_string) = crate::method::parse_cxref(&line)?;
        let file_path = crate::method::join_workspace_path(&cwd.uri, &file_path)?;

        let loc = crate::method::find_symbol_in_line(
            &file_path,
            line_number - 1,
            &symbol_name,
            low_precision,
        )
        .await?;
        let info = crate::method::create_symbol_information(symbol_name, loc, &rest_string);
        loc_list.push(info);
    }

    return Ok(Some(DocumentSymbolResponse::Flat(loc_list)));
}
