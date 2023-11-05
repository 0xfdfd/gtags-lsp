#include "method/__init__.h"
#include "runtime.h"
#include "utils/lsp_error.h"

static void _at_exit(void)
{
    uv_close((uv_handle_t*)&g_tags.tty_stdin, NULL);
    uv_close((uv_handle_t*)&g_tags.tty_stdout, NULL);

    /* Ensure all handle closed. */
    if (uv_run(&g_tags.loop, UV_RUN_DEFAULT) != 0)
    {
        abort();
    }

    /* Close loop. */
    if (uv_loop_close(&g_tags.loop) != 0)
    {
        abort();
    }

    lsp_parser_exit(&g_tags.parser);
    uv_mutex_destroy(&g_tags.work_queue_mutex);
    tag_lsp_cleanup_workspace_folders();
    tag_lsp_cleanup_client_capabilities();
    uv_library_shutdown();
}

static int _handle_request(lsp_parser_t* parser, cJSON* req)
{
    (void)parser;

    lsp_method_call(req);

    return 0;
}

static char** _initialize(int argc, char* argv[])
{
    argv = uv_setup_args(argc, argv);

    /* Initialize event loop. */
    if (uv_loop_init(&g_tags.loop) != 0)
    {
        abort();
    }

    if (uv_tty_init(&g_tags.loop, &g_tags.tty_stdin, STDIN_FILENO, 0) != 0)
    {
        abort();
    }

    if (uv_tty_init(&g_tags.loop, &g_tags.tty_stdout, STDOUT_FILENO, 0) != 0)
    {
        abort();
    }

    ev_list_init(&g_tags.work_queue);
    uv_mutex_init(&g_tags.work_queue_mutex);

    lsp_parser_init(&g_tags.parser, _handle_request);

    return argv;
}

static void my_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    (void)handle;
    char* addr = malloc(suggested_size);
    *buf = uv_buf_init(addr, (unsigned int)suggested_size);
}

static void _on_tty_stdin(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    (void)stream;

    if (nread < 0)
    {
        uv_stop(&g_tags.loop);
        goto finish;
    }

    lsp_parser_execute(&g_tags.parser, buf->base, nread);

finish:
    free(buf->base);
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

    /* Global initialize. */
    argv = _initialize(argc, argv);

    /* Register global cleanup hook. */
    atexit(_at_exit);

    if (uv_read_start((uv_stream_t*)&g_tags.tty_stdin, my_alloc_cb, _on_tty_stdin) != 0)
    {
        abort();
    }

    /* Run. */
    uv_run(&g_tags.loop, UV_RUN_DEFAULT);

    return 0;
}
