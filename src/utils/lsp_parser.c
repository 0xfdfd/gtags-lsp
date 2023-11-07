#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lsp_parser.h"
#include "defines.h"
#include "utils/alloc.h"
#include "utils/log.h"

#if defined(WIN32)
#   define sscanf(str, fmt, ...)    sscanf_s(str, fmt,##__VA_ARGS__)
#endif

typedef struct lsp_header_s
{
    char*           name;
    char*           value;
}lsp_header_t;

typedef enum lsp_parser_state
{
    LSP_PARSER_STATE_NEED_HEADER_NAME,
    LSP_PARSER_STATE_NEED_HEADER_VALUE_SPACE,
    LSP_PARSER_STATE_NEED_HEADER_VALUE,
    LSP_PARSER_STATE_NEED_HEADER_VALUE_WRAP,
    LSP_PARSER_STATE_CHECK_HEADER,
    LSP_PARSER_STATE_DROP_PRE_BODY,
    LSP_PARSER_STATE_NEED_BODY,
    LSP_PARSER_STATE_FINISH,
} lsp_parser_state_t;

struct lsp_parser
{
    lsp_parser_cb       cb;
    void*               arg;

    lsp_parser_state_t  state;
    lsp_header_t*       headers;
    size_t              header_sz;

    unsigned            body_cap;           /**< The value of `Content-Length`. */
    unsigned            body_sz;            /**< Current size of body. */
    char*               body;               /**< Always add pending NULL. */

    char*               cached_header_name;

    char*               cache;
    size_t              cache_sz;
};

static char* strnstr(const char* s, const char* find, size_t slen)
{
    char c, sc;
    size_t len;

    if ((c = *find++) != '\0')
    {
        len = strlen(find);
        do
        {
            do
            {
                if ((sc = *s++) == '\0' || slen-- < 1)
                {
                    return (NULL);
                }
            } while (sc != c);
            if (len > slen)
            {
                return (NULL);
            }
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return ((char*)s);
}

lsp_parser_t* lsp_parser_create(lsp_parser_cb cb, void* arg)
{
    lsp_parser_t* parser = malloc(sizeof(lsp_parser_t));
    if (parser == NULL)
    {
        return NULL;
    }
    memset(parser, 0, sizeof(*parser));

    parser->cb = cb;
    parser->arg = arg;

    return parser;
}

static void _lsp_parser_reset(lsp_parser_t* parser)
{
    size_t i;
    parser->state = LSP_PARSER_STATE_NEED_HEADER_NAME;

    for (i = 0; i < parser->header_sz; i++)
    {
        lsp_free(parser->headers[i].name);
        parser->headers[i].name = NULL;
        lsp_free(parser->headers[i].value);
        parser->headers[i].value = NULL;
    }
    lsp_free(parser->headers);
    parser->headers = NULL;
    parser->header_sz = 0;

    if (parser->body != NULL)
    {
        lsp_free(parser->body);
        parser->body = NULL;
    }
    parser->body_sz = 0;
    parser->body_cap = 0;

    if (parser->cached_header_name != NULL)
    {
        lsp_free(parser->cached_header_name);
        parser->cached_header_name = NULL;
    }

    if (parser->cache != NULL)
    {
        lsp_free(parser->cache);
        parser->cache = NULL;
    }
    parser->cache_sz = 0;
}

void lsp_parser_destroy(lsp_parser_t* parser)
{
    _lsp_parser_reset(parser);
    lsp_free(parser);
}

static size_t _lsp_execute_parser_header_name(lsp_parser_t* parser, const char* data, size_t size)
{
    const char* pos = strnstr(data, ":", size);
    if (pos == NULL)
    {
        size_t new_cache_sz = parser->cache_sz + size;

        /* Always add pending NULL for safe. */
        parser->cache = lsp_realloc(parser->cache, new_cache_sz + 1);
        memcpy(parser->cache + parser->cache_sz, data, size);
        parser->cache_sz = new_cache_sz;
        parser->cache[parser->cache_sz] = '\0';
        return size;
    }

    /* Calculate header name size. */
    size_t header_name_sz = parser->cache_sz + pos - data;

    /* Always add pending NULL for safe. */
    parser->cache = lsp_realloc(parser->cache, header_name_sz + 1);
    memcpy(parser->cache + parser->cache_sz, data, pos - data);
    parser->cache_sz = header_name_sz;
    parser->cache[parser->cache_sz] = '\0';

    /* Move item. */
    parser->cached_header_name = parser->cache;
    parser->cache = NULL;
    parser->cache_sz = 0;
    parser->state = LSP_PARSER_STATE_NEED_HEADER_VALUE_SPACE;

    return pos - data + 1;
}

static size_t _lsp_execute_parser_header_value_space(lsp_parser_t* parser, const char* data, size_t size)
{
    size_t pos = 0;
    for (; pos < size; pos++)
    {
        if (data[pos] != ' ')
        {
            parser->state = LSP_PARSER_STATE_NEED_HEADER_VALUE;
            break;
        }
    }

    return pos;
}

static size_t _lsp_execute_parser_header_value(lsp_parser_t* parser, const char* data, size_t size)
{
    const char* pos = strnstr(data, "\r", size);
    if (pos == NULL)
    {
        size_t new_cache_sz = parser->cache_sz + size;
        parser->cache = lsp_realloc(parser->cache, new_cache_sz + 1);
        memcpy(parser->cache + parser->cache_sz, data, size);
        parser->cache_sz = new_cache_sz;
        parser->cache[parser->cache_sz] = '\0';
        return size;
    }

    size_t value_sz = parser->cache_sz + pos - data;
    parser->cache = lsp_realloc(parser->cache, value_sz + 1);
    memcpy(parser->cache + parser->cache_sz, data, pos - data);
    parser->cache_sz = value_sz;
    parser->cache[parser->cache_sz] = '\0';

    parser->header_sz++;
    parser->headers = lsp_realloc(parser->headers, sizeof(lsp_header_t) * parser->header_sz);

    parser->headers[parser->header_sz - 1].name = parser->cached_header_name;
    parser->cached_header_name = NULL;

    parser->headers[parser->header_sz - 1].value = parser->cache;
    parser->cache = NULL;
    parser->cache_sz = 0;

    parser->state = LSP_PARSER_STATE_NEED_HEADER_VALUE_WRAP;
    return pos - data + 1;
}

static size_t _lsp_execute_parser_header_value_wrap(lsp_parser_t* parser, const char* data, size_t size)
{
    (void)data; (void)size;

    parser->state = LSP_PARSER_STATE_CHECK_HEADER;

    /* Drop '\n' */
    return 1;
}

static size_t _lsp_execute_parser_check_header(lsp_parser_t* parser, const char* data, size_t size)
{
    (void)size;

    /* Now we need to parser body. Drop '\r\n' */
    if (data[0] == '\r')
    {
        parser->state = LSP_PARSER_STATE_DROP_PRE_BODY;
        return 1;
    }

    parser->state = LSP_PARSER_STATE_NEED_HEADER_NAME;
    return 0;
}

static size_t _lsp_execute_parser_drop_pre_body(lsp_parser_t* parser, const char* data, size_t size)
{
    (void)data; (void)size;

    size_t i;
    int have_content_length = 0;

    for (i = 0; i < parser->header_sz; i++)
    {
        if (strcmp(parser->headers[i].name, "Content-Length") == 0)
        {
            if (sscanf(parser->headers[i].value, "%u", &parser->body_cap) != 1)
            {
                fprintf(stderr, "invalid: Content-Length: %s.\n", parser->headers[i].value);
                exit(EXIT_FAILURE);
            }

            have_content_length = 1;
            break;
        }
    }

    if (!have_content_length)
    {
        fprintf(stderr, "no Content-Length.\n");
        exit(EXIT_FAILURE);
    }

    parser->state = LSP_PARSER_STATE_NEED_BODY;
    return 1;
}

static size_t _lsp_execute_parser_need_body(lsp_parser_t* parser, const char* data, size_t size)
{
    size_t need_size = parser->body_cap - parser->body_sz;

    if (need_size > size)
    {
        size_t new_body_sz = parser->body_sz + size;
        parser->body = lsp_realloc(parser->body, new_body_sz + 1);
        memcpy(parser->body + parser->body_sz, data, size);
        parser->body_sz = (unsigned)new_body_sz;
        parser->body[parser->body_sz] = '\0';
        return size;
    }

    parser->body = lsp_realloc(parser->body, parser->body_cap + 1);
    memcpy(parser->body + parser->body_sz, data, need_size);
    parser->body_sz = parser->body_cap;
    parser->body[parser->body_sz] = '\0';

    parser->state = LSP_PARSER_STATE_FINISH;

    return need_size;
}

static size_t _lsp_execute_parser_finish(lsp_parser_t* parser, const char* data, size_t size)
{
    (void)data; (void)size;

    cJSON* msg = cJSON_ParseWithLength(parser->body, parser->body_sz);
    if (msg == NULL)
    {
        fprintf(stderr, "parser message body failed.\n");
        exit(EXIT_FAILURE);
    }

    parser->cb(msg, parser->arg);
    cJSON_Delete(msg);

    _lsp_parser_reset(parser);

    return 0;
}

void lsp_parser_execute(lsp_parser_t* parser, const char* data, size_t size)
{
    size_t ret;
    while (size > 0 || parser->state == LSP_PARSER_STATE_FINISH)
    {
        switch (parser->state)
        {
        case LSP_PARSER_STATE_NEED_HEADER_NAME:
            ret = _lsp_execute_parser_header_name(parser, data, size);
            break;

        case LSP_PARSER_STATE_NEED_HEADER_VALUE_SPACE:
            ret = _lsp_execute_parser_header_value_space(parser, data, size);
            break;

        case LSP_PARSER_STATE_NEED_HEADER_VALUE:
            ret = _lsp_execute_parser_header_value(parser, data, size);
            break;

        case LSP_PARSER_STATE_NEED_HEADER_VALUE_WRAP:
            ret = _lsp_execute_parser_header_value_wrap(parser, data, size);
            break;

        case LSP_PARSER_STATE_CHECK_HEADER:
            ret = _lsp_execute_parser_check_header(parser, data, size);
            break;

        case LSP_PARSER_STATE_DROP_PRE_BODY:
            ret = _lsp_execute_parser_drop_pre_body(parser, data, size);
            break;

        case LSP_PARSER_STATE_NEED_BODY:
            ret = _lsp_execute_parser_need_body(parser, data, size);
            break;

        case LSP_PARSER_STATE_FINISH:
            ret = _lsp_execute_parser_finish(parser, data, size);
            break;

        default:
            abort();
        }

        data += ret;
        size -= ret;
    }
}
