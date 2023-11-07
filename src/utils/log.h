#ifndef __TAGS_LSP_UTILS_LOG_H__
#define __TAGS_LSP_UTILS_LOG_H__

/**
 * @brief Log message to log file and post `window/logMessage` if support.
 */
#define LSP_LOG(type, fmt, ...)   \
    lsp_log(type, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

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

void lsp_log_init(void);
void lsp_log_exit(void);

/**
 * @brief Log message to log file and post `window/logMessage` if support.
 * @warning Always use #TAG_LSP_LOG().
 */
void lsp_log(lsp_trace_message_type_t type, const char* file, const char* func,
    int line, const char* fmt, ...);

/**
 * @brief Direct write data to logfile.
 * @param[in] data  Data to write.
 */
void lsp_direct_log(const char* data);

#ifdef __cplusplus
}
#endif
#endif
