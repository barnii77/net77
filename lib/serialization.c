#include "net77/serde.h"

#define BUBBLE_UP_ERR(err) if (err) return err

// append a literal whose size is known at compile time
#define STRING_BUILDER_APPEND_CONST_SIZED(builder, literal) stringBuilderAppend(builder, literal, sizeof(literal) - 1)

ErrorStatus serializeMethod(Method method, StringBuilder *builder) {
    if (method == METHOD_GET) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "GET");
    } else if (method == METHOD_HEAD) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "HEAD");
    } else if (method == METHOD_POST) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "POST");
    } else if (method == METHOD_PUT) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "PUT");
    } else if (method == METHOD_DELETE) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "DELETE");
    } else if (method == METHOD_CONNECT) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "CONNECT");
    } else if (method == METHOD_OPTIONS) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "OPTIONS");
    } else if (method == METHOD_TRACE) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "TRACE");
    } else if (method == METHOD_PATCH) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "PATCH");
    } else {
        return 1;
    }
    return 0;
}

ErrorStatus serializeURL(StringRef url, StringBuilder *builder) {
    stringBuilderAppend(builder, url.data, url.len);
    return 0;
}

ErrorStatus serializeHTTPVersion(Version v, StringBuilder *builder) {
    if (v == VERSION_HTTP09) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "HTTP/0.9");
    } else if (v == VERSION_HTTP10) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "HTTP/1.0");
    } else if (v == VERSION_HTTP11) {
        STRING_BUILDER_APPEND_CONST_SIZED(builder, "HTTP/1.1");
    } else {
        return 1;
    }
    return 0;
}

ErrorStatus serializeStatusCode(int status_code, StringBuilder *builder) {
    char status_code_str[] = "\0\0\0";
    for (int i = 0; i < sizeof(status_code_str) - 1; i++) {
        status_code_str[sizeof(status_code_str) - 2 - i] = (char) (status_code % 10 + '0');
        status_code /= 10;
    }
    STRING_BUILDER_APPEND_CONST_SIZED(builder, status_code_str);
    return 0;
}

ErrorStatus serializeHeader(Header *head, StringBuilder *builder) {
    if (head->type == HEADER_AS_STRUCT) {
        for (int i = 0; i < head->data.structure.count; i++) {
            HeaderField f = head->data.structure.fields[i];
            stringBuilderAppend(builder, f.name.data, f.name.len);
            STRING_BUILDER_APPEND_CONST_SIZED(builder, ":");
            stringBuilderAppend(builder, f.value.data, f.value.len);
            STRING_BUILDER_APPEND_CONST_SIZED(builder, "\r\n");
        }
    } else if (head->type == HEADER_AS_STRING) {
        stringBuilderAppend(builder, head->data.string.data, head->data.string.len);
    } else {
        return 1;
    }
    return 0;
}

ErrorStatus serializeRequest(Request *req, StringBuilder *builder) {
    BUBBLE_UP_ERR(serializeMethod(req->method, builder));
    STRING_BUILDER_APPEND_CONST_SIZED(builder, " ");
    BUBBLE_UP_ERR(serializeURL(req->url, builder));
    STRING_BUILDER_APPEND_CONST_SIZED(builder, " ");
    BUBBLE_UP_ERR(serializeHTTPVersion(req->version, builder));
    STRING_BUILDER_APPEND_CONST_SIZED(builder, "\r\n");
    BUBBLE_UP_ERR(serializeHeader(&req->head, builder));
    STRING_BUILDER_APPEND_CONST_SIZED(builder, "\r\n");
    if (req->body.len > 0)
        stringBuilderAppend(builder, req->body.data, req->body.len);
    return 0;
}

ErrorStatus serializeResponse(Response *resp, StringBuilder *builder) {
    BUBBLE_UP_ERR(serializeHTTPVersion(resp->version, builder));
    STRING_BUILDER_APPEND_CONST_SIZED(builder, " ");
    BUBBLE_UP_ERR(serializeStatusCode(resp->status_code, builder));
    STRING_BUILDER_APPEND_CONST_SIZED(builder, " ");
    stringBuilderAppend(builder, resp->status_msg.data, resp->status_msg.len);
    STRING_BUILDER_APPEND_CONST_SIZED(builder, "\r\n");
    BUBBLE_UP_ERR(serializeHeader(&resp->head, builder));
    STRING_BUILDER_APPEND_CONST_SIZED(builder, "\r\n");
    if (resp->body.len > 0)
        stringBuilderAppend(builder, resp->body.data, resp->body.len);
    return 0;
}