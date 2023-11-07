#include "__init__.h"
#include "runtime.h"
#include "utils/lsp_work.h"
#include "utils/lsp_msg.h"

static int _lsp_method_cancelrequest(cJSON* req, cJSON* rsp)
{
    (void)rsp;

    cJSON* j_params = cJSON_GetObjectItem(req, "params");
    cJSON* j_id = cJSON_GetObjectItem(j_params, "id");

    lsp_handle_cancel(j_id);

    return 0;
}

lsp_method_t lsp_method_cancelrequest = {
    "$/cancelRequest", 1, _lsp_method_cancelrequest, NULL,
};
