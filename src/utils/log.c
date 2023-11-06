#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "runtime.h"
#include "log.h"
#include "utils/lsp_msg.h"

typedef struct tag_lsp_log_s
{
    ev_list_node_t              node;                   /**< List node for #tags_ctx_t::log_queue */
    lsp_trace_message_type_t    type;                   /**< Message type. */
    const char*                 file;                   /**< File name. */
    const char*                 func;                   /**< Function name. */
    int                         line;                   /**< Line number. */
    char*                       message;                /**< The actual message. */
} tag_lsp_log_t;

typedef struct tag_lsp_log_ctx
{
    ev_list_t                   log_queue;              /**< log queue. */
    uv_mutex_t                  log_queue_mutex;        /**< Mutex for #tags_ctx_t::log_queue */
    uv_async_t                  log_queue_notifier;     /**< Notifier for #tags_ctx_t::log_queue. */
} tag_lsp_log_ctx_t;

static tag_lsp_log_ctx_t* s_log_ctx = NULL;

static tag_lsp_log_t* _pop_log_from_queue(void)
{
    ev_list_node_t* node;
    uv_mutex_lock(&s_log_ctx->log_queue_mutex);
    {
        node = ev_list_pop_front(&s_log_ctx->log_queue);
    }
    uv_mutex_unlock(&s_log_ctx->log_queue_mutex);

    if (node == NULL)
    {
        return NULL;
    }

    return container_of(node, tag_lsp_log_t, node);
}

static const char* _log_filename(const char* file)
{
    const char* pos = file;

    if (file == NULL)
    {
        return NULL;
    }

    for (; *file; ++file)
    {
        if (*file == '\\' || *file == '/')
        {
            pos = file + 1;
        }
    }
    return pos;
}

static void _log_post_lsp_log_message(tag_lsp_log_t* log)
{
    int len = snprintf(NULL, 0, "[%s:%d %s] %s",
        log->file, log->line, log->func, log->message);
    char* buf = malloc(len + 1);
    snprintf(buf, len + 1, "[%s:%d %s] %s",
        log->file, log->line, log->func, log->message);

    cJSON* params = cJSON_CreateObject();
    cJSON_AddNumberToObject(params, "type", log->type);
    cJSON_AddStringToObject(params, "message", buf);

    cJSON* msg = tag_lsp_create_notify("window/logMessage", params);

    tag_lsp_send_rsp(msg);
    cJSON_Delete(msg);

    free(buf);
}

static const char* _log_msg_type(lsp_trace_message_type_t type)
{
    switch (type)
    {
    case LSP_MSG_ERROR:
        return "E";

    case LSP_MSG_WARNING:
        return "W";

    case LSP_MSG_INFO:
        return "I";

    case LSP_MSG_LOG:
        return "L";

    default:
        break;
    }

    return "D";
}

static void _log_post_file_log_message(tag_lsp_log_t* log)
{
    const char* type = _log_msg_type(log->type);

    int len = snprintf(NULL, 0, "[%s %s:%d %s] %s\n",
        type, log->file, log->line, log->func, log->message);

    char* buf = malloc(len + 1);
    snprintf(buf, len + 1, "[%s %s:%d %s] %s\n",
        type, log->file, log->line, log->func, log->message);

    fprintf(stderr, "%s", buf);

    // TODO: write to log file.

    free(buf);
}

static void _on_log_queue(uv_async_t* handle)
{
    (void)handle;

    tag_lsp_log_t* log;
    while ((log = _pop_log_from_queue()) != NULL)
    {
        log->file = _log_filename(log->file);

        _log_post_lsp_log_message(log);
        _log_post_file_log_message(log);

        free(log);
    }
}

void tag_lsp_log_init(void)
{
    if ((s_log_ctx = malloc(sizeof(tag_lsp_log_ctx_t))) == NULL)
    {
        fprintf(stderr, "out of memory.\n");
        exit(EXIT_FAILURE);
    }

    ev_list_init(&s_log_ctx->log_queue);
    uv_mutex_init(&s_log_ctx->log_queue_mutex);
    uv_async_init(g_tags.loop, &s_log_ctx->log_queue_notifier, _on_log_queue);
}

static void _tag_lsp_on_log_exit(uv_handle_t* handle)
{
    (void)handle;

    tag_lsp_log_t* log;
    while ((log = _pop_log_from_queue()) != NULL)
    {
        free(log);
    }

    uv_mutex_destroy(&s_log_ctx->log_queue_mutex);

    free(s_log_ctx);
    s_log_ctx = NULL;
}

void tag_lsp_log_exit(void)
{
    if (s_log_ctx == NULL)
    {
        return;
    }

    uv_close((uv_handle_t*)&s_log_ctx->log_queue_notifier, _tag_lsp_on_log_exit);
}

void tag_lsp_log(lsp_trace_message_type_t type, const char* file, const char* func,
    int line, const char* fmt, ...)
{
    int msg_len = 0;

    va_list ap, ap_bak;
    va_start(ap, fmt);
    {
        va_copy(ap_bak, ap);

        msg_len = vsnprintf(NULL, 0, fmt, ap);
    }
    va_end(ap);

    size_t malloc_size = sizeof(tag_lsp_log_t) + msg_len + 1;
    tag_lsp_log_t* msg = malloc(malloc_size);
    if (msg == NULL)
    {
        abort();
    }

    msg->type = type;
    msg->file = file;
    msg->func = func;
    msg->line = line;
    msg->message = (char*)(msg + 1);

    vsnprintf(msg->message, msg_len + 1, fmt, ap_bak);
    va_end(ap_bak);

    uv_mutex_lock(&s_log_ctx->log_queue_mutex);
    {
        ev_list_push_back(&s_log_ctx->log_queue, &msg->node);
    }
    uv_mutex_unlock(&s_log_ctx->log_queue_mutex);

    uv_async_send(&s_log_ctx->log_queue_notifier);
}
