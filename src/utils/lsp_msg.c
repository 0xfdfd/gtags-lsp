#include <uv.h>
#include <string.h>
#include <stdlib.h>
#include "lsp_msg.h"
#include "runtime.h"
#include "utils/list.h"
#include "utils/lsp_error.h"
#include "utils/io.h"

typedef struct lsp_msg_req
{
    ev_list_node_t              node;
    uv_write_t                  req;
    uv_buf_t                    bufs[2];
} lsp_msg_req_t;

typedef struct lsp_msg_ctx
{
    ev_list_t                   msg_queue;
    uv_mutex_t                  msg_queue_mutex;
    uv_async_t                  msg_queue_notifier;
} lsp_msg_ctx_t;

static lsp_msg_ctx_t* s_msg_ctx = NULL;

static lsp_msg_req_t* _lsp_msg_get_one(void)
{
    ev_list_node_t* node;

    uv_mutex_lock(&s_msg_ctx->msg_queue_mutex);
    {
        node = ev_list_pop_front(&s_msg_ctx->msg_queue);
    }
    uv_mutex_unlock(&s_msg_ctx->msg_queue_mutex);

    if (node == NULL)
    {
        return NULL;
    }

    return container_of(node, lsp_msg_req_t, node);
}

static void _runtime_release_send_buf(lsp_msg_req_t* req)
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

    lsp_msg_req_t* impl = container_of(req, lsp_msg_req_t, req);
    _runtime_release_send_buf(impl);
}

static void _lsp_msg_on_notify(uv_async_t* handle)
{
    (void)handle;

    int ret;
    lsp_msg_req_t* req;

    while ((req = _lsp_msg_get_one()) != NULL)
    {
        ret = tag_lsp_io_write(&req->req, req->bufs, 2, _on_tty_stdout);
        if (ret != 0)
        {
            _runtime_release_send_buf(req);

            fprintf(stderr, "post write request failed.\n");
            uv_stop(g_tags.loop);
            return;
        }
    }
}

int tag_lsp_msg_init(void)
{
    if ((s_msg_ctx = malloc(sizeof(lsp_msg_ctx_t))) == NULL)
    {
        fprintf(stderr, "out of memory.\n");
        exit(EXIT_FAILURE);
    }

    ev_list_init(&s_msg_ctx->msg_queue);
    uv_mutex_init(&s_msg_ctx->msg_queue_mutex);
    uv_async_init(g_tags.loop, &s_msg_ctx->msg_queue_notifier, _lsp_msg_on_notify);

    return 0;
}

static void _lsp_msg_on_notifier_close(uv_handle_t* handle)
{
    (void)handle;

    uv_mutex_destroy(&s_msg_ctx->msg_queue_mutex);

    free(s_msg_ctx);
    s_msg_ctx = NULL;
}

void tag_lsp_msg_exit(void)
{
    if (s_msg_ctx == NULL)
    {
        return;
    }

    uv_close((uv_handle_t*)&s_msg_ctx->msg_queue_notifier, _lsp_msg_on_notifier_close);
}

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

cJSON* tag_lsp_create_error_fomr_req(cJSON* req, int code, cJSON* data)
{
    cJSON* rsp = tag_lsp_create_rsp_from_req(req);

    tag_lsp_set_error(rsp, code, data);

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

int tag_lsp_send_msg(cJSON* msg)
{
    lsp_msg_req_t* stdout_req = malloc(sizeof(lsp_msg_req_t));
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

    uv_mutex_lock(&s_msg_ctx->msg_queue_mutex);
    {
        ev_list_push_back(&s_msg_ctx->msg_queue, &stdout_req->node);
    }
    uv_mutex_unlock(&s_msg_ctx->msg_queue_mutex);

    uv_async_send(&s_msg_ctx->msg_queue_notifier);

    return 0;
}

int tag_lsp_send_error(cJSON* req, int code)
{
    cJSON* rsp = tag_lsp_create_error_fomr_req(req, code, NULL);
    tag_lsp_send_msg(rsp);
    cJSON_Delete(rsp);
    return 0;
}
