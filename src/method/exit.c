#include "__init__.h"
#include "runtime.h"

static int _lsp_method_exit(cJSON* req, cJSON* rsp)
{
    (void)req; (void)rsp;

    lsp_exit();

    return 0;
}

lsp_method_t lsp_method_exit = {
    "exit", 1, _lsp_method_exit, NULL,
};
