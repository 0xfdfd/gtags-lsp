pub async fn do_initialize(
    backend: &crate::TagsLspBackend,
    params: tower_lsp::lsp_types::InitializeParams,
) -> tower_lsp::jsonrpc::Result<tower_lsp::lsp_types::InitializeResult> {
    let mut rt = backend.rt.lock().await;
    copy_workspace_folder(&mut rt, &params);

    match check_executable_gtags().await {
        true => (),
        false => {
            return Err(tower_lsp::jsonrpc::Error {
                code: tower_lsp::jsonrpc::ErrorCode::ServerError(
                    crate::LspErrorCode::RequestFailed.code(),
                ),
                message: std::borrow::Cow::Borrowed("gtags: command not found"),
                data: Option::None,
            })
        }
    }

    return Ok(tower_lsp::lsp_types::InitializeResult {
        server_info: Some(tower_lsp::lsp_types::ServerInfo {
            name: rt.prog_name.clone(),
            version: Some(rt.prog_version.clone()),
        }),
        capabilities: get_server_capacity(),
        ..Default::default()
    });
}

async fn check_executable_gtags() -> bool {
    let output = tokio::process::Command::new("gtags")
        .arg("--version")
        .output();
    let output = output.await;

    match output {
        Ok(_) => return true,
        Err(_) => return false,
    }
}

/// Safe workspace folders from client initialize params.
///
/// # Arguments
///
/// + `dst` - A mut reference to Runtime.
/// + `src` - Reference to InitializeParams
fn copy_workspace_folder(dst: &mut crate::Runtime, src: &tower_lsp::lsp_types::InitializeParams) {
    match &src.root_uri {
        Some(value) => dst
            .workspace_folders
            .push(tower_lsp::lsp_types::WorkspaceFolder {
                name: String::from(""),
                uri: value.clone(),
            }),
        None => (),
    };

    match &src.workspace_folders {
        Some(value) => dst.workspace_folders = value.clone(),
        None => (),
    };
}

/// Get server capacity
fn get_server_capacity() -> tower_lsp::lsp_types::ServerCapabilities {
    return tower_lsp::lsp_types::ServerCapabilities {
        position_encoding: Some(tower_lsp::lsp_types::PositionEncodingKind::UTF8),
        text_document_sync: Some(tower_lsp::lsp_types::TextDocumentSyncCapability::Kind(
            tower_lsp::lsp_types::TextDocumentSyncKind::FULL,
        )),
        definition_provider: Some(tower_lsp::lsp_types::OneOf::Right(
            tower_lsp::lsp_types::DefinitionOptions {
                work_done_progress_options: tower_lsp::lsp_types::WorkDoneProgressOptions {
                    work_done_progress: Some(true),
                },
            },
        )),
        type_definition_provider: Some(
            tower_lsp::lsp_types::TypeDefinitionProviderCapability::Simple(true),
        ),
        implementation_provider: Some(
            tower_lsp::lsp_types::ImplementationProviderCapability::Simple(true),
        ),
        references_provider: Some(tower_lsp::lsp_types::OneOf::Right(
            tower_lsp::lsp_types::ReferencesOptions {
                work_done_progress_options: tower_lsp::lsp_types::WorkDoneProgressOptions {
                    work_done_progress: Some(true),
                },
            },
        )),
        workspace_symbol_provider: Some(tower_lsp::lsp_types::OneOf::Right(
            tower_lsp::lsp_types::WorkspaceSymbolOptions {
                work_done_progress_options: tower_lsp::lsp_types::WorkDoneProgressOptions {
                    work_done_progress: Some(true),
                },
                resolve_provider: Some(false),
            },
        )),
        declaration_provider: Some(tower_lsp::lsp_types::DeclarationCapability::Simple(true)),
        workspace: Some(tower_lsp::lsp_types::WorkspaceServerCapabilities {
            workspace_folders: Some(tower_lsp::lsp_types::WorkspaceFoldersServerCapabilities {
                supported: Some(true),
                change_notifications: Some(tower_lsp::lsp_types::OneOf::Left(false)),
            }),
            file_operations: Some(
                tower_lsp::lsp_types::WorkspaceFileOperationsServerCapabilities {
                    did_rename: Some(tower_lsp::lsp_types::FileOperationRegistrationOptions {
                        filters: vec![tower_lsp::lsp_types::FileOperationFilter {
                            scheme: Option::None,
                            pattern: tower_lsp::lsp_types::FileOperationPattern {
                                glob: String::from("*"),
                                ..Default::default()
                            },
                        }],
                    }),
                    did_delete: Some(tower_lsp::lsp_types::FileOperationRegistrationOptions {
                        filters: vec![tower_lsp::lsp_types::FileOperationFilter {
                            scheme: Option::None,
                            pattern: tower_lsp::lsp_types::FileOperationPattern {
                                glob: String::from("*"),
                                ..Default::default()
                            },
                        }],
                    }),
                    ..Default::default()
                },
            ),
        }),
        ..tower_lsp::lsp_types::ServerCapabilities::default()
    };
}
