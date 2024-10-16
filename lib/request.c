#include "net77/serde.h"
#include "net77/request.h"
#include "net77/sock.h"
#include "net77/utils.h"
#include "net77/int_includes.h"

#define BUBBLE_UP_ERR(err) if (err) return 1

int request(const char *host, int port, Request *req, String *out, ssize_t request_timeout_usec, size_t max_response_size) {
    StringBuilder builder = newStringBuilder(0);
    BUBBLE_UP_ERR(serializeRequest(req, &builder));
    String str = stringBuilderBuildAndDestroy(&builder);
    StringRef str_ref = {str.len, str.data};
    int err = newSocketSendReceiveClose(host, port, str_ref, out, -1, request_timeout_usec, max_response_size);
    if (!err)
        freeString(&str);
    return err;
}

int rawRequest(const char *host, int port, StringRef req, String *out, ssize_t request_timeout_usec, size_t max_response_size) {
    return newSocketSendReceiveClose(host, port, req, out, -1, request_timeout_usec, max_response_size);
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