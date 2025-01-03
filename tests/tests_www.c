#include "testing_helpers.h"

#define DEFINE_OUTPUT_CLIPPER_CALLBACK(size) \
static void takeFirst##size##Chars(String *out) { \
    if (out->len <= size) \
        return; \
    out->data[size] = '\0'; \
    out->len = size; \
}

static void removeAllNonConstInfo(String *s) {
    removeStrValueOfJsonField(s, "origin");
    removeStrValueOfJsonField(s, "X-Amzn-Trace-Id");
    removeLinesNotStartingWith(s, "{", 6);
    s->len = strlen(s->data);
}

DEFINE_OUTPUT_CLIPPER_CALLBACK(271);

// some tests that should pass
MAKE_RAW_REQUEST_TEST(GET_REQ_TEST1, "www.example.com", 80, GET_REQ_TARGET1, GetReq1, -1, NO_CALLBACK);

MAKE_RAW_REQUEST_N_TIMES_USING_SESSION(GET_REQ_HTTPBIN_ROUTE_GET1, "httpbin.org", 80, GET_REQ_HTTPBIN_ROUTE_GET_TARGET1,
                                       GetReq1WithReps, 3, -1, NO_CALLBACK);

MAKE_RAW_REQUEST_TEST(GET_REQ_HTTPBIN_ROUTE_DELAY1, "httpbin.org", 80, GET_REQ_HTTPBIN_ROUTE_DELAY_TARGET1,
                      GetReqWithDelay1, 4000000, takeFirst271Chars);

MAKE_RAW_REQUEST_N_TIMES_USING_SESSION(GET_REQ_HTTPBIN_ROUTE_DELAY1, "httpbin.org", 80,
                                       GET_REQ_HTTPBIN_ROUTE_DELAY_TARGET1, GetReqWithDelayWithReps1, 3, 4500000,
                                       takeFirst271Chars);

MAKE_RAW_REQUEST_N_TIMES_USING_SESSION(GET_REQ_HTTPBIN_ROUTE_STREAM1, "httpbin.org", 80,
                                       GET_REQ_HTTPBIN_ROUTE_STREAM_TARGET1, GetReqStreamResponse1, 2, 300000,
                                       removeAllNonConstInfo);

// some tests that should time out
MAKE_RAW_REQUEST_TEST(GET_REQ_HTTPBIN_ROUTE_DELAY1, "httpbin.org", 80, GET_REQ_HTTPBIN_ROUTE_DELAY_TARGET1,
                      GetReqWithDelay1_INNER_WARNING_INTERNAL, 2000000, takeFirst271Chars);

MAKE_RAW_REQUEST_N_TIMES_USING_SESSION(GET_REQ_HTTPBIN_ROUTE_DELAY1, "httpbin.org", 80,
                                       GET_REQ_HTTPBIN_ROUTE_DELAY_TARGET1,
                                       GetReqWithDelayWithReps1_INNER_WARNING_INTERNAL, 3, 2000000, takeFirst271Chars);

MAKE_INVERTED_TEST(GetReqWithDelayShouldTimeout1, GetReqWithDelay1_INNER_WARNING_INTERNAL);

MAKE_INVERTED_TEST(GetReqWithDelayWithRepsShouldTimeout1, GetReqWithDelayWithReps1_INNER_WARNING_INTERNAL);

// boilerplate from here
BEGIN_RUNNER_SETTINGS()
    print_on_pass = 1;
    print_pre_run_msg = 0;
    run_all_tests = 1;
    n_test_reps = 1;
    selected_test = "testGetReqStreamResponse1";
END_RUNNER_SETTINGS()

BEGIN_TEST_LIST()
    REGISTER_TEST_CASE(testGetReq1)
    REGISTER_TEST_CASE(testGetReq1WithReps)
    REGISTER_TEST_CASE(testGetReqWithDelay1)
    REGISTER_TEST_CASE(testGetReqWithDelayWithReps1)
    REGISTER_TEST_CASE(testGetReqWithDelayShouldTimeout1)
    REGISTER_TEST_CASE(testGetReqWithDelayWithRepsShouldTimeout1)
    REGISTER_TEST_CASE(testGetReqStreamResponse1)
END_TEST_LIST()

MAKE_TEST_SUITE_RUNNABLE()