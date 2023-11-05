#include "__init__.h"

static void _lsp_method_textdocument_didchange(cJSON* req, cJSON* rsp)
{
    (void)rsp;

    cJSON* j_params = cJSON_GetObjectItem(req, "params");

    /* params.textDocument */
    cJSON* j_textDocument = cJSON_GetObjectItem(j_params, "textDocument");
    /* params.contentChanges */
    cJSON* j_contentChanges = cJSON_GetObjectItem(j_params, "contentChanges");

    (void)j_textDocument; (void)j_contentChanges;
}

lsp_method_t lsp_method_textdocument_didchange = {
    "textDocument/didChange", _lsp_method_textdocument_didchange, 1,
};
