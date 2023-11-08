#include <stdlib.h>
#include <string.h>
#include "__init__.h"
#include "defines.h"
#include "runtime.h"
#include "utils/lsp_msg.h"
#include "utils/execute.h"
#include "utils/alloc.h"
#include "utils/log.h"

typedef struct lsp_work_task
{
	lsp_work_t* token;
	char*       workspace;
} lsp_work_task_t;

static void _lsp_task_work_done_progress_begin(cJSON* workDoneToken, const char* title)
{
    cJSON* params = cJSON_CreateObject();
    cJSON_AddItemToObject(params, "token", cJSON_Duplicate(workDoneToken, 1));

    cJSON* value = cJSON_CreateObject();
    {
        cJSON_AddStringToObject(value, "kind", "begin");
        cJSON_AddStringToObject(value, "title", title);
        cJSON_AddBoolToObject(value, "cancellable", 0);
        cJSON_AddNumberToObject(value, "percentage", 0);
    }
    cJSON_AddItemToObject(params, "value", value);

    cJSON* msg = lsp_create_notify("$/progress", params);

    lsp_send_notify(msg);
    cJSON_Delete(msg);
}

static void _lsp_task_work_done_process_report(cJSON* workDoneToken, unsigned percentage)
{
    cJSON* params = cJSON_CreateObject();
    cJSON_AddItemToObject(params, "token", cJSON_Duplicate(workDoneToken, 1));

    cJSON* value = cJSON_CreateObject();
    {
        cJSON_AddStringToObject(value, "kind", "report");
        cJSON_AddBoolToObject(value, "cancellable", 0);
        cJSON_AddNumberToObject(value, "percentage", percentage);
    }
    cJSON_AddItemToObject(params, "value", value);

    cJSON* msg = lsp_create_notify("$/progress", params);
    lsp_send_notify(msg);
    cJSON_Delete(msg);
}

static void _lsp_task_work_done_process_end(cJSON* workDoneToken)
{
    cJSON* params = cJSON_CreateObject();
    cJSON_AddItemToObject(params, "token", cJSON_Duplicate(workDoneToken, 1));

    cJSON* value = cJSON_CreateObject();
    {
        cJSON_AddStringToObject(value, "kind", "end");
    }
    cJSON_AddItemToObject(params, "value", value);

    cJSON* msg = lsp_create_notify("$/progress", params);
    lsp_send_notify(msg);
    cJSON_Delete(msg);
}

static void _lsp_task_update_tags_pre(cJSON* token)
{
    cJSON* params = cJSON_CreateObject();
    {
        cJSON_AddItemToObject(params, "token", cJSON_Duplicate(token, 1));
    }
    cJSON* lsp_req = lsp_create_req("window/workDoneProgress/create", params);

    cJSON* lsp_rsp = lsp_send_req(lsp_req);
    cJSON_Delete(lsp_req); lsp_req = NULL;
    cJSON_Delete(lsp_rsp); lsp_rsp = NULL;

    _lsp_task_work_done_progress_begin(token, "Indexing");
    _lsp_task_work_done_process_report(token, 1);
}

static void _lsp_task_update_tags_after(cJSON* token)
{
    _lsp_task_work_done_process_report(token, 100);
    _lsp_task_work_done_process_end(token);
}

static int _lsp_task_check_client_work_done_progress(void)
{
    int support = 0;
    cJSON* j_window = cJSON_GetObjectItem(g_tags.client_capabilities, "window");
    if (j_window != NULL)
    {
        cJSON* j_workDoneProgress = cJSON_GetObjectItem(j_window, "workDoneProgress");
        if (j_workDoneProgress != NULL)
        {
            support = cJSON_IsTrue(j_workDoneProgress);
        }
    }

    return support;
}

static void _lsp_task_on_update_tags(lsp_work_t* token, int cancel, void* arg)
{
    (void)token; (void)cancel;

    lsp_work_task_t* task = arg;

    int support = _lsp_task_check_client_work_done_progress();

    char id[32];
    lsp_new_id_str(id, sizeof(id));
    cJSON* j_id = cJSON_CreateString(id);

    if (support)
    {
        _lsp_task_update_tags_pre(j_id);
    }

    char* args[] = { "gtags", "-i", NULL };
    int ret = lsp_execute("gtags", args, task->workspace, NULL, NULL);
    if (ret != 0)
    {
        LSP_LOG(LSP_MSG_ERROR, "execute gtags failed: %s(%d).",
            uv_strerror(ret), ret);
        abort();
    }

    if (support)
    {
        _lsp_task_update_tags_after(j_id);
    }

    cJSON_Delete(j_id);

    lsp_free(task);
}

void lsp_task_update_tags(const char* workspace)
{
    lsp_work_task_t* task = lsp_malloc(sizeof(lsp_work_task_t));
    task->workspace = lsp_strdup(workspace);

    lsp_queue_work(&task->token, _lsp_task_on_update_tags, task);
}
