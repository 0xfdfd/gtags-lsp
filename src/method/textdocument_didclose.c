#include "__init__.h"

static void _lsp_method_textdocument_didclose(cJSON* req, cJSON* rsp)
{
    (void)rsp;

    cJSON* j_params = cJSON_GetObjectItem(req, "params");
    /* params.textDocument */
    cJSON* j_textDocument = cJSON_GetObjectItem(j_params, "textDocument");

    (void)j_textDocument;
}

lsp_method_t lsp_method_textdocument_didclose = {
    "textDocument/didClose", _lsp_method_textdocument_didclose, 1,
};
