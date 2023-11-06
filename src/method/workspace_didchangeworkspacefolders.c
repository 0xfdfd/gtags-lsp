#include <string.h>
#include <stdlib.h>
#include "__init__.h"
#include "runtime.h"

#if defined(_WIN32)
#   define strdup(s)    _strdup(s)
#endif

static void _lsp_method_workspace_didchangeworkspacefolders_remove(cJSON* j_removed)
{
    cJSON* ele = NULL;
    cJSON_ArrayForEach(ele, j_removed)
    {
        cJSON* j_name = cJSON_GetObjectItem(ele, "name");
        const char* c_name = cJSON_GetStringValue(j_name);

        size_t i;
        for (i = 0; i < g_tags.workspace_folder_sz; i++)
        {
            if (strcmp(c_name, g_tags.workspace_folders[i].name) == 0)
            {
                free(g_tags.workspace_folders[i].name);
                free(g_tags.workspace_folders[i].uri);

                size_t move_size = sizeof(workspace_folder_t) * (g_tags.workspace_folder_sz - i - 1);
                memmove(&g_tags.workspace_folders[i], &g_tags.workspace_folders[i + 1], move_size);
                g_tags.workspace_folder_sz--;
            }
        }
    }
}

static void _lsp_method_workspace_didchangeworkspacefolders_add(cJSON* j_added)
{
    cJSON* ele = NULL;
    cJSON_ArrayForEach(ele, j_added)
    {
        cJSON* j_name = cJSON_GetObjectItem(ele, "name");
        const char* c_name = cJSON_GetStringValue(j_name);

        cJSON* j_uri = cJSON_GetObjectItem(ele, "uri");
        const char* c_uri = cJSON_GetStringValue(j_uri);

        size_t new_sz = g_tags.workspace_folder_sz + 1;
        workspace_folder_t* new_folder = realloc(g_tags.workspace_folders, sizeof(workspace_folder_t) * new_sz);
        if (new_folder == NULL)
        {
            abort();
        }

        g_tags.workspace_folders = new_folder;
        g_tags.workspace_folder_sz = new_sz;

        g_tags.workspace_folders[g_tags.workspace_folder_sz - 1].name = strdup(c_name);
        g_tags.workspace_folders[g_tags.workspace_folder_sz - 1].uri = strdup(c_uri);
    }
}

static void _lsp_method_workspace_didchangeworkspacefolders(cJSON* req, cJSON* rsp)
{
    (void)rsp;

    cJSON* j_params = cJSON_GetObjectItem(req, "params");
    /* params.event */
    cJSON* j_event = cJSON_GetObjectItem(j_params, "event");

    /* params.event.added */
    cJSON* j_added = cJSON_GetObjectItem(j_event, "added");
    /* params.event.removed */
    cJSON* j_removed = cJSON_GetObjectItem(j_event, "removed");

    _lsp_method_workspace_didchangeworkspacefolders_remove(j_removed);
    _lsp_method_workspace_didchangeworkspacefolders_add(j_added);
}

lsp_method_t lsp_method_workspace_didchangeworkspacefolders = {
    "workspace/didChangeWorkspaceFolders", _lsp_method_workspace_didchangeworkspacefolders, 1,
};
