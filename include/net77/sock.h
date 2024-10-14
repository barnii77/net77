#ifndef NET77_SOCK_H
#define NET77_SOCK_H

#include "string_utils.h"

#define DEFAULT_CLIENT_BUF_SIZE (1024)

/**
 * open a socket, connect to the host, send the data and close the socket
 * @param host address of the host
 * @param port where to talk to the host
 * @param data the data to be sent
 * @param out where the received data will be written to if successful
 * @param client_buf_size how big the receive buffer should be initially before it starts growing to receive more data.
 * If -1 is passed, use default value of 1kB
 * @param request_timeout_usec after how many microseconds a request times out and fails
 * @param max_response_size how big the response must at most be (in bytes) before it gets rejected and the function
 * returns an error
 * @return err (0 means success)
 */
int newSocketSendReceiveClose(const char *host, int port, StringRef data, String *out, int client_buf_size,
                              int request_timeout_usec, size_t max_response_size);

#endif