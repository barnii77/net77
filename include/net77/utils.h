#ifndef NET77_UTILS_H
#define NET77_UTILS_H

#include "net77/request.h"

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

StringRef removeURLPrefix(StringRef url);

int isLetter(char c);

StringRef charPtrToStringRef(const char *data);

size_t getTimeInUSecs();

/// Cross-platform way to make a file descriptor non-delaying (deactivate Nagle's algorithm to reduce latency)
int setSocketNoDelay(size_t fd);

/// Cross-platform way to make a file descriptor non-blocking (recv and sendAllData will return immediately)
int makeSocketNonBlocking(size_t fd);

/// Cross-platform way to yield the CPU to another thread to avoid busy waiting
void schedYield();

#endif