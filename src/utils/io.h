#ifndef __TAGS_LSP_UTILS_IO_H__
#define __TAGS_LSP_UTILS_IO_H__

#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tag_lsp_io_type
{
    TAG_LSP_IO_STDIO,
    TAG_LSP_IO_PIPE,
    TAG_LSP_IO_PORT,
} tag_lsp_io_type_t;

typedef void (*tag_lsp_io_cb)(const char* data, ssize_t size);

typedef struct tag_lsp_io_cfg
{
    tag_lsp_io_type_t   type;

    union
    {
        const char*     file;
        int             port;
    }data;

    tag_lsp_io_cb       cb;
} tag_lsp_io_cfg_t;

int tag_lsp_io_init(tag_lsp_io_cfg_t* cfg);

void tag_lsp_io_exit(void);

int tag_lsp_io_write(uv_write_t* req, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb);

#ifdef __cplusplus
}
#endif
#endif
