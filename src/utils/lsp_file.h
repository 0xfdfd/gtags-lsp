#ifndef __TAGS_LSP_UTILS_LSP_FILE_H__
#define __TAGS_LSP_UTILS_LSP_FILE_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief URI to real path.
 * @warning Always use #lsp_free() for the returned value.
 * @param[in] path  LSP path.
 * @return          Real file system path.
 */
char* lsp_file_uri_to_real(const char* path);

#ifdef __cplusplus
}
#endif
#endif
