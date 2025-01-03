#include <assert.h>
#include "net77/net_includes.h"
#include "net77/serde.h"
#include "net77/request.h"
#include "net77/sock.h"
#include "net77/int_includes.h"
#include "net77/error_utils.h"

#define BUBBLE_UP_ERR(err) if (err) return err

// after the initial response bytes are received, we wait for this many more microseconds for more response bytes
#define REQUEST_RESPONSE_ACCUM_TIME_USEC 500000

Session newSession() {
    return (Session) {.socket_fd = 0, .is_open = 0};
}

ErrorStatus openSession(const char *host, int port, ssize_t connect_timeout_usec, Session *out) {
    assert(!out->is_open);
    ErrorStatus ret = connectSocket(host, port, connect_timeout_usec, 0, &out->socket_fd);
    if (!ret)
        out->is_open = 1;
    return ret;
}

ErrorStatus closeSession(Session *session) {
    if (session->is_open) {
        closeSocket(session->socket_fd);
        return 0;
    }
    return 1;
}

#define HELPER_STRINGIFY_REQUEST_STRUCT(req) \
    StringBuilder builder = newStringBuilder(0); \
    BUBBLE_UP_ERR(serializeRequest(req, &builder)); \
    String str = stringBuilderBuildAndDestroy(&builder); \
    StringRef str_ref = {str.len, str.data};

ErrorStatus
requestInSession(Session *session, Request *req, String *out, size_t max_response_size, ssize_t response_timeout_usec) {
    assert(session->is_open && "make sure you are calling openSession to start a connection");
    HELPER_STRINGIFY_REQUEST_STRUCT(req);

    ErrorStatus err = rawRequestInSession(session, str_ref, out, max_response_size, response_timeout_usec);
    freeString(&str);
    return err;
}

ErrorStatus rawRequestInSession(Session *session, StringRef req, String *out, size_t max_response_size,
                                ssize_t response_timeout_usec) {
    assert(session->is_open && "make sure you are calling openSession to start a connection");

    int closed_sock;
    // Send data
    ErrorStatus err = sendAllData(session->socket_fd, req.data, req.len, REQUEST_RESPONSE_ACCUM_TIME_USEC, &closed_sock);
    if (closed_sock || err < 0)
        session->is_open = 0;
    BUBBLE_UP_ERR(err);

    // Recv response
    err = waitThenRecvAllData(session->socket_fd, response_timeout_usec, REQUEST_RESPONSE_ACCUM_TIME_USEC, -1,
                              max_response_size, out, &closed_sock);
    if (closed_sock || err < 0)
        session->is_open = 0;

    return err;
}

ErrorStatus
request(const char *host, int port, Request *req, String *out, size_t max_response_size, ssize_t connect_timeout_usec,
        ssize_t response_timeout_usec) {
    HELPER_STRINGIFY_REQUEST_STRUCT(req);
    ErrorStatus err = rawRequest(host, port, str_ref, out, max_response_size, connect_timeout_usec,
                                 response_timeout_usec);
    freeString(&str);
    return err;
}

ErrorStatus rawRequest(const char *host, int port, StringRef req, String *out, size_t max_response_size,
                       ssize_t connect_timeout_usec, ssize_t response_timeout_usec) {
    return newSocketSendReceiveClose(host, port, req, out, -1, connect_timeout_usec, response_timeout_usec,
                                     max_response_size, REQUEST_RESPONSE_ACCUM_TIME_USEC, 0);
}