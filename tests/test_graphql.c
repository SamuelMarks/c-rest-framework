/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_graphql.h"
#include "c_rest_openapi.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_testing_mocks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static int g_malloc_fail_after = -1;
static void *fail_malloc_n(size_t size) {
  int do_fail = (g_malloc_fail_after == 0);
  g_malloc_fail_after -= (g_malloc_fail_after > 0);
  return do_fail ? NULL : malloc(size);
}

static c_rest_error_t dummy_resolver(const char *field_name, char **out_json,
                                     size_t *out_len, void *user_data) {
  const char *res = "{\"id\": 1}";
  int *called = (int *)user_data;
  size_t len = strlen(res);
  int is_null = (strcmp(field_name, "null_json") == 0);
  int is_user = (strcmp(field_name, "user") == 0);
  c_rest_error_t rc;

  if (is_null) {
    *out_json = NULL;
    *out_len = 0;
    return C_REST_OK;
  }

  if (called) {
    *called += is_user;
  }

  rc = C_REST_MALLOC(len + 1, (void **)out_json);
  if (rc != C_REST_OK)
    return rc;

  memcpy(*out_json, res, len + 1);
  *out_len = len;
  return C_REST_OK;
}

TEST test_graphql_basic_parse(void) {
  const char *query = "query getUser { user { id name } }";
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_node *op;
  struct c_rest_graphql_node *field_user;
  c_rest_error_t rc;

  rc = c_rest_graphql_parse(query, strlen(query), &doc);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(doc == NULL);
  ASSERT_FALSE(doc->type != C_REST_GRAPHQL_NODE_DOCUMENT);
  ASSERT_FALSE(doc->definitions->count != 1);
  op = doc->definitions->nodes[0];
  ASSERT_FALSE(op->type != C_REST_GRAPHQL_NODE_OPERATION);
  ASSERT_FALSE(op->op_type != C_REST_GRAPHQL_OP_QUERY);
  ASSERT_FALSE(strcmp(op->name, "getUser") != 0);
  ASSERT_FALSE(op->selection_set->count != 1);
  field_user = op->selection_set->nodes[0];
  ASSERT_FALSE(field_user->type != C_REST_GRAPHQL_NODE_FIELD);
  ASSERT_FALSE(strcmp(field_user->name, "user") != 0);
  ASSERT_FALSE(field_user->selection_set->count != 2);

  ASSERT_FALSE(c_rest_graphql_node_free(doc) != C_REST_OK);
  PASS();
}

TEST test_graphql_parse_alias(void) {
  const char *query = "{ myUser: user { id } }";
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_node *op;
  struct c_rest_graphql_node *field_user;
  c_rest_error_t rc;

  rc = c_rest_graphql_parse(query, strlen(query), &doc);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(doc == NULL);
  op = doc->definitions->nodes[0];
  field_user = op->selection_set->nodes[0];
  ASSERT_FALSE(strcmp(field_user->name, "user") != 0);
  ASSERT_FALSE(strcmp(field_user->alias, "myUser") != 0);

  ASSERT_FALSE(c_rest_graphql_node_free(doc) != C_REST_OK);
  PASS();
}

TEST test_graphql_resolve_dummy(void) {
  const char *query = "{ user { id } }";
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  char *json = NULL;
  size_t len = 0;
  c_rest_error_t rc;
  int called = 0;

  rc = c_rest_graphql_schema_init(&schema);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(schema, "user", dummy_resolver,
                                          &called);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(schema, "null_json", dummy_resolver,
                                          NULL);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(schema, "no_called", dummy_resolver,
                                          NULL);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Test resolver with no called pointer */
  rc = c_rest_graphql_parse("{ no_called }", 13, &doc);
  ASSERT_FALSE(rc != C_REST_OK);
  rc = c_rest_graphql_resolve(doc, schema, &json, &len);
  ASSERT_FALSE(rc != C_REST_OK);
  C_REST_FREE(json);
  json = NULL;
  ASSERT_FALSE(c_rest_graphql_node_free(doc) != C_REST_OK);
  doc = NULL;

  rc = c_rest_graphql_parse(query, strlen(query), &doc);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_graphql_resolve(doc, schema, &json, &len);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(json == NULL);
  ASSERT_FALSE(len == 0);
  ASSERT_FALSE(strcmp(json, "{\"data\": {}}") != 0);
  ASSERT_FALSE(called != 1);

  C_REST_FREE(json);
  json = NULL;
  ASSERT_FALSE(c_rest_graphql_node_free(doc) != C_REST_OK);

  /* Test null json return */
  doc = NULL;
  json = NULL;
  rc = c_rest_graphql_parse("{ null_json }", 13, &doc);
  ASSERT_FALSE(rc != C_REST_OK);
  rc = c_rest_graphql_resolve(doc, schema, &json, &len);
  ASSERT_FALSE(rc != C_REST_OK);
  C_REST_FREE(json);
  json = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));

  /* Test missing fields manually constructed */
  {
    struct c_rest_graphql_node *manual_doc = NULL;
    struct c_rest_graphql_node *op = NULL;
    struct c_rest_graphql_node *field = NULL;

    ASSERT_FALSE(C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node),
                               (void **)&manual_doc) != C_REST_OK);
    manual_doc->type = C_REST_GRAPHQL_NODE_DOCUMENT;
    ASSERT_EQ(C_REST_OK,
              c_rest_graphql_resolve(manual_doc, schema, &json, &len));
    C_REST_FREE(json);
    json = NULL;

    ASSERT_FALSE(C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node_list),
                               (void **)&manual_doc->definitions) != C_REST_OK);
    ASSERT_FALSE(C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node *),
                               (void **)&manual_doc->definitions->nodes) !=
                 C_REST_OK);
    manual_doc->definitions->count = 1;

    ASSERT_FALSE(C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node),
                               (void **)&op) != C_REST_OK);
    manual_doc->definitions->nodes[0] = op;

    op->type = C_REST_GRAPHQL_NODE_FIELD; /* wrong type */
    ASSERT_EQ(C_REST_OK,
              c_rest_graphql_resolve(manual_doc, schema, &json, &len));
    C_REST_FREE(json);
    json = NULL;

    op->type = C_REST_GRAPHQL_NODE_OPERATION;
    ASSERT_EQ(C_REST_OK,
              c_rest_graphql_resolve(manual_doc, schema, &json, &len));
    C_REST_FREE(json);
    json = NULL;

    ASSERT_FALSE(C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node_list),
                               (void **)&op->selection_set) != C_REST_OK);
    ASSERT_FALSE(C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node *),
                               (void **)&op->selection_set->nodes) !=
                 C_REST_OK);
    op->selection_set->count = 1;

    ASSERT_FALSE(C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node),
                               (void **)&field) != C_REST_OK);
    op->selection_set->nodes[0] = field;

    field->type = C_REST_GRAPHQL_NODE_DOCUMENT; /* wrong type */
    ASSERT_EQ(C_REST_OK,
              c_rest_graphql_resolve(manual_doc, schema, &json, &len));
    C_REST_FREE(json);
    json = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(manual_doc));
  }

  ASSERT_FALSE(c_rest_graphql_schema_free(schema) != C_REST_OK);
  PASS();
}

TEST test_graphql_malformed(void) {
  const char *query = "# This is a comment\n{ user { id ";
  struct c_rest_graphql_node *doc = NULL;
  c_rest_error_t rc;

  rc = c_rest_graphql_parse(query, strlen(query), &doc);
  ASSERT_FALSE(rc == C_REST_OK);
  ASSERT_FALSE(c_rest_graphql_node_free(doc) != C_REST_OK);

  query = "query { }";
  doc = NULL;
  rc = c_rest_graphql_parse(query, strlen(query), &doc);
  ASSERT_FALSE(rc == C_REST_OK);
  ASSERT_FALSE(c_rest_graphql_node_free(doc) != C_REST_OK);

  PASS();
}

static c_rest_error_t fail_resolver_alloc(const char *field, char **out_json,
                                          size_t *out_len, void *user_data) {
  (void)field;
  (void)user_data;
  *out_json = NULL;
  if (C_REST_MALLOC(4, out_json) == C_REST_OK && *out_json) {
    (*out_json)[0] = '{';
    (*out_json)[1] = '}';
    (*out_json)[2] = '\0';
  }
  *out_len = 2;
  return C_REST_ERROR_GENERIC;
}

TEST test_graphql_router(void) {
  struct c_rest_router *router = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  c_rest_error_t ret;
  int called = 0;
  const char *query = "query { user { id } }";
  struct c_rest_openapi_operation op_meta;

  ret = c_rest_router_init(&router);
  ASSERT_FALSE(ret != C_REST_OK);

  ret = c_rest_graphql_schema_init(&schema);
  ASSERT_FALSE(ret != C_REST_OK);

  ret = c_rest_graphql_schema_add_resolver(schema, "user", dummy_resolver,
                                           &called);
  ASSERT_FALSE(ret != C_REST_OK);

  memset(&op_meta, 0, sizeof(op_meta));
  op_meta.summary = "GraphQL API";

  ret = c_rest_router_add_graphql_openapi(router, "/graphql", schema, &op_meta);
  ASSERT_FALSE(ret != C_REST_OK);

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = (char *)"POST";
  req.path = (char *)"/graphql";
  req.body = (char *)query;
  req.body_len = strlen(query);

  ret = c_rest_router_dispatch(router, &req, &res);
  ASSERT_FALSE(ret != C_REST_OK);
  ASSERT_FALSE(res.status_code != 200);
  ASSERT_FALSE(called != 1);
  ASSERT_FALSE(res.body == NULL);
  ASSERT_FALSE(res.body_len == 0);
  ASSERT_FALSE(c_rest_response_cleanup(&res) != C_REST_OK);

  /* Resolver failure path + node_free failure path + response_json OOM path */
  {
    struct c_rest_router *r2 = NULL;
    struct c_rest_graphql_schema *s2 = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&r2));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_schema_init(&s2));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_schema_add_resolver(
                             s2, "user", fail_resolver_alloc, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_router_add_graphql(r2, "/graphql", s2));

    memset(&res, 0, sizeof(res));
    ret = c_rest_router_dispatch(r2, &req, &res);
    ASSERT_FALSE(ret == C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

#ifdef C_REST_TESTING_MALLOC_HOOK
    g_mock_graphql_free_countdown = 1;
    memset(&res, 0, sizeof(res));
    ret = c_rest_router_dispatch(r2, &req, &res);
    ASSERT_FALSE(ret == C_REST_OK);
    g_mock_graphql_free_countdown = -1;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
#endif

    {
      int k;
      for (k = 0; k < 15; k++) {
        c_rest_error_t d_rc;
        memset(&res, 0, sizeof(res));
        g_crf_malloc_hook = fail_malloc_n;
        g_malloc_fail_after = k;
        d_rc = c_rest_router_dispatch(router, &req, &res);
        ASSERT(d_rc == C_REST_OK || d_rc == C_REST_ERROR_OOM ||
               d_rc == C_REST_ERROR_GENERIC);
        g_crf_malloc_hook = NULL;
        g_malloc_fail_after = -1;
        ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
      }
    }

    ASSERT_EQ(C_REST_OK, c_rest_graphql_schema_free(s2));
    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r2));
  }

  ASSERT_FALSE(c_rest_graphql_schema_free(schema) != C_REST_OK);
  ASSERT_FALSE(c_rest_router_destroy(router) != C_REST_OK);

  PASS();
}

TEST test_graphql_errors(void) {
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  char *json = NULL;
  size_t len = 0;
  c_rest_error_t rc;
  int i;

  /* no brackets valid start */
  {
    const char *q9 = "query abc";
    ASSERT_NEQ(C_REST_OK, c_rest_graphql_parse(q9, strlen(q9), &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;
  }

  {
    const char *q3 = "query test";
    ASSERT_NEQ(C_REST_OK, c_rest_graphql_parse(q3, strlen(q3), &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;

    /* schema null, definitions valid */
    ASSERT_EQ(C_REST_OK, c_rest_graphql_parse("query { a }", 11, &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_resolve(doc, NULL, &json, &len));
    C_REST_FREE(json);
    json = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_graphql_parse("query { a }", 11, &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_resolve(doc, NULL, &json, &len));
    C_REST_FREE(json);
    json = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;

    {
      struct c_rest_graphql_node fake_doc;
      memset(&fake_doc, 0, sizeof(fake_doc));
      ASSERT_NEQ(C_REST_OK,
                 c_rest_graphql_resolve(&fake_doc, schema, NULL, NULL));
    }
  }

  {
    const char *q4 = "query { a } # test";
    const char *q5 = "query { a } # test\r";
    const char *q6 = "query { a, }";
    const char *q7 = "query\t{\ta\t}";
    const char *q8 = "query _test { a }";

    ASSERT_EQ(C_REST_OK, c_rest_graphql_parse(q4, strlen(q4), &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_graphql_parse(q5, strlen(q5), &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_graphql_parse(q6, strlen(q6), &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_graphql_parse(q7, strlen(q7), &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_graphql_parse(q8, strlen(q8), &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;
  }

  {
    const char *q_mb = "query test X";
    ASSERT_NEQ(C_REST_OK, c_rest_graphql_parse(q_mb, strlen(q_mb), &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;
  }

  {
    ASSERT_EQ(C_REST_OK, c_rest_graphql_parse("query { a }", 11, &doc));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_resolve(doc, NULL, &json, &len));
    C_REST_FREE(json);
    json = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;
  }

  rc = c_rest_graphql_parse(NULL, 10, &doc);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_graphql_parse("query {}", 8, NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = i;
    rc = c_rest_graphql_parse("query getUser { user }", 22, &doc);
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;
  }

  /* Test list_append failure combined with node_free failure */
  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = i;
    g_mock_graphql_free_countdown = 0;
    rc = c_rest_graphql_parse("query getUser { user }", 22, &doc);
    g_mock_graphql_free_countdown = -1;
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;
  }

  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    rc = c_rest_graphql_schema_init(&schema);
    ASSERT_EQ(C_REST_OK, c_rest_graphql_schema_free(schema));
    schema = NULL;
  }

  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

  rc = c_rest_graphql_schema_init(&schema);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(NULL, "f", dummy_resolver, NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(schema, NULL, dummy_resolver, NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(schema, "f", NULL, NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    rc = c_rest_graphql_schema_add_resolver(schema, "f", dummy_resolver, NULL);
  }
  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

  doc = NULL;
  rc = c_rest_graphql_resolve(doc, schema, &json, &len);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_graphql_parse("{ user { id } }", 15, &doc);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_graphql_resolve(NULL, schema, &json, &len);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_graphql_resolve(doc, schema, NULL, &len);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_graphql_resolve(doc, schema, &json, NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  g_crf_malloc_hook = fail_malloc_n;
  g_malloc_fail_after = 0;
  rc = dummy_resolver("user", &json, &len, NULL);
  ASSERT_FALSE(rc == C_REST_OK);
  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = i;
    rc = c_rest_graphql_resolve(doc, schema, &json, &len);
    C_REST_FREE(json);
    json = NULL;
  }
  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

  ASSERT_FALSE(c_rest_graphql_node_free(doc) != C_REST_OK);
  ASSERT_FALSE(c_rest_graphql_schema_free(schema) != C_REST_OK);

  PASS();
}

TEST test_graphql_mock_run(void);

TEST test_graphql_mutations_and_edge_cases(void) {
  c_rest_error_t rc;
  {
    struct c_rest_graphql_node *manual_node = NULL;
    struct c_rest_graphql_node *op;
    struct c_rest_graphql_node *field;
    ASSERT_EQ(C_REST_OK, c_rest_graphql_parse("{ a }", 5, &manual_node));
    op = manual_node->definitions->nodes[0];
    field = op->selection_set->nodes[0];
    field->value = malloc(10);
#if defined(_MSC_VER)
    strcpy_s(field->value, 6, "dummy");
#else
    strcpy(field->value, "dummy");
#endif
    field->arguments = malloc(sizeof(struct c_rest_graphql_node_list));
    field->arguments->count = 1;
    field->arguments->capacity = 1;
    field->arguments->nodes = malloc(sizeof(struct c_rest_graphql_node *));
    ASSERT_EQ(C_REST_OK,
              c_rest_graphql_parse("{ b }", 5, &field->arguments->nodes[0]));
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(manual_node));
  }

  {
    struct c_rest_graphql_node *doc = NULL;
    const char *mut_query = "mutation createUser { user { id } }";
    const char *mut_anon = "mutation { user { id } }";
    rc = c_rest_graphql_parse(mut_anon, strlen(mut_anon), &doc);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;
    rc = c_rest_graphql_parse(mut_query, strlen(mut_query), &doc);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;
  }

  ASSERT_NEQ(C_REST_OK, c_rest_graphql_schema_init(NULL));
  ASSERT_EQ(C_REST_OK, c_rest_graphql_schema_free(NULL));

  {
    struct c_rest_graphql_node *doc = NULL;
    const char *long_query = "query { f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 }";
    const char *eof_ws = "query {  ";
    const char *missing_brace = "query  ";
    const char *missing_alias = "query { alias: }";

    rc = c_rest_graphql_parse(long_query, strlen(long_query), &doc);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;

    rc = c_rest_graphql_parse(eof_ws, strlen(eof_ws), &doc);
    ASSERT_FALSE(rc == C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;

    rc = c_rest_graphql_parse(missing_brace, strlen(missing_brace), &doc);
    ASSERT_FALSE(rc == C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;

    rc = c_rest_graphql_parse(missing_alias, strlen(missing_alias), &doc);
    ASSERT_FALSE(rc == C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
    doc = NULL;
  }
  PASS();
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

TEST test_graphql_mock_run(void) {
#ifdef C_REST_TESTING_MALLOC_HOOK
  struct c_rest_graphql_node *doc = NULL;

  /* parse_field error branches */
  g_mock_graphql_free_countdown = 0;
  ASSERT_NEQ(C_REST_OK, c_rest_graphql_parse("query { a ( }", 13, &doc));

  g_mock_graphql_free_countdown = 0;
  ASSERT_NEQ(C_REST_OK, c_rest_graphql_parse("query { a { }", 13, &doc));

  /* parse_operation error branches */
  g_mock_graphql_free_countdown = 1;
  ASSERT_EQ(C_REST_OK, c_rest_graphql_parse("query { a }", 11, &doc));
  ASSERT_NEQ(C_REST_OK, c_rest_graphql_node_free(doc));
  g_mock_graphql_free_countdown = -1;
  ASSERT_EQ(C_REST_OK, c_rest_graphql_node_free(doc));
  doc = NULL;

  g_mock_graphql_free_countdown = 0;
  ASSERT_NEQ(C_REST_OK, c_rest_graphql_parse("query { ", 8, &doc));

  g_mock_graphql_free_countdown = 0;
  ASSERT_NEQ(C_REST_OK, c_rest_graphql_parse("query 123", 9, &doc));

  {
    int i;
    for (i = 0; i < 20; i++) {
      g_mock_graphql_free_countdown = i;
      ASSERT_NEQ(C_REST_OK,
                 c_rest_graphql_parse(
                     "query { alias: a { b (arg: 1) { c } ! } }", 41, &doc));
    }
    g_mock_graphql_free_countdown = -1;
  }

  {
    int i;
    for (i = 0; i < 5; i++) {
      g_mock_graphql_free_countdown = i;
      ASSERT_NEQ(C_REST_OK,
                 c_rest_graphql_parse("query { alias: 123 }", 20, &doc));
    }
    g_mock_graphql_free_countdown = -1;
  }

  /* Line 328: fail alloc_list for doc->definitions */
  g_fail_malloc_at = 0;
  fail_malloc_graphql(0);
  g_crf_malloc_hook = fail_malloc_graphql;
  g_fail_malloc_at = 2;
  g_mock_graphql_free_countdown = 0;
  ASSERT_NEQ(C_REST_OK, c_rest_graphql_parse("query { a }", 11, &doc));
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
    ASSERT_NEQ(C_REST_OK, c_rest_graphql_node_free(n));
    g_mock_graphql_free_countdown = -1;

    n->arguments = NULL;
    sel_list->nodes = (struct c_rest_graphql_node **)calloc(
        1, sizeof(struct c_rest_graphql_node *));
    sel_list->nodes[0] = sel;
    sel_list->count = 1;
    n->selection_set = sel_list;

    g_mock_graphql_free_countdown = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_graphql_node_free(n));
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

SUITE(graphql_suite) {
  RUN_TEST(test_graphql_basic_parse);
  RUN_TEST(test_graphql_parse_alias);
  RUN_TEST(test_graphql_resolve_dummy);
  RUN_TEST(test_graphql_malformed);
  RUN_TEST(test_graphql_mutations_and_edge_cases);
  RUN_TEST(test_graphql_router);
  RUN_TEST(test_graphql_errors);
  RUN_TEST(test_graphql_mock_run);
}
