#include "testing_helpers.h"

static const int test_server_n_msg_reps = 100000;

static void testServerHandlerBigData(void *server_handler_args) {
    LOG_MSG("called testServerHandlerBigData at %zd\n", getTimeInUSecs() / 1000);
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
    LOG_MSG("testServerHandlerBigData done with checking request at %zd\n", getTimeInUSecs() / 1000);
    if (all_strcmp) {
        int send_out = sendAllData(args->socket_fd, resp, n_msg_reps * strlen("hi\r\n"), -1, NULL);
        assert(!send_out);
        LOG_MSG("responded with 'hi\\r\\n' spam\n");
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
    const long long client_conn_timeout_usec = 100000;
    const long long recv_timeout_usec = 10000;
    UnsafeSignal server_killed = 0, kill_ack = 0, server_has_started = 0;
    size_t thread = launchServerOnThread(NULL, &thread_pool, host, port, 2, 2, -1, recv_timeout_usec, max_req_size,
                                         connection_timeout_usec, &server_killed, &kill_ack, 0, NULL,
                                         &server_has_started);
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
    int err = newSocketSendReceiveClose(host, port, data, &out, -1, -1, client_conn_timeout_usec, max_req_size, 0, 0);
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

MAKE_PARSE_TEST(REQ_TEST1, Request, ParseReq1, 1);

MAKE_PARSE_TEST(REQ_TEST_EMPTY_BODY_AND_HEADER1, Request, ParseReqMinimal1, 1);

MAKE_PARSE_TEST(RESP_TEST1, Response, ParseResp1, 1);

MAKE_PARSE_TEST(REQ_TEST1, Request, ParseReq1HeadToStr, 0);

MAKE_PARSE_TEST(REQ_TEST_EMPTY_BODY_AND_HEADER1, Request, ParseReqMinimal1HeadToStr, 0);

MAKE_PARSE_TEST(RESP_TEST1, Response, ParseResp1HeadToStr, 0);

MAKE_SERDE_TEST(REQ_TEST1, Request, SerdeReq1);

MAKE_SERDE_TEST(RESP_TEST1, Response, SerdeResp1);

MAKE_SERVER_TEST(10000, 4, 0, 42, 0xDEAD, 1, Server1);

MAKE_SERVER_TEST(10000, 0, 0, 99, 0xDEAD, 1, SingleThreadedServer1);

MAKE_SERVER_TEST(0, 4, 99, 42, 0, 0, ShouldTimeoutServer1);

BEGIN_RUNNER_SETTINGS()
    print_on_pass = 1;
    print_pre_run_msg = 0;
    run_all_tests = 1;
    n_test_reps = 1;
    selected_test = "testServerBigData1";
END_RUNNER_SETTINGS()

BEGIN_TEST_LIST()
    REGISTER_TEST_CASE(testParseReq1)
    REGISTER_TEST_CASE(testParseReqMinimal1)
    REGISTER_TEST_CASE(testParseResp1)
    REGISTER_TEST_CASE(testParseReq1HeadToStr)
    REGISTER_TEST_CASE(testParseReqMinimal1HeadToStr)
    REGISTER_TEST_CASE(testParseResp1HeadToStr)
    REGISTER_TEST_CASE(testSerdeReq1)
    REGISTER_TEST_CASE(testSerdeResp1)
    REGISTER_TEST_CASE(testServer1)
    REGISTER_TEST_CASE(testSingleThreadedServer1)
    REGISTER_TEST_CASE(testShouldTimeoutServer1)
    REGISTER_TEST_CASE(testServerBigData1)
END_TEST_LIST()

MAKE_TEST_SUITE_RUNNABLE()