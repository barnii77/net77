#ifndef NET77_NET77HTTP_H
#define NET77_NET77HTTP_H

#include <stddef.h>
#include "n77stringutils.h"

typedef enum Version {
    VERSION_HTTP10,
    VERSION_HTTP11,
} Version;

typedef enum Method {
    METHOD_GET,
    METHOD_POST,
    METHOD_HEAD,
    // TODO support PUT, LINK, ...
} Method;

typedef struct HeaderField {
    StringRef name;
    StringRef value;
} HeaderField;

typedef struct HeaderStruct {
    unsigned int count;
    HeaderField *fields;
} HeaderStruct;

typedef enum HeaderDataType {
    HEADER_AS_STRING,
    HEADER_AS_OWNED_STRING,
    HEADER_AS_STRUCT,
} HeaderDataType;

typedef union HeaderData {
    StringRef string;
    String owned_string;
    HeaderStruct structure;
} HeaderData;

typedef struct Header {
    HeaderDataType type;
    HeaderData data;
} Header;

typedef struct Request {
    Method method;
    StringRef url;
    Version version;
    Header head;
    StringRef body;
} Request;

typedef struct Response {
    Version version;
    int status_code;
    StringRef status_msg;
    Header head;
    StringRef body;
} Response;

int parseRequest(StringRef str, Request *req, int parse_header_into_structs);

int parseResponse(StringRef str, Response *resp, int parse_header_into_structs);

int serializeRequest(Request *req, StringBuilder *builder);

int serializeResponse(Response *resp, StringBuilder *builder);

void freeRequest(Request *req);

void freeResponse(Response *resp);

#endif