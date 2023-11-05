#include "__init__.h"
#include "runtime.h"

static void _lsp_method_exit(cJSON* req, cJSON* rsp)
{
    (void)req; (void)rsp;
    uv_stop(&g_tags.loop);
}

lsp_method_t lsp_method_exit = {
    "exit", _lsp_method_exit, 1,
};
