use tower_lsp::lsp_types::*;

pub async fn do_initialized(backend: &crate::TagsLspBackend, _: InitializedParams) {
    backend
        .client
        .log_message(MessageType::INFO, "server initialized!")
        .await;

    let rt = backend.rt.lock().await;
    for ele in &rt.workspace_folders {
        let path = ele.uri.clone();
        let client = backend.client.clone();
        let token = match client.next_request_id() {
            tower_lsp::jsonrpc::Id::Number(v) => v as i32,
            _ => panic!("unexcept type."),
        };

        tokio::task::spawn(async move {
            do_update_gtags(&client, token, path).await;
        });
    }
}

async fn do_update_gtags(client: &tower_lsp::Client, token: i32, path: Url) {
    do_work_done_progress_create(&client, token).await;

    do_notification_progress_begin(
        &client,
        token,
        String::from("Indexing"),
        String::from(path.path()),
    )
    .await;

    let mut child = tokio::process::Command::new("gtags")
        .current_dir(path.path())
        .arg("-i")
        .stdin(std::process::Stdio::null())
        .stdout(std::process::Stdio::null())
        .stderr(std::process::Stdio::null())
        .spawn()
        .expect("failed to spawn `gtags -i`");

    for i in 1..=99 {
        do_notification_progress_report(&client, token, String::from(path.path()), i).await;

        tokio::time::sleep(tokio::time::Duration::from_secs(1)).await;

        match child.try_wait() {
            Ok(v) => match v {
                Some(_) => break,
                None => (),
            },
            Err(_) => break,
        }
    }

    child.wait().await.expect("wait child failed.");

    do_notification_progress_report(&client, token, String::from(path.path()), 100).await;
    do_notification_progress_end(&client, token).await;
}

async fn do_work_done_progress_create(client: &tower_lsp::Client, token: i32) {
    client
        .send_request::<request::WorkDoneProgressCreate>(WorkDoneProgressCreateParams {
            token: ProgressToken::Number(token),
        })
        .await
        .expect("WorkDoneProgressCreate failed");
}

/// Start progress reporting
async fn do_notification_progress_begin(
    client: &tower_lsp::Client,
    token: i32,
    title: String,
    message: String,
) {
    client
        .send_notification::<notification::Progress>(ProgressParams {
            token: ProgressToken::Number(token),
            value: ProgressParamsValue::WorkDone(WorkDoneProgress::Begin(WorkDoneProgressBegin {
                title: title,
                message: Some(message),
                percentage: Some(0),
                ..Default::default()
            })),
        })
        .await;
}

/// Reporting progress
async fn do_notification_progress_report(
    client: &tower_lsp::Client,
    token: i32,
    message: String,
    percentage: u32,
) {
    client
        .send_notification::<notification::Progress>(ProgressParams {
            token: ProgressToken::Number(token),
            value: ProgressParamsValue::WorkDone(WorkDoneProgress::Report(
                WorkDoneProgressReport {
                    cancellable: Some(false),
                    message: Some(message),
                    percentage: Some(percentage),
                },
            )),
        })
        .await;
}

/// Signaling the end of a progress reporting
async fn do_notification_progress_end(client: &tower_lsp::Client, token: i32) {
    client
        .send_notification::<notification::Progress>(ProgressParams {
            token: ProgressToken::Number(token),
            value: ProgressParamsValue::WorkDone(WorkDoneProgress::End(WorkDoneProgressEnd {
                message: Option::None,
            })),
        })
        .await;
}
