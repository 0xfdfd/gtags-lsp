#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lsp_parser.h"
#include "defines.h"

static int _lsp_parser_on_header_field(llhttp_t* parser, const char* at, size_t length)
{
    (void)parser; (void)at; (void)length;
    return 0;
}

static int _lsp_parser_on_header_field_complete(llhttp_t* parser)
{
    (void)parser;
    return 0;
}

static int _lsp_parser_on_header_value(llhttp_t* parser, const char* at, size_t length)
{
    (void)parser; (void)at; (void)length;
    return 0;
}

static int _lsp_parser_on_header_value_complete(llhttp_t* parser)
{
    (void)parser;
    return 0;
}

static int _lsp_parser_on_body(llhttp_t* parser, const char* at, size_t length)
{
    lsp_parser_t* impl = container_of(parser, lsp_parser_t, parser);

    size_t new_sz = impl->cache_sz + length;
    char* new_cache = realloc(impl->cache, new_sz);
    if (new_cache == NULL)
    {
        abort();
    }

    impl->cache = new_cache;
    memcpy(impl->cache + impl->cache_sz, at, length);
    impl->cache_sz = new_sz;

    return 0;
}

static int _lsp_parser_on_message_complete(llhttp_t* parser)
{
    lsp_parser_t* impl = container_of(parser, lsp_parser_t, parser);

    cJSON* obj = cJSON_ParseWithLength(impl->cache, impl->cache_sz);
    impl->cb(impl, obj);
    cJSON_Delete(obj);

    free(impl->cache);
    impl->cache = NULL;
    impl->cache_sz = 0;

    return HPE_PAUSED;
}

static int _lsp_parser_init(lsp_parser_t* parser, lsp_parser_cb cb)
{
    llhttp_settings_init(&parser->settings);

    parser->settings.on_header_field = _lsp_parser_on_header_field;
    parser->settings.on_header_field_complete = _lsp_parser_on_header_field_complete;
    parser->settings.on_header_value = _lsp_parser_on_header_value;
    parser->settings.on_header_value_complete = _lsp_parser_on_header_value_complete;
    parser->settings.on_body = _lsp_parser_on_body;
    parser->settings.on_message_complete = _lsp_parser_on_message_complete;
    llhttp_init(&parser->parser, HTTP_REQUEST, &parser->settings);

    parser->cb = cb;

    llhttp_pause(&parser->parser);

    return 0;
}

lsp_parser_t* lsp_parser_create(lsp_parser_cb cb)
{
    lsp_parser_t* parser = malloc(sizeof(lsp_parser_t));
    if (parser == NULL)
    {
        return NULL;
    }

    _lsp_parser_init(parser, cb);
    return parser;
}

static void _lsp_parser_exit(lsp_parser_t* parser)
{
    if (parser->cache != NULL)
    {
        free(parser->cache);
        parser->cache = NULL;
    }
    parser->cache_sz = 0;
}

void lsp_parser_destroy(lsp_parser_t* parser)
{
    _lsp_parser_exit(parser);
    free(parser);
}

int lsp_parser_execute(lsp_parser_t* parser, const char* data, size_t size)
{
    while (1)
    {
        int ret = llhttp_execute(&parser->parser, data, size);
        if (ret == HPE_PAUSED)
        {
            const char* pos = llhttp_get_error_pos(&parser->parser);
            size -= pos - data;
            data = pos;

            llhttp_resume(&parser->parser);

            const char* req = "GET / HTTP/1.1\r\n";
            llhttp_execute(&parser->parser, req, strlen(req));
        }
        else if (ret != 0)
        {
            fprintf(stderr, "llhttp_execute failed: %s. errno:%d.\n",
                llhttp_errno_name(ret), ret);
            abort();
        }

        break;
    }

    return 0;
}
