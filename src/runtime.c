#include <string.h>
#include <stdlib.h>
#include "runtime.h"
#include "utils/lsp_error.h"

typedef struct tty_stdout_req
{
    uv_write_t      req;
    uv_buf_t        bufs[2];
} tty_stdout_req_t;

tags_ctx_t g_tags; /**< Global runtime. */

cJSON* tag_lsp_create_rsp_from_req(cJSON* req)
{
    cJSON* rsp = cJSON_CreateObject();

    cJSON* id = cJSON_GetObjectItem(req, "id");
    if (id != NULL)
    {
        cJSON_AddItemToObject(rsp, "id", cJSON_Duplicate(id, 1));
    }

    cJSON_AddStringToObject(rsp, "jsonrpc", "2.0");

    return rsp;
}

cJSON* tag_lsp_create_notify(const char* method, cJSON* params)
{
    cJSON* msg = cJSON_CreateObject();

    cJSON_AddStringToObject(msg, "jsonrpc", "2.0");
    cJSON_AddStringToObject(msg, "method", method);

    if (params != NULL)
    {
        cJSON_AddItemToObject(msg, "params", params);
    }

    return msg;
}

cJSON* tag_lsp_create_error_fomr_req(cJSON* req, int code, cJSON* data)
{
    cJSON* rsp = tag_lsp_create_rsp_from_req(req);

    tag_lsp_set_error(rsp, code, data);

    return rsp;
}

void tag_lsp_set_error(cJSON* rsp, int code, cJSON* data)
{
    cJSON* errobj = cJSON_CreateObject();
    cJSON_AddNumberToObject(errobj, "code", code);
    cJSON_AddStringToObject(errobj, "message", lsp_strerror(code));

    if (data != NULL)
    {
        cJSON_AddItemToObject(errobj, "data", data);
    }

    cJSON_AddItemToObject(rsp, "error", errobj);
}

static void _runtime_release_send_buf(tty_stdout_req_t* req)
{
    free(req->bufs[0].base);
    cJSON_free(req->bufs[1].base);
    free(req);
}

static void _on_tty_stdout(uv_write_t* req, int status)
{
    if (status != 0)
    {
        uv_stop(g_tags.loop);
        return;
    }

    tty_stdout_req_t* impl = container_of(req, tty_stdout_req_t, req);
    _runtime_release_send_buf(impl);
}

int tag_lsp_send_msg(cJSON* msg)
{
    int ret;
    tty_stdout_req_t* stdout_req = malloc(sizeof(tty_stdout_req_t));
    if (stdout_req == NULL)
    {
        abort();
    }

    {
        char* dat = cJSON_PrintUnformatted(msg);
        unsigned int dat_sz = (unsigned int)strlen(dat);
        stdout_req->bufs[1] = uv_buf_init(dat, dat_sz);
    }

    size_t header_sz = 100;
    stdout_req->bufs[0].base = malloc(header_sz);

    stdout_req->bufs[0].len = snprintf(stdout_req->bufs[0].base, header_sz,
        "Content-Length:%lu\r\n"
        "Content-Type:application/vscode-jsonrpc; charset=utf-8\r\n\r\n",
        stdout_req->bufs[1].len);

    ret = tag_lsp_io_write(&stdout_req->req, stdout_req->bufs, 2, _on_tty_stdout);
    if (ret != 0)
    {
        _runtime_release_send_buf(stdout_req);
        return ret;
    }

    return 0;
}

int tag_lsp_send_error(cJSON* req, int code)
{
    cJSON* rsp = tag_lsp_create_error_fomr_req(req, code, NULL);
    tag_lsp_send_msg(rsp);
    cJSON_Delete(rsp);
    return 0;
}

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
