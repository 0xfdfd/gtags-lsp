use tower_lsp::lsp_types::*;

pub async fn did_open(backend: &crate::TagsLspBackend, params: DidOpenTextDocumentParams) {
    let mut rt = backend.rt.lock().await;

    let doc_uri = params.text_document.uri;
    let doc_dat = params.text_document.text;

    rt.open_files.insert(doc_uri, doc_dat.clone());

    let mut parser = tree_sitter::Parser::new();
    parser.set_language(tree_sitter_c::language()).unwrap();

    let tree = match parser.parse(&doc_dat, None) {
        Some(v) => v,
        None => return,
    };
    let root_node = tree.root_node();
    print_node_info(&doc_dat, root_node, 0);
}

fn print_node_info(doc: &str, node: tree_sitter::Node, level: usize) {
    let indent = " ".repeat(level);

    let range = node.range();
    let symbol = &doc[range.start_byte..range.end_byte];

    tracing::debug!(
        "{}symbol:`{}`, kind:`{}({})`, range:`{},{} -> {},{}`",
        indent,
        symbol,
        node.kind(),
        node.kind_id(),
        range.start_point.row,
        range.start_point.column,
        range.end_point.row,
        range.end_point.column
    );

    let mut cursor = node.walk();
    for child in node.children(&mut cursor) {
        print_node_info(doc, child, level + 1);
    }
}
