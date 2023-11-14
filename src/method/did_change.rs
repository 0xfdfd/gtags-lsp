use tower_lsp::lsp_types::*;

pub async fn did_change(backend: &crate::TagsLspBackend, params: DidChangeTextDocumentParams) {
    let mut rt = backend.rt.lock().await;

    let doc_uri = params.text_document.uri;
    let doc_dat = &params.content_changes[0].text;

    rt.open_files
        .entry(doc_uri)
        .and_modify(|e| *e = doc_dat.clone());
}
