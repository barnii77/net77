#ifndef N77NETINCLUDES_H
#define N77NETINCLUDES_H

// Detect Windows system
#if defined(_WIN32) || defined(_WIN64)
// Include Windows-specific headers
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// Define types and constants that Windows doesn't have, but POSIX systems do
typedef int socklen_t; // Windows uses int for socket length
#define close closesocket // POSIX uses close(), Windows uses closesocket()

// Ensure Winsock initialization in programs that use this header
static int windows_socket_init()
{
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

// Ensure Winsock cleanup in programs that use this header
static void windows_socket_cleanup()
{
    WSACleanup();
}

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR  // Fallback to SO_REUSEADDR if necessary
#endif

// Detect POSIX systems (Linux, macOS, etc.)
#else
// Include POSIX-specific headers
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h> // for close()
#ifndef SO_REUSEPORT
#define SO_REUSEPORT 0  // No-op on systems that don't support it
#endif
#endif

#endif
