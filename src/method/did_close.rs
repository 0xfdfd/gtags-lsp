use tower_lsp::lsp_types::*;

pub async fn did_close(backend: &crate::TagsLspBackend, params: DidCloseTextDocumentParams) {
    let mut rt = backend.rt.lock().await;

    let doc_uri = params.text_document.uri;

    rt.open_files.remove(&doc_uri);
}
