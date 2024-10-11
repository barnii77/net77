#include <string.h>
#include <stdio.h>
#include "n77serde.h"
#include "n77request.h"
#include "n77init.h"
#include "n77server.h"
#include "n77utils.h"
#include "n77netincludes.h"

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
    int err = rawRequest(removeURLPrefix(charPtrToStringRef(test_url)).data, test_port, dref, &out); \
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

int testServerHandler(void *ctx, size_t socket_fd, char *data, size_t size) {
    const char resp[] = "hi";
    if (strcmp(data, "hello") == 0 && size == strlen(resp)) {
        send(socket_fd, resp, sizeof(resp));
        return 0;
    }
    return 1;
}

int testServer(void) {
    runServer();
    return 0;
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

int (*const tests[])(void) = {testParseReq1, testParseReqMinimal1, testParseResp1, testParseReq1HeadToStr,
                              testParseReqMinimal1HeadToStr, testParseResp1HeadToStr, testSerdeReq1, testSerdeResp1,
                              testGetReq1, testGetReq1UrlPrefix1, testGetReq1UrlPrefix2};

int print_on_pass = 0;
int print_pre_run_msg = 0;
int run_all_tests = 1;
const char *selected_test = "testGetReq1";
const char *names[] = {"testParseReq1", "testParseReqMinimal1", "testParseResp1", "testParseReq1HeadToStr",
                       "testParseReqMinimal1HeadToStr", "testParseResp1HeadToStr", "testSerdeReq1", "testSerdeResp1",
                       "testGetReq1", "testGetReq1UrlPrefix1", "testGetReq1UrlPrefix2"};

int main(void) {
    socketInit();
    if (sizeof(names) / sizeof(char *) != sizeof(tests) / sizeof(int (*const)(void)))
        printf("Warning: not every test has a name entry!\n");

    int all_passed = 1;
    for (int i = 0; i < sizeof(tests) / sizeof(int (*const)(void)); i++) {
        const char *name = names[i];
        if (!run_all_tests && strcmp(name, selected_test) != 0)
            continue;
        if (print_pre_run_msg)
            printf("Running test %s...\n", name);
        int err = tests[i]();
        if (err) {
            all_passed = 0;
            printf("Test %s Error: Code %d\n", name, err);
        } else if (print_on_pass) {
            printf("Test %s Passed\n", name);
        }
    }
    if (all_passed)
        printf("All tests passed!\n");
    socketCleanup();
    return 0;
}