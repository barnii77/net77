#ifndef NET77_SOCK_H
#define NET77_SOCK_H

#include "net77/string_utils.h"
#include "net77/error_utils.h"

/**
 * connect a socket to a host (IP addr or URL) at a certain port and write out the socket file descriptor
 * @param host the host IP address or URL to connect to
 * @param port the port to connect with
 * @param server_connect_timeout_usec after how many microseconds the connection attempt should time out and
 * the function should return an error
 * @param enable_delaying_sockets if zero, nagle's algorithm will be disabled. This should reduce latency.
 * @param out_fd out param to write the file descriptor of the connected socket to *on success*
 * @return err (0 means success)
 */
ErrorStatus connectSocket(const char *host, int port, ssize_t server_connect_timeout_usec, int enable_delaying_sockets,
                          size_t *out_fd);

/**
 * poll for a period of time to wait for a response, then read the response and write it to out
 * @param fd the socket file descriptor to read from
 * @param server_response_timeout_usec how many microseconds after sending the data the function waits for a response
 * @param response_done_timeout_usec after how many microseconds the function stops waiting for more response content
 * and just returns
 * @param client_buf_size how big the receive buffer should be initially before it starts growing to receive more data.
 * If -1 is passed, use default value of 8kB
 * @param max_response_size how big the response must at most be (in bytes) before it gets rejected and the function
 * @param out where the received data will be written to if successful
 * @param out_closed_sock *optional* out param set to 1 if the socket was closed or is invalid, otherwise set to 0
 * @return err (0 means success)
 */
ErrorStatus waitThenRecvAllData(size_t fd, ssize_t server_response_timeout_usec, ssize_t response_done_timeout_usec,
                                int client_buf_size, size_t max_response_size, String *out, int *out_closed_sock);

/**
 * open a socket, connect to the host, send the data, receive response and close the socket
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
 * @param enable_delaying_sockets if zero, nagle's algorithm will be disabled. This should reduce latency.
 * @return err (0 means success)
 */
ErrorStatus newSocketSendReceiveClose(const char *host, int port, StringRef data, String *out, int client_buf_size,
                                      ssize_t server_connect_timeout_usec, ssize_t server_response_timeout_usec,
                                      size_t max_response_size, ssize_t response_done_timeout_usec,
                                      int enable_delaying_sockets);

#endif