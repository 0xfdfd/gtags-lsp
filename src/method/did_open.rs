use tower_lsp::lsp_types::*;

pub async fn did_open(backend: &crate::TagsLspBackend, params: DidOpenTextDocumentParams) {
    let mut rt = backend.rt.lock().await;

    let doc_uri = params.text_document.uri;
    let doc_dat = params.text_document.text;

    rt.open_files.insert(doc_uri, doc_dat);
}
