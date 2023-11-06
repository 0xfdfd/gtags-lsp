#ifndef __TAGS_LSP_UTILS_LOG_H__
#define __TAGS_LSP_UTILS_LOG_H__

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

void tag_lsp_log_init(void);
void tag_lsp_log_exit(void);

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
