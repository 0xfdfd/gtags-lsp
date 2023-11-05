#include "__init__.h"
#include "runtime.h"

static void _lsp_method_set_trace(cJSON* req, cJSON* rsp)
{
    (void)rsp;

    cJSON* j_params = cJSON_GetObjectItem(req, "params");
    cJSON* j_value = cJSON_GetObjectItem(j_params, "value");
    const char* c_value = cJSON_GetStringValue(j_value);

    if (strcmp(c_value, "messages") == 0)
    {
        g_tags.trace_value = LSP_TRACE_VALUE_MESSAGES;
    }
    else if (strcmp(c_value, "verbose") == 0)
    {
        g_tags.trace_value = LSP_TRACE_VALUE_VERBOSE;
    }
    else
    {
        g_tags.trace_value = LSP_TRACE_VALUE_OFF;
    }
}

lsp_method_t lsp_method_set_trace = {
    "$/setTrace", _lsp_method_set_trace, 1,
};
