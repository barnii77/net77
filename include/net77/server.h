#ifndef NET77_SERVER_H
#define NET77_SERVER_H

#include <stddef.h>
#include "net77/thread_pool.h"
#include "net77/mcfss.h"
#include "net77/type_utils.h"
#include "net77/net_includes.h"

#define DEFAULT_SERVER_BUF_SIZE (32768)
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

typedef void (*ServerHandler)(void *callback_args);

typedef void (*ServerWatcherCallbackFunc)(size_t n_open_conns, int new_conn_attempt, int poll_ret,
                                          MultiCategoryFixedSizeSet *conn_states, void *callback_args);

/**
 * helper function for hosting a tcp server
 * @param callback_args some opaque data all callbacks are passed that may point to some state which is controlled by another thread
 * @param thread_pool pointer to thread pool with handler that takes (some_opaque_data, socket_fd, sock_is_closed, data_received, data_received_size, conn_state)
 * @param host the host address to serve under, eg "127.0.0.1" (may also be IPv6). If NULL, then accepts requests from any local address (127.0.0.1, 192.168.0.1, 10.1.1.xy, ...)
 * @param port the port to serve under
 * @param max_concurrent_connections the maximum number of connections (sockets) that may simultaneously be open.
 * If x < 0 is passed, use default of 16. For unlimited, just pass 9999999 :)... \n WARNING: this parameter immediately
 * pre-allocates an array of exactly that number of elements, so don't pass ridiculously high values.
 * @param max_conns_per_ip the maximum number of connections that a single ip can maintain concurrently to the server.
 * If x < 0 is passed, use default of 4. For unlimited, just pass 9999999 :)
 * @param server_buf_size the init size of the buffer that the received data is written to before it is realloc-ed to expand.
 * If x < 0 is passed, use default of 1kB
 * @param recv_timeout_usec after how many microseconds a recv times out and received data is processed
 * @param max_request_size maximum size (in bytes) that the data of a httpRequest sent to the server must have before it is discarded and ignored. 0 means no limit.
 * @param default_connection_timeout_usec after how many microseconds a httpRequest times out
 * @param server_killed pointer to int. server terminates gracefully when the value pointed to turns non-zero.
 * @param kill_ack set to one when the server has been killed, all conns have been closed and all buffers freed.
 * @param enable_delaying_sockets if non-zero, nagle's algorithm will *not* be disabled. This may introduce latency.
 * @param server_watcher_callback nullable callback (function) that gets called every server main loop iteration. May
 * potentially intervene with the server directly by modifying connection states, so "watcher" here refers to either
 * passive watching and monitoring of the server or active modification of connection states. It gets called after
 * receiving all the new data from the connections and passing it to the handler thread pool, but before the connection
 * timeout checks.
 * @param keep_receiving_controller nullable callback that gets called while in the loop receiving data and is used
 * to determine based on the data already received whether/how much to keep receiving (by e.g. looking at headers)
 * @return err (0 means success)
 */
ErrorStatus
runServer(void *callback_args, ThreadPool *thread_pool, const char *host, int port, int max_concurrent_connections,
          int max_conns_per_ip, int server_buf_size, ssize_t recv_timeout_usec, size_t max_request_size,
          ssize_t default_connection_timeout_usec, const UnsafeSignal *server_killed, UnsafeSignal *kill_ack,
          int enable_delaying_sockets, ServerWatcherCallbackFunc watcher_callback,
          RecvAllDataControllerCallback *keep_receiving_controller);

size_t launchServerOnThread(void *callback_args, ThreadPool *thread_pool, const char *host, int port,
                            int max_concurrent_connections, int max_conns_per_ip, int server_buf_size,
                            ssize_t recv_timeout_usec, size_t max_request_size, ssize_t default_connection_timeout_usec,
                            const UnsafeSignal *server_killed, UnsafeSignal *kill_ack, int enable_delaying_sockets,
                            ServerWatcherCallbackFunc watcher_callback,
                            RecvAllDataControllerCallback *keep_receiving_controller, UnsafeSignal *has_started_signal);

#endif