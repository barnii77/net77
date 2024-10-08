#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "n77netincludes.h"
#include "n77server.h"
#include "n77serde.h"

// TODO support HTTP/1.1's keep-alive connections: session support (so connections aren't immediately closed)

#if defined(_WIN32) || defined(_WIN64)
int runServer(int (*handler)(void *, size_t, char *, size_t), void *handler_data, const char *host,
              int port, int max_concurrent_connections, int server_buf_size) {
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

        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char *)&opt, sizeof(opt))) {
            close(server_fd);
            continue;
        }

        // Bind to the address
        if (bind(server_fd, p->ai_addr, (socklen_t)p->ai_addrlen) == 0) {
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

    char *buffer = (char *)malloc(server_buf_size);
    if (!buffer)
        return 1;

    while (!stop) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen)) == INVALID_SOCKET) {
            free(buffer);
            return 1;
        }

        StringBuilder builder = newStringBuilder(server_buf_size);
        ssize_t read_chars;
        do {
            // recv requires an additional flags argument, set to 0 for default behavior
            read_chars = recv(new_socket, buffer, server_buf_size, 0);
            if (read_chars < 0) {
                free(buffer);
                return 1;
            }
            stringBuilderAppend(&builder, buffer, read_chars);
        } while (read_chars != 0);

        String str = stringBuilderBuildAndDestroy(&builder);
        stop = handler(handler_data, new_socket, str.len == 0 ? NULL : str.data, str.len);

        close(new_socket);
    }

    free(buffer);
    close(server_fd);
    return 0;
}
#else
int runServer(int (*handler)(void *, size_t, char *, size_t), void *handler_data, const char *host,
              int port, int max_concurrent_connections, int server_buf_size) {
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

        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
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

    int new_socket, stop = 0;
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof(client_addr);

    char *buffer = malloc(server_buf_size);
    if (!buffer)
        return 1;
    while (!stop) {
        if ((new_socket = accept(server_fd, (struct sockaddr *) &client_addr, &addrlen)) < 0) {
            return 1;
        }

        StringBuilder builder = newStringBuilder(server_buf_size);
        ssize_t read_chars;
        do {
            if ((read_chars = read(new_socket, buffer, server_buf_size)) < 0) {
                free(buffer);
                return 1;
            }
            stringBuilderAppend(&builder, buffer, read_chars);
        } while (read_chars != 0);

        String str = stringBuilderBuildAndDestroy(&builder);
        stop = handler(handler_data, new_socket, str.len == 0 ? NULL : str.data, str.len);

        close(new_socket);
    }
    free(buffer);

    close(server_fd);
    return 0;
}
#endif