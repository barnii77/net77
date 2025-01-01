#ifndef NET77_NETINCLUDES_H
#define NET77_NETINCLUDES_H

#include <stddef.h>
#include "net77/init.h"
#include "net77/int_includes.h"
#include "net77/string_utils.h"
#include "net77/error_utils.h"

ErrorStatus setSendRecvTimeout(size_t fd, ssize_t timeout_usec);

/**
 * @param socket_fd the socket file descriptor
 * @param buf the buffer to send the data from
 * @param len the length of the buffer
 * @param timeout_usec the timeout in microseconds
 * @return 0 on success, x != 0 on error: returns 1 on normal error and -1 on error that lead to the socket being closed.
 */
ErrorStatus sendAllData(size_t socket_fd, const char *buf, size_t len, ssize_t timeout_usec);

/**
 * @param socket_fd the socket file descriptor
 * @param buf the buffer to write the data to
 * @param len the length of the buffer
 * @param timeout_usec the timeout in microseconds
 * @param out_bytes_received if not NULL, will be set to the number of bytes received
 * @return 0 on success, x != 0 on error: returns 1 on normal error and -1 on error that lead to the socket being closed.
 */
ErrorStatus recvAllData(size_t socket_fd, char *buf, size_t len, ssize_t timeout_usec, size_t *out_bytes_received);

/**
 * @param socket_fd the socket file descriptor
 * @param builder the StringBuilder to write the data to
 * @param max_len the maximum length of the data to receive. If max_len <= 0, there is no limit.
 * @param timeout_usec the timeout in microseconds
 * @param sb_min_cap the minimum remaining capacity of the StringBuilder:
 * sb.cap - sb.len < sb_min_cap -> stringBuilderExpandBuf(sb, sb.len + sb_min_cap).
 * If sb_min_cap < 0, the default value (DEFAULT_RECV_ALL_DATA_SB_MIN_CAP) is used.
 * @return 0 on success, x != 0 on error: returns 1 on normal error and -1 on error that lead to the socket being closed.
 * @note It is the callers responsibility to free the StringBuilder
 */
ErrorStatus recvAllDataSb(size_t socket_fd, StringBuilder *builder, ssize_t max_len, ssize_t timeout_usec, ssize_t sb_min_cap);

#if defined(_WIN32) || defined(_WIN64)
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
// TODO fix this stuff eventually

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR  // Fallback to SO_REUSEADDR if necessary
#endif

#else
// Include POSIX-specific headers
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <poll.h>

#ifndef SO_REUSEPORT
#define SO_REUSEPORT 0  // No-op on systems that don't support it
#endif
#endif
#endif