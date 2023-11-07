#include <uv.h>
#include <stdlib.h>
#include "lsp_work.h"
#include "runtime.h"
#include "utils/alloc.h"

struct lsp_work
{
    ev_list_node_t      node;
    uv_work_t           token;

    lsp_work_cb         cb;
    void*               arg;

    int                 cancel;
};

typedef struct lsp_work_ctx
{
    ev_list_t           pend_queue;             /**< #lsp_work_t */
    ev_list_t           work_queue;             /**< #lsp_work_t */
    uv_mutex_t          queue_mutex;            /**< Mutex for #tags_ctx_t::work_queue */

    uv_async_t          work_queue_notifier;    /**< Notifier */
} lsp_work_ctx_t;

static lsp_work_ctx_t*  s_work_ctx = NULL;

static void _lsp_on_work(uv_work_t* req)
{
    lsp_work_t* work = container_of(req, lsp_work_t, token);
    work->cb(work, work->cancel, work->arg);
}

static void _lsp_on_after_work(uv_work_t* req, int status)
{
    (void)status;
    lsp_work_t* work = container_of(req, lsp_work_t, token);

    uv_mutex_lock(&s_work_ctx->queue_mutex);
    {
        ev_list_erase(&s_work_ctx->work_queue, &work->node);
    }
    uv_mutex_unlock(&s_work_ctx->queue_mutex);

    lsp_free(work);
}

static int _lsp_work_queue_once(void)
{
    int ret = 0;
    ev_list_node_t* it;
    lsp_work_t* work;

    uv_mutex_lock(&s_work_ctx->queue_mutex);
    {
        it = ev_list_pop_front(&s_work_ctx->pend_queue);
        if (it != NULL)
        {
            work = container_of(it, lsp_work_t, node);
            ev_list_push_back(&s_work_ctx->work_queue, it);
            ret = uv_queue_work(g_tags.loop, &work->token, _lsp_on_work, _lsp_on_after_work);
        }
    }
    uv_mutex_unlock(&s_work_ctx->queue_mutex);

	if (ret != 0)
	{
		abort();
	}

    return it != NULL;
}

static void _lsp_on_work_queue(uv_async_t* handle)
{
    (void)handle;

    while (_lsp_work_queue_once())
    {
    }
}

static void _lsp_work_on_close(uv_handle_t* handle)
{
    (void)handle;

    uv_mutex_destroy(&s_work_ctx->queue_mutex);

    lsp_free(s_work_ctx);
    s_work_ctx = NULL;
}

void lsp_work_init(void)
{
    s_work_ctx = lsp_malloc(sizeof(lsp_work_ctx_t));

    ev_list_init(&s_work_ctx->pend_queue);
    ev_list_init(&s_work_ctx->work_queue);
    uv_mutex_init(&s_work_ctx->queue_mutex);
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

void lsp_queue_work(lsp_work_t** token, lsp_work_cb cb, void* arg)
{
    lsp_work_t* tmp_token = lsp_malloc(sizeof(lsp_work_t));
    tmp_token->cb = cb;
    tmp_token->arg = arg;
    tmp_token->cancel = 0;

    *token = tmp_token;

    uv_mutex_lock(&s_work_ctx->queue_mutex);
    {
        ev_list_push_back(&s_work_ctx->pend_queue, &tmp_token->node);
    }
    uv_mutex_unlock(&s_work_ctx->queue_mutex);

    uv_async_send(&s_work_ctx->work_queue_notifier);
}

void lsp_work_cancel(lsp_work_t* work)
{
    work->cancel = 1;
}

size_t lsp_work_queue_size(void)
{
    return ev_list_size(&s_work_ctx->work_queue) + ev_list_size(&s_work_ctx->pend_queue);
}
