#ifndef __TAGS_LSP_PARSER_H__
#define __TAGS_LSP_PARSER_H__

#include <cjson/cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Language server protocol parser.
 */
typedef struct lsp_parser lsp_parser_t;

typedef int (*lsp_parser_cb)(cJSON* obj, void* arg);

lsp_parser_t* lsp_parser_create(lsp_parser_cb cb, void* arg);

void lsp_parser_destroy(lsp_parser_t* parser);

void lsp_parser_execute(lsp_parser_t* parser, const char* data, size_t size);

#ifdef __cplusplus
}
#endif
#endif
