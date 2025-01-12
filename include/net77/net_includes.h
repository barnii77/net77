#ifndef NET77_NET_INCLUDES_H
#define NET77_NET_INCLUDES_H

#include <stddef.h>
#include "net77/init.h"
#include "net77/int_includes.h"
#include "net77/string_utils.h"
#include "net77/type_utils.h"

typedef enum RecvAllDataControllerCommandedAction {
    RECV_ALL_DATA_ACTION_NONE = 0,
    RECV_ALL_DATA_ACTION_REJECT_REQUEST,
    RECV_ALL_DATA_ACTION_FINISHED,
} RecvAllDataControllerCommandedAction;

typedef struct StringBuilder StringBuilder;

/**
 * The function that controls the data receiving procedure of the recvAllData function family based on HTTP headers.
 * @param buf the buffer accumulating what has been read so far. Allows the next higher protocol to check
 * headers without the receiver function having to know about that layer.
 * @param cap the buffer capacity
 * @param n number of bytes received so far
 * @param max_len pointer to the max_len that the user wishes to receive. This may be modified by this callback based
 * on information in header fields that might already be available in the buffer. Of course, you must not overwrite
 * the value to something bigger than what the user passed (unless you REALLY know what you are doing). To stop
 * receiving immediately, set max_len to size.
 * @param next_action out parameter for the callback to instruct the recvAllData function on how to behave.
 * Possible values: ACTION_NONE, ACTION_REJECT_REQUEST.
 * For example, might be used when the request is missing a required header field.
 * @param is_finalize_call set to true (1) for the finalization call, which is a special invocation of this callback
 * that happens after all data has been received and right before the recvAllData function returns it's result.
 * It can be used to apply transformations to the received data by modifying buf.
 * @param controller_state opaque pointer to be passed to the callback so the callback can be stateful across multiple
 * calls.
 */
typedef void (*RecvAllDataControllerCallbackFunc)(const char *nonnull buf, size_t cap, size_t n,
                                                  ssize_t *nonnull max_len,
                                                  RecvAllDataControllerCommandedAction *nonnull next_action,
                                                  void *nonnull controller_state);

/// A function bundled with state metadata and optional state
typedef struct RecvAllDataControllerCallback {
    /// the callback function pointer
    RecvAllDataControllerCallbackFunc nonnull fn;
    /// *required*. the size of the opaque state that the callback takes as it's last argument
    size_t sizeof_state;
    /// *nullable*. Allows the caller to pass in the state themselves and/or share state. If NULL, the callee will
    /// allocate sufficient zero-initialized memory to hold a state struct of that size.
    void *nullable shared_state;
} RecvAllDataControllerCallback;

/**
 * close an open socket (this is kind of unsafe and does not check if the socket is open at all)
 * @param fd the socket file descriptor as a size_t for OS-independence
 */
void closeSocket(size_t fd);

ErrorStatus setSendRecvTimeout(size_t fd, ssize_t timeout_usec);

/**
 * @param socket_fd the socket file descriptor
 * @param buf the buffer to send the data from
 * @param len the length of the buffer
 * @param timeout_usec the timeout in microseconds
 * @param out_closed_sock *optional* out param set to 1 if the socket was closed or is invalid, otherwise set to 0
 * @return 0 on success, x != 0 on error: returns 1 on normal error and -1 on error that lead to the socket being closed.
 */
ErrorStatus
sendAllData(size_t socket_fd, const char *nonnull buf, size_t len, ssize_t timeout_usec,
            bool *nullable out_closed_sock);

/**
 * @param socket_fd the socket file descriptor
 * @param builder the StringBuilder to write the data to
 * @param max_len the maximum length of the data to receive. If max_len <= 0, there is no limit.
 * @param timeout_usec the timeout in microseconds
 * @param timeout_is_error whether a timeout sets returned ErrorStatus to 1 (recoverable error) or 0 (no error)
 * @param sb_min_cap the minimum remaining capacity of the StringBuilder:
 * sb.cap - sb.len < sb_min_cap -> stringBuilderExpandBuf(sb, sb.len + sb_min_cap).
 * If sb_min_cap < 0, the default value (DEFAULT_RECV_ALL_DATA_SB_MIN_CAP) is used.
 * @param out_closed_sock *optional* out param set to 1 if the socket was closed or is invalid, otherwise set to 0
 * @param keep_receiving_controller *nullable* callback that allows controlling whether to keep reding or not based on
 * what has already been received
 * @return 0 on success, x != 0 on error: returns 1 on normal error and -1 on error that lead to the socket being
 * closed. WARNING: the return value being 0 (success) does NOT imply the socket is still open. E.g. in HTTP/1.0, the
 * socket is closed when the full response has been sent but even though the socket has been closed, the data was
 * still successfully received. It is therefore important to ALWAYS check out_closed_sock.
 * @note It is the callers responsibility to free the StringBuilder
 */
ErrorStatus
recvAllData(size_t socket_fd, StringBuilder *nonnull builder, ssize_t max_len, ssize_t timeout_usec,
            bool timeout_is_error, ssize_t sb_min_cap, bool *nullable out_closed_sock,
            RecvAllDataControllerCallback *nullable keep_receiving_controller);

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

// helper to select both types of TCP inputs (normal and prioritizes)
#define POLLALLIN POLLIN  // should maybe be (POLLIN | POLLPRI)

#ifndef SO_REUSEPORT
#define SO_REUSEPORT 0  // No-op on systems that don't support it
#endif
#endif
#endif