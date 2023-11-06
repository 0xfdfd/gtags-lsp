#include <uv.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "lsp_msg.h"
#include "runtime.h"
#include "method/__init__.h"
#include "utils/list.h"
#include "utils/lsp_error.h"
#include "utils/io.h"

typedef struct lsp_msg_ele
{
    ev_list_node_t              node;
    uv_write_t                  req;
    uv_buf_t                    bufs[2];
} lsp_msg_ele_t;

typedef struct lsp_msg_req
{
    ev_list_node_t              node;
    cJSON*                      req;
    cJSON*                      rsp;
    uv_sem_t                    sem;
} lsp_msg_req_t;

typedef struct lsp_msg_ctx
{
    ev_list_t                   msg_queue;          /**< Send queue. */
    uv_mutex_t                  msg_queue_mutex;    /**< Mutex for send queue. */
    uv_async_t                  msg_queue_notifier; /**< Notifier for send queue. */

    ev_list_t                   req_queue;
    uv_mutex_t                  req_queue_mutex;

    uint64_t                    req_id;
    uv_mutex_t                  req_id_mutex;
} lsp_msg_ctx_t;

static lsp_msg_ctx_t*           s_msg_ctx = NULL;

static lsp_msg_ele_t* _lsp_msg_get_one(void)
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

    return container_of(node, lsp_msg_ele_t, node);
}

static void _runtime_release_send_buf(lsp_msg_ele_t* req)
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

    lsp_msg_ele_t* impl = container_of(req, lsp_msg_ele_t, req);
    _runtime_release_send_buf(impl);
}

static void _lsp_msg_on_notify(uv_async_t* handle)
{
    (void)handle;

    int ret;
    lsp_msg_ele_t* rsp;

    while ((rsp = _lsp_msg_get_one()) != NULL)
    {
        ret = tag_lsp_io_write(&rsp->req, rsp->bufs, 2, _on_tty_stdout);
        if (ret != 0)
        {
            _runtime_release_send_buf(rsp);

            fprintf(stderr, "post write request failed.\n");
            uv_stop(g_tags.loop);
            return;
        }
    }
}

static void _lsp_msg_on_notifier_close(uv_handle_t* handle)
{
    (void)handle;

    uv_mutex_destroy(&s_msg_ctx->msg_queue_mutex);
    uv_mutex_destroy(&s_msg_ctx->req_queue_mutex);
    uv_mutex_destroy(&s_msg_ctx->req_id_mutex);

    free(s_msg_ctx);
    s_msg_ctx = NULL;
}

static uint64_t _tag_lsp_get_new_id(void)
{
    uint64_t id;
    uv_mutex_lock(&s_msg_ctx->req_id_mutex);
    {
        id = s_msg_ctx->req_id++;
    }
    uv_mutex_unlock(&s_msg_ctx->req_id_mutex);

    return id;
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

    ev_list_init(&s_msg_ctx->req_queue);
    uv_mutex_init(&s_msg_ctx->req_queue_mutex);

    s_msg_ctx->req_id = 0;
    uv_mutex_init(&s_msg_ctx->req_id_mutex);

    return 0;
}

void tag_lsp_msg_exit(void)
{
    if (s_msg_ctx == NULL)
    {
        return;
    }

    uv_close((uv_handle_t*)&s_msg_ctx->msg_queue_notifier, _lsp_msg_on_notifier_close);
}

lsp_msg_type_t lsp_msg_type(cJSON* msg)
{
    cJSON* j_method = cJSON_GetObjectItem(msg, "method");
    if (j_method == NULL)
    {
        return LSP_MSG_RSP;
    }

    cJSON* j_id = cJSON_GetObjectItem(msg, "id");
    if (j_id == NULL)
    {
        return LSP_MSG_NFY;
    }

    return LSP_MSG_REQ;
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

cJSON* tag_lsp_create_request(const char* method, cJSON* params)
{
    char buffer[32];
    uint64_t id = _tag_lsp_get_new_id();
    snprintf(buffer, sizeof(buffer), "%" PRIu64, id);

    cJSON* req = tag_lsp_create_notify(method, params);
    cJSON_AddStringToObject(req, "id", buffer);
    return req;
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

int tag_lsp_send_rsp(cJSON* msg)
{
    lsp_msg_ele_t* stdout_req = malloc(sizeof(lsp_msg_ele_t));
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
    tag_lsp_send_rsp(rsp);
    cJSON_Delete(rsp);
    return 0;
}

static void _lsp_method_on_work(uv_work_t* req)
{
    int ret = 0;
    tag_lsp_work_t* work = container_of(req, tag_lsp_work_t, token);

    /* Check whether it is a notify. */
    if (!work->notify)
    {
        work->rsp = tag_lsp_create_rsp_from_req(work->req);
    }

    /* Reject any request if we are shutdown. */
    if (!work->notify && g_tags.flags.shutdown)
    {
        tag_lsp_set_error(work->rsp, TAG_LSP_ERR_INVALID_REQUEST, NULL);
        return;
    }

    /* Call method. */
    ret = lsp_method_call(work->req, work->rsp, work->notify);
    if (!work->notify && ret < 0)
    {
        tag_lsp_set_error(work->rsp, ret, NULL);
        return;
    }
    else if (!work->notify && ret == LSP_METHOD_ASYNC)
    {
        work->rsp = NULL;
    }
}

static void _lsp_method_after_work(uv_work_t* req, int status)
{
    tag_lsp_work_t* work = container_of(req, tag_lsp_work_t, token);

    if (!work->notify)
    {
        if (status != 0)
        {
            tag_lsp_send_error(work->req, TAG_LSP_ERR_REQUEST_CANCELLED);
        }
        else
        {
            tag_lsp_send_rsp(work->rsp);
        }
    }

    /* Remove record. */
    uv_mutex_lock(&g_tags.work_queue_mutex);
    ev_list_erase(&g_tags.work_queue, &work->node);
    uv_mutex_unlock(&g_tags.work_queue_mutex);

    if (work->req != NULL)
    {
        cJSON_Delete(work->req);
        work->req = NULL;
    }
    if (work->rsp != NULL)
    {
        cJSON_Delete(work->rsp);
        work->rsp = NULL;
    }
    free(work);
}

void tag_lsp_handle_req(cJSON* req, int is_notify)
{
    tag_lsp_work_t* work = malloc(sizeof(tag_lsp_work_t));
    if (work == NULL)
    {
        abort();
    }

    work->req = cJSON_Duplicate(req, 1);
    work->rsp = NULL;
    work->notify = is_notify;
    work->cancel = 0;

    uv_mutex_lock(&g_tags.work_queue_mutex);
    ev_list_push_back(&g_tags.work_queue, &work->node);
    uv_mutex_unlock(&g_tags.work_queue_mutex);

    int ret = uv_queue_work(g_tags.loop, &work->token, _lsp_method_on_work, _lsp_method_after_work);
    if (ret != 0)
    {
        abort();
    }
}

void tag_lsp_handle_rsp(cJSON* rsp)
{
    ev_list_node_t* it;
    cJSON* j_id = cJSON_GetObjectItem(rsp, "id");

    uv_mutex_lock(&s_msg_ctx->req_queue_mutex);
    {
        it = ev_list_begin(&s_msg_ctx->req_queue);
        for (; it != NULL; it = ev_list_next(it))
        {
            lsp_msg_req_t* msg_req = container_of(it, lsp_msg_req_t, node);
            cJSON* orig_id = cJSON_GetObjectItem(msg_req->req, "id");
            if (cJSON_Compare(j_id, orig_id, 0))
            {
                msg_req->rsp = cJSON_Duplicate(rsp, 1);
                uv_sem_post(&msg_req->sem);
                break;
            }
        }
    }
    uv_mutex_unlock(&s_msg_ctx->req_queue_mutex);
}

cJSON* tag_lsp_send_req(cJSON* req)
{
    lsp_msg_req_t msg_req;
    msg_req.req = req;
    msg_req.rsp = NULL;
    uv_sem_init(&msg_req.sem, 0);

    uv_mutex_lock(&s_msg_ctx->req_queue_mutex);
    {
        ev_list_push_back(&s_msg_ctx->req_queue, &msg_req.node);
    }
    uv_mutex_unlock(&s_msg_ctx->req_queue_mutex);

    tag_lsp_send_rsp(req);

    uv_sem_wait(&msg_req.sem);

    uv_mutex_lock(&s_msg_ctx->req_queue_mutex);
    {
        ev_list_erase(&s_msg_ctx->req_queue, &msg_req.node);
    }
    uv_mutex_unlock(&s_msg_ctx->req_queue_mutex);

    return msg_req.rsp;
}
