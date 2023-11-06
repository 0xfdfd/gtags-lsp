#ifndef __TAG_LSP_UTILS_ALLOC_H__
#define __TAG_LSP_UTILS_ALLOC_H__

#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Alloc helper for `uv_read_start()`.
 * @note Use #tag_lsp_free to free buffer.
 */
void tag_lsp_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

/**
 * @brief Free helper for #tag_lsp_alloc().
 */
void tag_lsp_free(void* ptr);

#ifdef __cplusplus
}
#endif
#endif
