#include "testing_helpers.h"
#define TEST_SERVER_N_MSG_REPS 1000000  // the number of msg reps for the big data test
#include "big_data_test.h"

MAKE_PARSE_TEST(REQ_TEST1, HttpRequest, ParseReq1, 1);

MAKE_PARSE_TEST(REQ_TEST_EMPTY_BODY_AND_HEADER1, HttpRequest, ParseReqMinimal1, 1);

MAKE_PARSE_TEST(RESP_TEST1, HttpResponse, ParseResp1, 1);

MAKE_PARSE_TEST(REQ_TEST1, HttpRequest, ParseReq1HeadToStr, 0);

MAKE_PARSE_TEST(REQ_TEST_EMPTY_BODY_AND_HEADER1, HttpRequest, ParseReqMinimal1HeadToStr, 0);

MAKE_PARSE_TEST(RESP_TEST1, HttpResponse, ParseResp1HeadToStr, 0);

MAKE_SERDE_TEST(REQ_TEST1, HttpRequest, SerdeReq1);

MAKE_SERDE_TEST(RESP_TEST1, HttpResponse, SerdeResp1);

MAKE_SERVER_TEST(10000, 4, 0, 42, 0xDEAD, 1, Server1);

MAKE_SERVER_TEST(10000, 0, 0, 99, 0xDEAD, 1, SingleThreadedServer1);

MAKE_SERVER_TEST(0, 4, 99, 42, 0, 0, ShouldTimeoutServer1);

BEGIN_RUNNER_SETTINGS()
    print_on_pass = 1;
    print_pre_run_msg = 0;
    run_all_tests = 1;
    n_test_reps = 20;
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