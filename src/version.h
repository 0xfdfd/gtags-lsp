#ifndef __TAGS_LSP_VERSION_H__
#define __TAGS_LSP_VERSION_H__

#define TAGS_LSP_VERSION_MANJOR     1
#define TAGS_LSP_VERSION_MINOR      0
#define TAGS_LSP_VERSION_PATCH      0

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get version string.
 * @return      Version string.
 */
const char* tag_lsp_version(void);

#ifdef __cplusplus
}
#endif
#endif
