#include "testing_helpers.h"
#define TEST_SERVER_N_MSG_REPS 100000000  // the number of msg reps for the big data test
#include "big_data_test.h"

BEGIN_RUNNER_SETTINGS()
    print_on_pass = 1;
    print_pre_run_msg = 0;
    run_all_tests = 1;
    n_test_reps = 50;
    selected_test = "testServerBigData1";
END_RUNNER_SETTINGS()

BEGIN_TEST_LIST()
    REGISTER_TEST_CASE(testServerBigData1)
END_TEST_LIST()

MAKE_TEST_SUITE_RUNNABLE()