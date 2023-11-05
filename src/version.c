#include "version.h"

#define STRINGIFY(x)    STRINGIFY2(x)
#define STRINGIFY2(x)   #x

#define TAG_LSP_VERSION_STR \
    STRINGIFY(TAGS_LSP_VERSION_MANJOR) "." STRINGIFY(TAGS_LSP_VERSION_MINOR) "." STRINGIFY(TAGS_LSP_VERSION_PATCH)

const char* tag_lsp_version(void)
{
    return TAG_LSP_VERSION_STR;
}
