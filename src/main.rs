mod method;

use tower_lsp::{lsp_types::*, ClientSocket, LspService};

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

#[derive(Debug, clap::Parser, Default, Clone)]
#[command(author, version, about, long_about = None)]
struct TagsLspConfig {
    #[arg(
        long,
        conflicts_with = "port",
        help = "Uses stdio as the communication channel"
    )]
    stdio: bool,

    #[arg(
        long,
        conflicts_with = "stdio",
        help = "Uses a socket as the communication channel",
        long_help = "The LSP server start as TCP client and connect to the specified port."
    )]
    port: Option<u16>,

    #[arg(
        long,
        value_name = "DIR",
        help = "Specifies a directory to use for logging"
    )]
    logdir: Option<String>,

    #[arg(
        long,
        value_name = "STRING",
        help = "Set log leve",
        long_help = "Possible values are: [OFF | TRACE | DEBUG | INFO | WARN | ERROR]. By default
`INFO` is used. Case insensitive."
    )]
    loglevel: Option<String>,

    #[arg(
        long,
        help = "Enable `low_precision` mode",
        long_help = "By default symbols are matched accurately, so when LSP client jump to the symbol
place, the cursor will surround the symbol. However that behaviour is very
costly. Enable `low_precision` mode makes the matching result point to the
begin of line, which should not affect experience very much."
    )]
    low_precision: bool,
}

#[derive(Debug, Default, Clone)]
struct Runtime {
    /// Program name.
    prog_name: String,

    /// Program version.
    prog_version: String,

    /// Configuration.
    config: TagsLspConfig,

    /// Workspace folder list.
    workspace_folders: Vec<WorkspaceFolder>,
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
        params: InitializeParams,
    ) -> tower_lsp::jsonrpc::Result<InitializeResult> {
        return method::initialize::do_initialize(self, params).await;
    }

    async fn initialized(&self, params: InitializedParams) {
        return method::initialized::do_initialized(self, params).await;
    }

    async fn shutdown(&self) -> tower_lsp::jsonrpc::Result<()> {
        Ok(())
    }

    async fn goto_definition(
        &self,
        params: GotoDefinitionParams,
    ) -> tower_lsp::jsonrpc::Result<Option<GotoDefinitionResponse>> {
        return method::definition::goto_definition(self, params).await;
    }

    async fn references(
        &self,
        params: ReferenceParams,
    ) -> tower_lsp::jsonrpc::Result<Option<Vec<Location>>> {
        return method::references::do_references(self, params).await;
    }

    async fn goto_implementation(
        &self,
        params: request::GotoImplementationParams,
    ) -> tower_lsp::jsonrpc::Result<Option<request::GotoImplementationResponse>> {
        return method::implementation::goto_implementation(self, params).await;
    }

    async fn symbol(
        &self,
        params: WorkspaceSymbolParams,
    ) -> tower_lsp::jsonrpc::Result<Option<Vec<SymbolInformation>>> {
        return method::symbol::do_symbol(self, params).await;
    }

    async fn document_symbol(
        &self,
        params: DocumentSymbolParams,
    ) -> tower_lsp::jsonrpc::Result<Option<DocumentSymbolResponse>> {
        return method::document_symbol::do_document_symbol(self, params).await;
    }
}

fn setup_command_line_arguments(prog_name: &str) -> TagsLspConfig {
    use clap::Parser;
    let args = TagsLspConfig::parse();

    // Get log level.
    let loglevel = match &args.loglevel {
        Some(v) => v.clone(),
        None => String::from("INFO"),
    };

    // Parse log level.
    let loglevel = match loglevel.to_lowercase().as_str() {
        "off" => tracing::metadata::LevelFilter::OFF,
        "trace" => tracing::metadata::LevelFilter::TRACE,
        "debug" => tracing::metadata::LevelFilter::DEBUG,
        "info" => tracing::metadata::LevelFilter::INFO,
        "warn" => tracing::metadata::LevelFilter::WARN,
        "error" => tracing::metadata::LevelFilter::ERROR,
        unmatched => panic!(
            "Parser command line argument failed: unknown option value `{}`",
            unmatched
        ),
    };

    // Setup logging system.
    match &args.logdir {
        Some(path) => {
            let logfile = format!("{}.log", prog_name);
            let file_appender = tracing_appender::rolling::never(path, logfile);
            tracing_subscriber::fmt()
                .with_max_level(loglevel)
                .with_writer(file_appender)
                .with_ansi(false)
                .init();
        }
        None => {
            tracing_subscriber::fmt()
                .with_max_level(loglevel)
                .with_writer(std::io::stderr)
                .init();
        }
    }

    return args;
}

fn show_welcome(prog_name: &str, prog_version: &str) {
    tracing::info!("{} - v{}", prog_name, prog_version);
    tracing::info!("PID: {}", std::process::id());
}

async fn start_lsp_using_stdio(service: LspService<TagsLspBackend>, socket: ClientSocket) {
    let stdin = tokio::io::stdin();
    let stdout = tokio::io::stdout();

    tower_lsp::Server::new(stdin, stdout, socket)
        .serve(service)
        .await;
}

async fn start_lsp_using_socket(
    service: LspService<TagsLspBackend>,
    socket: ClientSocket,
    port: u16,
) {
    let addr = format!("127.0.0.1:{}", port);
    let mut stream = match tokio::net::TcpStream::connect(addr).await {
        Ok(v) => v,
        Err(e) => panic!("Cannot connect to port `{}`: {}", port, e.to_string()),
    };

    let (r, w) = stream.split();
    tower_lsp::Server::new(r, w, socket).serve(service).await;
}

#[tokio::main]
async fn main() {
    const PROG_NAME: &str = env!("CARGO_PKG_NAME");
    const PROG_VERSION: &str = env!("CARGO_PKG_VERSION");

    let config = setup_command_line_arguments(PROG_NAME);
    show_welcome(PROG_NAME, PROG_VERSION);

    let rt = tokio::sync::Mutex::new(Runtime {
        prog_name: PROG_NAME.to_string(),
        prog_version: PROG_VERSION.to_string(),
        workspace_folders: Vec::new(),
        config: config.clone(),
    });

    let (service, socket) = tower_lsp::LspService::new(|client| TagsLspBackend { client, rt });

    match config.port {
        Some(port) => {
            start_lsp_using_socket(service, socket, port).await;
        }
        None => {
            start_lsp_using_stdio(service, socket).await;
        }
    }
}
