#include <string.h>
#include <assert.h>
#include "net77/logging.h"
#include "net77/net_includes.h"
#include "net77/sock.h"
#include "net77/string_utils.h"
#include "net77/utils.h"
#include "net77/type_utils.h"

#define DEFAULT_CLIENT_BUF_SIZE (8*1024)
#define DEFAULT_SERVER_CONNECT_TIMEOUT_USEC 5000000
#define DEFAULT_SERVER_RESPONSE_TIMEOUT_USEC 5000000

#if defined(_WIN32) || defined(_WIN64)

int newSocketSendReceiveClose(const char *host, int port, StringRef data, String *out, int client_buf_size,
                              ssize_t request_timeout_usec) {
    if (client_buf_size < 0)
        client_buf_size = DEFAULT_CLIENT_BUF_SIZE;

    size_t client_fd;
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
        if ((client_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == INVALID_SOCKET) {
            continue;
        }

        // set socket options
        int opt = 1;
        if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (const char *) &opt, sizeof(opt))) {
            close(client_fd);
            continue;
        }
        if (setSendRecvTimeout(client_fd, request_timeout_usec)) {
            close(client_fd);
            continue;
        }

        // Connect to the server
        if (connect(client_fd, p->ai_addr, (socklen_t) p->ai_addrlen) == 0) {
            break;  // Successfully connected
        }

        close(client_fd);
    }

    freeaddrinfo(res);

    if (p == NULL) {  // No address succeeded
        return 1;
    }

    // Send data
    if (sendAllData(client_fd, data.data, (int) data.len) < 0) {
        close(client_fd);
        return 1;
    }

    // Receive response
    StringBuilder builder = newStringBuilder(client_buf_size);

    ssize_t read_chars;
    char *buffer = malloc(client_buf_size);
    do {
        read_chars = recv(client_fd, buffer, client_buf_size, 0);
        if (read_chars < 0) {
            close(client_fd);
            free(buffer);
            return 1;
        }
        stringBuilderAppend(&builder, buffer, read_chars);
    } while (read_chars != 0);

    *out = stringBuilderBuildAndDestroy(&builder);

    // Close the socket
    close(client_fd);
    free(buffer);
    return 0;
}

#else

ErrorStatus connectSocket(const char *host, int port, ssize_t server_connect_timeout_usec, int enable_delaying_sockets,
                          size_t *out_fd) {
    LOG_MSG("called connectSocket");
    if (server_connect_timeout_usec < 0)
        server_connect_timeout_usec = DEFAULT_SERVER_CONNECT_TIMEOUT_USEC;
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
        return -1;
    }

    // Try each address until we successfully connect
    for (p = res; p != NULL; p = p->ai_next) {
        // Create socket
        if ((client_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }

        // set socket options
        int opt = 1;
        if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))
            || setSendRecvTimeout(client_fd, server_connect_timeout_usec)
            || setSocketKeepalive(client_fd)
            || enable_delaying_sockets ? 0 : setSocketNoDelay(client_fd)) {
            close(client_fd);
            continue;
        }

        // Connect to the server
        if (connect(client_fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;  // Successfully connected
        }
        LOG_MSG("connectSocket connect process failed");

        close(client_fd);
    }

    freeaddrinfo(res);

    if (p == NULL) {  // No address succeeded
        return -1;
    }

    if (makeSocketNonBlocking(client_fd)) {
        close(client_fd);
        return -1;
    }

    *out_fd = client_fd;
    return 0;
}

#define SET_OPTIONAL_SIGNAL(sig, value) if (sig) *sig = value

ErrorStatus waitThenRecvAllData(size_t fd, ssize_t server_response_timeout_usec, ssize_t response_done_timeout_usec,
                                bool response_done_timeout_is_error, int client_buf_size, size_t max_response_size,
                                String *out, bool *out_closed_sock,
                                RecvAllDataControllerCallback *keep_receiving_controller) {
    if (client_buf_size < 0)
        client_buf_size = DEFAULT_CLIENT_BUF_SIZE;
    if (server_response_timeout_usec < 0)
        server_response_timeout_usec = DEFAULT_SERVER_RESPONSE_TIMEOUT_USEC;
    SET_OPTIONAL_SIGNAL(out_closed_sock, 0);
    int client_fd = (int) fd;
    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLALLIN | POLLERR | POLLHUP | POLLNVAL;
    pfd.revents = 0;
    int timeout_ms = (int) ((server_response_timeout_usec + 999) / 1000);

    // Poll once with server_response_timeout_usec timeout to give the server a chance to respond
    int poll_out = poll(&pfd, 1, timeout_ms);

    if (poll_out < 0 || !poll_out) {
        // timeout
        const char *msg;
        if (poll_out == 0)
            msg = "waitThenRecvAllData poll timeout";
        else
            msg = "waitThenRecvAllData unknown poll error";
        LOG_MSG(msg);
        return 1;
    } else if (pfd.revents & POLLERR) {
        // socket error
        close(client_fd);
        SET_OPTIONAL_SIGNAL(out_closed_sock, 1);
        LOG_MSG("waitThenRecvAllData POLLERR (-> closing socket)");
        return -1;
    } else if (pfd.revents & POLLHUP) {
        // other side closed conn
        close(client_fd);
        SET_OPTIONAL_SIGNAL(out_closed_sock, 1);
        LOG_MSG("waitThenRecvAllData POLLHUP (-> closing socket)");
        return 0;
    } else if (pfd.revents & POLLNVAL) {
        // socket isn't even open
        SET_OPTIONAL_SIGNAL(out_closed_sock, 1);
        LOG_MSG("waitThenRecvAllData POLLNVAL");
        // return -1 to signify the socket is already closed / has never existed
        return -1;
    } else {
        // ok
        assert(pfd.revents & POLLALLIN);
        LOG_MSG("waitThenRecvAllData initial poll done (POLLIN/POLLPRI set)");
    }

    // Receive response
    StringBuilder builder = newStringBuilder(client_buf_size);
    size_t ssize_t_max = (size_t) ((ssize_t) -1);
    ssize_t max_resp_size = (ssize_t) (max_response_size > ssize_t_max ? ssize_t_max : max_response_size);
    ErrorStatus err = recvAllData(client_fd, &builder, max_resp_size, response_done_timeout_usec,
                                  response_done_timeout_is_error, client_buf_size, out_closed_sock,
                                  keep_receiving_controller);
    if (err) {
        stringBuilderDestroy(&builder);
        return err;
    }

    *out = stringBuilderBuildAndDestroy(&builder);

    return 0;
}

#define HANDLE_REQUEST_ERR(err) \
if ((err)) { \
    if ((err) > 0)  /* (err < 0) => socket was force-closed */ \
        closeSocket(fd); \
    return 1; \
}

#endif

ErrorStatus newSocketSendReceiveClose(const char *host, int port, StringRef data, String *out, int client_buf_size,
                                      ssize_t server_connect_timeout_usec, ssize_t server_response_timeout_usec,
                                      size_t max_response_size, ssize_t response_done_timeout_usec,
                                      bool response_done_timeout_is_error, bool enable_delaying_sockets,
                                      RecvAllDataControllerCallback *keep_receiving_controller) {
    LOG_MSG("called newSocketSendRecvClose");

    // Connect to peer
    size_t fd;
    ErrorStatus err = connectSocket(host, port, server_connect_timeout_usec, enable_delaying_sockets, &fd);
    HANDLE_REQUEST_ERR(err);

    bool closed_sock = false;
    // Send data
    err = sendAllData(fd, data.data, data.len, response_done_timeout_usec, &closed_sock);
    if (closed_sock)
        return 1;
    HANDLE_REQUEST_ERR(err);

    // Recv response
    err = waitThenRecvAllData(fd, server_response_timeout_usec, response_done_timeout_usec,
                              response_done_timeout_is_error, client_buf_size, max_response_size, out, &closed_sock,
                              keep_receiving_controller);
    HANDLE_REQUEST_ERR(err);

    // Close the socket
    if (!closed_sock)
        closeSocket(fd);
    LOG_MSG("newSocketSendRecvClose closing socket");

#ifdef NET77_ENABLE_LOGGING
    usleep(1000);
    LOG_MSG("newSocketSendRecvClose 1ms after closing socket");
#endif

    return 0;
}