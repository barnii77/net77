#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "net77/utils.h"
#include "net77/logging.h"
#include "net77/net_includes.h"
#include "net77/type_utils.h"
#include "net77/math_utils.h"

StringRef removeURLPrefix(StringRef url) {
    StringRef init = url;
    while (url.len > 0 && isLetter(url.data[0]))
        url.len--, url.data++;
    if (url.len > 0 && url.data[0] == ':')
        url.len--, url.data++;
    else
        return init;
    for (int i = 0; i < 2; i++) {
        if (url.len > 0 && url.data[0] == '/')
            url.len--, url.data++;
        else
            return init;
    }
    return url;
}

StringRef charPtrToStringRef(const char *data) {
    StringRef out = {strlen(data), data};
    return out;
}

#if defined(_WIN32) || defined(_WIN64)

int setSendRecvTimeout(size_t fd, ssize_t timeout_usec) {
    if (timeout_usec >= 0) {
        struct timeval timeout;
        timeout.tv_sec = timeout_usec / 1000000;
        timeout.tv_usec = timeout_usec % 1000000;
        if (setsockopt((SOCKET) fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout))) {
            return 1;
        }
        if (setsockopt((SOCKET) fd, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout))) {
            return 1;
        }
    }
    return 0;
}

#else

#include <fcntl.h>
#include <sched.h>
#include <sys/time.h>

size_t getTimeInUSecs() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (size_t) (time.tv_sec) * 1000000 + (size_t) (time.tv_usec);
}

ErrorStatus makeSocketNonBlocking(size_t fd) {
    int flags, fco;
    int ifd = (int) fd;
    if ((flags = fcntl(ifd, F_GETFL, 0)) < 0)
        return flags;
    if ((fco = fcntl(ifd, F_SETFL, flags | O_NONBLOCK)) < 0)
        return fco;
    return 0;
}

ErrorStatus setSocketKeepalive(size_t fd) {
    int opt = 1;
    int ifd = (int) fd;
    return setsockopt(ifd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
}

ErrorStatus setSocketNoDelay(size_t fd) {
    int opt = 1;
    int ifd = (int) fd;
    return setsockopt(ifd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

ErrorStatus setSendRecvTimeout(size_t fd, ssize_t timeout_usec) {
    if (timeout_usec >= 0) {
        struct timeval timeout;
        timeout.tv_sec = timeout_usec / 1000000;
        timeout.tv_usec = timeout_usec % 1000000;
        if (setsockopt((int) fd, SOL_SOCKET, SO_RCVTIMEO, (const void *) &timeout, sizeof(timeout))
            || setsockopt((int) fd, SOL_SOCKET, SO_SNDTIMEO, (const void *) &timeout, sizeof(timeout)))
            return 1;
    }
    return 0;
}

void closeSocket(size_t fd) {
    close((int) fd);
}

void schedYield() {
    sched_yield();
}

#define SET_OPTIONAL_SIGNAL(sig, value) if (sig) *sig = value

ErrorStatus sendAllData(size_t socket_fd, const char *buf, size_t len, ssize_t timeout_usec, bool *out_closed_sock) {
    LOG_MSG("called sendAllData");
    SET_OPTIONAL_SIGNAL(out_closed_sock, 0);
    int sockfd = (int) socket_fd;
    size_t bytes_sent = 0;
    ssize_t bytes_sent_this_iter;
    const int timeout_ms = (int) (timeout_usec < 0 ? -1 : (timeout_usec + 999) / 1000);
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLOUT | POLLERR | POLLHUP | POLLNVAL;

    while (bytes_sent < len) {
        LOG_MSG("sendAllData has sent %zu bytes so far", bytes_sent);
        pfd.revents = 0;
        // poll for ready-to-send event (or errors)
        int poll_out = poll(&pfd, 1, timeout_ms);
        if (poll_out < 0)
            return 1;
        else if (!poll_out)
            break;

        // error handling
        if (pfd.revents & POLLERR) {
            close(sockfd);
            SET_OPTIONAL_SIGNAL(out_closed_sock, 1);
            LOG_MSG("socket closed due to ERR (TCP RST)");
            return -1;
        } else if (pfd.revents & POLLHUP) {
            close(sockfd);
            SET_OPTIONAL_SIGNAL(out_closed_sock, 1);
            LOG_MSG("socket closed due to HUP (TCP FIN)");
            return -1;
        } else if (pfd.revents & POLLNVAL) {
            SET_OPTIONAL_SIGNAL(out_closed_sock, 1);
            LOG_MSG("closed due to NVAL");
            return -1;
        }

        // socket not ready to send
        if (!(pfd.revents & POLLOUT))
            assert(0 && "unreachable since *something* must have happened to trigger the poll");

        // send data
        if ((bytes_sent_this_iter = send(sockfd, buf + bytes_sent, len - bytes_sent, 0)) < 0)
            assert(0 && "unreachable because of the poll");
        bytes_sent += bytes_sent_this_iter;
    }

    LOG_MSG("sendAllData has sent all %zu bytes", len);
    return 0;
}

#define RECV_CONTROLLER_STATE_MAX_STACK_ALLOC_SIZE 128
#define DEFAULT_RECV_ALL_DATA_SB_MIN_CAP 1024
#define DO_RECV_CONTROLLER_COMMANDED_ACTION(action) \
do { \
    switch (action) { \
        case RECV_ALL_DATA_ACTION_NONE: \
            break; \
        case RECV_ALL_DATA_ACTION_REJECT_REQUEST: \
            ec = 1; \
            goto done; \
        case RECV_ALL_DATA_ACTION_FINISHED: \
            goto done; \
    } \
} while (0)

ErrorStatus
recvAllData(size_t socket_fd, StringBuilder *builder, ssize_t max_len, ssize_t timeout_usec, bool timeout_is_error,
            ssize_t sb_min_cap, bool *out_closed_sock, RecvAllDataControllerCallback *keep_receiving_controller) {
    assert(builder);
    if (sb_min_cap < 0)
        sb_min_cap = DEFAULT_RECV_ALL_DATA_SB_MIN_CAP;
    if (builder->cap < sb_min_cap)
        stringBuilderExpandBuf(builder, sb_min_cap);

    LOG_MSG("called recvAllData(StringBuilder)");

    SET_OPTIONAL_SIGNAL(out_closed_sock, 0);
    const int sockfd = (int) socket_fd;
    size_t bytes_received = 0;
    ssize_t bytes_received_this_iter;
    const int timeout_ms = (int) (timeout_usec < 0 ? -1 : (timeout_usec + 999) / 1000);
    struct pollfd pfd;
    size_t initial_sb_len = builder->len;
    pfd.fd = sockfd;
    pfd.events = POLLALLIN | POLLERR | POLLHUP | POLLNVAL;

    // the error code to be returned
    int ec = 0;

    char stack_controller_state[RECV_CONTROLLER_STATE_MAX_STACK_ALLOC_SIZE] = {0};
    char *heap_controller_state = NULL;
    char *controller_state;
    RecvAllDataControllerCommandedAction next_action = RECV_ALL_DATA_ACTION_NONE;
    if (keep_receiving_controller && keep_receiving_controller->shared_state) {
        controller_state = keep_receiving_controller->shared_state;
    } else if (keep_receiving_controller &&
               keep_receiving_controller->sizeof_state > RECV_CONTROLLER_STATE_MAX_STACK_ALLOC_SIZE) {
        heap_controller_state = calloc(1, keep_receiving_controller->sizeof_state);
        controller_state = heap_controller_state;
    } else {
        controller_state = stack_controller_state;
    }

    // check whether to even receive more / how much more to receive
    assert(bytes_received == builder->len - initial_sb_len);
    if (keep_receiving_controller) {
        keep_receiving_controller->fn(builder->data + initial_sb_len, builder->cap - initial_sb_len, bytes_received,
                                      &max_len, &next_action, controller_state);
        DO_RECV_CONTROLLER_COMMANDED_ACTION(next_action);
    }

    while (max_len < 0 || bytes_received < max_len) {
        pfd.revents = 0;
        // poll for ready-to-send event (or errors)
        int poll_out = poll(&pfd, 1, timeout_ms);
        if (poll_out < 0) {
            LOG_MSG("recvAllData poll error");
            ec = 1;
            goto done;
        } else if (!poll_out) {
            LOG_MSG("recvAllData poll timeout");
            ec = timeout_is_error ? 1 : 0;
            goto done;
        }

        // error handling
        if (pfd.revents & POLLERR) {
            close(sockfd);
            SET_OPTIONAL_SIGNAL(out_closed_sock, 1);
            LOG_MSG("socket closed due to ERR (TCP RST)");
            ec = -1;
            goto done;
        } else if (pfd.revents & POLLHUP) {
            if (!(pfd.revents & POLLALLIN)) {
                // socket closed and read buffer empty
                close(sockfd);
                SET_OPTIONAL_SIGNAL(out_closed_sock, 1);
                LOG_MSG("socket closed due to HUP (TCP FIN)");
                ec = 0;
                goto done;
            }
            // else: still something in the read buffer -> continue reading until done
        } else if (pfd.revents & POLLNVAL) {
            SET_OPTIONAL_SIGNAL(out_closed_sock, 1);
            LOG_MSG("already closed due to NVAL");
            ec = -1;
            goto done;
        }

        // socket not ready to recv
        if (!(pfd.revents & POLLALLIN))
            assert(0 && "unreachable since *something* must have happened to cause the poll ret");

        // recv data
        if (builder->cap - builder->len < sb_min_cap)
            stringBuilderExpandBuf(builder, builder->cap + (sb_min_cap + builder->len - builder->cap));
        ssize_t bytes_to_read = optMin((ssize_t) (builder->cap - builder->len), (ssize_t) (max_len - bytes_received));
        if (bytes_to_read <= 0)
            bytes_to_read = -1;
        if ((bytes_received_this_iter = recv(sockfd, builder->data + builder->len, bytes_to_read, 0)) < 0)
            assert(0 && "unreachable because of the poll");

        // socket has been closed
        if (bytes_received_this_iter == 0) {
            close(sockfd);
            SET_OPTIONAL_SIGNAL(out_closed_sock, 1);
            LOG_MSG("(recvAllData) socket closed by peer and read buffer empty");
            ec = 0;
            goto done;
        }

        bytes_received += bytes_received_this_iter;
        builder->len += bytes_received_this_iter;

        // check whether to even receive more / how much more to receive
        assert(bytes_received == builder->len - initial_sb_len);
        if (keep_receiving_controller) {
            keep_receiving_controller->fn(builder->data + initial_sb_len, builder->cap - initial_sb_len, bytes_received,
                                          &max_len, &next_action, controller_state);
            DO_RECV_CONTROLLER_COMMANDED_ACTION(next_action);
        }
    }

done:
    if (heap_controller_state)
        free(heap_controller_state);
    if (max_len >= 0 && bytes_received > max_len)
        ec = 1;
    return ec;
}

#endif