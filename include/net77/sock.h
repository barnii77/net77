#ifndef NET77_SOCK_H
#define NET77_SOCK_H

#include "net77/string_utils.h"
#include "net77/error_utils.h"

// TODO I should return some more detailed error codes (eg to tell caller whether server failed to respond or client failed to connect, etc)
/**
 * open a socket, connect to the host, sendAllData the data, receive response and close the socket
 * @param host address of the host
 * @param port where to talk to the host
 * @param data the data to be sent
 * @param out where the received data will be written to if successful
 * @param client_buf_size how big the receive buffer should be initially before it starts growing to receive more data.
 * If -1 is passed, use default value of 1kB
 * @param server_connect_timeout_usec after how many microseconds a connect attempt times out and the function returns an error
 * @param server_response_timeout_usec how many microseconds after sending the data the function waits for a response
 * @param max_response_size how big the response must at most be (in bytes) before it gets rejected and the function
 * @param response_done_timeout_usec after how many microseconds the function stops waiting for more response content
 * and just returns
 * @param enable_delaying_sockets if non-zero, nagle's algorithm will *not* be disabled. This may introduce latency.
 * returns an error
 * @return err (0 means success)
 */
ErrorStatus newSocketSendReceiveClose(const char *host, int port, StringRef data, String *out, int client_buf_size,
                              ssize_t server_connect_timeout_usec, ssize_t server_response_timeout_usec,
                              size_t max_response_size, ssize_t response_done_timeout_usec,
                              int enable_delaying_sockets);

#endif