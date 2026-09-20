/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include "c_rest_graphql.h"
#include "c_rest_testing_mocks.h"
#include <string.h>
/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_graphql_free_countdown = -1;
#endif
}

TEST test_graphql_error_branches(void) {
  struct c_rest_graphql_node *doc = NULL;

#ifdef C_REST_TESTING_MALLOC_HOOK
  /* parse_field error branches */
  g_mock_graphql_free_countdown = 0;
  /* fail in parse_arguments */
  c_rest_graphql_parse("query { a ( }", 13, &doc);

  g_mock_graphql_free_countdown = 0;
  /* fail in parse_selection_set */
  c_rest_graphql_parse("query { a { }", 13, &doc);

  /* parse_operation error branches */
  g_mock_graphql_free_countdown = 1;
  c_rest_graphql_parse("query { a }", 11, &doc);
  c_rest_graphql_node_free(doc);
  doc = NULL;

  g_mock_graphql_free_countdown = 0;
  /* fail in parse_selection_set */
  c_rest_graphql_parse("query { ", 8, &doc);

  g_mock_graphql_free_countdown = 0;
  /* fail in parse_name */
  c_rest_graphql_parse("query 123", 9, &doc);
#endif

  PASS();
}

SUITE_EXTERN(graphql_mock_suite);
SUITE(graphql_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_graphql_error_branches);
}
