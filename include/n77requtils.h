#ifndef NET77_N77REQUTILS_H
#define NET77_N77REQUTILS_H

#include "n77request.h"

typedef struct RequestBuilder {
    Request req;
    StringBuilder head_builder;
} RequestBuilder;

typedef struct ResponseBuilder {
    Response resp;
    StringBuilder head_builder;
} ResponseBuilder;

RequestBuilder newRequestBuilder(Method method, StringRef url, Version version);

void requestBuilderSetBody(RequestBuilder *rb, StringRef body);

void requestBuilderAddHeader(RequestBuilder *rb, StringRef name, StringRef value);

Request requestBuilderBuild(RequestBuilder *rb);

Request requestBuilderBuildAndDestroy(RequestBuilder *rb);

ResponseBuilder newResponseBuilder(Version version, int status_code, StringRef status_msg);

void responseBuilderSetBody(ResponseBuilder *rb, StringRef body);

void responseBuilderAddHeader(ResponseBuilder *rb, StringRef name, StringRef value);

Response responseBuilderBuild(ResponseBuilder *rb);

Response responseBuilderBuildAndDestroy(ResponseBuilder *rb);

#endif