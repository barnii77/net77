#ifndef NET77_N77SERVER_H
#define NET77_N77SERVER_H

#define SERVER_BUF_SIZE (1024)

#define DEFAULT_MAX_ACCEPTED_CONNECTIONS (16)

/**
 *
 * @param handler pointer to function that takes (some_opaque_data, socket_fd, data_received, data_received_size)
 * @param handler_data some opaque data the handler function is passed that may point to some state which is controlled by another thread
 * @param host the host address to serve under, eg "127.0.0.1" (may also be IPv6). If NULL, then accepts requests from any local address (127.0.0.1, 192.168.0.1, 10.1.1.xy, ...)
 * @param port the port to serve under
 * @param max_concurrent_connections the maximum number of connections (sockets) that may simultaneously be open.
 * If -1 is passed, use default of 16
 * @param server_buf_size the init size of the buffer that the received data is written to before it is realloc-ed to expand.
 * If -1 is passed, use default of 1kB
 * @return err (0 means success)
 */
int runServer(int (*handler)(void *, size_t, char *, size_t), void *handler_data, const char *host,
              int port, int max_concurrent_connections, int server_buf_size);

#endif