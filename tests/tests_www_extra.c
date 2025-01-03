#include "testing_helpers.h"

MAKE_RAW_REQUEST_TEST(GET_REQ_TEST1, "http://www.example.com", 80, GET_REQ_TARGET1, GetReq1UrlPrefix1, -1, NO_CALLBACK);

MAKE_RAW_REQUEST_TEST(GET_REQ_TEST1, "https://www.example.com", 80, GET_REQ_TARGET1, GetReq1UrlPrefix2, -1,
                      NO_CALLBACK);

MAKE_RAW_REQUEST_N_TIMES_USING_SESSION(GET_REQ_HTTPBIN_ROUTE_GET1, "http://httpbin.org", 80,
                                       GET_REQ_HTTPBIN_ROUTE_GET_TARGET1, GetReq1UrlPrefix1WithReps, 3, -1,
                                       NO_CALLBACK);

MAKE_RAW_REQUEST_N_TIMES_USING_SESSION(GET_REQ_HTTPBIN_ROUTE_GET1, "https://httpbin.org", 80,
                                       GET_REQ_HTTPBIN_ROUTE_GET_TARGET1, GetReq1UrlPrefix2WithReps, 3, -1,
                                       NO_CALLBACK);

BEGIN_RUNNER_SETTINGS()
    print_on_pass = 1;
    print_pre_run_msg = 0;
    run_all_tests = 1;
    n_test_reps = 1;
    selected_test = "testGetReq1WithReps";
END_RUNNER_SETTINGS()

BEGIN_TEST_LIST()
    REGISTER_TEST_CASE(testGetReq1UrlPrefix1)
    REGISTER_TEST_CASE(testGetReq1UrlPrefix2)
    REGISTER_TEST_CASE(testGetReq1UrlPrefix1WithReps)
    REGISTER_TEST_CASE(testGetReq1UrlPrefix2WithReps)
END_TEST_LIST()

MAKE_TEST_SUITE_RUNNABLE()