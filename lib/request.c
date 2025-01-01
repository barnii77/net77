#include <string.h>
#include "net77/serde.h"
#include "net77/request.h"
#include "net77/sock.h"
#include "net77/utils.h"
#include "net77/int_includes.h"
#include "net77/error_utils.h"

#define BUBBLE_UP_ERR(err) if (err) return 1

// after the initial response bytes are received, we wait for this many more microseconds for more response bytes
#define REQUEST_RESPONSE_ACCUM_TIME_USEC 50000

ErrorStatus
request(const char *host, int port, Request *req, String *out, size_t max_response_size, ssize_t connect_timeout_usec,
        ssize_t response_timeout_usec) {
    StringBuilder builder = newStringBuilder(0);
    BUBBLE_UP_ERR(serializeRequest(req, &builder));
    String str = stringBuilderBuildAndDestroy(&builder);
    StringRef str_ref = {str.len, str.data};
    int err = newSocketSendReceiveClose(host, port, str_ref, out, -1, connect_timeout_usec, response_timeout_usec,
                                        max_response_size, REQUEST_RESPONSE_ACCUM_TIME_USEC, 0);
    if (!err)
        freeString(&str);
    return err;
}

ErrorStatus rawRequest(const char *host, int port, StringRef req, String *out, size_t max_response_size,
                       ssize_t connect_timeout_usec, ssize_t response_timeout_usec) {
    return newSocketSendReceiveClose(host, port, req, out, -1, connect_timeout_usec, response_timeout_usec,
                                     max_response_size, REQUEST_RESPONSE_ACCUM_TIME_USEC, 0);
}

RequestBuilder newRequestBuilder(Method method, StringRef url, Version version) {
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

Request requestBuilderBuild(RequestBuilder *rb) {
    String str = stringBuilderBuild(&rb->head_builder);
    Header head = {.type = HEADER_AS_OWNED_STRING, .data = {.owned_string = str}};
    Request req = rb->req;
    req.head = head;
    return req;
}

Request requestBuilderBuildAndDestroy(RequestBuilder *rb) {
    String str = stringBuilderBuildAndDestroy(&rb->head_builder);
    Header head = {.type = HEADER_AS_OWNED_STRING, .data = {.owned_string = str}};
    Request req = rb->req;
    req.head = head;
    return req;
}

ResponseBuilder newResponseBuilder(Version version, int status_code, StringRef status_msg) {
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

Response responseBuilderBuild(ResponseBuilder *rb) {
    String str = stringBuilderBuild(&rb->head_builder);
    Header head = {.type = HEADER_AS_OWNED_STRING, .data = {.owned_string = str}};
    Response resp = rb->resp;
    resp.head = head;
    return resp;
}

Response responseBuilderBuildAndDestroy(ResponseBuilder *rb) {
    String str = stringBuilderBuildAndDestroy(&rb->head_builder);
    Header head = {.type = HEADER_AS_OWNED_STRING, .data = {.owned_string = str}};
    Response resp = rb->resp;
    resp.head = head;
    return resp;
}

int isValidRequest(Request *req) {
    // TODO this can always be extended with more checks
    // TODO possible extension: the METHOD|RFC|...RULES...| table at https://en.wikipedia.org/wiki/HTTP
    if (!(req->version == VERSION_HTTP09 || req->version == VERSION_HTTP10 || req->version == VERSION_HTTP11) &&
        (req->method == METHOD_GET || req->method == METHOD_POST || req->method == METHOD_PUT ||
         req->method == METHOD_DELETE || req->method == METHOD_HEAD || req->method == METHOD_OPTIONS ||
         req->method == METHOD_TRACE || req->method == METHOD_CONNECT))
        return 0;
    if (req->version == VERSION_HTTP11) {
        const char host[] = "host";
        int found_host = 0;
        switch (req->head.type) {
            case HEADER_AS_STRING:
                if (findAsciiSubstringCaseInsensitive(req->head.data.string.data, req->head.data.string.len, host,
                                                      sizeof(host)))
                    return 0;
                break;
            case HEADER_AS_OWNED_STRING:
                if (findAsciiSubstringCaseInsensitive(req->head.data.owned_string.data, req->head.data.owned_string.len,
                                                      host, sizeof(host)))
                    return 0;
                break;
            case HEADER_AS_STRUCT:
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
    return 1;
}