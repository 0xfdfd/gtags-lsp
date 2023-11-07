#ifndef __TAGS_LSP_TASK_INIT_H__
#define __TAGS_LSP_TASK_INIT_H__

#include <cjson/cJSON.h>
#include "utils/lsp_work.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lsp_work_task
{
    lsp_work_t  token;
    char*       workspace;
} lsp_work_task_t;

/**
 * @brief Update tags.
 * @param[in] workspace Workspace folder.
 */
void lsp_task_update_tags(const char* workspace);

#ifdef __cplusplus
}
#endif
#endif
