#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include "n77sock.h"
#include "n77stringutils.h"

int newSocketSendReceiveClose(const char *host, int port, StringRef data, String *out, int client_buf_size) {
    if (client_buf_size < 0)
        client_buf_size = DEFAULT_CLIENT_BUF_SIZE;

    int client_fd;
    struct addrinfo hints, *res, *p;
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);

    // Setting up hints for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP stream socket

    // Resolve the host name or IP address
    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        return 1;
    }

    // Try each address until we successfully connect
    for (p = res; p != NULL; p = p->ai_next) {
        // Create socket
        if ((client_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }

        // Connect to the server
        if (connect(client_fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;  // Successfully connected
        }

        close(client_fd);
    }

    freeaddrinfo(res);

    if (p == NULL) {  // No address succeeded
        return 1;
    }

    // Send data
    if (send(client_fd, data.data, data.len, 0) < 0) {
        close(client_fd);
        return 1;
    }

    // Receive response
    StringBuilder builder = newStringBuilder(client_buf_size);

    ssize_t read_chars;
    do {
        char buffer[client_buf_size];
        read_chars = read(client_fd, buffer, client_buf_size);
        if (read_chars < 0) {
            close(client_fd);
            return 1;
        }
        stringBuilderAppend(&builder, buffer, read_chars);
    } while (read_chars != 0);

    *out = stringBuilderBuildAndDestroy(&builder);

    // Close the socket
    close(client_fd);
    return 0;
}