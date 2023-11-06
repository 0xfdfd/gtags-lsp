#ifndef __TAGS_LSP_UTILS_MSG_H__
#define __TAGS_LSP_UTILS_MSG_H__

#include <cjson/cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

int tag_lsp_msg_init(void);
void tag_lsp_msg_exit(void);

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
 * @brief Set error code for msg.
 * @param[in] rsp   Message to set.
 * @param[in] code  Error code.
 * @param[in] data  Error data. This function take the ownership of \p data.
 */
void tag_lsp_set_error(cJSON* rsp, int code, cJSON* data);

/**
 * @brief Send LSP JSON-RPC.
 * @note MT-Safe.
 * @param[in] msg   Message.
 * @return          Error code.
 */
int tag_lsp_send_msg(cJSON* msg);

int tag_lsp_send_error(cJSON* req, int code);

#ifdef __cplusplus
}
#endif
#endif
