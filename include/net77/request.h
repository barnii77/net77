#ifndef NET77_REQUEST_H
#define NET77_REQUEST_H

#include "net77/serde.h"
#include "net77/int_includes.h"

int
request(const char *host, int port, Request *req, String *out, size_t max_response_size, ssize_t connect_timeout_usec,
        ssize_t response_timeout_usec);

int rawRequest(const char *host, int port, StringRef req, String *out, size_t max_response_size,
               ssize_t connect_timeout_usec, ssize_t response_timeout_usec);

#endif