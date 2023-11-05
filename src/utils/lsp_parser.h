#ifndef __TAGS_LSP_PARSER_H__
#define __TAGS_LSP_PARSER_H__

#include <cjson/cJSON.h>
#include <llhttp.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lsp_parser lsp_parser_t;

typedef int (*lsp_parser_cb)(lsp_parser_t* parser, cJSON* obj);

struct lsp_parser
{
    llhttp_t            parser;
    llhttp_settings_t   settings;
    lsp_parser_cb       cb;

    char*               cache;
    size_t              cache_sz;
};

int lsp_parser_init(lsp_parser_t* parser, lsp_parser_cb cb);

void lsp_parser_exit(lsp_parser_t* parser);

int lsp_parser_execute(lsp_parser_t* parser, const char* data, size_t size);

#ifdef __cplusplus
}
#endif
#endif
