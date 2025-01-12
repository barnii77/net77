#include <string.h>
#include "net77/http/utils.h"
#include "net77/http/serde.h"

RequestBuilder newRequestBuilder(HttpMethod method, StringRef url, HttpVersion version) {
    RequestBuilder out = {
            .req = {.method = method, .url = url, .version = version},
            .head_builder = newStringBuilder(0)
    };
    return out;
}

void requestBuilderSetBody(RequestBuilder *rb, StringRef body) {
    rb->req.body = body;
}

void requestBuilderAddHeader(RequestBuilder *rb, StringRef name, StringRef value) {
    stringBuilderAppend(&rb->head_builder, name.data, name.len);
    stringBuilderAppend(&rb->head_builder, ":", 1);
    stringBuilderAppend(&rb->head_builder, value.data, value.len);
    stringBuilderAppend(&rb->head_builder, "\r\n", 2);
}

HttpRequest requestBuilderBuild(RequestBuilder *rb) {
    String str = stringBuilderBuild(&rb->head_builder);
    HttpHeader head = {.type = HEADER_AS_OWNED_STRING, .data = {.owned_string = str}};
    HttpRequest req = rb->req;
    req.head = head;
    return req;
}

HttpRequest requestBuilderBuildAndDestroy(RequestBuilder *rb) {
    String str = stringBuilderBuildAndDestroy(&rb->head_builder);
    HttpHeader head = {.type = HEADER_AS_OWNED_STRING, .data = {.owned_string = str}};
    HttpRequest req = rb->req;
    req.head = head;
    return req;
}

ResponseBuilder newResponseBuilder(HttpVersion version, int status_code, StringRef status_msg) {
    ResponseBuilder out = {
            .resp = {.version = version, .status_code = status_code, .status_msg = status_msg},
            .head_builder = newStringBuilder(0)
    };
    return out;
}

void responseBuilderSetBody(ResponseBuilder *rb, StringRef body) {
    rb->resp.body = body;
}

void responseBuilderAddHeader(ResponseBuilder *rb, StringRef name, StringRef value) {
    stringBuilderAppend(&rb->head_builder, name.data, name.len);
    stringBuilderAppend(&rb->head_builder, ":", 1);
    stringBuilderAppend(&rb->head_builder, value.data, value.len);
    stringBuilderAppend(&rb->head_builder, "\r\n", 2);
}

HttpResponse responseBuilderBuild(ResponseBuilder *rb) {
    String str = stringBuilderBuild(&rb->head_builder);
    HttpHeader head = {.type = HEADER_AS_OWNED_STRING, .data = {.owned_string = str}};
    HttpResponse resp = rb->resp;
    resp.head = head;
    return resp;
}

HttpResponse responseBuilderBuildAndDestroy(ResponseBuilder *rb) {
    String str = stringBuilderBuildAndDestroy(&rb->head_builder);
    HttpHeader head = {.type = HEADER_AS_OWNED_STRING, .data = {.owned_string = str}};
    HttpResponse resp = rb->resp;
    resp.head = head;
    return resp;
}

int isValidHttpRequest(HttpRequest *req) {
    // TODO this can always be extended with more checks
    // TODO possible extension: the METHOD|RFC|...RULES...| table at https://en.wikipedia.org/wiki/HTTP
    if (!(req->version == VERSION_HTTP09 || req->version == VERSION_HTTP10 || req->version == VERSION_HTTP11) &&
        (req->method == METHOD_GET || req->method == METHOD_POST || req->method == METHOD_PUT ||
         req->method == METHOD_DELETE || req->method == METHOD_HEAD || req->method == METHOD_OPTIONS ||
         req->method == METHOD_TRACE || req->method == METHOD_CONNECT))
        return 0;
    if (req->version == VERSION_HTTP11) {
        const char host[] = "host";
        switch (req->head.type) {
            case HEADER_AS_STRING:
            {
                if (findAsciiSubstringCaseInsensitive(req->head.data.string.data, req->head.data.string.len, host,
                                                      sizeof(host)))
                    return 0;
                break;
            }
            case HEADER_AS_OWNED_STRING:
            {
                if (findAsciiSubstringCaseInsensitive(req->head.data.owned_string.data, req->head.data.owned_string.len,
                                                      host, sizeof(host)))
                    return 0;
                break;
            }
            case HEADER_AS_STRUCT:
            {
                int found_host = 0;
                for (size_t i = 0; i < req->head.data.structure.count; i++) {
                    StringRef *name = &req->head.data.structure.fields[i].name;
                    if (name->len == sizeof(host) && !strncmp(name->data, host, sizeof(host)))
                        found_host = 1;
                }
                if (!found_host)
                    return 0;
                break;
            }
        }
    }
    return 1;
}