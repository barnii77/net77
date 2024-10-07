#include <string.h>
#include <stdio.h>
#include "n77serde.h"
#include "n77request.h"

const char *REQ_TEST1 = "GET /file/hello.jpg HTTP/1.1\r\nHost:www.example.com\r\n\r\n{\"json\": \"body\"}";
const char *RESP_TEST1 = "HTTP/1.0 200 OK\r\nHost:www.example.com\r\n\r\n{\"json\": \"body\"}";
const char *GET_REQ_TEST1 = "GET / HTTP/1.0\r\nHost:www.example.com\r\n\r\n";
const char *GET_REQ_TARGET1 = "HTTP/1.0 200 OK\r\nAccept-Ranges: bytes\r\nAge: \r\nCache-Control: max-age=604800\r\nContent-Type: text/html; charset=UTF-8\r\nDate: \r\nEtag: \"3147526947+gzip\"\r\nExpires: \r\nLast-Modified: \r\nServer: \r\nVary: Accept-Encoding\r\nX-Cache: HIT\r\nContent-Length: 1256\r\nConnection: close\r\n\r\n<!doctype html>\n<html>\n<head>\n    <title>Example Domain</title>\n\n    <meta charset=\"utf-8\" />\n    <meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\" />\n    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n    <style type=\"text/css\">\n    body {\n        background-color: #f0f0f2;\n        margin: 0;\n        padding: 0;\n        font-family: -apple-system, system-ui, BlinkMacSystemFont, \"Segoe UI\", \"Open Sans\", \"Helvetica Neue\", Helvetica, Arial, sans-serif;\n        \n    }\n    div {\n        width: 600px;\n        margin: 5em auto;\n        padding: 2em;\n        background-color: #fdfdff;\n        border-radius: 0.5em;\n        box-shadow: 2px 3px 7px 2px rgba(0,0,0,0.02);\n    }\n    a:link, a:visited {\n        color: #38488f;\n        text-decoration: none;\n    }\n    @media (max-width: 700px) {\n        div {\n            margin: 0 auto;\n            width: auto;\n        }\n    }\n    </style>    \n</head>\n\n<body>\n<div>\n    <h1>Example Domain</h1>\n    <p>This domain is for use in illustrative examples in documents. You may use this\n    domain in literature without prior coordination or asking for permission.</p>\n    <p><a href=\"https://www.iana.org/domains/example\">More information...</a></p>\n</div>\n</body>\n</html>\n";

void replaceHeader(char *http_response, const char *date_header) {
    char *date_pos = strstr(http_response, date_header);

    if (date_pos) {
        // Move pointer to the value part (after "Date: ")
        date_pos += strlen(date_header);

        // Find the end of the Date header line (look for '\r' or '\n')
        char *end_of_line = strchr(date_pos, '\r');
        if (!end_of_line) {
            end_of_line = strchr(date_pos, '\n');
        }

        // Replace the Date value with "xyz" (ensure it fits)
        if (end_of_line) {
            size_t remaining_length = strlen(end_of_line);
            size_t new_value_length = strlen("");
            strncpy(date_pos, "", new_value_length);
            // Move the rest of the string (after the new value) to the correct position
            memmove(date_pos + new_value_length, end_of_line, remaining_length + 1);
        }
    }
}

#define MAKE_PARSE_TEST(test_data, test_type, test_name) \
int test##test_name(void) { \
    size_t len = strlen(test_data); \
    StringRef s = {len, test_data}; \
    test_type out; \
    int err = parse##test_type(s, &out, 1); \
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
    int err = rawRequest(test_url, test_port, dref, &out); \
    if (!err) { \
        replaceHeader(out.data, "Age: "); \
        replaceHeader(out.data, "Date: "); \
        replaceHeader(out.data, "Expires: "); \
        replaceHeader(out.data, "Last-Modified: "); \
        replaceHeader(out.data, "Server: "); \
        if (strcmp(out.data, test_exp_outcome) != 0) { \
            freeString(&out); \
            return -1; \
        } \
        freeString(&out); \
    } \
    return err; \
}

MAKE_PARSE_TEST(REQ_TEST1, Request, ParseReq1);

MAKE_PARSE_TEST(RESP_TEST1, Response, ParseResp1);

MAKE_SERDE_TEST(REQ_TEST1, Request, SerdeReq1);

MAKE_SERDE_TEST(RESP_TEST1, Response, SerdeResp1);

// FIXME doesn't like "http://" prefix
// MAKE_RAW_REQUEST_TEST(GET_REQ_TEST1, "www.example.com", 80, GET_REQ_TARGET1, GetReq1);

int testGetReq1(void) {
    StringRef dref = {strlen(GET_REQ_TEST1), GET_REQ_TEST1};
    String out;
    int err = rawRequest("www.example.com", 80, dref, &out);
    if (!err) {
        replaceHeader(out.data, "Age: ");
        replaceHeader(out.data, "Date: ");
        replaceHeader(out.data, "Expires: ");
        replaceHeader(out.data, "Last-Modified: ");
        replaceHeader(out.data, "Server: ");
        if (strcmp(out.data, GET_REQ_TARGET1) != 0) {
            freeString(&out);
            return -1;
        }
        freeString(&out);
    }
    return err;
}

// FIXME DNS (getaddrinfo) in sock.c:23 fails on windows if I use mingw and the windows socket api headers instead of the posix ones

int (*const tests[])(void) = {testParseReq1, testParseResp1, testSerdeReq1, testSerdeResp1, testGetReq1};

int run_all_tests = 0;
const char *selected_test = "testGetReq1";
const char *names[] = {"testParseReq1", "testParseResp1", "testSerdeReq1", "testSerdeResp1", "testGetReq1"};

int main(void) {
    for (int i = 0; i < sizeof(tests) / sizeof(int (*const)(void)); i++) {
        const char *name = names[i];
        if (!run_all_tests && strcmp(name, selected_test) != 0)
            continue;
        printf("Running test %s... ", name);
        int err = tests[i]();
        if (err) {
            printf("Error: Code %d\n", err);
        } else {
            printf("Passed\n");
        }
    }
    return 0;
}