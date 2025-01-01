#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "net77/serde.h"
#include "net77/request.h"
#include "net77/init.h"
#include "net77/server.h"
#include "net77/utils.h"
#include "net77/logging.h"
#include "net77/net_includes.h"
#include "net77/sock.h"

#define printAndFlush(msg, ...) {printf(msg, ##__VA_ARGS__); fflush(stdout);}

const char *REQ_TEST1 = "GET /file/hello.jpg HTTP/1.1\r\nHost:www.example.com\r\n\r\n{\"json\": \"body\"}";
const char *REQ_TEST_EMPTY_BODY_AND_HEADER1 = "GET / HTTP/1.1\r\n\r\nxyz";
const char *RESP_TEST1 = "HTTP/1.0 200 OK\r\nHost:www.example.com\r\n\r\n{\"json\": \"body\"}";
const char *GET_REQ_TEST1 = "GET / HTTP/1.0\r\nHost:www.example.com\r\n\r\n";
const char *GET_REQ_TARGET1 = "HTTP/1.0 200 OK\r\nCache-Control: max-age=604800\r\nContent-Type: text/html; charset=UTF-8\r\nVary: Accept-Encoding\r\nX-Cache: HIT\r\nContent-Length: 1256\r\nConnection: close\r\n\r\n<!doctype html>\n<html>\n<head>\n    <title>Example Domain</title>\n\n    <meta charset=\"utf-8\" />\n    <meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\" />\n    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n    <style type=\"text/css\">\n    body {\n        background-color: #f0f0f2;\n        margin: 0;\n        padding: 0;\n        font-family: -apple-system, system-ui, BlinkMacSystemFont, \"Segoe UI\", \"Open Sans\", \"Helvetica Neue\", Helvetica, Arial, sans-serif;\n        \n    }\n    div {\n        width: 600px;\n        margin: 5em auto;\n        padding: 2em;\n        background-color: #fdfdff;\n        border-radius: 0.5em;\n        box-shadow: 2px 3px 7px 2px rgba(0,0,0,0.02);\n    }\n    a:link, a:visited {\n        color: #38488f;\n        text-decoration: none;\n    }\n    @media (max-width: 700px) {\n        div {\n            margin: 0 auto;\n            width: auto;\n        }\n    }\n    </style>    \n</head>\n\n<body>\n<div>\n    <h1>Example Domain</h1>\n    <p>This domain is for use in illustrative examples in documents. You may use this\n    domain in literature without prior coordination or asking for permission.</p>\n    <p><a href=\"https://www.iana.org/domains/example\">More information...</a></p>\n</div>\n</body>\n</html>\n";

void removeHeaderField(char *http_response, const char *field_name) {
    char *date_pos = strstr(http_response, field_name);

    if (date_pos) {
        // find "\r" and move 2 chars to the right to also remove "\r\n"
        char *end_of_line = strchr(date_pos, '\r') + 2;
        if (end_of_line) {
            size_t remaining_length = strlen(end_of_line);
            memmove(date_pos, end_of_line, remaining_length + 1);
        }
    }
}

#define MAKE_PARSE_TEST(test_data, test_type, test_name, parse_header_as_structs) \
int test##test_name(void) { \
    size_t len = strlen(test_data); \
    StringRef s = {len, test_data}; \
    test_type out; \
    int err = parse##test_type(s, &out, parse_header_as_structs); \
    return err; \
}

// serialize/deserialize should result in the same thing
#define MAKE_SERDE_TEST(test_data, test_type, test_name) \
int test##test_name(void) { \
    size_t len = strlen(test_data); \
    StringRef s = {len, test_data}; \
    test_type out; \
    int err = parse##test_type(s, &out, 1); \
    if (err) \
        return err; \
    StringBuilder builder = newStringBuilder(0); \
    err = serialize##test_type(&out, &builder); \
    if (err) \
        return err; \
    String ser = stringBuilderBuildAndDestroy(&builder); \
    int c = strcmp(ser.data, test_data) != 0; \
    freeString(&ser); \
    if (c) { \
        return 1; \
    } \
    return 0; \
}

#define MAKE_RAW_REQUEST_TEST(test_data, test_url, test_port, test_exp_outcome, test_name) \
int test##test_name(void) { \
    StringRef dref = {strlen(test_data), test_data}; \
    String out; \
    int err = rawRequest(removeURLPrefix(charPtrToStringRef(test_url)).data, test_port, dref, &out, -1, -1, -1); \
    if (!err) { \
        removeHeaderField(out.data, "Age"); \
        removeHeaderField(out.data, "Date"); \
        removeHeaderField(out.data, "Expires"); \
        removeHeaderField(out.data, "Last-Modified"); \
        removeHeaderField(out.data, "Server"); \
        removeHeaderField(out.data, "Etag"); \
        removeHeaderField(out.data, "Accept-Ranges"); \
        if (strcmp(out.data, test_exp_outcome) != 0) { \
            freeString(&out); \
            return -1; \
        } \
        freeString(&out); \
    } \
    return err; \
}

void testServerHandler(void *server_handler_args) {
    ServerHandlerArgs *args = server_handler_args;
    const char resp[] = "hi\r\n";
    if (strcmp(args->data, "hello\r\n") == 0) {
        sendAllData(args->socket_fd, resp, sizeof(resp), -1);
    }
    free(args->data);
    if (args->heap_allocated)
        free(args);
}

#define MAKE_SERVER_TEST(connection_timeout, num_threads, on_equals_ret, else_ret, on_err_ret, run_server_at_all, name_suffix) \
int test##name_suffix(void) { \
    const char *host = "127.0.0.1"; \
    const int port = 54321; \
    ThreadPool thread_pool = newThreadPool(num_threads, testServerHandler); \
    const long long connection_timeout_usec = connection_timeout; \
    UnsafeSignal server_killed = 0, kill_ack = 0, server_has_started = 0; \
    if (run_server_at_all) { \
        size_t thread = launchServerOnThread(NULL, &thread_pool, host, port, 2, 2, -1, 50000, 128, connection_timeout_usec, &server_killed, &kill_ack, 0, NULL, &server_has_started); \
        if (thread == -1) \
            return -0xBAD; \
        while (!server_has_started);  /* wait for server to start */ \
    } \
    usleep(5000); \
    const char msg[] = "hello\r\n"; \
    StringRef data = {sizeof(msg), msg}; \
    String out = {0, NULL}; \
    int err = newSocketSendReceiveClose(host, port, data, &out, -1, -1, 500000, 128, 0, 0); \
    if (run_server_at_all) { \
        server_killed = 1; \
        while (!kill_ack); \
    } \
    threadPoolDestroy(&thread_pool); \
    if (err) \
        return on_err_ret; \
    if (strcmp(out.data, "hi\r\n") == 0) \
        return on_equals_ret; \
    return else_ret; \
}

const int test_server_n_msg_reps = 100000;

void testServerHandlerBigData(void *server_handler_args) {
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
        int send_out = sendAllData(args->socket_fd, resp, n_msg_reps * strlen("hi\r\n"), -1);
        assert(!send_out);
        LOG_MSG("responded with 'hi\\r\\n' spam\n");
    }
    free(args->data);
    if (args->heap_allocated)
        free(args);
}

int testServerBigData1(void) {
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
    if (err)
        return err;
    int all_strcmp = 1;
    for (int i = 0; i < n_msg_reps; i++) {
        if (strncmp(&out.data[i * strlen("hi\r\n")], "hi\r\n", strlen("hi\r\n")) != 0) {
            all_strcmp = 0;
            break;
        }
    }
    if (out.len != 4 * n_msg_reps)
        printAndFlush("\nTHE RECEIVED DATA HAS LENGTH %zu, BUT SHOULD HAVE %d\n", out.len, 4 * n_msg_reps);
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

MAKE_RAW_REQUEST_TEST(GET_REQ_TEST1, "www.example.com", 80, GET_REQ_TARGET1, GetReq1);

MAKE_RAW_REQUEST_TEST(GET_REQ_TEST1, "http://www.example.com", 80, GET_REQ_TARGET1, GetReq1UrlPrefix1);

MAKE_RAW_REQUEST_TEST(GET_REQ_TEST1, "https://www.example.com", 80, GET_REQ_TARGET1, GetReq1UrlPrefix2);

MAKE_SERVER_TEST(10000, 4, 0, 42, 0xDEAD, 1, Server1);

MAKE_SERVER_TEST(10000, 0, 0, 99, 0xDEAD, 1, SingleThreadedServer1);

MAKE_SERVER_TEST(0, 4, 99, 42, 0, 0, ShouldTimeoutServer1);

int print_on_pass = 1;
int print_pre_run_msg = 0;
int run_all_tests = 1;
int n_test_reps = 1;//000;
const char *selected_test = "testServerBigData1";

#define REGISTER_TEST_CASE(test_name) \
{ \
    *names = realloc(*names, (n_test_cases + 1) * sizeof(char *)); \
    (*names)[n_test_cases] = #test_name; \
    *tests = realloc(*tests, (n_test_cases + 1) * sizeof(int (*)(void))); \
    (*tests)[n_test_cases] = test_name; \
    n_test_cases++; \
}

int buildTestCases(const char ***names, int (***tests)(void)) {
    int n_test_cases = 0;
    *names = malloc(1 * sizeof(char *));
    *tests = malloc(1 * sizeof(int (*)(void)));

    // Evil macro magic
    REGISTER_TEST_CASE(testParseReq1);
    REGISTER_TEST_CASE(testParseReqMinimal1);
    REGISTER_TEST_CASE(testParseResp1);
    REGISTER_TEST_CASE(testParseReq1HeadToStr);
    REGISTER_TEST_CASE(testParseReqMinimal1HeadToStr);
    REGISTER_TEST_CASE(testParseResp1HeadToStr);
    REGISTER_TEST_CASE(testSerdeReq1);
    REGISTER_TEST_CASE(testSerdeResp1);
    REGISTER_TEST_CASE(testGetReq1);
    REGISTER_TEST_CASE(testGetReq1UrlPrefix1);
    REGISTER_TEST_CASE(testGetReq1UrlPrefix2);
    REGISTER_TEST_CASE(testServer1);
    REGISTER_TEST_CASE(testSingleThreadedServer1);
    REGISTER_TEST_CASE(testShouldTimeoutServer1);
    REGISTER_TEST_CASE(testServerBigData1);

    return n_test_cases;
}

int main(void) {
    const char **names;
    int (**tests)(void);
    int n_tests = buildTestCases(&names, &tests);
    socketInit();
    int tests_always_passed = 1;
    int n_total_failed = 0;
    for (int k = 0; k < n_test_reps; k++) {
        int all_passed = 1;
        int n_failed = 0;
        for (int i = 0; i < n_tests; i++) {
            const char *name = names[i];
            if (!run_all_tests && strcmp(name, selected_test) != 0)
                continue;
            if (print_pre_run_msg) printAndFlush("Running test %s...\n", name);
            int err = tests[i]();
            if (err) {
                all_passed = 0;
                tests_always_passed = 0;
                n_failed++;
                n_total_failed++;
                printAndFlush("Test %s... Error: Code 0x%X (%d)\n", name, err, err);
            } else if (print_on_pass) {
                printAndFlush("Test %s... Passed\n", name);
            }
        }
        if (n_test_reps > 1 && all_passed) printAndFlush("All tests passed this iteration!\n")
        else if (n_test_reps > 1) printAndFlush("%d/%d tests failed this iteration!\n", n_failed, n_tests);
    }
    if (tests_always_passed) printAndFlush("*** ALL TESTS PASSED EVERY SINGLE TIME! ***\n")
    else printAndFlush("*** %d/%d TESTS FAILED OVERALL! ***\n", n_total_failed, n_test_reps * (run_all_tests ? n_tests : 1));
    socketCleanup();
    return 0;
}