use tower_lsp::lsp_types::*;

pub async fn goto_type_definition(
    backend: &crate::TagsLspBackend,
    params: request::GotoTypeDefinitionParams,
) -> tower_lsp::jsonrpc::Result<Option<request::GotoTypeDefinitionResponse>> {
    let ret = crate::method::definition::goto_definition(backend, params).await?;
    return Ok(ret);
}
