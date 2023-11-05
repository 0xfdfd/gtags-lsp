#include "lsp_error.h"

const char* lsp_strerror(int code)
{
    switch (code)
    {
    case TAG_LSP_ERR_METHOD_NOT_FOUND:
        return "Method Not Found";

    case TAG_LSP_ERR_INVALID_REQUEST:
        return "Invalid Request";

    case TAG_LSP_ERR_INTERNAL_ERROR:
        return "Internal Error";

    case TAG_LSP_ERR_SERVER_NOT_INITIALIZED:
        return "Server Not Initialized";

    case TAG_LSP_ERR_REQUEST_FAILED:
        return "Request Failed";

    case TAG_LSP_ERR_SERVER_CANCELLED:
        return "Server Cancelled";

    case TAG_LSP_ERR_CONTENT_MODIFIED:
        return "Content Modified";

    case TAG_LSP_ERR_REQUEST_CANCELLED:
        return "Request Cancelled";

    default:
        break;
    }

    return "";
}
