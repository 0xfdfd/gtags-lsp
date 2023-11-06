#include <uv.h>
#include <stdlib.h>
#include "lsp_work.h"
#include "runtime.h"

typedef struct lsp_work_ctx
{
    ev_list_t           work_queue;             /**< #tag_lsp_work_t */
    uv_mutex_t          work_queue_mutex;       /**< Mutex for #tags_ctx_t::work_queue */
    uv_async_t          work_queue_notifier;    /**< Notifier */
} lsp_work_ctx_t;

static lsp_work_ctx_t*  s_work_ctx = NULL;

static lsp_work_t* _lsp_work_get_one(void)
{
    ev_list_node_t* node;

    uv_mutex_lock(&s_work_ctx->work_queue_mutex);
    {
        node = ev_list_pop_front(&s_work_ctx->work_queue);
    }
    uv_mutex_unlock(&s_work_ctx->work_queue_mutex);

    if (node == NULL)
    {
        return NULL;
    }

    return container_of(node, lsp_work_t, node);
}

static void _lsp_on_work(uv_work_t* req)
{
    lsp_work_t* work = container_of(req, lsp_work_t, token);

    work->work_cb(work);
}

static void _lsp_on_after_work(uv_work_t* req, int status)
{
    lsp_work_t* work = container_of(req, lsp_work_t, token);

    uv_mutex_lock(&s_work_ctx->work_queue_mutex);
    {
        ev_list_erase(&s_work_ctx->work_queue, &work->node);
    }
    uv_mutex_unlock(&s_work_ctx->work_queue_mutex);

    work->after_work_cb(work, status);
}

static void _lsp_on_work_queue(uv_async_t* handle)
{
    (void)handle;

    int ret;
    lsp_work_t* work;

    while ((work = _lsp_work_get_one()) != NULL)
    {
        ret = uv_queue_work(g_tags.loop, &work->token, _lsp_on_work, _lsp_on_after_work);
        if (ret != 0)
        {
            abort();
        }
    }
}

static void _lsp_work_on_close(uv_handle_t* handle)
{
    (void)handle;

    uv_mutex_destroy(&s_work_ctx->work_queue_mutex);

    free(s_work_ctx);
    s_work_ctx = NULL;
}

void lsp_work_init(void)
{
    if ((s_work_ctx = malloc(sizeof(lsp_work_ctx_t))) == NULL)
    {
        fprintf(stderr, "out of memory.\n");
        abort();
    }

    ev_list_init(&s_work_ctx->work_queue);
    uv_mutex_init(&s_work_ctx->work_queue_mutex);
    uv_async_init(g_tags.loop, &s_work_ctx->work_queue_notifier, _lsp_on_work_queue);
}

void lsp_work_exit(void)
{
    if (s_work_ctx == NULL)
    {
        return;
    }

    uv_close((uv_handle_t*)&s_work_ctx->work_queue_notifier, _lsp_work_on_close);
}

void lsp_queue_work(lsp_work_t* work, lsp_work_type_t type, lsp_work_cb work_cb, lsp_after_work_cb after_work_cb)
{
    work->type = type;
    work->work_cb = work_cb;
    work->after_work_cb = after_work_cb;

    uv_mutex_lock(&s_work_ctx->work_queue_mutex);
    {
        ev_list_push_back(&s_work_ctx->work_queue, &work->node);
    }
    uv_mutex_unlock(&s_work_ctx->work_queue_mutex);

    uv_async_send(&s_work_ctx->work_queue_notifier);
}

void lsp_work_cancel(lsp_work_t* work)
{
    uv_cancel((uv_req_t*)&work->token);
}

size_t lsp_work_queue_size(void)
{
    return ev_list_size(&s_work_ctx->work_queue);
}

void lsp_work_foreach(lsp_foreach_cb cb, void* arg)
{
    ev_list_node_t* it;

    uv_mutex_lock(&s_work_ctx->work_queue_mutex);
    {
        it = ev_list_begin(&s_work_ctx->work_queue);
        for (; it != NULL; it = ev_list_next(it))
        {
            lsp_work_t* work = container_of(it, lsp_work_t, node);
            if (cb(work, arg) != 0)
            {
                break;
            }
        }
    }
    uv_mutex_unlock(&s_work_ctx->work_queue_mutex);
}
