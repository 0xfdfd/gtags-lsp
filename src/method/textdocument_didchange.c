#include "__init__.h"

static int _lsp_method_textdocument_didchange(cJSON* req, cJSON* rsp)
{
    (void)rsp;

    cJSON* j_params = cJSON_GetObjectItem(req, "params");

    /* params.textDocument */
    cJSON* j_textDocument = cJSON_GetObjectItem(j_params, "textDocument");
    /* params.contentChanges */
    cJSON* j_contentChanges = cJSON_GetObjectItem(j_params, "contentChanges");

    (void)j_textDocument; (void)j_contentChanges;

    return 0;
}

lsp_method_t lsp_method_textdocument_didchange = {
    "textDocument/didChange", 1, _lsp_method_textdocument_didchange, NULL,
};
