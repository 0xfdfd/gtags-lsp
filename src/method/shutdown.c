#include <stdlib.h>
#include "__init__.h"
#include "runtime.h"
#include "utils/lsp_error.h"
#include "utils/lsp_msg.h"
#include "utils/lsp_work.h"
#include "utils/alloc.h"
#include "utils/log.h"

typedef struct lsp_shutdown_ctx
{
    cJSON*      rsp;
    uv_thread_t tid;
} lsp_shutdown_ctx_t;

static lsp_shutdown_ctx_t* s_shutdown_ctx = NULL;

static void _lsp_shutdown_thread(void* arg)
{
    (void)arg;

    /* Waiting for all task complete. */
    while (lsp_work_queue_size() != 0)
    {
        uv_sleep(100);
    }

    /* Set response. */
    cJSON_AddNullToObject(s_shutdown_ctx->rsp, "result");

    lsp_send_rsp(s_shutdown_ctx->rsp);

    cJSON_Delete(s_shutdown_ctx->rsp);
    s_shutdown_ctx->rsp = NULL;
}

static int _lsp_method_shutdown(cJSON* req, cJSON* rsp)
{
    (void)req;

    int ret;

    /* Set global shutdown flag. */
    g_tags.flags.shutdown = 1;

    if (s_shutdown_ctx != NULL)
    {
        return TAG_LSP_ERR_INVALID_REQUEST;
    }

    s_shutdown_ctx = lsp_malloc(sizeof(lsp_shutdown_ctx_t));
    s_shutdown_ctx->rsp = rsp;

    /* Create new thread to wait for other tasks. */
    ret = uv_thread_create(&s_shutdown_ctx->tid, _lsp_shutdown_thread, NULL);
    if (ret != 0)
    {
        lsp_free(s_shutdown_ctx);
        s_shutdown_ctx = NULL;

        fprintf(stderr, "create shutdown thread failed.\n");
        exit(EXIT_FAILURE);
    }

    return LSP_METHOD_ASYNC;
}

static void _lsp_method_shutdown_cleanup(void)
{
    int ret;
    if (s_shutdown_ctx == NULL)
    {
        return;
    }

    if ((ret = uv_thread_join(&s_shutdown_ctx->tid)) != 0)
    {
        LSP_LOG(LSP_MSG_ERROR, "join shutdown thread failed.");
        abort();
    }

    if (s_shutdown_ctx->rsp != NULL)
    {
        cJSON_Delete(s_shutdown_ctx->rsp);
        s_shutdown_ctx->rsp = NULL;
    }

    lsp_free(s_shutdown_ctx);
    s_shutdown_ctx = NULL;
}

lsp_method_t lsp_method_shutdown = {
    "shutdown", 0,
    _lsp_method_shutdown, _lsp_method_shutdown_cleanup,
};
