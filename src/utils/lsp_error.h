#ifndef __TAGS_LSP_UTILS_ERROR_H__
#define __TAGS_LSP_UTILS_ERROR_H__

#define TAG_LSP_ERR_METHOD_NOT_FOUND        (-32601)
#define TAG_LSP_ERR_INVALID_REQUEST         (-32600)
#define TAG_LSP_ERR_INTERNAL_ERROR          (-32603)

/**
 * @brief Error code indicating that a server received a notification or request
 *   before the server has received the `initialize` request.
 */
#define TAG_LSP_ERR_SERVER_NOT_INITIALIZED  (-32002)

/**
 * @brief A request failed but it was syntactically correct, e.g the method name
 *   was known and the parameters were valid.
 *
 * The error message should contain human readable information about why the
 * request failed.
 */
#define TAG_LSP_ERR_REQUEST_FAILED          (-32803)

/**
 * @brief The server cancelled the request.
 *
 * This error code should only be used for requests that explicitly support
 * being server cancellable.
 */
#define TAG_LSP_ERR_SERVER_CANCELLED        (-32802)

/**
 * @brief The server detected that the content of a document got modified
 *   outside normal conditions.
 *
 * A server should NOT send this error code if it detects a content change in
 * it unprocessed messages. The result even computed on an older state might
 * still be useful for the client.
 *
 * If a client decides that a result is not of any use anymore the client should
 * cancel the request.
 */
#define TAG_LSP_ERR_CONTENT_MODIFIED        (-32801)

/**
 * @brief The client has canceled a request and a server as detected the cancel.
 */
#define TAG_LSP_ERR_REQUEST_CANCELLED       (-32800)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get c string that describes the error code.
 * @param[in] code  Error code.
 * @return          Error code string.
 */
const char* lsp_strerror(int code);

#ifdef __cplusplus
}
#endif
#endif
