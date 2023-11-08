#include <stdlib.h>
#include "runtime.h"
#include "utils/alloc.h"

tags_ctx_t g_tags; /**< Global runtime. */

void tag_lsp_cleanup_workspace_folders(void)
{
    size_t i;
    for (i = 0; i < g_tags.workspace_folder_sz; i++)
    {
        lsp_free(g_tags.workspace_folders[i].name);
        lsp_free(g_tags.workspace_folders[i].uri);
    }
    lsp_free(g_tags.workspace_folders);
    g_tags.workspace_folders = NULL;
    g_tags.workspace_folder_sz = 0;
}

void tag_lsp_cleanup_client_capabilities(void)
{
    if (g_tags.client_capabilities != NULL)
    {
        cJSON_Delete(g_tags.client_capabilities);
        g_tags.client_capabilities = NULL;
    }
}

void lsp_exit(void)
{
    g_tags.flags.shutdown = 1;
    uv_async_send(g_tags.exit_notifier);
}
