#include <stdlib.h>
#include <string.h>
#include "alloc.h"

void tag_lsp_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    (void)handle;
    char* addr = lsp_malloc(suggested_size + 1);
    *buf = uv_buf_init(addr, (unsigned int)suggested_size);
}

void lsp_free(void* ptr)
{
    free(ptr);
}

void* lsp_malloc(size_t size)
{
    void* ptr = malloc(size);
    if (ptr == NULL)
    {
        fprintf(stderr, "out of memory.\n");
        abort();
    }

    return ptr;
}

void* lsp_realloc(void* ptr, size_t size)
{
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL)
    {
        fprintf(stderr, "out of memory.\n");
        abort();
    }

    return new_ptr;
}

char* lsp_strdup(const char* str)
{
    size_t len = strlen(str) + 1;
    char* copy = lsp_malloc(len);

    memcpy(copy, str, len);
    return (copy);
}
