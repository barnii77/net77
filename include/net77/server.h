#ifndef NET77_SERVER_H
#define NET77_SERVER_H

#include <stddef.h>
#include "thread_pool.h"

#define SERVER_BUF_SIZE (1024)

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
 * @param request_timeout_usec after how many microseconds a request times out
 * @param max_request_size maximum size (in bytes) that the data of a request sent to the server must have before it is discarded and ignored. 0 means no limit.
 * @param connection_timeout_usec after how many microseconds a request times out
 * @return err (0 means success)
 */
int runServer(ThreadPool *thread_pool, void *handler_data, const char *host, int port, int max_concurrent_connections,
              int server_buf_size, int request_timeout_usec, size_t max_request_size, size_t connection_timeout_usec);

size_t launchServerOnThread(ThreadPool *thread_pool, void *handler_data, const char *host, int port,
                            int max_concurrent_connections, int server_buf_size, int request_timeout_usec,
                            size_t max_request_size, size_t connection_timeout_usec);

#endif