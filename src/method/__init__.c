#include <string.h>
#include <stdlib.h>
#include <uv.h>
#include "__init__.h"
#include "runtime.h"
#include "utils/lsp_error.h"
#include "utils/lsp_msg.h"

static lsp_method_t* s_lsp_methods[] = {
    /* Base Protocol. */
    &lsp_method_cancelrequest,

    /* Server lifecycle. */
    &lsp_method_initialize,
    &lsp_method_set_trace,
    &lsp_method_shutdown,
    &lsp_method_exit,

    /* Text Document Synchronization. */
    &lsp_method_textdocument_didopen,
    &lsp_method_textdocument_didchange,
    &lsp_method_textdocument_didsave,
    &lsp_method_textdocument_didclose,

    /* Workspace Features. */
    &lsp_method_workspace_didchangeworkspacefolders,
};

int lsp_method_call(cJSON* req, cJSON* rsp, int is_notify)
{
    size_t i;

	cJSON* j_method = cJSON_GetObjectItem(req, "method");
	const char* method = cJSON_GetStringValue(j_method);

    for (i = 0; i < ARRAY_SIZE(s_lsp_methods); i++)
    {
        if (strcmp(method, s_lsp_methods[i]->name) == 0)
        {
            if (s_lsp_methods[i]->notify != is_notify)
            {
                return TAG_LSP_ERR_METHOD_NOT_FOUND;
            }

            return s_lsp_methods[i]->entry(req, rsp);
        }
    }

    return TAG_LSP_ERR_METHOD_NOT_FOUND;
}

void lsp_method_cleanup(void)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(s_lsp_methods); i++)
    {
        if (s_lsp_methods[i]->cleanup != NULL)
        {
            s_lsp_methods[i]->cleanup();
        }
    }
}
