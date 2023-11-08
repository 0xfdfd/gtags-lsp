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

int lsp_msg_init(void);

void lsp_msg_cancel_all_pending_request();
void tag_lsp_msg_exit(void);

/**
 * @brief Get the type of message.
 * @param[in] msg   Message.
 * @return          Message type.
 */
lsp_msg_type_t lsp_msg_type(cJSON* msg);

/**
 * @brief Create request message.
 * @param[in] method    Request method.
 * @param[in] params    Request params. This function take the ownership of \p params.
 */
cJSON* lsp_create_req(const char* method, cJSON* params);

/**
 * @brief Create response.
 * @param[in] req   Request message.
 * @return          Response message.
 */
cJSON* lsp_create_rsp(cJSON* req);

/**
 * @brief Create notification message.
 * @param[in] method    Notification method.
 * @param[in] params    Notification params. This function take the ownership of \p params.
 */
cJSON* lsp_create_notify(const char* method, cJSON* params);

/**
 * @brief Set error code for msg.
 * @param[in] rsp   Message to set.
 * @param[in] code  Error code.
 * @param[in] msg   Error message. If NULL, the string of error code will be used.
 * @param[in] data  Error data. This function take the ownership of \p data.
 */
void lsp_set_error(cJSON* rsp, int code, const char* msg, cJSON* data);

/**
 * @brief Send request and wait for response.
 * @param[in] req   Request message.
 * @return          Response message. User must take care of lifecycle.
 */
cJSON* lsp_send_req(cJSON* req);

/**
 * @brief Send LSP response message.
 * @note MT-Safe.
 * @param[in] msg   Message.
 */
void lsp_send_rsp(cJSON* msg);

/**
 * @brief Send LSP notification message.
 * @note MT-Safe.
 * @param[in] msg   Message.
 */
void lsp_send_notify(cJSON* msg);

/**
 * @brief Handle incoming message.
 * @param[in] msg   Incoming message.
 */
void lsp_handle_msg(cJSON* msg);

void lsp_handle_cancel(cJSON* id);

/**
 * @brief Generate a new id.
 * This id can be used as message id, token, or other usage.
 * @return          A new id.
 */
uint64_t lsp_new_id(void);

/**
 * @see lsp_new_id
 */
void lsp_new_id_str(char* buffer, size_t size);

#ifdef __cplusplus
}
#endif
#endif
