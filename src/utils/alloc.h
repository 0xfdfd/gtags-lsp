#ifndef __TAGS_LSP_UTILS_ALLOC_H__
#define __TAGS_LSP_UTILS_ALLOC_H__

#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Alloc helper for `uv_read_start()`.
 *
 * This allocater always add extra 1 byte at the end of \p suggested_size.
 *
 * @warning Use #lsp_free to free buffer.
 */
void tag_lsp_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

/**
 * @brief Free helper for #tag_lsp_alloc().
 */
void lsp_free(void* ptr);

/**
 * @see malloc().
 */
void* lsp_malloc(size_t size);

/**
 * @see realloc().
 */
void* lsp_realloc(void* ptr, size_t size);

/**
 * @see strdup().
 */
char* lsp_strdup(const char* str);

#ifdef __cplusplus
}
#endif
#endif
