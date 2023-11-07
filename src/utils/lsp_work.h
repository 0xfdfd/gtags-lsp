#ifndef __TAGS_LSP_UTILS_WORK_H__
#define __TAGS_LSP_UTILS_WORK_H__

#include <uv.h>
#include "utils/list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lsp_work lsp_work_t;

typedef void (*lsp_work_cb)(lsp_work_t* token, int cancel, void* arg);

/**
 * @brief Initialize work queue.
 */
void lsp_work_init(void);

/**
 * @brief Exit work queue.
 */
void lsp_work_exit(void);

/**
 * @brief Submit work request.
 * @param[in] work              Work token.
 * @param[in] type              Token type.
 * @param[in] work_cb           Work callback.
 * @param[in] after_work_cb     After work callback.
 */
void lsp_queue_work(lsp_work_t** token, lsp_work_cb work_cb, void* arg);

/**
 * @brief Cancel work.
 * @param[in] work              Work token.
 */
void lsp_work_cancel(lsp_work_t* token);

size_t lsp_work_queue_size(void);

#ifdef __cplusplus
}
#endif
#endif
