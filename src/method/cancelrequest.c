#include "__init__.h"
#include "runtime.h"
#include "utils/lsp_work.h"
#include "utils/lsp_msg.h"

static int _lsp_method_on_foreach(lsp_work_t* work, void* arg)
{
    cJSON* j_id = arg;

    if (work->type != LSP_WORK_METHOD)
    {
        return 0;
    }

    tag_lsp_work_method_t* method_work = container_of(work, tag_lsp_work_method_t, token);
    cJSON* orig_id = cJSON_GetObjectItem(method_work->req, "id");

    if (cJSON_Compare(j_id, orig_id, 1))
    {
        /* Set cancel flag so processing request can try best to cancel. */
        method_work->cancel = 1;

        /* Pending request is revoked. */
        lsp_work_cancel(work);

        return 1;
    }

    return 0;
}

static int _lsp_method_cancelrequest(cJSON* req, cJSON* rsp)
{
    (void)rsp;

    cJSON* j_params = cJSON_GetObjectItem(req, "params");
    cJSON* j_id = cJSON_GetObjectItem(j_params, "id");

    lsp_work_foreach(_lsp_method_on_foreach, j_id);

    return 0;
}

lsp_method_t lsp_method_cancelrequest = {
    "$/cancelRequest", 1, _lsp_method_cancelrequest, NULL,
};
