#ifndef NET77_BIG_DATA_TEST_H
#define NET77_BIG_DATA_TEST_H

#include "testing_helpers.h"

#ifdef TEST_SERVER_N_MSG_REPS
static const int test_server_n_msg_reps = TEST_SERVER_N_MSG_REPS;
#else
#error "must define TEST_SERVER_N_MSG_REPS macro before including big_data_test.h"
#endif

static int max(int a, int b) {
    return a > b ? a : b;
}

static void testServerHandlerBigData(void *server_handler_args) {
    LOG_MSG("called testServerHandlerBigData");
    const int n_msg_reps = test_server_n_msg_reps;
    ServerHandlerArgs *args = server_handler_args;
    char *resp = malloc(n_msg_reps * strlen("hi\r\n"));
    for (int i = 0; i < n_msg_reps; i++) {
        memcpy(&resp[i * strlen("hi\r\n")], "hi\r\n", strlen("hi\r\n"));
    }

    int all_strcmp = 1;
    for (int i = 0; i < n_msg_reps; i++) {
        if (strncmp(&args->data[i * strlen("hello\r\n")], "hello\r\n", strlen("hello\r\n")) != 0) {
            all_strcmp = 0;
            break;
        }
    }
    LOG_MSG("testServerHandlerBigData done with checking request");
    if (all_strcmp) {
        int send_out = sendAllData(args->socket_fd, resp, n_msg_reps * strlen("hi\r\n"), -1, NULL);
        assert(!send_out);
        LOG_MSG("responded with 'hi\\r\\n' spam");
    }
    free(args->data);
    if (args->heap_allocated)
        free(args);
}

static int testServerBigData1(void) {
    const int n_msg_reps = test_server_n_msg_reps;
    const size_t max_req_size = 999999999;
    const char *host = "127.0.0.1";
    const int port = 54321;
    ThreadPool thread_pool = newThreadPool(0, testServerHandlerBigData);
    const long long connection_timeout_usec = 1000000000;
    const long long server_response_timeout_usec = max(n_msg_reps / 20, 5000);
    const long long recv_timeout_usec = 25000;
    UnsafeSignal server_killed = 0, kill_ack = 0, server_has_started = 0;
    size_t thread = launchServerOnThread(NULL, &thread_pool, host, port, 2, 2, -1, recv_timeout_usec, max_req_size,
                                         connection_timeout_usec, &server_killed, &kill_ack, 0, NULL,
                                         NULL, &server_has_started);
    if (thread == -1)
        return -1;
    while (!server_has_started);  // wait for server to start
    usleep(5000);
    char *msg = malloc(strlen("hello\r\n") * n_msg_reps);
    for (int i = 0; i < n_msg_reps; i++) {
        memcpy(&msg[i * strlen("hello\r\n")], "hello\r\n", strlen("hello\r\n"));
    }
    StringRef data = {strlen("hello\r\n") * n_msg_reps, msg};
    String out = {0, NULL};
    int err = newSocketSendReceiveClose(host, port, data, &out, -1, -1, server_response_timeout_usec, max_req_size,
                                        recv_timeout_usec, false, false, NULL);
    server_killed = 1;
    while (!kill_ack);
    threadPoolDestroy(&thread_pool);
    if (err) {
        free(msg);
        return err;
    }
    int all_strcmp = 1;
    for (int i = 0; i < n_msg_reps; i++) {
        if (strncmp(&out.data[i * strlen("hi\r\n")], "hi\r\n", strlen("hi\r\n")) != 0) {
            all_strcmp = 0;
            break;
        }
    }
    if (out.len != 4 * n_msg_reps) printAndFlush("\nTHE RECEIVED DATA HAS LENGTH %zu, BUT SHOULD HAVE %d\n", out.len,
                                                 4 * n_msg_reps);
    freeString(&out);
    free(msg);
    if (all_strcmp)
        return 0;
    return 42;
}

#endif