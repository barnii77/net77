#ifndef NET77_SERVER_H
#define NET77_SERVER_H

#include <stddef.h>
#include "net77/thread_pool.h"

#define SERVER_BUF_SIZE (32768)
#define DEFAULT_MAX_ACCEPTED_CONNECTIONS (16)

typedef struct ConnState {
    int stop_server;
    int discard_conn;
} ConnState;

typedef struct ServerHandlerArgs {
    void *ctx;
    size_t socket_fd;
    char *data;
    size_t len;
    ConnState *conn_state;
    int sock_is_closed;
    int heap_allocated;
} ServerHandlerArgs;

typedef void (*ServerHandler)(void *server_handler_args);

// TODO I could add a callback parameter that gets called with poll_ret (ie occupation data), number of connections, etc.
// which gets called every server main loop iteration. This could allow monitoring and all derivatives of that.
// TODO this function may accept a ton of requests from a single client and then block other clients from connecting
// the above could be fixed by taking in a callback that determines whether to accept a connection or not or
// by having a max number of requests that can be accepted from a single client. Probably the latter is better.
// TODO handle SIGPIPE and kill the connection if it is received... not quite sure how to implement that
// TODO conditional timeouts: the handler should be able to decide on a per-connection basis what the timeout for this
// connection should be. This is so that you can easily support keep-alive connections while still providing
// timeouts for other connections.
/**
 * helper function for hosting a tcp server
 * @param thread_pool pointer to thread pool with handler that takes (some_opaque_data, socket_fd, sock_is_closed, data_received, data_received_size, conn_state)
 * @param handler_data some opaque data the handler function is passed that may point to some state which is controlled by another thread
 * @param host the host address to serve under, eg "127.0.0.1" (may also be IPv6). If NULL, then accepts requests from any local address (127.0.0.1, 192.168.0.1, 10.1.1.xy, ...)
 * @param port the port to serve under
 * @param max_concurrent_connections the maximum number of connections (sockets) that may simultaneously be open.
 * If -1 is passed, use default of 16
 * @param server_buf_size the init size of the buffer that the received data is written to before it is realloc-ed to expand.
 * If -1 is passed, use default of 1kB
 * @param recv_timeout_usec after how many microseconds a recv times out and received data is processed
 * @param max_request_size maximum size (in bytes) that the data of a request sent to the server must have before it is discarded and ignored. 0 means no limit.
 * @param connection_timeout_usec after how many microseconds a request times out
 * @param server_killed pointer to int. server terminates gracefully when the value pointed to turns non-zero.
 * @param kill_ack set to one when the server has been killed, all conns have been closed and all buffers freed.
 * @param enable_delaying_sockets if non-zero, nagle's algorithm will *not* be disabled. This may introduce latency.
 * @return err (0 means success)
 */
int runServer(ThreadPool *thread_pool, void *handler_data, const char *host, int port, int max_concurrent_connections,
              int server_buf_size, ssize_t recv_timeout_usec, size_t max_request_size, ssize_t connection_timeout_usec,
              const UnsafeSignal *server_killed, UnsafeSignal *kill_ack, int enable_delaying_sockets);

size_t launchServerOnThread(ThreadPool *thread_pool, void *handler_data, const char *host, int port,
                            int max_concurrent_connections, int server_buf_size, ssize_t recv_timeout_usec,
                            size_t max_request_size, ssize_t connection_timeout_usec, const UnsafeSignal *server_killed,
                            UnsafeSignal *kill_ack, int enable_delaying_sockets, UnsafeSignal *has_started_signal);

#endif