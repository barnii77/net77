#ifndef NET77_HTTP_SERDE_H
#define NET77_HTTP_SERDE_H

#include <stddef.h>
#include "net77/string_utils.h"
#include "net77/type_utils.h"

typedef enum HttpVersion {
    VERSION_HTTP09 = 9,
    VERSION_HTTP10 = 10,
    VERSION_HTTP11 = 11,
} HttpVersion;

typedef enum HttpMethod {
    METHOD_GET,
    METHOD_HEAD,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_CONNECT,
    METHOD_OPTIONS,
    METHOD_TRACE,
    METHOD_PATCH,
} HttpMethod;

typedef struct HttpHeaderField {
    StringRef name;
    StringRef value;
} HttpHeaderField;

typedef struct HttpHeaderStruct {
    unsigned int count;
    HttpHeaderField *nullable fields;
} HttpHeaderStruct;

typedef enum HttpHeaderDataType {
    HEADER_AS_STRING,
    HEADER_AS_OWNED_STRING,
    HEADER_AS_STRUCT,
} HttpHeaderDataType;

typedef union HttpHeaderData {
    StringRef string;
    String owned_string;
    HttpHeaderStruct structure;
} HttpHeaderData;

typedef struct HttpHeader {
    HttpHeaderDataType type;
    HttpHeaderData data;
} HttpHeader;

typedef struct HttpRequest {
    HttpMethod method;
    StringRef url;
    HttpVersion version;
    HttpHeader head;
    StringRef body;
} HttpRequest;

typedef struct Response {
    HttpVersion version;
    int status_code;
    StringRef status_msg;
    HttpHeader head;
    StringRef body;
} HttpResponse;

int isValidHttpRequest(HttpRequest *nonnull req);

ErrorStatus serializeHttpHeader(HttpHeader *nonnull head, StringBuilder *nonnull builder);

ErrorStatus parseHttpHeader(StringRef str, HttpHeader *nonnull head, bool parse_header_into_structs);

ErrorStatus parseHttpRequest(StringRef str, HttpRequest *nonnull req, int parse_header_into_structs);

ErrorStatus parseHttpResponse(StringRef str, HttpResponse *nonnull resp, int parse_header_into_structs);

ErrorStatus serializeHttpRequest(HttpRequest *nonnull req, StringBuilder *nonnull builder);

ErrorStatus serializeHttpResponse(HttpResponse *nonnull resp, StringBuilder *nonnull builder);

void freeHttpRequest(HttpRequest *nonnull req);

void freeHttpResponse(HttpResponse *nonnull resp);

#endif