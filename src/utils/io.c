#include <stdlib.h>
#include "io.h"
#include "runtime.h"
#include "utils/alloc.h"
#include "utils/log.h"

#if !defined(WIN32)
#   include <unistd.h>
#endif

typedef struct tag_lsp_io
{
    tag_lsp_io_type_t   type;

    tag_lsp_io_cb       cb;

    union
    {
        struct
        {
            uv_tty_t    tty_in;     /**< Stdin. */
            uv_tty_t    tty_out;    /**< Stdout. */
            int         cnt_close;  /**< Close counter. */
        } as_stdio;

        struct
        {
            uv_pipe_t   pip;
        } as_pipe;

        struct
        {
            uv_tcp_t    sock;
        } as_socket;
    } backend;
} tag_lsp_io_t;

static tag_lsp_io_t* s_io_ctx = NULL;

static void _on_tty_stdin(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    (void)stream;

    if (nread >= 0)
    {
        buf->base[nread] = '\0';
    }

    s_io_ctx->cb(buf->base, nread);

    lsp_free(buf->base);
}

static int _tag_lsp_init_io_as_stdio(tag_lsp_io_cfg_t* cfg)
{
    (void)cfg;

    int ret;
    s_io_ctx->backend.as_stdio.cnt_close = 0;

    ret = uv_tty_init(g_tags.loop, &s_io_ctx->backend.as_stdio.tty_in, STDIN_FILENO, 0);
    if (ret != 0)
    {
        fprintf(stderr, "initialize tty_in failed: %s (%d): stdin fileno: %d\n",
            uv_strerror(ret), ret, STDIN_FILENO);
        abort();
    }

    ret = uv_tty_init(g_tags.loop, &s_io_ctx->backend.as_stdio.tty_out, STDOUT_FILENO, 0);
    if (ret != 0)
    {
        abort();
    }

    ret = uv_read_start((uv_stream_t*)&s_io_ctx->backend.as_stdio.tty_in, tag_lsp_alloc, _on_tty_stdin);
    if (ret != 0)
    {
        abort();
    }

    return 0;
}

static void _tag_lsp_on_pipe_connect(uv_connect_t* req, int status)
{
    free(req);

    if (status != 0)
    {
        fprintf(stderr, "connect to pipe failed.\n");
        exit(EXIT_FAILURE);
    }

    if (uv_read_start((uv_stream_t*)&s_io_ctx->backend.as_pipe.pip, tag_lsp_alloc, _on_tty_stdin) != 0)
    {
        abort();
    }
}

static int _tag_lsp_init_io_as_pipe(tag_lsp_io_cfg_t* cfg)
{
    int ret;

    if ((ret = uv_pipe_init(g_tags.loop, &s_io_ctx->backend.as_pipe.pip, 0)) != 0)
    {
        abort();
    }

    uv_connect_t* req = malloc(sizeof(uv_connect_t));
    uv_pipe_connect(req, &s_io_ctx->backend.as_pipe.pip, cfg->data.file, _tag_lsp_on_pipe_connect);

    return 0;
}

static void _tag_lsp_on_socket_connect(uv_connect_t* req, int status)
{
    free(req);

    if (status != 0)
    {
        fprintf(stderr, "connect to socket failed.\n");
        exit(EXIT_FAILURE);
    }

    if (uv_read_start((uv_stream_t*)&s_io_ctx->backend.as_socket.sock, tag_lsp_alloc, _on_tty_stdin) != 0)
    {
        abort();
    }
}

static int _tag_lsp_init_io_as_socket(tag_lsp_io_cfg_t* cfg)
{
    int ret;

    if ((ret = uv_tcp_init(g_tags.loop, &s_io_ctx->backend.as_socket.sock)) != 0)
    {
        abort();
    }

    uv_connect_t* req = malloc(sizeof(uv_connect_t));

    struct sockaddr_in addr;
    if ((ret = uv_ip4_addr("127.0.0.1", cfg->data.port, &addr)) != 0)
    {
        abort();
    }

    ret = uv_tcp_connect(req, &s_io_ctx->backend.as_socket.sock,
        (struct sockaddr*)&addr, _tag_lsp_on_socket_connect);
    if (ret != 0)
    {
        abort();
    }

    return 0;
}

void tag_lsp_io_init(tag_lsp_io_cfg_t* cfg)
{
    if (s_io_ctx != NULL)
    {
        return;
    }

    if ((s_io_ctx = malloc(sizeof(tag_lsp_io_t))) == NULL)
    {
        fprintf(stderr, "out of memory.\n");
        exit(EXIT_FAILURE);
    }

    s_io_ctx->type = cfg->type;
    s_io_ctx->cb = cfg->cb;

    switch (cfg->type)
    {
    case TAG_LSP_IO_STDIO:
        LSP_LOG(LSP_MSG_DEBUG, "initialize stdio.");
        _tag_lsp_init_io_as_stdio(cfg);
        return;

    case TAG_LSP_IO_PIPE:
        LSP_LOG(LSP_MSG_DEBUG, "initialize pipe.");
        _tag_lsp_init_io_as_pipe(cfg);
        return;

    case TAG_LSP_IO_PORT:
        LSP_LOG(LSP_MSG_DEBUG, "initialize socket.");
        _tag_lsp_init_io_as_socket(cfg);
        return;

    default:
        break;
    }

    abort();
}

static void _tag_lsp_on_pipe_close(uv_handle_t* handle)
{
    (void)handle;

    free(s_io_ctx);
    s_io_ctx = NULL;
}

static void _tag_lsp_on_socket_close(uv_handle_t* handle)
{
    _tag_lsp_on_pipe_close(handle);
}

static void _tag_lsp_on_stdio_close(uv_handle_t* handle)
{
    s_io_ctx->backend.as_stdio.cnt_close++;

    if (s_io_ctx->backend.as_stdio.cnt_close != 2)
    {
        return;
    }

    _tag_lsp_on_pipe_close(handle);
}

static void _tag_lsp_io_exit_as_stdio(void)
{
    uv_close((uv_handle_t*)&s_io_ctx->backend.as_stdio.tty_in, _tag_lsp_on_stdio_close);
    uv_close((uv_handle_t*)&s_io_ctx->backend.as_stdio.tty_out, _tag_lsp_on_stdio_close);
}

static void _tag_lsp_io_exit_as_pipe(void)
{
    uv_close((uv_handle_t*)&s_io_ctx->backend.as_pipe.pip, _tag_lsp_on_pipe_close);
}

static void _tag_lsp_io_exit_as_socket(void)
{
    uv_close((uv_handle_t*)&s_io_ctx->backend.as_socket.sock, _tag_lsp_on_socket_close);
}

void tag_lsp_io_exit(void)
{
    if (s_io_ctx == NULL)
    {
        return;
    }

    switch (s_io_ctx->type)
    {
    case TAG_LSP_IO_STDIO:
        _tag_lsp_io_exit_as_stdio();
        return;

    case TAG_LSP_IO_PIPE:
        _tag_lsp_io_exit_as_pipe();
        return;

    case TAG_LSP_IO_PORT:
        _tag_lsp_io_exit_as_socket();
        return;

    default:
        break;
    }

    abort();
}

int tag_lsp_io_write(uv_write_t* req, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb)
{
    uv_stream_t* handle = NULL;
    switch (s_io_ctx->type)
    {
    case TAG_LSP_IO_STDIO:
        handle = (uv_stream_t*)&s_io_ctx->backend.as_stdio.tty_out;
        break;

    case TAG_LSP_IO_PIPE:
        handle = (uv_stream_t*)&s_io_ctx->backend.as_pipe.pip;
        break;

    case TAG_LSP_IO_PORT:
        handle = (uv_stream_t*)&s_io_ctx->backend.as_socket.sock;
        break;

    default:
        abort();
    }

    return uv_write(req, handle, bufs, nbufs, cb);
}
