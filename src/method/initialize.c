#include <stdlib.h>
#include <string.h>
#include "__init__.h"
#include "runtime.h"
#include "version.h"
#include "utils/lsp_msg.h"
#include "utils/execute.h"
#include "task/__init__.h"
#include "utils/log.h"
#include "utils/alloc.h"

static void _lsp_method_initialize_set_one_workspace_folder(const char* path)
{
    tag_lsp_cleanup_workspace_folders();

    g_tags.workspace_folder_sz = 1;
    g_tags.workspace_folders = lsp_malloc(sizeof(workspace_folder_t));
    g_tags.workspace_folders[0].name = lsp_strdup("");
    g_tags.workspace_folders[0].uri = lsp_strdup(path);
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
            workspace_folder_t* new_folder = lsp_realloc(g_tags.workspace_folders, sizeof(workspace_folder_t) * new_sz);

            g_tags.workspace_folders = new_folder;
            g_tags.workspace_folder_sz = new_sz;

            g_tags.workspace_folders[g_tags.workspace_folder_sz - 1].name = lsp_strdup(name);
            g_tags.workspace_folders[g_tags.workspace_folder_sz - 1].uri = lsp_strdup(uri);
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

static void _lsp_method_initialize_generate_rsp(cJSON* rsp)
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
        cJSON_AddStringToObject(server_info, "name", TAGS_LSP_PROG_NAME);
        cJSON_AddStringToObject(server_info, "version", tag_lsp_version());
        cJSON_AddItemToObject(InitializeResult, "serverInfo", server_info);
    }

    cJSON_AddItemToObject(rsp, "result", InitializeResult);
}

static void _lsp_method_gtags_version(const char* data, size_t size, void* arg)
{
    (void)size; (void)arg;
    LSP_LOG(LSP_MSG_DEBUG, "gtags information:\n%s", data);
}

static int _lsp_method_init_check_gtags(void)
{
    char* args[] = { "gtags", "--version", NULL };
    int ret = lsp_execute("gtags", args, NULL, _lsp_method_gtags_version, NULL);

    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

static void _lsp_method_init_update_tags(void)
{
    size_t i;
    for (i = 0; i < g_tags.workspace_folder_sz; i++)
    {
        lsp_task_update_tags(g_tags.workspace_folders[i].uri);
    }
}

static int _lsp_method_initialize(cJSON* req, cJSON* rsp)
{
    int ret;
    cJSON* params = cJSON_GetObjectItem(req, "params");

    if ((ret = _lsp_method_init_check_gtags()) != 0)
    {
        goto error;
    }

    if ((ret = _lsp_method_init_workspace_folders(params)) != 0)
    {
        goto error;
    }

    if ((ret = _lsp_method_init_client_capabilities(params)) != 0)
    {
        goto error;
    }

    _lsp_method_initialize_generate_rsp(rsp);
    _lsp_method_init_update_tags();
    g_tags.flags.initialized = 1;

    return 0;

error:
    {
        cJSON* err_dat = cJSON_CreateObject();
        cJSON_AddBoolToObject(err_dat, "retry", 0);
        lsp_set_error(rsp, ret, err_dat);
    }

    return 0;
}

lsp_method_t lsp_method_initialize = {
    "initialize", 0, _lsp_method_initialize, NULL,
};
