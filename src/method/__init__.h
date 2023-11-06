#ifndef __TAGS_LSP_METHOD_INIT_H__
#define __TAGS_LSP_METHOD_INIT_H__

#include <cjson/cJSON.h>
#include "utils/lsp_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum lsp_method_ret
{
    /**
     * @brief Method call success.
     *
     * For a request, the \p rsp message will send to peer. For a notification,
     * nothing will happen.
     */
    LSP_METHOD_SUCCESS = 0,

    /**
     * @brief Set this call as async.
     *
     * If it is a request, the ownership of \p rsp message transfer to user, you
     * must send the \p rsp later (e.g. by #tag_lsp_send_msg()) and release it.
     *
     * If it is a notification, nothing happen.
     */
    LSP_METHOD_ASYNC = 1,
} lsp_method_ret_t;

/**
 * @brief LSP method callback.
 * @param[in] req   Request message.
 * @param[in] rsp   Response message. If it is a notification, the value is `NULL`.
 * @return          #lsp_method_ret_t or LSP error code.
 *
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#requestMessage
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#responseMessage
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#notificationMessage
 */
typedef int (*lsp_method_fn)(cJSON* req, cJSON* rsp);

typedef void (*lsp_method_cleanup_fn)(void);

typedef struct lsp_method
{
    const char*             name;       /**< Method name. */
    int                     notify;     /**< This is a notification. It must either 0 or 1. */

    lsp_method_fn           entry;      /**< Method function. */
    lsp_method_cleanup_fn   cleanup;    /**< Cleanup function. */
} lsp_method_t;

/**
 * @defgroup LSP_BASE_PROTOCOL Base Protocol
 * @{
 */

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#cancelRequest
 */
extern lsp_method_t lsp_method_cancelrequest;

/**
 * @}
 */

/**
 * @defgroup LSP_SERVER_LIFECYCLE Server lifecycle
 * @{
 */

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#initialize
 */
extern lsp_method_t lsp_method_initialize;

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#setTrace
 */
extern lsp_method_t lsp_method_set_trace;

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#shutdown
 */
extern lsp_method_t lsp_method_shutdown;

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#exit
 */
extern lsp_method_t lsp_method_exit;

/**
 * @}
 */

/**
 * @defgroup LSP_TEXT_DOCUMENT_SYNCHRONIZATION Text Document Synchronization
 * @{
 */

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_didOpen
 */
extern lsp_method_t lsp_method_textdocument_didopen;

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_didChange
 */
extern lsp_method_t lsp_method_textdocument_didchange;

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_didSave
 */
extern lsp_method_t lsp_method_textdocument_didsave;

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_didClose
 */
extern lsp_method_t lsp_method_textdocument_didclose;

/**
 * @}
 */

/**
 * @defgroup LSP_WORKSPACE_FEATURES Workspace Features
 * @{
 */

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#workspace_didChangeWorkspaceFolders
 */
extern lsp_method_t lsp_method_workspace_didchangeworkspacefolders;

/**
 * @}
 */

int lsp_method_call(cJSON* req, cJSON* rsp, int is_notify);

void lsp_method_cleanup(void);

#ifdef __cplusplus
}
#endif
#endif
