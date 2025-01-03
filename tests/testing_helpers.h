#ifndef NET77_TESTING_HELPERS_H
#define NET77_TESTING_HELPERS_H

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

static const char *REQ_TEST1 = "GET /file/hello.jpg HTTP/1.1\r\nHost:www.example.com\r\n\r\n{\"json\": \"body\"}";
static const char *REQ_TEST_EMPTY_BODY_AND_HEADER1 = "GET / HTTP/1.0\r\n\r\nxyz";
static const char *RESP_TEST1 = "HTTP/1.0 200 OK\r\nHost:www.example.com\r\n\r\n{\"json\": \"body\"}";
static const char *GET_REQ_TEST1 = "GET / HTTP/1.0\r\nHost:www.example.com\r\n\r\n";
static const char *GET_REQ_TARGET1 = "HTTP/1.0 200 OK\r\nCache-Control: max-age=604800\r\nContent-Type: text/html; charset=UTF-8\r\nVary: Accept-Encoding\r\nX-Cache: HIT\r\nContent-Length: 1256\r\nConnection: close\r\n\r\n<!doctype html>\n<html>\n<head>\n    <title>Example Domain</title>\n\n    <meta charset=\"utf-8\" />\n    <meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\" />\n    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n    <style type=\"text/css\">\n    body {\n        background-color: #f0f0f2;\n        margin: 0;\n        padding: 0;\n        font-family: -apple-system, system-ui, BlinkMacSystemFont, \"Segoe UI\", \"Open Sans\", \"Helvetica Neue\", Helvetica, Arial, sans-serif;\n        \n    }\n    div {\n        width: 600px;\n        margin: 5em auto;\n        padding: 2em;\n        background-color: #fdfdff;\n        border-radius: 0.5em;\n        box-shadow: 2px 3px 7px 2px rgba(0,0,0,0.02);\n    }\n    a:link, a:visited {\n        color: #38488f;\n        text-decoration: none;\n    }\n    @media (max-width: 700px) {\n        div {\n            margin: 0 auto;\n            width: auto;\n        }\n    }\n    </style>    \n</head>\n\n<body>\n<div>\n    <h1>Example Domain</h1>\n    <p>This domain is for use in illustrative examples in documents. You may use this\n    domain in literature without prior coordination or asking for permission.</p>\n    <p><a href=\"https://www.iana.org/domains/example\">More information...</a></p>\n</div>\n</body>\n</html>\n";

static const char *GET_REQ_HTTPBIN_ROUTE_GET1 = "GET /json HTTP/1.1\r\nHost: httpbin.org\r\n\r\n";
static const char *GET_REQ_HTTPBIN_ROUTE_GET_TARGET1 = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 429\r\nConnection: keep-alive\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Credentials: true\r\n\r\n{\n  \"slideshow\": {\n    \"author\": \"Yours Truly\", \n    \"date\": \"date of publication\", \n    \"slides\": [\n      {\n        \"title\": \"Wake up to WonderWidgets!\", \n        \"type\": \"all\"\n      }, \n      {\n        \"items\": [\n          \"Why <em>WonderWidgets</em> are great\", \n          \"Who <em>buys</em> WonderWidgets\"\n        ], \n        \"title\": \"Overview\", \n        \"type\": \"all\"\n      }\n    ], \n    \"title\": \"Sample Slide Show\"\n  }\n}\n";

static const char *GET_REQ_HTTPBIN_ROUTE_DELAY1 = "GET /delay/3 HTTP/1.1\r\nHost: httpbin.org\r\n\r\n";
static const char *GET_REQ_HTTPBIN_ROUTE_DELAY_TARGET1 = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 248\r\nConnection: keep-alive\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Credentials: true\r\n\r\n{\n  \"args\": {}, \n  \"data\": \"\", \n  \"files\": {}, \n  \"form\": {}, \n  \"headers\": {\n    \"Host\": \"httpbin.org\"";

static const char *GET_REQ_HTTPBIN_ROUTE_STREAM1 = "GET /stream/30 HTTP/1.1\r\nHost: httpbin.org\r\n\r\n";
static const char *GET_REQ_HTTPBIN_ROUTE_STREAM_TARGET1 = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\nConnection: keep-alive\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Credentials: true\r\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 0}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 1}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 2}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 3}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 4}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 5}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 6}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 7}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 8}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 9}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 10}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 11}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 12}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 13}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 14}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 15}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 16}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 17}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 18}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 19}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 20}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 21}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 22}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 23}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 24}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 25}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 26}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 27}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 28}\n{\"url\": \"http://httpbin.org/stream/30\", \"args\": {}, \"headers\": {\"Host\": \"httpbin.org\", \"X-Amzn-Trace-Id\": \"\"}, \"origin\": \"\", \"id\": 29}\n";

static void removeHeaderField(char *http_response, const char *field_name) {
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

static void removeStrValueOfJsonField(String *http_response_str, const char *field_name) {
    unsigned long name_size = strlen(field_name);
    char *needle = malloc(name_size + 4);
    assert(needle);
    memcpy(needle + 1, field_name, name_size);
    needle[0] = '"';
    needle[name_size + 1] = '"';
    needle[name_size + 2] = ':';
    needle[name_size + 3] = '\0';
    char *http_response = http_response_str->data;
    char *origin_pos = strstr(http_response, needle);

    while (origin_pos) {
        char *value_start = origin_pos + name_size + 3;
        while (*value_start == ' ' || *value_start == '\t') {
            value_start++;
        }

        if (*value_start == '"') {
            // Find the closing quote for the string value
            char *value_end = strchr(value_start + 1, '"');
            if (value_end) {
                // Shift the remaining characters after the value to overwrite it
                size_t len_after_value = strlen(value_end + 1);
                memmove(value_start + 2, value_end + 1, len_after_value + 1); // include null terminator

                // Replace the value with an empty string: `""`
                value_start[0] = '"';
                value_start[1] = '"';
            }
        }

        origin_pos = strstr(value_start, needle);
    }

    free(needle);
}

static void removeLinesNotStartingWith(String *str, const char *line_start, unsigned int n_lines_to_skip) {
    char *string = str->data;
    char *read_ptr = string;
    char *write_ptr = string;
    size_t line_start_len = strlen(line_start);
    unsigned int lines_skipped = 0;

    while (*read_ptr) {
        // Check if we have skipped the first n lines
        if (lines_skipped < n_lines_to_skip) {
            // Skip the current line (move read_ptr to the next '\n')
            while (*read_ptr && *read_ptr != '\n') {
                read_ptr++;
                write_ptr++;
            }
            // Skip the '\n' itself, if present
            if (*read_ptr) {
                read_ptr++;
                write_ptr++;
            }
            lines_skipped++;
            continue;
        }

        // Check if the current line starts with line_start
        if (strncmp(read_ptr, line_start, line_start_len) != 0) {
            // Skip the entire line (move read_ptr to the next '\n')
            while (*read_ptr && *read_ptr != '\n') {
                read_ptr++;
            }
            // Skip the '\n' itself, if present
            if (*read_ptr) {
                read_ptr++;
            }
        } else {
            // Copy the line that starts with line_start to the write_ptr
            while (*read_ptr && *read_ptr != '\n') {
                *write_ptr = *read_ptr;
                write_ptr++;
                read_ptr++;
            }
            // Append the newline after the line
            if (*read_ptr) {
                *write_ptr = *read_ptr;
                write_ptr++;
                read_ptr++;
            }
        }
    }

    // Null-terminate the modified string
    *write_ptr = '\0';
}

static void NO_CALLBACK(String *s) {}

#define MAKE_PARSE_TEST(test_data, test_type, test_name, parse_header_as_structs) \
static int test##test_name(void) { \
    size_t len = strlen(test_data); \
    StringRef s = {len, test_data}; \
    test_type out; \
    int err = parse##test_type(s, &out, parse_header_as_structs); \
    return err; \
}

// serialize/deserialize should result in the same thing
#define MAKE_SERDE_TEST(test_data, test_type, test_name) \
static int test##test_name(void) { \
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

#define MAKE_RAW_REQUEST_TEST(test_data, test_url, test_port, test_exp_outcome, test_name, response_timeout, out_transformation_callback) \
static int test##test_name(void) { \
    StringRef dref = {strlen(test_data), test_data}; \
    String out; \
    int err = rawRequest(removeURLPrefix(charPtrToStringRef(test_url)).data, test_port, dref, &out, -1, -1, response_timeout); \
    if (!err) { \
        removeHeaderField(out.data, "Age"); \
        removeHeaderField(out.data, "Date"); \
        removeHeaderField(out.data, "Expires"); \
        removeHeaderField(out.data, "Last-Modified"); \
        removeHeaderField(out.data, "Server"); \
        removeHeaderField(out.data, "Etag"); \
        removeHeaderField(out.data, "Accept-Ranges"); \
        out_transformation_callback(&out); \
        if (strcmp(out.data, test_exp_outcome) != 0) { \
            freeString(&out); \
            return -1; \
        } \
        freeString(&out); \
    } \
    return err; \
}

#define MAKE_RAW_REQUEST_N_TIMES_USING_SESSION(test_data, test_url, test_port, test_exp_outcome, test_name, n_request_repeats, response_timeout, out_transformation_callback) \
static int test##test_name(void) { \
    Session sess = newSession(); \
    openSession(removeURLPrefix(charPtrToStringRef(test_url)).data, test_port, -1, &sess); \
    StringRef dref = {strlen(test_data), test_data}; \
    int err; \
    for (int i = 0; i < n_request_repeats; i++) { \
        String out; \
        err = rawRequestInSession(&sess, dref, &out, -1, response_timeout); \
        if (!err) { \
            removeHeaderField(out.data, "Age"); \
            removeHeaderField(out.data, "Date"); \
            removeHeaderField(out.data, "Expires"); \
            removeHeaderField(out.data, "Last-Modified"); \
            removeHeaderField(out.data, "Server"); \
            removeHeaderField(out.data, "Etag"); \
            removeHeaderField(out.data, "Accept-Ranges"); \
            out_transformation_callback(&out); \
            if (strcmp(out.data, test_exp_outcome) != 0) { \
                freeString(&out); \
                return -1; \
            } \
            freeString(&out); \
        } \
    } \
    closeSession(&sess); \
    return err; \
}

#define MAKE_INVERTED_TEST(test_name, inner_test_name) \
static int test##test_name(void) { \
    return !test##inner_test_name(); \
}

static void testServerHandler(void *server_handler_args) {
    ServerHandlerArgs *args = server_handler_args;
    const char resp[] = "hi\r\n";
    if (strcmp(args->data, "hello\r\n") == 0) {
        sendAllData(args->socket_fd, resp, sizeof(resp), -1, NULL);
    }
    free(args->data);
    if (args->heap_allocated)
        free(args);
}

#define MAKE_SERVER_TEST(connection_timeout, num_threads, on_equals_ret, else_ret, on_err_ret, run_server_at_all, name_suffix) \
static int test##name_suffix(void) { \
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

static int print_on_pass;  // = 0;
static int print_pre_run_msg;  // = 0;
static int run_all_tests;  // = 1;
static int n_test_reps;  // = 1;
static const char *selected_test;  // = "";

#define REGISTER_TEST_CASE(test_name) \
{ \
    *names = realloc(*names, (n_test_cases + 1) * sizeof(char *)); \
    (*names)[n_test_cases] = #test_name; \
    *tests = realloc(*tests, (n_test_cases + 1) * sizeof(int (*)(void))); \
    (*tests)[n_test_cases] = test_name; \
    n_test_cases++; \
}

static void initStaticVars(void);

static int buildTestCases(const char ***names, int (***tests)(void));

#define BEGIN_TEST_LIST() \
static int buildTestCases(const char ***names, int (***tests)(void)) { \
    int n_test_cases = 0; \
    *names = malloc(1 * sizeof(char *)); \
    *tests = malloc(1 * sizeof(int (*)(void))); \
    // Evil macro magic

#define END_TEST_LIST() \
    return n_test_cases; \
}

#define BEGIN_RUNNER_SETTINGS() \
static void initStaticVars() {

#define END_RUNNER_SETTINGS() \
}

#define MAKE_TEST_SUITE_RUNNABLE() \
int main(void) { \
    runTests(); \
    return 0; \
}

void runTests(void) {
    initStaticVars();
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
        else if (n_test_reps > 1) printAndFlush("%d/%d tests failed this iteration!\n", n_failed,
                                                (run_all_tests ? n_tests : 1));
    }
    if (tests_always_passed) printAndFlush("*** ALL TESTS PASSED EVERY SINGLE TIME! ***\n")
    else printAndFlush("*** %d/%d TESTS FAILED OVERALL! ***\n", n_total_failed,
                       n_test_reps * (run_all_tests ? n_tests : 1));
    socketCleanup();
}

#endif