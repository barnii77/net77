#include <string.h>
#include "net77/utils.h"
#include "net77/net_includes.h"

#undef send
#undef recv

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

int setSendRecvTimeout(size_t fd, ssize_t timeout_usec) {
    if (timeout_usec >= 0) {
        struct timeval timeout;
        timeout.tv_sec = timeout_usec / 1000000;
        timeout.tv_usec = timeout_usec % 1000000;
        if (setsockopt((int) fd, SOL_SOCKET, SO_RCVTIMEO, (const void *) &timeout, sizeof(timeout))) {
            return 1;
        }
        if (setsockopt((int) fd, SOL_SOCKET, SO_SNDTIMEO, (const void *) &timeout, sizeof(timeout))) {
            return 1;
        }
    }
    return 0;
}

/**
 * loop and send until all data has been sent. return 0 on success, x != 0 on error:
 * returns 1 on normal error and -1 on error that lead to the socket being closed.
 */
int sendAllData(size_t socket, const void *buf, size_t len) {
    int sockfd = (int) socket;
    size_t bytes_sent = 0;
    ssize_t bytes_sent_this_iter;
    struct timeval timeout_tval;
    socklen_t timeout_size = sizeof(timeout_tval);
    ssize_t timeout_ms = -1;
    // FIXME I think this fails to retrieve the data even if it is set
    if (!getsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout_tval, &timeout_size))
        timeout_ms = timeout_tval.tv_sec * 1000 + timeout_tval.tv_usec / 1000;

    while (bytes_sent < len) {
        // poll for ready-to-send event (or errors)
        struct pollfd pfd;
        pfd.fd = sockfd;
        pfd.events = POLLOUT | POLLERR | POLLHUP | POLLNVAL;
        pfd.revents = 0;
        int poll_out = poll(&pfd, 1, (int) timeout_ms);
        if (!poll_out)
            return 1;
        if (pfd.revents & POLLERR || pfd.revents & POLLHUP) {
            close(sockfd);
            return -1;
        }
        if (pfd.revents & POLLNVAL)
            return -1;
        if (!(pfd.revents & POLLOUT))
            return 1;

        // send data
        if ((bytes_sent_this_iter = send(sockfd, buf, len, MSG_NOSIGNAL)) < 0)
            return 1;
        bytes_sent += bytes_sent_this_iter;
    }

    return 0;
}

#endif