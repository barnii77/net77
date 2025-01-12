#ifndef NET77_UTILS_H
#define NET77_UTILS_H

#include "net77/string_utils.h"
#include "net77/type_utils.h"
#include "net77/int_includes.h"

StringRef removeURLPrefix(StringRef url);

int isLetter(char c);

StringRef charPtrToStringRef(const char *data);

size_t getTimeInUSecs();

/// Cross-platform way to make a file descriptor non-delaying (deactivate Nagle's algorithm to reduce latency)
ErrorStatus setSocketNoDelay(size_t fd);

/// Cross-platform way to make a file descriptor non-blocking (recv and sendAllData will return immediately)
ErrorStatus makeSocketNonBlocking(size_t fd);

/// Cross-platform way to enable TCP keepalive, which periodically sends empty packets to prevent timeout disconnect
ErrorStatus setSocketKeepalive(size_t fd);

/// Cross-platform way to yield the CPU to another thread to avoid busy waiting. Does not guarantee yielding.
void schedYield();

#endif