#include <stdlib.h>
#include <memory.h>
#include "net77/http/serde.h"
#include "net77/type_utils.h"

void freeHttpRequest(HttpRequest *nonnull req) {
    if (req->head.type == HEADER_AS_STRUCT) {
        memset(req->head.data.structure.fields, 0, req->head.data.structure.count);
        free(req->head.data.structure.fields);
    } else if (req->head.type == HEADER_AS_OWNED_STRING) {
        memset(req->head.data.owned_string.data, 0, req->head.data.owned_string.len);
        free(req->head.data.owned_string.data);
    }
}

void freeHttpResponse(HttpResponse *nonnull resp) {
    if (resp->head.type == HEADER_AS_STRUCT) {
        memset(resp->head.data.structure.fields, 0, resp->head.data.structure.count);
        free(resp->head.data.structure.fields);
    } else if (resp->head.type == HEADER_AS_OWNED_STRING) {
        memset(resp->head.data.owned_string.data, 0, resp->head.data.owned_string.len);
        free(resp->head.data.owned_string.data);
    }
}

void freeString(String *nonnull str) {
    if (str->data) {
        memset(str->data, 0, str->len);
        free(str->data);
    }
}