#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "net77/net_includes.h"
#include "net77/server.h"
#include "net77/serde.h"
#include "net77/thread_includes.h"
#include "net77/mcfss.h"

// TODO support HTTP/1.1's keep-alive connections: session support (so connections aren't immediately closed)

typedef struct RunServerArgs {
    ThreadPool *thread_pool;
    void *handler_data;
    const char *host;
    int port;
    int max_concurrent_connections;
    int server_buf_size;
    int request_timeout_usec;
    size_t max_request_size;
    size_t connection_timeout_usec;
} RunServerArgs;

void threadStartServer(void *run_server_args) {
    RunServerArgs *args = run_server_args;
    ThreadPool *thread_pool = args->thread_pool;
    void *handler_data = args->handler_data;
    const char *host = args->host;
    int port = args->port;
    int max_concurrent_connections = args->max_concurrent_connections;
    int server_buf_size = args->server_buf_size;
    int request_timeout_usec = args->request_timeout_usec;
    size_t max_request_size = args->max_request_size;
    size_t connection_timeout_usec = args->connection_timeout_usec;
    free(run_server_args);
    runServer(thread_pool, handler_data, host, port, max_concurrent_connections, server_buf_size, request_timeout_usec,
              max_request_size, connection_timeout_usec);
}

size_t launchServerOnThread(ThreadPool *thread_pool, void *handler_data, const char *host,
                            int port, int max_concurrent_connections, int server_buf_size, int request_timeout_usec,
                            size_t max_request_size, size_t connection_timeout_usec) {
    RunServerArgs *args = malloc(sizeof(RunServerArgs));
    args->thread_pool = thread_pool;
    args->handler_data = handler_data;
    args->host = host;
    args->port = port;
    args->max_concurrent_connections = max_concurrent_connections;
    args->server_buf_size = server_buf_size;
    args->request_timeout_usec = request_timeout_usec;
    args->max_request_size = max_request_size;
    args->connection_timeout_usec = connection_timeout_usec;
    return threadCreate(threadStartServer, args);
}

#if defined(_WIN32) || defined(_WIN64)

// TODO implement max_request_size and all the fancy features in the linux version
int runServer(ServerHandler handler, void *handler_data, const char *host,
              int port, int max_concurrent_connections, int server_buf_size, int request_timeout_usec, size_t max_request_size) {
    if (max_concurrent_connections < 0)
        max_concurrent_connections = DEFAULT_MAX_ACCEPTED_CONNECTIONS;
    if (server_buf_size < 0)
        server_buf_size = SERVER_BUF_SIZE;

    size_t server_fd;
    struct addrinfo hints, *res, *p;
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);

    // Setting up hints for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;  // For wildcard IP address if host is NULL

    // Resolve the host name or IP address
    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        return 1;
    }

    // Try each address until we successfully bind
    for (p = res; p != NULL; p = p->ai_next) {
        // Create socket
        if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == INVALID_SOCKET) {
            continue;
        }

        // set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char *) &opt, sizeof(opt))) {
            close(server_fd);
            continue;
        }
        if (setSendRecvTimeout(server_fd, request_timeout_usec)) {
            close(server_fd);
            continue;
        }

        // Bind to the address
        if (bind(server_fd, p->ai_addr, (socklen_t) p->ai_addrlen) == 0) {
            break;  // Successfully bound
        }

        close(server_fd);
    }

    freeaddrinfo(res);

    if (p == NULL) {  // No address succeeded
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, max_concurrent_connections) < 0) {
        return 1;
    }

    size_t new_socket;
    int stop = 0;
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof(client_addr);

    char *buffer = (char *) malloc(server_buf_size);
    if (!buffer)
        return 1;

    while (!stop) {
        if ((new_socket = accept(server_fd, (struct sockaddr *) &client_addr, &addrlen)) < 0)
            continue;
        if (setSendRecvTimeout(new_socket, request_timeout_usec)) {
            close(new_socket);
            continue;
        }
        // FIXME recv actually returns 0 when connection close, not when all data sent
        StringBuilder builder = newStringBuilder(server_buf_size);
        ssize_t read_chars;
        int failed = 0;
        do {
            if ((read_chars = recv(new_socket, buffer, server_buf_size)) < 0) {
                failed = 1;
                break;
            }
            stringBuilderAppend(&builder, buffer, read_chars);
        } while (read_chars != 0);

        if (failed) {
            stringBuilderDestroy(&builder);
        } else {
            String str = stringBuilderBuildAndDestroy(&builder);
            stop = handler(handler_data, new_socket, str.data, str.len);
        }

        close(new_socket);
    }

    free(buffer);
    close(server_fd);
    return 0;
}

#else

#include <poll.h>
#include <fcntl.h>
#include <sys/time.h>

static size_t getTimeInUSecs() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (size_t) (time.tv_sec) * 1000000 + (size_t) (time.tv_usec);
}

int makeNonBlocking(int fd) {
    int flags, fco;
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
        return flags;
    if ((fco = fcntl(fd, F_SETFL, flags | O_NONBLOCK)) < 0)
        return fco;
    return 0;
}

int runServer(ThreadPool *thread_pool, void *handler_data, const char *host, int port, int max_concurrent_connections,
              int server_buf_size, int request_timeout_usec, size_t max_request_size, size_t connection_timeout_usec) {
    if (max_concurrent_connections < 0)
        max_concurrent_connections = DEFAULT_MAX_ACCEPTED_CONNECTIONS;
    if (server_buf_size < 0)
        server_buf_size = SERVER_BUF_SIZE;

    int server_fd;
    struct addrinfo hints, *res, *p;
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);

    // Setting up hints for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;  // For wildcard IP address if host is NULL

    // Resolve the host name or IP address
    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        return 1;
    }

    // Try each address until we successfully bind
    for (p = res; p != NULL; p = p->ai_next) {
        // Create socket
        if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }

        // set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))
            || makeNonBlocking(server_fd)) {
            close(server_fd);
            continue;
        }

        // Bind to the address
        if (bind(server_fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;  // Successfully bound
        }

        close(server_fd);
    }

    freeaddrinfo(res);

    if (p == NULL) {  // No address succeeded
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, max_concurrent_connections) < 0) {
        return 1;
    }

    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof(client_addr);

    char *buffer = malloc(server_buf_size);
    if (!buffer)
        return 1;
    size_t item_sizes[2] = {sizeof(struct pollfd), sizeof(size_t)};
    MultiCategoryFixedSizeSet timed_conn_pollfds = newMcfsSet(max_concurrent_connections, item_sizes, 2);
    if (!timed_conn_pollfds.data[0])
        return 1;
    ConnState conn_state = {.stop_server = 0, .discard_conn = 0};

    while (!conn_state.stop_server) {
        int new_socket;
        if (timed_conn_pollfds.len < timed_conn_pollfds.cap &&
            (new_socket = accept(server_fd, (struct sockaddr *) &client_addr, &addrlen)) >= 0) {
            if (setSendRecvTimeout(new_socket, request_timeout_usec)) {
                close(new_socket);
            } else {
                struct pollfd pfd;
                pfd.fd = new_socket;
                pfd.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
                pfd.revents = 0;
                size_t time = getTimeInUSecs();
                const char *items[] = {(const char *) &pfd, (const char *) &time};
                if (mcfsSetAdd(&timed_conn_pollfds, items, 0))
                    close(new_socket);
            }
        }

        int poll_ret = poll((struct pollfd *) timed_conn_pollfds.data[0], timed_conn_pollfds.len, 0);
        if (poll_ret > 0) {
            struct pollfd *end = ((struct pollfd *) timed_conn_pollfds.data[0]) + timed_conn_pollfds.len;
            for (struct pollfd *pfd = (struct pollfd *) timed_conn_pollfds.data[0]; pfd < end; pfd++) {
                int conn_was_closed = 0;
                int fd = pfd->fd;

                if (pfd->revents & POLLIN) {
                    StringBuilder builder = newStringBuilder(server_buf_size);
                    ssize_t read_chars;
                    int failed = 0;
                    for (;;) {
                        if ((read_chars = recv(fd, buffer, server_buf_size)) <= 0)
                            break;
                        stringBuilderAppend(&builder, buffer, read_chars);
                        if (builder.len > max_request_size) {
                            // discard request (too big)
                            failed = 1;
                            break;
                        }
                    }

                    // connection was closed
                    if (read_chars == 0 || failed) {
                        pfd->revents = 0;
                        if (!mcfsSetRemove(&timed_conn_pollfds, (const char *) pfd, 0))
                            close(fd);
                        conn_was_closed = 1;
                    } else {
                        // reset the last usage timer used for connection timeouts
                        size_t *time_last_usage_ptr = (size_t *) mcfsSetGetAssocItemPtr(&timed_conn_pollfds,
                                                                                        (const char *) pfd, 0, 1);
                        *time_last_usage_ptr = getTimeInUSecs();
                    }

                    if (failed) {
                        stringBuilderDestroy(&builder);
                    } else {
                        String str = stringBuilderBuildAndDestroy(&builder);
                        ServerHandlerArgs args_data = {
                                .ctx = handler_data,
                                .socket_fd = fd,
                                .sock_is_closed = conn_was_closed,
                                .data = str.data,
                                .len = str.len,
                                .conn_state = &conn_state,
                                .heap_allocated = 0,
                        };
                        ServerHandlerArgs *args;
                        if (thread_pool->size) {
                            args = malloc(sizeof(ServerHandlerArgs));
                            *args = args_data;
                        } else {
                            args = &args_data;
                        }
                        threadPoolDispatchWork(thread_pool, args);
                    }

                    if (conn_state.discard_conn && !conn_was_closed) {
                        pfd->revents = 0;
                        if (!mcfsSetRemove(&timed_conn_pollfds, (const char *) &pfd, 0))
                            close(fd);
                        conn_was_closed = 1;
                    }
                    conn_state.discard_conn = 0;
                }
                if (conn_was_closed)
                    continue;

                if (pfd->revents & (POLLERR | POLLHUP)) {
                    pfd->revents = 0;
                    if (!mcfsSetRemove(&timed_conn_pollfds, (const char *) &pfd, 0))
                        close(fd);
                    conn_was_closed = 1;
                }
                if (conn_was_closed)
                    continue;

                if (pfd->revents & POLLNVAL) {
                    pfd->revents = 0;
                    mcfsSetRemove(&timed_conn_pollfds, (const char *) &pfd, 0);
                    conn_was_closed = 1;
                }
                if (conn_was_closed)
                    continue;
            }
        }

        size_t time_now = getTimeInUSecs();
        for (int i = 0; i < timed_conn_pollfds.len; i++) {
            size_t pfd_bytes_idx = i * sizeof(timed_conn_pollfds.item_sizes[0]);
            struct pollfd *pfd = (struct pollfd *) &timed_conn_pollfds.data[0][pfd_bytes_idx];
            size_t time_bytes_idx = i * sizeof(timed_conn_pollfds.item_sizes[1]);
            size_t time_last_usage = *(size_t *) &timed_conn_pollfds.data[1][time_bytes_idx];
            if (time_now - time_last_usage > connection_timeout_usec) {
                mcfsSetRemove(&timed_conn_pollfds, (const char *) pfd, 0);
                close(pfd->fd);
                i--;
            }
        }
    }

    free(buffer);

    // close all open connections
    struct pollfd *end = (struct pollfd *) timed_conn_pollfds.data[0] + timed_conn_pollfds.len;
    for (struct pollfd *pfd = (struct pollfd *) timed_conn_pollfds.data[0]; pfd < end; pfd++) {
        close(pfd->fd);
    }
    mcfsSetDestroy(&timed_conn_pollfds);

    close(server_fd);
    return 0;
}

#endif