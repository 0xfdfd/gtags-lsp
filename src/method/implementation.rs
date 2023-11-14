use tower_lsp::lsp_types::*;

pub async fn goto_implementation(
    backend: &crate::TagsLspBackend,
    params: request::GotoImplementationParams,
) -> tower_lsp::jsonrpc::Result<Option<request::GotoImplementationResponse>> {
    let ret = crate::method::definition::goto_definition(backend, params).await?;
    return Ok(ret);
}
