#ifndef N77NETINCLUDES_H
#define N77NETINCLUDES_H

#include <stddef.h>
#include "init.h"

int setSendRecvTimeout(size_t fd, int timeout_usec);

// Detect Windows system
#ifdef _MSC_VER
// Include Windows-specific headers
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <stdint.h>
#include <basetsd.h>
typedef SSIZE_T ssize_t;

// Define types and constants that Windows doesn't have, but POSIX systems do
typedef int socklen_t;  // Windows uses int for socket length
#define close closesocket  // POSIX uses close(), Windows uses closesocket()
#define send(socket, buf, len) send(socket, buf, len, 0)
#define recv(socket, buf, len) recv(socket, buf, len, 0)

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR  // Fallback to SO_REUSEADDR if necessary
#endif

// Detect POSIX systems (Linux, macOS, etc.)
#else
// Include POSIX-specific headers
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>  // for close(), recv(), send()
#include <stdint.h>

#define send(socket, buf, len) send(socket, buf, len, MSG_NOSIGNAL)
#define recv(socket, buf, len) recv(socket, buf, len, 0)
#ifndef SO_REUSEPORT
#define SO_REUSEPORT 0  // No-op on systems that don't support it
#endif
#endif
#endif