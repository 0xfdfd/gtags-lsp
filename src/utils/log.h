#ifndef __TAG_LSP_UTILS_LOG_H__
#define __TAG_LSP_UTILS_LOG_H__

/**
 * @brief Log message to log file and post `window/logMessage` if support.
 */
#define TAG_LSP_LOG(type, fmt, ...)   \
    tag_lsp_log(type, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#messageType
 */
typedef enum lsp_trace_message_type
{
    LSP_MSG_ERROR    = 1,
    LSP_MSG_WARNING  = 2,
    LSP_MSG_INFO     = 3,
    LSP_MSG_LOG      = 4,
    LSP_MSG_DEBUG    = 5,
} lsp_trace_message_type_t;

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

void tag_lsp_log_init(void);
void tag_lsp_log_exit_1(void);
void tag_lsp_log_exit_2(void);

/**
 * @brief Log message to log file and post `window/logMessage` if support.
 * @warning Always use #TAG_LSP_LOG().
 */
void tag_lsp_log(lsp_trace_message_type_t type, const char* file, const char* func,
    int line, const char* fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
