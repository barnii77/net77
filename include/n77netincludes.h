#ifndef N77NETINCLUDES_H
#define N77NETINCLUDES_H

#include "n77init.h"

// Detect Windows system
#if defined(_WIN32) || defined(_WIN64)
// Include Windows-specific headers
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdint.h>
#include <basetsd.h>
typedef SSIZE_T ssize_t;

// Define types and constants that Windows doesn't have, but POSIX systems do
typedef int socklen_t;  // Windows uses int for socket length
#define close closesocket  // POSIX uses close(), Windows uses closesocket()
#define read recv  // POSIX uses read, Windows uses recv()

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR  // Fallback to SO_REUSEADDR if necessary
#endif

// Detect POSIX systems (Linux, macOS, etc.)
#else
// Include POSIX-specific headers
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>  // for close(), read(), write()
#include <stdint.h>
#ifndef SO_REUSEPORT
#define SO_REUSEPORT 0  // No-op on systems that don't support it
#endif
#endif
#endif