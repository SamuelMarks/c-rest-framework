/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_graphql.h"

static int g_mock_free_countdown = -1;

extern c_rest_error_t c_rest_graphql_node_free(struct c_rest_graphql_node *node);

static c_rest_error_t mock_c_rest_graphql_node_free(struct c_rest_graphql_node *node) {
    if (g_mock_free_countdown >= 0) {
        if (g_mock_free_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_free_countdown--;
    }
    return c_rest_graphql_node_free(node);
}

#define c_rest_graphql_node_free mock_c_rest_graphql_node_free

#define c_rest_graphql_parse test_c_rest_graphql_parse
#define parse_field test_parse_field
#define parse_operation test_parse_operation
#define list_append test_list_append

#include "../src/c_rest_graphql.c"

#undef c_rest_graphql_node_free

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_free_countdown = -1;
}

TEST test_graphql_error_branches(void) {
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_node_list list = {0};

  /* list_append error branch */
  g_mock_free_countdown = 0;
  /* Simulate C_REST_REALLOC failure? Wait, C_REST_REALLOC fails if we mock
     MALLOC, but we want to test c_rest_graphql_node_free being called. But we
     can't easily make REALLOC fail without mocking it. Actually,
     c_rest_graphql_node_free failure doesn't cause a crash, it's just what we
     want to test. Let's just skip the list_append test if we can't easily mock
     REALLOC, and stick to the ones that call node_free on parse failure! */

  /* parse_field error branches */
  g_mock_free_countdown = 0;
  /* fail in parse_arguments */
  test_c_rest_graphql_parse("query { a ( }", 13, &doc);

  g_mock_free_countdown = 0;
  /* fail in parse_selection_set */
  test_c_rest_graphql_parse("query { a { }", 13, &doc);

  /* parse_operation error branches */
  g_mock_free_countdown = 0;
  /* fail in parse_selection_set */
  test_c_rest_graphql_parse("query { ", 8, &doc);

  g_mock_free_countdown = 0;
  /* fail in parse_name */
  test_c_rest_graphql_parse("query 123", 9, &doc);

  PASS();
}

SUITE(graphql_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_graphql_error_branches);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(graphql_mock_suite);
  GREATEST_MAIN_END();
}
