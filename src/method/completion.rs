use std::collections::BTreeSet;

use tower_lsp::lsp_types::*;

pub async fn do_completion(
    backend: &crate::TagsLspBackend,
    params: CompletionParams,
) -> tower_lsp::jsonrpc::Result<Option<CompletionResponse>> {
    /// The maximum limit of return symbols.
    const SYMBOL_LIMIT: usize = 16;

    let mut item_list = Vec::<CompletionItem>::new();
    let rt = backend.rt.lock().await;

    let doc_uri = params.text_document_position.text_document.uri;
    let doc_pos = params.text_document_position.position;
    let doc_dat = rt.open_files.get(&doc_uri).unwrap();
    let cwd = crate::method::find_belong_workspace_folder(&rt.workspace_folders, &doc_uri)?;

    let symbol = crate::method::get_symbol_by_pos_from_dat(doc_dat, &doc_pos)?;

    // Execute `gtags -c`
    let args = vec!["-c", symbol.as_str()];
    let future_completion = crate::method::execute_and_split_lines("global", &cwd.uri, &args);

    // Execute `gtags -sx`
    let args = format!("{}.", symbol.as_str());
    let args = vec!["-sx", &args];
    let future_symbol = crate::method::execute_and_split_lines("global", &cwd.uri, &args);

    let lines_completion = future_completion.await?;
    let lines_symbol = future_symbol.await?;

    // Remove duplication
    let mut ret_set: BTreeSet<String> = BTreeSet::new();
    for ele in lines_completion {
        ret_set.insert(ele);
    }
    for ele in lines_symbol {
        let (name, _, _, _) = crate::method::parse_cxref(&ele)?;
        ret_set.insert(name);
    }

    // Parse as result.
    for ele in ret_set {
        let item = CompletionItem {
            label: ele,
            ..Default::default()
        };
        item_list.push(item);

        if item_list.len() >= SYMBOL_LIMIT {
            break;
        }
    }

    return Ok(Some(CompletionResponse::Array(item_list)));
}
