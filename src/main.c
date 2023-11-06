#include <stdlib.h>
#include <string.h>
#include "method/__init__.h"
#include "utils/lsp_error.h"
#include "utils/lsp_msg.h"
#include "utils/io.h"
#include "utils/log.h"
#include "runtime.h"

#if defined(WIN32)
#   define sscanf(str, fmt, ...)    sscanf_s(str, fmt, ##__VA_ARGS__)
#endif

static const char* s_help =
"tags-lsp - Language server protocol wrapper for gtags.\n"
"Usage: tags-lsp [OPTIONS]\n"
"\n"
"OPTIONS:\n"
"  --stdio\n"
"    Uses stdio as the communication channel. If no option specific, use this as\n"
"    default option.\n"
"\n"
"  --pipe=[FILE]\n"
"    Use pipes (Windows) or socket files (Linux, Mac) as the communication channel.\n"
"\n"
"  --port=[NUMBER]\n"
"    Uses a socket as the communication channel.\n"
"\n"
"  -h, --help\n"
"    Show this help and exit.\n";

static void _cleanup_loop(void)
{
    if (g_tags.loop == NULL)
    {
        return;
    }

    /* Ensure all handle closed. */
    if (uv_run(g_tags.loop, UV_RUN_DEFAULT) != 0)
    {
        abort();
    }

    /* Close loop. */
    if (uv_loop_close(g_tags.loop) != 0)
    {
        abort();
    }

    free(g_tags.loop);
    g_tags.loop = NULL;
}

static void _at_exit_stage_1(void)
{
    tag_lsp_msg_exit();
    tag_lsp_io_exit();
    tag_lsp_log_exit();
}

static void _at_exit_stage_2(void)
{
    if (g_tags.parser != NULL)
    {
        lsp_parser_destroy(g_tags.parser);
        g_tags.parser = NULL;
    }

    uv_mutex_destroy(&g_tags.work_queue_mutex);
    tag_lsp_cleanup_workspace_folders();
    tag_lsp_cleanup_client_capabilities();
}

static void _at_exit(void)
{
    lsp_method_cleanup();

    _at_exit_stage_1();
    _cleanup_loop();
    _at_exit_stage_2();

    uv_library_shutdown();
}

static int _handle_request(lsp_parser_t* parser, cJSON* msg)
{
    (void)parser;

    lsp_msg_type_t msg_type = lsp_msg_type(msg);

    if (msg_type == LSP_MSG_RSP)
    {
        tag_lsp_handle_rsp(msg);
    }
    else
    {
        tag_lsp_handle_req(msg, msg_type == LSP_MSG_NFY);
    }

    return 0;
}

static void _on_io_in(const char* data, ssize_t size)
{
    if (size < 0)
    {
        uv_stop(g_tags.loop);
        return;
    }

    lsp_parser_execute(g_tags.parser, data, size);
}

static void _setup_io_or_help(char* argv[])
{
    size_t i;
    int ret = 0;
    const char* opt = NULL;

    tag_lsp_io_cfg_t io_cfg;
    memset(&io_cfg, 0, sizeof(io_cfg));
    io_cfg.cb = _on_io_in;

    for (i = 0; argv[i] != NULL; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            fprintf(stderr, "%s", s_help);
            exit(EXIT_SUCCESS);
        }

        if (strcmp(argv[i], "--stdio") == 0)
        {
            goto init_io_as_stdio;
        }

        opt = "--pipe=";
        if (strncmp(argv[i], opt, strlen(opt)) == 0)
        {
            opt = argv[i] + strlen(opt);
            goto init_io_as_pipe;
        }

        opt = "--port=";
        if (strncmp(argv[i], opt, strlen(opt)) == 0)
        {
            opt = argv[i] + strlen(opt);
            if (sscanf(opt, "%d", &ret) != 1)
            {
                fprintf(stderr, "invalid value for `--port`: %s.\n", opt);
                exit(EXIT_FAILURE);
            }
            goto init_io_as_socket;
        }
    }

init_io_as_stdio:
    io_cfg.type = TAG_LSP_IO_STDIO;
    goto finish;

init_io_as_pipe:
    io_cfg.type = TAG_LSP_IO_PIPE;
    io_cfg.data.file = opt;
    goto finish;

init_io_as_socket:
    io_cfg.type = TAG_LSP_IO_PORT;
    io_cfg.data.port = ret;
    goto finish;

finish:
    if ((ret = tag_lsp_io_init(&io_cfg)) != 0)
    {
        abort();
    }
}

static char** _initialize(int argc, char* argv[])
{
    argv = uv_setup_args(argc, argv);
    uv_disable_stdio_inheritance();

    /* Initialize event loop. */
    g_tags.loop = malloc(sizeof(uv_loop_t));
    if (uv_loop_init(g_tags.loop) != 0)
    {
        abort();
    }

    _setup_io_or_help(argv);

    ev_list_init(&g_tags.work_queue);
    uv_mutex_init(&g_tags.work_queue_mutex);

    tag_lsp_msg_init();
    tag_lsp_log_init();

    g_tags.parser = lsp_parser_create(_handle_request);

    return argv;
}

int main(int argc, char* argv[])
{
    /* Register global cleanup hook. */
    atexit(_at_exit);

    /* Global initialize. */
    argv = _initialize(argc, argv);

    /* Run. */
    uv_run(g_tags.loop, UV_RUN_DEFAULT);

    return 0;
}
