#ifndef NET77_REQUEST_H
#define NET77_REQUEST_H

#include "serde.h"
#include "net77/int_includes.h"

int request(const char *host, int port, Request *req, String *out, ssize_t request_timeout_usec, size_t max_response_size);

int rawRequest(const char *host, int port, StringRef req, String *out, ssize_t request_timeout_usec, size_t max_response_size);

#endif