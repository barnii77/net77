#ifndef NET77_N77_REQUEST_H
#define NET77_N77_REQUEST_H

#include "n77_serde.h"

int request(const char *host, int port, Request *req, String *out, int request_timeout_usec);

int rawRequest(const char *host, int port, StringRef req, String *out, int request_timeout_usec);

#endif