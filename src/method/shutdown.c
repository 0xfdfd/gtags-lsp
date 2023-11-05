#include "__init__.h"
#include "runtime.h"

static void _lsp_method_shutdown(cJSON* req, cJSON* rsp)
{
    (void)req;

    /* Set global shutdown flag. */
    g_tags.flags.shutdown = 1;

    /* Waiting for only this task left. */
    while (ev_list_size(&g_tags.work_queue) != 1)
    {
        uv_sleep(10);
    }

    /* Set response. */
    cJSON_AddNullToObject(rsp, "result");

    return;
}

lsp_method_t lsp_method_shutdown = {
    "shutdown", _lsp_method_shutdown, 0,
};
