mod method;

#[derive(Debug)]
struct Runtime {
    /// Program name.
    prog_name: String,

    /// Program version.
    prog_version: String,

    /// Workspace folder list.
    workspace_folders: Vec<tower_lsp::lsp_types::WorkspaceFolder>,
}

#[derive(Debug)]
pub struct TagsLspBackend {
    client: tower_lsp::Client,
    rt: tokio::sync::Mutex<Runtime>,
}

#[tower_lsp::async_trait]
impl tower_lsp::LanguageServer for TagsLspBackend {
    async fn initialize(
        &self,
        params: tower_lsp::lsp_types::InitializeParams,
    ) -> tower_lsp::jsonrpc::Result<tower_lsp::lsp_types::InitializeResult> {
        return method::initialize::do_initialize(self, params).await;
    }

    async fn initialized(&self, _: tower_lsp::lsp_types::InitializedParams) {
        self.client
            .log_message(
                tower_lsp::lsp_types::MessageType::INFO,
                "server initialized!",
            )
            .await;
    }

    async fn shutdown(&self) -> tower_lsp::jsonrpc::Result<()> {
        Ok(())
    }
}

#[tokio::main]
async fn main() {
    let stdin = tokio::io::stdin();
    let stdout = tokio::io::stdout();

    const PROG_NAME: &str = env!("CARGO_PKG_NAME");
    const PROG_VERSION: &str = env!("CARGO_PKG_VERSION");

    let rt = tokio::sync::Mutex::new(Runtime {
        prog_name: PROG_NAME.to_string(),
        prog_version: PROG_VERSION.to_string(),
        workspace_folders: Vec::new(),
    });

    let (service, socket) = tower_lsp::LspService::new(|client| TagsLspBackend { client, rt });

    tower_lsp::Server::new(stdin, stdout, socket)
        .serve(service)
        .await;
}
