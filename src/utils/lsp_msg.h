#ifndef __TAGS_LSP_UTILS_MSG_H__
#define __TAGS_LSP_UTILS_MSG_H__

#include <cjson/cJSON.h>
#include "utils/lsp_work.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum lsp_msg_type_e
{
    LSP_MSG_REQ,    /**< Request message. */
    LSP_MSG_RSP,    /**< Response message. */
    LSP_MSG_NFY,    /**< Notification message. */
} lsp_msg_type_t;

typedef struct tag_lsp_work_method
{
    lsp_work_t                  token;                  /**< Work token. */

    cJSON*                      req;                    /**< LSP request. */
    cJSON*                      rsp;                    /**< LSP response. `NULL` if #tag_lsp_work_t::notify is true. */

    int                         notify;                 /**< (Boolean) request is a notification. */
    int                         cancel;                 /**< (Boolean) request can be cancel. */
} tag_lsp_work_method_t;

int tag_lsp_msg_init(void);
void tag_lsp_msg_exit(void);

/**
 * @brief Get the type of message.
 * @param[in] msg   Message.
 * @return          Message type.
 */
lsp_msg_type_t lsp_msg_type(cJSON* msg);

cJSON* tag_lsp_create_rsp_from_req(cJSON* req);

/**
 * @brief Create a response from request with error code.
 * @param[in] req   Request message.
 * @param[in] code  Error code.
 * @param[in] data  Error data. This function take the ownership of \p data.
 */
cJSON* tag_lsp_create_error_fomr_req(cJSON* req, int code, cJSON* data);

/**
 * @brief Create notification message.
 * @param[in] method    Notification method.
 * @param[in] params    Notification params. This function take the ownership of \p params.
 */
cJSON* tag_lsp_create_notify(const char* method, cJSON* params);

/**
 * @brief Create request message.
 * @param[in] method    Request method.
 * @param[in] params    Request params. This function take the ownership of \p params.
 */
cJSON* tag_lsp_create_request(const char* method, cJSON* params);

/**
 * @brief Set error code for msg.
 * @param[in] rsp   Message to set.
 * @param[in] code  Error code.
 * @param[in] data  Error data. This function take the ownership of \p data.
 */
void tag_lsp_set_error(cJSON* rsp, int code, cJSON* data);

/**
 * @brief Send LSP response or notification message.
 * @note MT-Safe.
 * @param[in] msg   Message.
 * @return          Error code.
 */
int tag_lsp_send_rsp(cJSON* msg);

int tag_lsp_send_error(cJSON* req, int code);

void tag_lsp_handle_req(cJSON* req, int is_notify);
void tag_lsp_handle_rsp(cJSON* rsp);

/**
 * @brief Send request and wait for response.
 * @param[in] req   Request message.
 * @return          Response message. User must take care of lifecycle.
 */
cJSON* tag_lsp_send_req(cJSON* req);

#ifdef __cplusplus
}
#endif
#endif
