mod method;

/// LSP error code that [`tower_lsp::jsonrpc::ErrorCode`] not defined.
pub enum LspErrorCode {
    /// Error code indicating that a server received a notification or
    /// request before the server has received the `initialize` request.
    ServerNotInitialized,

    /// A request failed but it was syntactically correct, e.g the
    /// method name was known and the parameters were valid. The error
    /// message should contain human readable information about why
    /// the request failed.
    RequestFailed,
}

impl LspErrorCode {
    pub const fn code(&self) -> i64 {
        match self {
            LspErrorCode::ServerNotInitialized => -32002,
            LspErrorCode::RequestFailed => -32803,
        }
    }
}

#[derive(Debug, clap::Parser)]
#[command(author, version, about, long_about = None)]
struct TagsLspArgs {
    #[arg(
        long,
        conflicts_with = "port",
        help = "Uses stdio as the communication channel"
    )]
    stdio: bool,

    #[arg(
        long,
        conflicts_with = "stdio",
        help = "Uses a socket as the communication channel"
    )]
    port: Option<i32>,

    #[arg(
        long,
        value_name = "FILE",
        help = "Specifies a directory to use for logging"
    )]
    log: Option<String>,
}

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

    async fn initialized(&self, params: tower_lsp::lsp_types::InitializedParams) {
        return method::initialized::do_initialized(self, params).await;
    }

    async fn shutdown(&self) -> tower_lsp::jsonrpc::Result<()> {
        Ok(())
    }
}

fn setup_command_line_arguments(prog_name: &str) {
    use clap::Parser;
    let args = TagsLspArgs::parse();

    // Setup logging system.
    match args.log {
        Some(path) => {
            let file_appender = tracing_appender::rolling::hourly(path, prog_name);
            let (non_blocking, _) = tracing_appender::non_blocking(file_appender);
            tracing_subscriber::fmt().with_writer(non_blocking).init();
        }
        None => {
            tracing_subscriber::fmt()
                .with_writer(std::io::stderr)
                .init();
        }
    }
}

#[tokio::main]
async fn main() {
    const PROG_NAME: &str = env!("CARGO_PKG_NAME");
    const PROG_VERSION: &str = env!("CARGO_PKG_VERSION");

    setup_command_line_arguments(PROG_NAME);

    let stdin = tokio::io::stdin();
    let stdout = tokio::io::stdout();

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
