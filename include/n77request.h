#ifndef NET77_N77REQUEST_H
#define NET77_N77REQUEST_H

#include "n77serde.h"

int request(const char *host, int port, Request *req, String *out);

int rawRequest(const char *host, int port, StringRef req, String *out);

#endif