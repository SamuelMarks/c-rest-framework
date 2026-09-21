/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include "c_rest_graphql.h"
#include "c_rest_mem.h"
#include "c_rest_testing_mocks.h"
#include <string.h>
/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
#ifdef C_REST_TESTING_MALLOC_HOOK
  g_mock_graphql_free_countdown = -1;
#endif
}

#ifdef C_REST_TESTING_MALLOC_HOOK
static int g_fail_malloc_at = 0;
static void *fail_malloc_graphql(size_t size) {
  static int alloc_count = 0;
  if (g_fail_malloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_malloc_at) {
    alloc_count = 0;
    g_fail_malloc_at = 0;
    return NULL;
  }
  return malloc(size);
}
#endif

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

  /* Line 195: fail c_rest_graphql_node_free in parse_selection_set error
   * handler */
  g_mock_graphql_free_countdown = 0;
  c_rest_graphql_parse("query { a { b ! } }", 19, &doc);

  /* Line 230: fail in parse_name for alias target */
  g_mock_graphql_free_countdown = 0;
  c_rest_graphql_parse("query { alias: 123 }", 20, &doc);

  /* Line 328: fail alloc_list for doc->definitions */
  g_fail_malloc_at = 0;
  fail_malloc_graphql(0);
  g_crf_malloc_hook = fail_malloc_graphql;
  g_fail_malloc_at = 2;
  g_mock_graphql_free_countdown = 0;
  c_rest_graphql_parse("query { a }", 11, &doc);
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = 0;

  /* Lines 383 & 394: fail c_rest_graphql_node_free inside arguments /
   * selection_set */
  {
    struct c_rest_graphql_node *n;
    struct c_rest_graphql_node *arg;
    struct c_rest_graphql_node *sel;
    struct c_rest_graphql_node_list *args_list;
    struct c_rest_graphql_node_list *sel_list;

    n = (struct c_rest_graphql_node *)calloc(
        1, sizeof(struct c_rest_graphql_node));
    arg = (struct c_rest_graphql_node *)calloc(
        1, sizeof(struct c_rest_graphql_node));
    sel = (struct c_rest_graphql_node *)calloc(
        1, sizeof(struct c_rest_graphql_node));
    args_list = (struct c_rest_graphql_node_list *)calloc(
        1, sizeof(struct c_rest_graphql_node_list));
    sel_list = (struct c_rest_graphql_node_list *)calloc(
        1, sizeof(struct c_rest_graphql_node_list));

    args_list->nodes = (struct c_rest_graphql_node **)calloc(
        1, sizeof(struct c_rest_graphql_node *));
    args_list->nodes[0] = arg;
    args_list->count = 1;
    n->arguments = args_list;

    g_mock_graphql_free_countdown = 1;
    c_rest_graphql_node_free(n);
    g_mock_graphql_free_countdown = -1;

    n->arguments = NULL;
    sel_list->nodes = (struct c_rest_graphql_node **)calloc(
        1, sizeof(struct c_rest_graphql_node *));
    sel_list->nodes[0] = sel;
    sel_list->count = 1;
    n->selection_set = sel_list;

    g_mock_graphql_free_countdown = 1;
    c_rest_graphql_node_free(n);
    g_mock_graphql_free_countdown = -1;

    free(args_list->nodes);
    free(args_list);
    free(arg);
    free(sel_list->nodes);
    free(sel_list);
    free(sel);
    free(n);
  }
#endif

  PASS();
}

SUITE_EXTERN(graphql_mock_suite);
SUITE(graphql_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_graphql_error_branches);
}
