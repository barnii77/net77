#ifndef NET77_REQUEST_H
#define NET77_REQUEST_H

#include "serde.h"

int request(const char *host, int port, Request *req, String *out, int request_timeout_usec);

int rawRequest(const char *host, int port, StringRef req, String *out, int request_timeout_usec);

#endif