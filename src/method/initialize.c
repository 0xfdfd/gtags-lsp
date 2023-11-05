#include "__init__.h"
#include "runtime.h"
#include "version.h"

#if defined(_WIN32)
#   define strdup(s)    _strdup(s)
#endif

static void _lsp_method_initialize_set_one_workspace_folder(const char* path)
{
    tag_lsp_cleanup_workspace_folders();

    g_tags.workspace_folder_sz = 1;
    g_tags.workspace_folders = malloc(sizeof(workspace_folder_t));
    g_tags.workspace_folders[0].name = strdup("");
    g_tags.workspace_folders[0].uri = strdup(path);
}

static int _lsp_method_init_workspace_folders(cJSON* params)
{
    cJSON* json_root_path = cJSON_GetObjectItem(params, "rootPath");
    if (json_root_path != NULL)
    {
        const char* root_path = cJSON_GetStringValue(json_root_path);
        if (root_path != NULL)
        {
            _lsp_method_initialize_set_one_workspace_folder(root_path);
        }
    }

    cJSON* json_root_uri = cJSON_GetObjectItem(params, "rootUri");
    if (json_root_uri != NULL)
    {
        const char* root_uri = cJSON_GetStringValue(json_root_uri);
        if (root_uri != NULL)
        {
            _lsp_method_initialize_set_one_workspace_folder(root_uri);
        }
    }

    cJSON* json_workspace_folders = cJSON_GetObjectItem(params, "workspaceFolders");
    if (json_workspace_folders != NULL)
    {
        tag_lsp_cleanup_workspace_folders();

        cJSON* ele = NULL;
        cJSON_ArrayForEach(ele, json_workspace_folders)
        {
            cJSON* json_name = cJSON_GetObjectItem(ele, "name");
            const char* name = cJSON_GetStringValue(json_name);

            cJSON* json_uri = cJSON_GetObjectItem(ele, "uri");
            const char* uri = cJSON_GetStringValue(json_uri);

            size_t new_sz = g_tags.workspace_folder_sz + 1;
            workspace_folder_t* new_folder = realloc(g_tags.workspace_folders, sizeof(workspace_folder_t) * new_sz);
            if (new_folder == NULL)
            {
                abort();
            }

            g_tags.workspace_folders = new_folder;
            g_tags.workspace_folder_sz = new_sz;

            g_tags.workspace_folders[g_tags.workspace_folder_sz - 1].name = strdup(name);
            g_tags.workspace_folders[g_tags.workspace_folder_sz - 1].uri = strdup(uri);
        }
    }

    return 0;
}

static int _lsp_method_init_client_capabilities(cJSON* params)
{
    cJSON* json_capabilities = cJSON_GetObjectItem(params, "capabilities");
    if (json_capabilities != NULL)
    {
        tag_lsp_cleanup_client_capabilities();
        g_tags.client_capabilities = cJSON_Duplicate(json_capabilities, 1);
    }

    return 0;
}

static void _lsp_method_init_generate_server_capabilities(cJSON* capabilities)
{
    cJSON_AddStringToObject(capabilities, "positionEncoding", "utf-8");

    {
        cJSON* textDocumentSync = cJSON_CreateObject();
        cJSON_AddBoolToObject(textDocumentSync, "openClose", 1);
        cJSON_AddNumberToObject(textDocumentSync, "change", 1);
        cJSON_AddBoolToObject(textDocumentSync, "save", 1);
        cJSON_AddItemToObject(capabilities, "textDocumentSync", textDocumentSync);
    }

    cJSON_AddBoolToObject(capabilities, "declarationProvider", 1);
    cJSON_AddBoolToObject(capabilities, "definitionProvider", 1);
    cJSON_AddBoolToObject(capabilities, "typeDefinitionProvider", 1);
    cJSON_AddBoolToObject(capabilities, "implementationProvider", 1);
    cJSON_AddBoolToObject(capabilities, "referencesProvider", 1);

    {
        cJSON* workspace = cJSON_CreateObject();
        {
            cJSON* workspaceFolders = cJSON_CreateObject();
            cJSON_AddBoolToObject(workspaceFolders, "supported", 1);
            cJSON_AddBoolToObject(workspaceFolders, "changeNotifications", 1);
            cJSON_AddItemToObject(workspace, "workspaceFolders", workspaceFolders);
        }
        cJSON_AddItemToObject(capabilities, "workspace", workspace);
    }
}

static void _lsp_method_initialize_finished(cJSON* rsp)
{
    cJSON* InitializeResult = cJSON_CreateObject();

    /* The capabilities the language server provides. */
    {
        cJSON* capabilities = cJSON_CreateObject();
        _lsp_method_init_generate_server_capabilities(capabilities);
        cJSON_AddItemToObject(InitializeResult, "capabilities", capabilities);
    }

    /* Information about the server. */
    {
        cJSON* server_info = cJSON_CreateObject();
        cJSON_AddStringToObject(server_info, "name", "gtags-lsp");
        cJSON_AddStringToObject(server_info, "version", tag_lsp_version());
        cJSON_AddItemToObject(InitializeResult, "serverInfo", server_info);
    }

    cJSON_AddItemToObject(rsp, "result", InitializeResult);
}

static void _lsp_method_initialize(cJSON* req, cJSON* rsp)
{
    int ret;
    cJSON* params = cJSON_GetObjectItem(req, "params");

    if ((ret = _lsp_method_init_workspace_folders(params)) != 0)
    {
        goto error;
    }

    if ((ret = _lsp_method_init_client_capabilities(params)) != 0)
    {
        goto error;
    }

    _lsp_method_initialize_finished(rsp);
    return;

error:
    tag_lsp_set_error(rsp, ret, NULL);
    return;
}

lsp_method_t lsp_method_initialize = {
    "initialize", _lsp_method_initialize, 0,
};
