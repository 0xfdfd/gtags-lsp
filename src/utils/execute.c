#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "execute.h"
#include "lsp_error.h"
#include "utils/alloc.h"
#include "defines.h"

typedef struct tag_lsp_execute_ctx
{
    uv_loop_t                   loop;
    uv_process_t                process;
    uv_process_options_t        option;
    uv_stdio_container_t        container[3];
    uv_pipe_t                   pip_stdout;

    lsp_execute_stdout_cb       cb;
    void*                       arg;
} tag_lsp_execute_ctx_t;

static void _tag_lsp_on_process_exit(uv_process_t* process, int64_t exit_status, int term_signal)
{
    (void)process; (void)exit_status; (void)term_signal;
}

static void _tag_lsp_on_stdout(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    tag_lsp_execute_ctx_t* exe_ctx = container_of((uv_pipe_t*)stream, tag_lsp_execute_ctx_t, pip_stdout);

    if (nread > 0 && exe_ctx->cb != NULL)
    {
        buf->base[nread] = '\0';
        exe_ctx->cb(buf->base, nread, exe_ctx->arg);
    }

    lsp_free(buf->base);
}

int lsp_execute(const char* file, char** args, const char* cwd,
    lsp_execute_stdout_cb cb, void* arg)
{
    int ret;

    tag_lsp_execute_ctx_t exe_ctx;
    memset(&exe_ctx, 0, sizeof(exe_ctx));
    exe_ctx.cb = cb;
    exe_ctx.arg = arg;

    if ((ret = uv_loop_init(&exe_ctx.loop)) != 0)
    {
        abort();
    }

    if ((ret = uv_pipe_init(&exe_ctx.loop, &exe_ctx.pip_stdout, 0)) != 0)
    {
        abort();
    }

    exe_ctx.option.exit_cb = _tag_lsp_on_process_exit;
    exe_ctx.option.file = file;
    exe_ctx.option.args = args;
    exe_ctx.option.cwd = cwd;
    exe_ctx.option.flags = UV_PROCESS_WINDOWS_HIDE;
    exe_ctx.option.stdio_count = 3;
    exe_ctx.option.stdio = exe_ctx.container;
    exe_ctx.container[1].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    exe_ctx.container[1].data.stream = (uv_stream_t*)&exe_ctx.pip_stdout;

    ret = uv_spawn(&exe_ctx.loop, &exe_ctx.process, &exe_ctx.option);
    if (ret != 0)
    {
        ret = TAG_LSP_ERR_REQUEST_FAILED;
        goto finish;
    }

    ret = uv_read_start((uv_stream_t*)&exe_ctx.pip_stdout, tag_lsp_alloc, _tag_lsp_on_stdout);
    if (ret != 0)
    {
        abort();
    }

    uv_run(&exe_ctx.loop, UV_RUN_DEFAULT);

finish:
    uv_close((uv_handle_t*)&exe_ctx.pip_stdout, NULL);
    uv_run(&exe_ctx.loop, UV_RUN_DEFAULT);
    if ((ret = uv_loop_close(&exe_ctx.loop)) != 0)
    {
        abort();
    }
    return ret;
}
