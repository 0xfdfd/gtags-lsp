#ifndef __TAGS_LSP_RUNTIME_H__
#define __TAGS_LSP_RUNTIME_H__

#include <uv.h>
#include <cjson/cJSON.h>
#include "utils/list.h"
#include "utils/lsp_parser.h"
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @see https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#traceValue
 */
typedef enum lsp_trace_value
{
    /**
     * @brief The server should not send any logTrace notification.
     */
    LSP_TRACE_VALUE_OFF,

    /**
     * @brief The server should not add the 'verbose' field in the `LogTraceParams`.
     */
    LSP_TRACE_VALUE_MESSAGES,

    /**
     * @brief Full `LogTraceParams`.
     */
    LSP_TRACE_VALUE_VERBOSE,
} lsp_trace_value_t;

typedef struct workspace_folder
{
    /**
     * @brief The name of the workspace folder.
     * Used to refer to this workspace folder in the user interface.
     */
    char*                       name;

    /**
     * @brief The associated URI for this workspace folder.
     */
    char*                       uri;
} workspace_folder_t;

typedef struct tag_lsp_work
{
    ev_list_node_t              node;                   /**< List node for #tags_ctx_t::work_queue */
    uv_work_t                   token;                  /**< Work token. */

    cJSON*                      req;                    /**< LSP request. */
    cJSON*                      rsp;                    /**< LSP response. `NULL` if #tag_lsp_work_t::notify is true. */

    int                         notify;                 /**< (Boolean) request is a notification. */
    int                         cancel;                 /**< (Boolean) request can be cancel. */
} tag_lsp_work_t;

typedef struct tags_ctx_s
{
    uv_loop_t*                  loop;                   /**< Event loop. */
    lsp_parser_t*               parser;                 /**< parser for language server protocol. */

    workspace_folder_t*         workspace_folders;      /**< Workspace folders list. */
    size_t                      workspace_folder_sz;    /**< Workspace folders list size. */
    cJSON*                      client_capabilities;    /**< Client capabilities. */
    lsp_trace_value_t           trace_value;            /**< Trace value. */

    struct
    {
        /**
         * @brief Asks the server to shut down, but to not exit.
         * If a server receives requests after a shutdown request those requests
         * should error with `InvalidRequest`.
         */
        int                     shutdown;
    } flags;

    ev_list_t                   work_queue;             /**< #tag_lsp_work_t */
    uv_mutex_t                  work_queue_mutex;       /**< Mutex for #tags_ctx_t::work_queue */
} tags_ctx_t;

extern tags_ctx_t               g_tags;                 /**< Global runtime. */

void tag_lsp_cleanup_workspace_folders(void);
void tag_lsp_cleanup_client_capabilities(void);

#ifdef __cplusplus
}
#endif
#endif
