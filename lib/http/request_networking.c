#include <assert.h>
#include "net77/net_includes.h"
#include "net77/http/serde.h"
#include "net77/http/request.h"
#include "net77/sock.h"
#include "net77/int_includes.h"
#include "net77/type_utils.h"
#include "net77/string_utils.h"
#include "net77/http/recv_controller.h"

#define BUBBLE_UP_ERR(err) if (err) return err

// after the initial response bytes are received, we wait for this many more microseconds for more response bytes
#define REQUEST_RESPONSE_ACCUM_TIME_USEC 10000000

Session newSession() {
    return (Session) {.socket_fd = 0, .is_open = 0};
}

ErrorStatus openSession(const char *nonnull host, int port, ssize_t connect_timeout_usec, Session *nonnull out) {
    assert(!out->is_open);
    ErrorStatus ret = connectSocket(host, port, connect_timeout_usec, 0, &out->socket_fd);
    if (!ret)
        out->is_open = 1;
    return ret;
}

ErrorStatus closeSession(Session *nonnull session) {
    if (session->is_open) {
        closeSocket(session->socket_fd);
        return 0;
    }
    return 1;
}

#define HELPER_STRINGIFY_REQUEST_STRUCT(req) \
    StringBuilder builder = newStringBuilder(0); \
    BUBBLE_UP_ERR(serializeHttpRequest(req, &builder)); \
    String str = stringBuilderBuildAndDestroy(&builder); \
    StringRef str_ref = {str.len, str.data};

ErrorStatus
httpRequestInSession(Session *nonnull session, HttpRequest *nonnull req, String *nonnull out, size_t max_response_size,
                     ssize_t response_timeout_usec) {
    assert(session->is_open && "make sure you are calling openSession to start a connection");
    HELPER_STRINGIFY_REQUEST_STRUCT(req);

    ErrorStatus err = httpRawRequestInSession(session, str_ref, out, max_response_size, response_timeout_usec);
    freeString(&str);
    return err;
}

ErrorStatus
httpRawRequestInSession(Session *nonnull session, StringRef req, String *nonnull out, size_t max_response_size,
                        ssize_t response_timeout_usec) {
    assert(session->is_open && "make sure you are calling openSession to start a connection");

    bool closed_sock = false;
    // Send data
    ErrorStatus err = sendAllData(session->socket_fd, req.data, req.len, REQUEST_RESPONSE_ACCUM_TIME_USEC,
                                  &closed_sock);
    if (closed_sock)
        session->is_open = 0;
    BUBBLE_UP_ERR(err);

    // Recv response
    err = waitThenRecvAllData(session->socket_fd, response_timeout_usec, REQUEST_RESPONSE_ACCUM_TIME_USEC, true, -1,
                              max_response_size, out, &closed_sock, &http_recv_controller);
    if (closed_sock)
        session->is_open = 0;

    return err;
}

ErrorStatus
httpRequest(const char *nonnull host, int port, HttpRequest *nonnull req, String *nonnull out, size_t max_response_size,
            ssize_t connect_timeout_usec, ssize_t response_timeout_usec) {
    HELPER_STRINGIFY_REQUEST_STRUCT(req);
    ErrorStatus err = httpRawRequest(host, port, str_ref, out, max_response_size, connect_timeout_usec,
                                     response_timeout_usec);
    freeString(&str);
    return err;
}

ErrorStatus
httpRawRequest(const char *nonnull host, int port, StringRef req, String *nonnull out, size_t max_response_size,
               ssize_t connect_timeout_usec, ssize_t response_timeout_usec) {
    return newSocketSendReceiveClose(host, port, req, out, -1, connect_timeout_usec, response_timeout_usec,
                                     max_response_size, REQUEST_RESPONSE_ACCUM_TIME_USEC, true, false,
                                     &http_recv_controller);
}