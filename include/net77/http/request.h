#ifndef NET77_HTTP_REQUEST_H
#define NET77_HTTP_REQUEST_H

#include "net77/http/serde.h"
#include "net77/int_includes.h"
#include "net77/type_utils.h"

typedef struct Session {
    size_t socket_fd;
    int is_open;
} Session;

/**
 * create a new unconnected (but initialized) session
 * @return initialized session struct
 */
Session newSession();

/**
 * create a session by connecting to the host at a given port
 * @param host the host IP address or URL
 * @param port the host port to connect to
 * @param connect_timeout_usec after how many microseconds to time out connection process and return an error
 * @param enable_delaying_sockets if zero, disable nagle's algorithm to reduce latency
 * @param out out param to write session info to
 * @return err (0 means success)
 */
ErrorStatus openSession(const char *host, int port, ssize_t connect_timeout_usec, Session *out);

/**
 * sends a httpRequest using a Request struct given an existing session
 * @param session the session
 * @param req Request struct pointer
 * @param out out param to write the response to (as a string)
 * @param max_response_size in bytes
 * @param response_timeout_usec how many microseconds the function should wait for the response to arrive
 * @return err (0 means success)
 */
ErrorStatus
httpRequestInSession(Session *session, HttpRequest *req, String *out, size_t max_response_size, ssize_t response_timeout_usec);

/**
 * make a request using a httpRequest string given an existing session
 * @param session the session
 * @param req httpRequest content as a string
 * @param out out param to write the response to (as a string)
 * @param max_response_size in bytes
 * @param response_timeout_usec how many microseconds the function should wait for the response to arrive
 * @return err (0 means success)
 */
ErrorStatus httpRawRequestInSession(Session *session, StringRef req, String *out, size_t max_response_size,
                                    ssize_t response_timeout_usec);

/**
 * close session
 * @param session the session
 * @return err (0 means success)
 */
ErrorStatus closeSession(Session *session);

/**
 * make a standalone httpRequest using a Request struct
 * @param host the host IP address or URL
 * @param port the host port to connect to
 * @param req Request struct pointer
 * @param out out param to write the response to (as a string)
 * @param max_response_size in bytes
 * @param connect_timeout_usec after how many microseconds the connection attempt will time out and the function will
 * return an error
 * @param response_timeout_usec how many microseconds the function should wait for the response to arrive
 * @return err (0 means success)
 */
ErrorStatus
httpRequest(const char *host, int port, HttpRequest *req, String *out, size_t max_response_size, ssize_t connect_timeout_usec,
            ssize_t response_timeout_usec);

/**
 * make a standalone request using a httpRequest string
 * @param host the host IP address or URL
 * @param port the host port to connect to
 * @param req httpRequest content as a string
 * @param out out param to write the response to (as a string)
 * @param max_response_size in bytes
 * @param connect_timeout_usec after how many microseconds the connection attempt will time out and the function will
 * return an error
 * @param response_timeout_usec how many microseconds the function should wait for the response to arrive
 * @return err (0 means success)
 */
ErrorStatus httpRawRequest(const char *host, int port, StringRef req, String *out, size_t max_response_size,
                           ssize_t connect_timeout_usec, ssize_t response_timeout_usec);

#endif