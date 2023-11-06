#include <string.h>
#include <stdlib.h>
#include <uv.h>
#include "__init__.h"
#include "runtime.h"
#include "utils/lsp_error.h"
#include "utils/lsp_msg.h"

static lsp_method_t* s_lsp_methods[] = {
    /* Base Protocol. */
    &lsp_method_cancelrequest,

    /* Server lifecycle. */
    &lsp_method_initialize,
    &lsp_method_set_trace,
    &lsp_method_shutdown,
    &lsp_method_exit,

    /* Text Document Synchronization. */
    &lsp_method_textdocument_didopen,
    &lsp_method_textdocument_didchange,
    &lsp_method_textdocument_didsave,
    &lsp_method_textdocument_didclose,

    /* Workspace Features. */
    &lsp_method_workspace_didchangeworkspacefolders,
};

static void _lsp_method_on_work(uv_work_t* req)
{
    int ret = 0;
    tag_lsp_work_t* work = container_of(req, tag_lsp_work_t, token);

    /* Check whether it is a notify. */
    if (cJSON_GetObjectItem(work->req, "id") != NULL)
    {
        work->rsp = tag_lsp_create_rsp_from_req(work->req);
    }
    else
    {
        work->notify = 1;
    }

    /* Get method. */
    cJSON* json_method = cJSON_GetObjectItem(work->req, "method");
    if (json_method == NULL)
    {
        goto error_method_not_found;
    }
    const char* method = cJSON_GetStringValue(json_method);

    /* Reject any request if we are shutdown. */
    if (!work->notify && g_tags.flags.shutdown)
    {
        tag_lsp_set_error(work->rsp, TAG_LSP_ERR_INVALID_REQUEST, NULL);
        return;
    }

    /* Call method. */
    size_t i;
    for (i = 0; i < ARRAY_SIZE(s_lsp_methods); i++)
    {
        if (strcmp(method, s_lsp_methods[i]->name) == 0)
        {
            if (s_lsp_methods[i]->notify == work->notify)
            {
                ret = s_lsp_methods[i]->entry(work->req, work->rsp);
                goto finish;
            }
            else
            {
                goto error_method_not_found;
            }
        }
    }
    goto error_method_not_found;

finish:
    if (!work->notify && ret < 0)
    {
        tag_lsp_set_error(work->rsp, ret, NULL);
    }
    else if (!work->notify && ret == LSP_METHOD_ASYNC)
    {
        work->rsp = NULL;
    }
    return;

error_method_not_found:
    /* In this case method is not found. */
    if (!work->notify)
    {
        tag_lsp_set_error(work->rsp, TAG_LSP_ERR_METHOD_NOT_FOUND, NULL);
    }
    return;
}

static void _lsp_method_after_work(uv_work_t* req, int status)
{
    tag_lsp_work_t* work = container_of(req, tag_lsp_work_t, token);

    if (!work->notify)
    {
        if (status != 0)
        {
            tag_lsp_send_error(work->req, TAG_LSP_ERR_REQUEST_CANCELLED);
        }
        else
        {
            tag_lsp_send_msg(work->rsp);
        }
    }

    /* Remove record. */
    uv_mutex_lock(&g_tags.work_queue_mutex);
    ev_list_erase(&g_tags.work_queue, &work->node);
    uv_mutex_unlock(&g_tags.work_queue_mutex);

    if (work->req != NULL)
    {
        cJSON_Delete(work->req);
        work->req = NULL;
    }
    if (work->rsp != NULL)
    {
        cJSON_Delete(work->rsp);
        work->rsp = NULL;
    }
    free(work);
}

int lsp_method_call(cJSON* req)
{
    tag_lsp_work_t* work = malloc(sizeof(tag_lsp_work_t));
    if (work == NULL)
    {
        abort();
    }

    work->req = cJSON_Duplicate(req, 1);
    work->rsp = NULL;
    work->notify = 0;
    work->cancel = 0;

    uv_mutex_lock(&g_tags.work_queue_mutex);
    ev_list_push_back(&g_tags.work_queue, &work->node);
    uv_mutex_unlock(&g_tags.work_queue_mutex);

    int ret = uv_queue_work(g_tags.loop, &work->token, _lsp_method_on_work, _lsp_method_after_work);
    if (ret != 0)
    {
        abort();
    }

    return 0;
}

void lsp_method_cleanup(void)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(s_lsp_methods); i++)
    {
        if (s_lsp_methods[i]->cleanup != NULL)
        {
            s_lsp_methods[i]->cleanup();
        }
    }
}
