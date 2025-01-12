#ifndef NET77_HTTP_UTILS_H
#define NET77_HTTP_UTILS_H

#include "net77/http/request.h"

typedef struct RequestBuilder {
    HttpRequest req;
    StringBuilder head_builder;
} RequestBuilder;

typedef struct ResponseBuilder {
    HttpResponse resp;
    StringBuilder head_builder;
} ResponseBuilder;

RequestBuilder newRequestBuilder(HttpMethod method, StringRef url, HttpVersion version);

void requestBuilderSetBody(RequestBuilder *rb, StringRef body);

void requestBuilderAddHeader(RequestBuilder *rb, StringRef name, StringRef value);

HttpRequest requestBuilderBuild(RequestBuilder *rb);

HttpRequest requestBuilderBuildAndDestroy(RequestBuilder *rb);

ResponseBuilder newResponseBuilder(HttpVersion version, int status_code, StringRef status_msg);

void responseBuilderSetBody(ResponseBuilder *rb, StringRef body);

void responseBuilderAddHeader(ResponseBuilder *rb, StringRef name, StringRef value);

HttpResponse responseBuilderBuild(ResponseBuilder *rb);

HttpResponse responseBuilderBuildAndDestroy(ResponseBuilder *rb);

#endif