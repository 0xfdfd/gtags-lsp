#include "__init__.h"
#include "runtime.h"

static void _lsp_method_cancelrequest_nolock(cJSON* j_id)
{
    ev_list_node_t* it = ev_list_begin(&g_tags.work_queue);

    for (; it != NULL; it = ev_list_next(it))
    {
        tag_lsp_work_t* work = container_of(it, tag_lsp_work_t, node);
        cJSON* orig_id = cJSON_GetObjectItem(work->req, "id");

        if (cJSON_Compare(orig_id, j_id, 1))
        {
            /* Set cancel flag so processing request can try best to cancel. */
            work->cancel = 1;

            /* Pending request is revoked. */
            uv_cancel((uv_req_t*)&work->token);
            break;
        }
    }
}

static void _lsp_method_cancelrequest(cJSON* req, cJSON* rsp)
{
    (void)rsp;

    cJSON* j_params = cJSON_GetObjectItem(req, "params");
    cJSON* j_id = cJSON_GetObjectItem(j_params, "id");

    uv_mutex_lock(&g_tags.work_queue_mutex);
    _lsp_method_cancelrequest_nolock(j_id);
    uv_mutex_unlock(&g_tags.work_queue_mutex);
}

lsp_method_t lsp_method_cancelrequest = {
    "$/cancelRequest", _lsp_method_cancelrequest, 1,
};
