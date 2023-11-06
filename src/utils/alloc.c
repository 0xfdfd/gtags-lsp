#include <stdlib.h>
#include "alloc.h"

void tag_lsp_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    (void)handle;
    char* addr = malloc(suggested_size);
    *buf = uv_buf_init(addr, (unsigned int)suggested_size);
}

void tag_lsp_free(void* ptr)
{
    free(ptr);
}
