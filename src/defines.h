#ifndef __TAGS_LSP_DEFINES_H__
#define __TAGS_LSP_DEFINES_H__

#ifndef TAGS_LSP_PROG_NAME
#   define TAGS_LSP_PROG_NAME   "tags-lsp"
#endif

#if defined(__GNUC__) || defined(__clang__)
#   define container_of(ptr, type, member)   \
        ({ \
            const typeof(((type *)0)->member)*__mptr = (ptr); \
            (type *)((char *)__mptr - offsetof(type, member)); \
        })
#else
#   define container_of(ptr, type, member)   \
        ((type *) ((char *) (ptr) - offsetof(type, member)))
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#if defined(_WIN32)
#   define STDIN_FILENO     _fileno(stdin)
#   define STDOUT_FILENO    _fileno(stdout)
#endif

#endif
