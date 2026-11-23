/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_graphql.h"
#include "test_protos.h"
#include <string.h>
/* clang-format on */

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
static c_rest_error_t ext_echo_resolver(const char *field_name, char **out_json,
                                        size_t *out_len, void *user_data) {
  int *called = (int *)user_data;
  size_t len = strlen(field_name) + 2;
  char *buf = NULL;
  if (called)
    (*called)++;
  if (C_REST_MALLOC(len + 1, &buf) != 0)
    return C_REST_ERROR_OOM;
  memcpy(buf, field_name, len - 2);
  buf[len - 2] = '\0';
  *out_json = buf;
  *out_len = strlen(buf);
  return C_REST_OK;
}

static c_rest_error_t ext_failing_alloc_resolver(const char *field_name,
                                                 char **out_json,
                                                 size_t *out_len,
                                                 void *user_data) {
  char *buf = NULL;
  (void)field_name;
  (void)user_data;
  if (C_REST_MALLOC(4, &buf) == 0) {
    buf[0] = 'x';
    buf[1] = '\0';
    *out_json = buf;
    *out_len = 1;
  }
  return C_REST_ERROR_GENERIC;
}

TEST test_graphql_ext_nested_and_resolvers(void) {
  const char *query =
      "mutation UpdateItem { updated: updateItem { id title } }";
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  char *json = NULL;
  size_t json_len = 0;
  int called = 0;

  ASSERT_EQ(C_REST_OK, c_rest_graphql_parse(query, strlen(query), &doc));
  ASSERT(doc != NULL);
  if (doc) {
    ASSERT_EQ((size_t)1, doc->definitions->count);
    ASSERT_EQ(C_REST_GRAPHQL_OP_MUTATION, doc->definitions->nodes[0]->op_type);
    ASSERT_STR_EQ("UpdateItem", doc->definitions->nodes[0]->name);
    ASSERT_STR_EQ("updated",
                  doc->definitions->nodes[0]->selection_set->nodes[0]->alias);
  }

  ASSERT_EQ(C_REST_OK, c_rest_graphql_schema_init(&schema));
  ASSERT_EQ(C_REST_OK, c_rest_graphql_schema_add_resolver(
                           schema, "updateItem", ext_echo_resolver, &called));
  ASSERT_EQ(C_REST_OK, c_rest_graphql_resolve(doc, schema, &json, &json_len));
  ASSERT_EQ(1, called);
  ASSERT(json != NULL);
  if (json) {
    ASSERT_EQ((size_t)12, json_len);
    C_REST_FREE(json);
    json = NULL;
  }

  ASSERT_EQ(C_REST_OK,
            c_rest_graphql_schema_add_resolver(
                schema, "updateItem", ext_failing_alloc_resolver, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_graphql_resolve(doc, schema, &json, &json_len));
  ASSERT_EQ(NULL, json);

  ASSERT_EQ(C_REST_OK, c_rest_graphql_schema_free(schema));
  ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
  PASS();
}
#else
TEST test_graphql_ext_nested_and_resolvers(void) { PASS(); }
#endif

SUITE(graphql_ext_suite) { RUN_TEST(test_graphql_ext_nested_and_resolvers); }
