#include <stdlib.h>
#include "runtime.h"

tags_ctx_t g_tags; /**< Global runtime. */

void tag_lsp_cleanup_workspace_folders(void)
{
    size_t i;
    for (i = 0; i < g_tags.workspace_folder_sz; i++)
    {
        free(g_tags.workspace_folders[i].name);
        free(g_tags.workspace_folders[i].uri);
    }
    free(g_tags.workspace_folders);
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

void lsp_want_exit(void)
{
    g_tags.flags.shutdown = 1;
    uv_stop(g_tags.loop);
}
