#include "__init__.h"

static int _lsp_method_textdocument_didopen(cJSON* req, cJSON* rsp)
{
    (void)rsp;

    cJSON* j_params = cJSON_GetObjectItem(req, "params");
    /* params.textDocument */
    cJSON* j_textDocument = cJSON_GetObjectItem(j_params, "textDocument");

    cJSON* j_uri = cJSON_GetObjectItem(j_textDocument, "uri");
    cJSON* j_languageId = cJSON_GetObjectItem(j_textDocument, "languageId");
    cJSON* j_version = cJSON_GetObjectItem(j_textDocument, "version");
    cJSON* j_text = cJSON_GetObjectItem(j_textDocument, "text");

    (void)j_uri; (void)j_languageId; (void)j_version; (void)j_text;

    return 0;
}

lsp_method_t lsp_method_textdocument_didopen = {
    "textDocument/didOpen", 1, _lsp_method_textdocument_didopen, NULL,
};
