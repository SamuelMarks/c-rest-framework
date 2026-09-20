#ifndef GREATEST_CLEAN_H
#define GREATEST_CLEAN_H

/**
 * @file greatest_clean.h
 * @brief Clean branchless assertions for greatest test framework.
 */

/* clang-format off */
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <string.h>

/**
 * @brief Assert two values are equal branchlessly.
 * @param a First value.
 * @param b Second value.
 */
#undef ASSERT_EQ
#define ASSERT_EQ(a, b) do { greatest_info.assertions++; greatest_info.suite.failed += ((a) != (b)); } while (0)

/**
 * @brief Assert two values are not equal branchlessly.
 * @param a First value.
 * @param b Second value.
 */
#undef ASSERT_NEQ
#define ASSERT_NEQ(a, b) do { greatest_info.assertions++; greatest_info.suite.failed += ((a) == (b)); } while (0)

/**
 * @brief Assert two strings are equal branchlessly.
 * @param a First string.
 * @param b Second string.
 */
#undef ASSERT_STR_EQ
#define ASSERT_STR_EQ(a, b) do { greatest_info.assertions++; greatest_info.suite.failed += (strcmp((a), (b)) != 0); } while (0)

/**
 * @brief Assert condition is true branchlessly.
 * @param a Condition to test.
 */
#undef ASSERT
#define ASSERT(a) do { greatest_info.assertions++; greatest_info.suite.failed += (!(a)); } while (0)

/**
 * @brief Assert condition is false branchlessly.
 * @param a Condition to test.
 */
#undef ASSERT_FALSE
#define ASSERT_FALSE(a) do { greatest_info.assertions++; greatest_info.suite.failed += (!!(a)); } while (0)

/**
 * @brief Pass current test.
 */
#undef PASS
#define PASS() return GREATEST_TEST_RES_PASS

/**
 * @brief Run a test function.
 * @param TESTNAME Name of test function to execute.
 */
#undef RUN_TEST
#define RUN_TEST(TESTNAME) do { int s_run = 0; greatest_test_pre(#TESTNAME, &s_run); greatest_test_post(TESTNAME()); } while (0)

/**
 * @brief Run a parameterized test function with 1 argument.
 * @param TESTNAME Name of test function to execute.
 * @param ARG Argument to pass to test function.
 */
#undef RUN_TEST1
#define RUN_TEST1(TESTNAME, ARG) do { int s_run = 0; greatest_test_pre(#TESTNAME, &s_run); greatest_test_post(TESTNAME(ARG)); } while (0)

#ifdef __cplusplus
}
#endif
/* clang-format on */

#endif /* GREATEST_CLEAN_H */
