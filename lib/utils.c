#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "net77/utils.h"
#include "net77/logging.h"
#include "net77/net_includes.h"

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

int makeSocketNonBlocking(size_t fd) {
    int flags, fco;
    int ifd = (int) fd;
    if ((flags = fcntl(ifd, F_GETFL, 0)) < 0)
        return flags;
    if ((fco = fcntl(ifd, F_SETFL, flags | O_NONBLOCK)) < 0)
        return fco;
    return 0;
}

int setSocketNoDelay(size_t fd) {
    int opt = 1;
    int ifd = (int) fd;
    return setsockopt(ifd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

int setSendRecvTimeout(size_t fd, ssize_t timeout_usec) {
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

int sendAllData(size_t socket_fd, const char *buf, size_t len, ssize_t timeout_usec) {
    LOG_MSG("called sendAllData at %zd\n", getTimeInUSecs() / 1000);
    int sockfd = (int) socket_fd;
    size_t bytes_sent = 0;
    ssize_t bytes_sent_this_iter;
    const int timeout_ms = (int) (timeout_usec < 0 ? -1 : (timeout_usec + 999) / 1000);
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLOUT | POLLERR | POLLHUP | POLLNVAL;

    while (bytes_sent < len) {
        pfd.revents = 0;
        // poll for ready-to-send event (or errors)
        int poll_out = poll(&pfd, 1, timeout_ms);
        if (poll_out < 0)
            return 1;
        else if (!poll_out)
            break;

        // error handling
        if (pfd.revents & POLLERR || pfd.revents & POLLHUP) {
            close(sockfd);
            LOG_MSG("socket closed due to ERR/HUP\n");
            return -1;
        }
        if (pfd.revents & POLLNVAL) {
            LOG_MSG("closed due to NVAL\n");
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

    return 0;
}

int recvAllData(size_t socket_fd, char *buf, size_t len, ssize_t timeout_usec, size_t *out_bytes_received) {
    assert(buf);
    LOG_MSG("called recvAllData at %zd\n", getTimeInUSecs() / 1000);
    int sockfd = (int) socket_fd;
    size_t bytes_received = 0;
    ssize_t bytes_received_this_iter;
    const int timeout_ms = (int) (timeout_usec < 0 ? -1 : (timeout_usec + 999) / 1000);
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;

    while (bytes_received < len) {
        pfd.revents = 0;
        // poll for ready-to-send event (or errors)
        int poll_out = poll(&pfd, 1, timeout_ms);
        if (poll_out < 0)
            return 1;
        else if (!poll_out)
            break;

        // error handling
        if (pfd.revents & POLLERR || pfd.revents & POLLHUP) {
            close(sockfd);
            LOG_MSG("socket closed due to ERR/HUP\n");
            return -1;
        }
        if (pfd.revents & POLLNVAL) {
            LOG_MSG("closed due to NVAL\n");
            return -1;
        }

        // socket not ready to recv
        if (!(pfd.revents & POLLIN))
            assert(0 && "unreachable since *something* must have happened to trigger the poll");

        // recv data
        if ((bytes_received_this_iter = recv(sockfd, buf + bytes_received, len - bytes_received, 0)) < 0)
            assert(0 && "unreachable because of the poll");
        bytes_received += bytes_received_this_iter;
    }

    if (out_bytes_received)
        *out_bytes_received = bytes_received;
    return 0;
}

#define DEFAULT_RECV_ALL_DATA_SB_MIN_CAP (1024)

int recvAllDataSb(size_t socket_fd, StringBuilder *builder, ssize_t max_len, ssize_t timeout_usec, ssize_t sb_min_cap) {
    assert(builder);
    if (sb_min_cap < 0)
        sb_min_cap = DEFAULT_RECV_ALL_DATA_SB_MIN_CAP;
    if (builder->cap < sb_min_cap)
        stringBuilderExpandBuf(builder, sb_min_cap);

    LOG_MSG("called recvAllData(StringBuilder) at %zd\n", getTimeInUSecs() / 1000);

    int sockfd = (int) socket_fd;
    size_t bytes_received = 0;
    ssize_t bytes_received_this_iter;
    const int timeout_ms = (int) (timeout_usec < 0 ? -1 : (timeout_usec + 999) / 1000);
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;

    while (max_len <= 0 || bytes_received <= max_len) {
        pfd.revents = 0;
        // poll for ready-to-send event (or errors)
        int poll_out = poll(&pfd, 1, timeout_ms);
        if (poll_out < 0)
            return 1;
        else if (!poll_out)
            break;

        // error handling
        if (pfd.revents & POLLERR || pfd.revents & POLLHUP) {
            close(sockfd);
            LOG_MSG("socket closed due to ERR/HUP\n");
            return -1;
        }
        if (pfd.revents & POLLNVAL) {
            LOG_MSG("closed due to NVAL\n");
            return -1;
        }

        // socket not ready to recv
        if (!(pfd.revents & POLLIN))
            assert(0 && "unreachable since *something* must have happened to trigger the poll");

        // recv data
        if (builder->cap - builder->len < sb_min_cap)
            stringBuilderExpandBuf(builder, builder->len + sb_min_cap);
        if ((bytes_received_this_iter = recv(sockfd, builder->data + builder->len, builder->cap - builder->len, 0)) < 0)
            assert(0 && "unreachable because of the poll");

        // socket has been closed
        if (bytes_received_this_iter == 0)
            break;

        bytes_received += bytes_received_this_iter;
        builder->len += bytes_received_this_iter;
    }
    if (max_len > 0 && bytes_received > max_len)
        return 1;

    return 0;
}

void schedYield() {
    sched_yield();
}

#endif