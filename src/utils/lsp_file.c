#include <string.h>
#include "lsp_file.h"
#include "alloc.h"

char* lsp_file_uri_to_real(const char* path)
{
    const char* prefix = "file://";
    if (strncmp(path, prefix, strlen(prefix)) == 0)
    {
        path += strlen(prefix);
        return lsp_strdup(path);
    }
    return lsp_strdup(path);
}
