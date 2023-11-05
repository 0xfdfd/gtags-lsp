#include "__init__.h"

static void _lsp_method_textdocument_didsave(cJSON* req, cJSON* rsp)
{
    (void)rsp;

    cJSON* j_params = cJSON_GetObjectItem(req, "params");
    /* params.textDocument */
    cJSON* j_textDocument = cJSON_GetObjectItem(j_params, "textDocument");
    /* params.text */
    cJSON* j_text = cJSON_GetObjectItem(j_params, "text");

    (void)j_textDocument; (void)j_text;
}

lsp_method_t lsp_method_textdocument_didsave = {
    "textDocument/didSave", _lsp_method_textdocument_didsave, 1,
};
