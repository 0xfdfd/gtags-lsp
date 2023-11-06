#ifndef __TAGS_LSP_UTILS_WORK_H__
#define __TAGS_LSP_UTILS_WORK_H__

#include <uv.h>
#include "utils/list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lsp_work lsp_work_t;

typedef void (*lsp_work_cb)(lsp_work_t* req);
typedef void (*lsp_after_work_cb)(lsp_work_t* req, int status);
typedef int (*lsp_foreach_cb)(lsp_work_t* work, void* arg);

typedef enum lsp_work_type
{
    LSP_WORK_METHOD,
} lsp_work_type_t;

struct lsp_work
{
    lsp_work_type_t     type;

    ev_list_node_t      node;
    uv_work_t           token;

    lsp_work_cb         work_cb;
    lsp_after_work_cb   after_work_cb;
};

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
void lsp_queue_work(lsp_work_t* work, lsp_work_type_t type,
    lsp_work_cb work_cb, lsp_after_work_cb after_work_cb);

/**
 * @brief Cancel work.
 * @param[in] work              Work token.
 */
void lsp_work_cancel(lsp_work_t* work);

size_t lsp_work_queue_size(void);

void lsp_work_foreach(lsp_foreach_cb cb, void* arg);

#ifdef __cplusplus
}
#endif
#endif
