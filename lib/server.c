#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include "net77/net_includes.h"
#include "net77/server.h"
#include "net77/http/serde.h"
#include "net77/thread_includes.h"
#include "net77/mcfss.h"
#include "net77/utils.h"
#include "net77/logging.h"
#include "net77/type_utils.h"

// TODO support HTTP/1.1's keep-alive connections: session support (so connections aren't immediately closed)

typedef struct RunServerArgs {
    void *callback_args;
    ThreadPool *thread_pool;
    const char *host;
    ServerWatcherCallbackFunc watcher_callback;
    RecvAllDataControllerCallback *keep_receiving_controller;
    ssize_t recv_timeout_usec;
    size_t max_request_size;
    ssize_t default_connection_timeout_usec;
    const UnsafeSignal *server_killed;
    UnsafeSignal *kill_ack;
    UnsafeSignal *has_started_signal;
    int port;
    int max_concurrent_connections;
    int max_conns_per_ip;
    int server_buf_size;
    int enable_delaying_sockets;
} RunServerArgs;

void threadStartServer(void *run_server_args) {
    RunServerArgs *args = run_server_args;
    void *callback_args = args->callback_args;
    ThreadPool *thread_pool = args->thread_pool;
    const char *host = args->host;
    int port = args->port;
    int max_concurrent_connections = args->max_concurrent_connections;
    int max_conns_per_ip = args->max_conns_per_ip;
    int server_buf_size = args->server_buf_size;
    ssize_t recv_timeout_usec = args->recv_timeout_usec;
    size_t max_request_size = args->max_request_size;
    ssize_t default_connection_timeout_usec = args->default_connection_timeout_usec;
    const UnsafeSignal *server_killed = args->server_killed;
    UnsafeSignal *kill_ack = args->kill_ack;
    UnsafeSignal *has_started_signal = args->has_started_signal;
    int enable_delaying_sockets = args->enable_delaying_sockets;
    ServerWatcherCallbackFunc watcher_callback = args->watcher_callback;
    RecvAllDataControllerCallback *keep_receiving_controller = args->keep_receiving_controller;
    free(run_server_args);
    if (has_started_signal)
        *has_started_signal = 1;
    runServer(callback_args, thread_pool, host, port, max_concurrent_connections, max_conns_per_ip, server_buf_size,
              recv_timeout_usec, max_request_size, default_connection_timeout_usec, server_killed, kill_ack,
              enable_delaying_sockets, watcher_callback, keep_receiving_controller);
}

size_t launchServerOnThread(void *callback_args, ThreadPool *thread_pool, const char *host, int port,
                            int max_concurrent_connections, int max_conns_per_ip, int server_buf_size,
                            ssize_t recv_timeout_usec, size_t max_request_size, ssize_t default_connection_timeout_usec,
                            const UnsafeSignal *server_killed, UnsafeSignal *kill_ack, int enable_delaying_sockets,
                            ServerWatcherCallbackFunc watcher_callback,
                            RecvAllDataControllerCallback *keep_receiving_controller,
                            UnsafeSignal *has_started_signal) {
    RunServerArgs *args = malloc(sizeof(RunServerArgs));
    args->callback_args = callback_args;
    args->thread_pool = thread_pool;
    args->host = host;
    args->port = port;
    args->max_concurrent_connections = max_concurrent_connections;
    args->max_conns_per_ip = max_conns_per_ip;
    args->server_buf_size = server_buf_size;
    args->recv_timeout_usec = recv_timeout_usec;
    args->max_request_size = max_request_size;
    args->default_connection_timeout_usec = default_connection_timeout_usec;
    args->server_killed = server_killed;
    args->kill_ack = kill_ack;
    args->has_started_signal = has_started_signal;
    args->enable_delaying_sockets = enable_delaying_sockets;
    args->watcher_callback = watcher_callback;
    args->keep_receiving_controller = keep_receiving_controller;
    return threadCreate(threadStartServer, args);
}

#if defined(_WIN32) || defined(_WIN64)

// TODO implement max_request_size and all the fancy features in the linux version
int runServer(ServerHandler handler, void *handler_data, const char *host,
              int port, int max_concurrent_connections, int server_buf_size, ssize_t request_timeout_usec, size_t max_request_size) {
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
            if ((read_chars = recv(new_socket, buffer, server_buf_size, 0)) < 0) {
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

typedef struct IpAddrStorage {
    int family;
    char addr[16];
} IpAddrStorage;

static IpAddrStorage ipAddrStorageFromSockaddr(struct sockaddr *client_addr) {
    IpAddrStorage out = {0, {0}};
    if (client_addr->sa_family == AF_INET) {
        out.family = AF_INET;
        void *addr_ptr = &((struct sockaddr_in *) client_addr)->sin_addr;
        memcpy(&out.addr, addr_ptr, sizeof(*addr_ptr));
    } else if (client_addr->sa_family == AF_INET6) {
        out.family = AF_INET6;
        void *addr_ptr = &((struct sockaddr_in6 *) client_addr)->sin6_addr;
        memcpy(&out.addr, addr_ptr, sizeof(*addr_ptr));
    } else {
        assert(0 && "Unknown address family.\n");
    }
    return out;
}

static size_t countInArray(const char *data, size_t len, const char *item, size_t item_size) {
    size_t count = 0;
    for (size_t i = 0; i < len; i++)
        if (memcmp(&data[i * item_size], item, item_size) == 0)
            count++;
    return count;
}

ErrorStatus
runServer(void *callback_args, ThreadPool *thread_pool, const char *host, int port, int max_concurrent_connections,
          int max_conns_per_ip, int server_buf_size, ssize_t recv_timeout_usec, size_t max_request_size,
          ssize_t default_connection_timeout_usec, const UnsafeSignal *server_killed, UnsafeSignal *kill_ack,
          int enable_delaying_sockets, ServerWatcherCallbackFunc watcher_callback,
          RecvAllDataControllerCallback *keep_receiving_controller) {
    assert(0 <= port && port <= 65536);
    if (max_concurrent_connections < 0)
        max_concurrent_connections = DEFAULT_MAX_ACCEPTED_CONNECTIONS;
    if (server_buf_size < 0)
        server_buf_size = DEFAULT_SERVER_BUF_SIZE;

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
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))
            || setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))
            || makeSocketNonBlocking(server_fd)) {
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

    // create "multi-category fixed-size set" for storing pollfds and last usage times in semantically joint, but
    // physically separate arrays
    size_t item_sizes[] = {sizeof(struct pollfd), sizeof(size_t), sizeof(ssize_t), sizeof(IpAddrStorage)};
    MultiCategoryFixedSizeSet timed_conn_pollfds = newMcfsSet(max_concurrent_connections, item_sizes,
                                                              sizeof(item_sizes) / sizeof(size_t));
    if (!timed_conn_pollfds.data[0])
        return 1;
    ConnState conn_state = {.stop_server = 0, .discard_conn = 0};

    while (!conn_state.stop_server && !*server_killed) {
        // accept new connections
        int new_socket;
        int new_conn_attempt = 0;
        while (timed_conn_pollfds.len < timed_conn_pollfds.cap
               && (new_socket = accept(server_fd, (struct sockaddr *) &client_addr, &addrlen)) >= 0) {
            new_conn_attempt = 1;
            if (makeSocketNonBlocking(new_socket)
                || setSocketKeepalive(new_socket)
                || enable_delaying_sockets ? 0 : setSocketNoDelay(new_socket)) {
                close(new_socket);
            } else {
                struct pollfd pfd;
                pfd.fd = new_socket;
                pfd.events = POLLALLIN | POLLERR | POLLHUP | POLLNVAL;
                pfd.revents = 0;
                size_t time = getTimeInUSecs();
                IpAddrStorage ip_addr_storage = ipAddrStorageFromSockaddr((struct sockaddr *) &client_addr);
                size_t n_conns_from_ip = countInArray(timed_conn_pollfds.data[3], timed_conn_pollfds.len,
                                                      (const char *) &ip_addr_storage, sizeof(ip_addr_storage));
                if (n_conns_from_ip < max_conns_per_ip) {
                    const char *items[] = {(const char *) &pfd, (const char *) &time,
                                           (const char *) &default_connection_timeout_usec,
                                           (const char *) &ip_addr_storage};
                    if (mcfsSetAdd(&timed_conn_pollfds, items, 0))
                        close(new_socket);  // weird error
                } else {
                    close(new_socket);  // client has too many open connections to this server
                }
            }
        }

        // poll for incoming data
        int poll_ret = poll((struct pollfd *) timed_conn_pollfds.data[0], timed_conn_pollfds.len, 0);
        if (poll_ret > 0) {
            struct pollfd *end = ((struct pollfd *) timed_conn_pollfds.data[0]) + timed_conn_pollfds.len;
            for (struct pollfd *pfd = (struct pollfd *) timed_conn_pollfds.data[0]; pfd < end; pfd++) {
                bool conn_was_closed = false;
                int fd = pfd->fd;

                if (pfd->revents & POLLALLIN) {
                    StringBuilder builder = newStringBuilder(server_buf_size);
                    size_t ssize_t_max = 0x7FFFFFFFFFFFFFFF;
                    ssize_t max_req_size = (ssize_t) (max_request_size > ssize_t_max ? ssize_t_max : max_request_size);
                    ErrorStatus err = recvAllData(fd, &builder, max_req_size, recv_timeout_usec, false,
                                                  server_buf_size, &conn_was_closed, keep_receiving_controller);
                    LOG_MSG("runServer received data from fd %d", fd);

                    // connection was closed
                    if (err || conn_was_closed || builder.len == 0) {
                        pfd->revents = 0;
                        if (!mcfsSetRemove(&timed_conn_pollfds, (const char *) pfd, 0) && err > 0)
                            close(fd);
                        conn_was_closed = 1;
                    } else {
                        // reset the last usage timer used for connection timeouts
                        size_t *time_last_usage_ptr = (size_t *) mcfsSetGetAssocItemPtr(&timed_conn_pollfds,
                                                                                        (const char *) pfd, 0, 1);
                        *time_last_usage_ptr = getTimeInUSecs();
                    }

                    // poll will trigger a POLLIN/POLLPRI event where recv will just return 0 instantly if connection is closed
                    if (err || (builder.len == 0 && conn_was_closed)) {
                        if (!err && conn_was_closed)
                            LOG_MSG(
                                    "runServer received data, but connection was closed before it could respond.");
                        stringBuilderDestroy(&builder);
                    } else {
                        String str = stringBuilderBuildAndDestroy(&builder);
                        ServerHandlerArgs args_data = {
                                .ctx = callback_args,
                                .socket_fd = fd,
                                .sock_is_closed = conn_was_closed,
                                .data = str.data,
                                .len = str.len,
                                .conn_state = &conn_state,
                                .heap_allocated = 0,
                        };
                        ServerHandlerArgs *args;
                        if (thread_pool->size) {
                            args_data.heap_allocated = 1;
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
                if (conn_was_closed) {
                    pfd--;
                    continue;
                }

                if (pfd->revents & (POLLERR | POLLHUP)) {
                    pfd->revents = 0;
                    if (!mcfsSetRemove(&timed_conn_pollfds, (const char *) &pfd, 0))
                        close(fd);
                    conn_was_closed = 1;
                }
                if (conn_was_closed) {
                    pfd--;
                    continue;
                }

                if (pfd->revents & POLLNVAL) {
                    pfd->revents = 0;
                    mcfsSetRemove(&timed_conn_pollfds, (const char *) &pfd, 0);
                    conn_was_closed = 1;
                }
                if (conn_was_closed) {
                    pfd--;
                    continue;
                }
            }
        }

        if (watcher_callback)
            watcher_callback(timed_conn_pollfds.len, new_conn_attempt, poll_ret, &timed_conn_pollfds, callback_args);

        // check for connection timeouts
        size_t time_now = getTimeInUSecs();
        for (size_t i = 0; i < timed_conn_pollfds.len; i++) {
            // create pollfd pointer
            size_t pfd_bytes_idx = i * sizeof(timed_conn_pollfds.item_sizes[0]);
            struct pollfd *pfd = (struct pollfd *) &timed_conn_pollfds.data[0][pfd_bytes_idx];
            // load last usage time
            size_t time_bytes_idx = i * sizeof(timed_conn_pollfds.item_sizes[1]);
            size_t time_last_usage = *(size_t *) &timed_conn_pollfds.data[1][time_bytes_idx];
            // load timeout
            size_t timeout_bytes_idx = i * sizeof(timed_conn_pollfds.item_sizes[2]);
            ssize_t conn_timeout = *(ssize_t *) &timed_conn_pollfds.data[2][timeout_bytes_idx];

            // check timeout
            if (conn_timeout >= 0 && time_now - time_last_usage > conn_timeout) {
                mcfsSetRemove(&timed_conn_pollfds, (const char *) pfd, 0);
                close(pfd->fd);
                i--;
            }
        }

        // if no events were triggered, yield the CPU
        if (poll_ret <= 0 && !new_conn_attempt)
            schedYield();
    }

    // close all open connections
    struct pollfd *end = (struct pollfd *) timed_conn_pollfds.data[0] + timed_conn_pollfds.len;
    for (struct pollfd *pfd = (struct pollfd *) timed_conn_pollfds.data[0]; pfd < end; pfd++) {
        close(pfd->fd);
    }
    mcfsSetDestroy(&timed_conn_pollfds);

    close(server_fd);

    // signal that the server is dead now (useful in multithreaded contexts)
    *kill_ack = 1;
    LOG_MSG("(runServer) server termination accepted (kill_ack set)");

    return 0;
}

#endif