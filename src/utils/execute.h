#ifndef __TAGS_LSP_UTILS_EXECUTE_H__
#define __TAGS_LSP_UTILS_EXECUTE_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stdout data callback.
 * @param[in] data  Stdout data.
 * @param[in] size  Stdout data size.
 * @param[in] arg   User defined argument.
 */
typedef void (*lsp_execute_stdout_cb)(const char* data, size_t size, void* arg);

/**
 * @brief Execute cmd and get stdout.
 * @param[in] file              Executable file.
 * @param[in] args              Command line arguments.
 * @param[in] cwd               Working directory.
 * @param[in] cb                Stdout callback.
 * @param[in] arg               User defined argument pass to \p cb.
 * @return                      0 if success, otherwise failed.
 */
int lsp_execute(const char* file, char** args, const char* cwd,
    lsp_execute_stdout_cb cb, void* arg);

#ifdef __cplusplus
}
#endif
#endif
