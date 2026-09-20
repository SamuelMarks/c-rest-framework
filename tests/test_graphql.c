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

  if (is_null) {
    *out_json = NULL;
    *out_len = 0;
    return C_REST_OK;
  }

  if (called) {
    *called += is_user;
  }

  (void)!C_REST_MALLOC(len + 1, out_json);

  memcpy(*out_json, res, len + 1);
  *out_len = len;
  return C_REST_OK;
}

static int test_graphql_basic_parse(void) {
  int failed = 0;
  const char *query = "query getUser { user { id name } }";
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_node *op;
  struct c_rest_graphql_node *field_user;
  c_rest_error_t rc;

  rc = c_rest_graphql_parse(query, strlen(query), &doc);
  failed += (rc != C_REST_OK);
  failed += (doc == NULL);
  failed += (doc->type != C_REST_GRAPHQL_NODE_DOCUMENT);
  failed += (doc->definitions->count != 1);
  op = doc->definitions->nodes[0];
  failed += (op->type != C_REST_GRAPHQL_NODE_OPERATION);
  failed += (op->op_type != C_REST_GRAPHQL_OP_QUERY);
  failed += (strcmp(op->name, "getUser") != 0);
  failed += (op->selection_set->count != 1);
  field_user = op->selection_set->nodes[0];
  failed += (field_user->type != C_REST_GRAPHQL_NODE_FIELD);
  failed += (strcmp(field_user->name, "user") != 0);
  failed += (field_user->selection_set->count != 2);

  failed += (c_rest_graphql_node_free(doc) != C_REST_OK);
  return failed;
}

static int test_graphql_parse_alias(void) {
  int failed = 0;
  const char *query = "{ myUser: user { id } }";
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_node *op;
  struct c_rest_graphql_node *field_user;
  c_rest_error_t rc;

  rc = c_rest_graphql_parse(query, strlen(query), &doc);
  failed += (rc != C_REST_OK);
  failed += (doc == NULL);
  op = doc->definitions->nodes[0];
  field_user = op->selection_set->nodes[0];
  failed += (strcmp(field_user->name, "user") != 0);
  failed += (strcmp(field_user->alias, "myUser") != 0);

  failed += (c_rest_graphql_node_free(doc) != C_REST_OK);
  return failed;
}

static int test_graphql_resolve_dummy(void) {
  int failed = 0;
  const char *query = "{ user { id } }";
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  char *json = NULL;
  size_t len = 0;
  c_rest_error_t rc;
  int called = 0;

  rc = c_rest_graphql_schema_init(&schema);
  failed += (rc != C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(schema, "user", dummy_resolver,
                                          &called);
  failed += (rc != C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(schema, "null_json", dummy_resolver,
                                          NULL);
  failed += (rc != C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(schema, "no_called", dummy_resolver,
                                          NULL);
  failed += (rc != C_REST_OK);

  /* Test resolver with no called pointer */
  rc = c_rest_graphql_parse("{ no_called }", 13, &doc);
  failed += (rc != C_REST_OK);
  rc = c_rest_graphql_resolve(doc, schema, &json, &len);
  failed += (rc != C_REST_OK);
  C_REST_FREE(json);
  json = NULL;
  failed += (c_rest_graphql_node_free(doc) != C_REST_OK);
  doc = NULL;

  rc = c_rest_graphql_parse(query, strlen(query), &doc);
  failed += (rc != C_REST_OK);

  rc = c_rest_graphql_resolve(doc, schema, &json, &len);
  failed += (rc != C_REST_OK);
  failed += (json == NULL);
  failed += (len == 0);
  failed += (strcmp(json, "{\"data\": {}}") != 0);
  failed += (called != 1);

  C_REST_FREE(json);
  json = NULL;
  failed += (c_rest_graphql_node_free(doc) != C_REST_OK);

  /* Test null json return */
  doc = NULL;
  json = NULL;
  rc = c_rest_graphql_parse("{ null_json }", 13, &doc);
  failed += (rc != C_REST_OK);
  rc = c_rest_graphql_resolve(doc, schema, &json, &len);
  failed += (rc != C_REST_OK);
  C_REST_FREE(json);
  json = NULL;
  c_rest_graphql_node_free(doc);

  /* Test missing fields manually constructed */
  {
    struct c_rest_graphql_node *manual_doc = NULL;
    struct c_rest_graphql_node *op = NULL;
    struct c_rest_graphql_node *field = NULL;

    (void)C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node),
                        (void **)&manual_doc);
    manual_doc->type = C_REST_GRAPHQL_NODE_DOCUMENT;
    c_rest_graphql_resolve(manual_doc, schema, &json, &len);
    C_REST_FREE(json);
    json = NULL;

    (void)C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node_list),
                        (void **)&manual_doc->definitions);
    (void)C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node *),
                        (void **)&manual_doc->definitions->nodes);
    manual_doc->definitions->count = 1;

    (void)C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node), (void **)&op);
    manual_doc->definitions->nodes[0] = op;

    op->type = C_REST_GRAPHQL_NODE_FIELD; /* wrong type */
    c_rest_graphql_resolve(manual_doc, schema, &json, &len);
    C_REST_FREE(json);
    json = NULL;

    op->type = C_REST_GRAPHQL_NODE_OPERATION;
    c_rest_graphql_resolve(manual_doc, schema, &json, &len);
    C_REST_FREE(json);
    json = NULL;

    (void)C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node_list),
                        (void **)&op->selection_set);
    (void)C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node *),
                        (void **)&op->selection_set->nodes);
    op->selection_set->count = 1;

    (void)C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node), (void **)&field);
    op->selection_set->nodes[0] = field;

    field->type = C_REST_GRAPHQL_NODE_DOCUMENT; /* wrong type */
    c_rest_graphql_resolve(manual_doc, schema, &json, &len);
    C_REST_FREE(json);
    json = NULL;

    c_rest_graphql_node_free(manual_doc);
  }

  failed += (c_rest_graphql_schema_free(schema) != C_REST_OK);
  return failed;
}

static int test_graphql_malformed(void) {
  int failed = 0;
  const char *query = "# This is a comment\n{ user { id ";
  struct c_rest_graphql_node *doc = NULL;
  c_rest_error_t rc;

  rc = c_rest_graphql_parse(query, strlen(query), &doc);
  failed += (rc == C_REST_OK);
  failed += (c_rest_graphql_node_free(doc) != C_REST_OK);

  query = "query { }";
  doc = NULL;
  rc = c_rest_graphql_parse(query, strlen(query), &doc);
  failed += (rc == C_REST_OK);
  failed += (c_rest_graphql_node_free(doc) != C_REST_OK);

  return failed;
}

static int test_graphql_router(void) {
  int failed = 0;
  struct c_rest_router *router = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  c_rest_error_t ret;
  int called = 0;
  const char *query = "query { user { id } }";
  struct c_rest_openapi_operation op_meta;

  ret = c_rest_router_init(&router);
  failed += (ret != C_REST_OK);

  ret = c_rest_graphql_schema_init(&schema);
  failed += (ret != C_REST_OK);

  ret = c_rest_graphql_schema_add_resolver(schema, "user", dummy_resolver,
                                           &called);
  failed += (ret != C_REST_OK);

  memset(&op_meta, 0, sizeof(op_meta));
  op_meta.summary = "GraphQL API";

  ret = c_rest_router_add_graphql_openapi(router, "/graphql", schema, &op_meta);
  failed += (ret != C_REST_OK);

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = (char *)"POST";
  req.path = (char *)"/graphql";
  req.body = (char *)query;
  req.body_len = strlen(query);

  ret = c_rest_router_dispatch(router, &req, &res);
  failed += (ret != C_REST_OK);
  failed += (res.status_code != 200);
  failed += (called != 1);
  failed += (res.body == NULL);
  failed += (res.body_len == 0);

  failed += (c_rest_response_cleanup(&res) != C_REST_OK);
  failed += (c_rest_graphql_schema_free(schema) != C_REST_OK);
  failed += (c_rest_router_destroy(router) != C_REST_OK);

  return failed;
}

static int test_graphql_errors(void) {
  int failed = 0;
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  char *json = NULL;
  size_t len = 0;
  c_rest_error_t rc;
  int i;

  /* no brackets valid start */
  {
    const char *q9 = "query abc";
    c_rest_graphql_parse(q9, strlen(q9), &doc);
    c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  {
    const char *q3 = "query test";
    c_rest_graphql_parse(q3, strlen(q3), &doc);
    c_rest_graphql_node_free(doc);
    doc = NULL;

    /* schema null, definitions valid */
    c_rest_graphql_parse("query { a }", 11, &doc);
    c_rest_graphql_resolve(doc, NULL, &json, &len);
    CRF_FREE(json);
    json = NULL;
    c_rest_graphql_node_free(doc);
    doc = NULL;

    c_rest_graphql_parse("query { a }", 11, &doc);
    c_rest_graphql_resolve(doc, NULL, &json, &len);
    CRF_FREE(json);
    json = NULL;
    c_rest_graphql_node_free(doc);
    doc = NULL;

    {
      struct c_rest_graphql_node fake_doc;
      memset(&fake_doc, 0, sizeof(fake_doc));
      c_rest_graphql_resolve(&fake_doc, schema, NULL, NULL);
    }
  }

  {
    const char *q4 = "query { a } # test";
    const char *q5 = "query { a } # test\r";
    const char *q6 = "query { a, }";
    const char *q7 = "query\t{\ta\t}";
    const char *q8 = "query _test { a }";

    c_rest_graphql_parse(q4, strlen(q4), &doc);
    c_rest_graphql_node_free(doc);
    doc = NULL;

    c_rest_graphql_parse(q5, strlen(q5), &doc);
    c_rest_graphql_node_free(doc);
    doc = NULL;

    c_rest_graphql_parse(q6, strlen(q6), &doc);
    c_rest_graphql_node_free(doc);
    doc = NULL;

    c_rest_graphql_parse(q7, strlen(q7), &doc);
    c_rest_graphql_node_free(doc);
    doc = NULL;

    c_rest_graphql_parse(q8, strlen(q8), &doc);
    c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  {
    const char *q_mb = "query test X";
    c_rest_graphql_parse(q_mb, strlen(q_mb), &doc);
    c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  {
    c_rest_graphql_parse("query { a }", 11, &doc);
    c_rest_graphql_resolve(doc, NULL, &json, &len);
    CRF_FREE(json);
    json = NULL;
    c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  rc = c_rest_graphql_parse(NULL, 10, &doc);
  failed += (rc == C_REST_OK);

  rc = c_rest_graphql_parse("query {}", 8, NULL);
  failed += (rc == C_REST_OK);

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = i;
    rc = c_rest_graphql_parse("query getUser { user }", 22, &doc);
    c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    rc = c_rest_graphql_schema_init(&schema);
    c_rest_graphql_schema_free(schema);
    schema = NULL;
  }

  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

  rc = c_rest_graphql_schema_init(&schema);
  failed += (rc != C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(NULL, "f", dummy_resolver, NULL);
  failed += (rc == C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(schema, NULL, dummy_resolver, NULL);
  failed += (rc == C_REST_OK);

  rc = c_rest_graphql_schema_add_resolver(schema, "f", NULL, NULL);
  failed += (rc == C_REST_OK);

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    rc = c_rest_graphql_schema_add_resolver(schema, "f", dummy_resolver, NULL);
  }
  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

  doc = NULL;
  rc = c_rest_graphql_resolve(doc, schema, &json, &len);
  failed += (rc == C_REST_OK);

  rc = c_rest_graphql_parse("{ user { id } }", 15, &doc);
  failed += (rc != C_REST_OK);

  rc = c_rest_graphql_resolve(NULL, schema, &json, &len);
  failed += (rc == C_REST_OK);

  rc = c_rest_graphql_resolve(doc, schema, NULL, &len);
  failed += (rc == C_REST_OK);

  rc = c_rest_graphql_resolve(doc, schema, &json, NULL);
  failed += (rc == C_REST_OK);

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = i;
    rc = c_rest_graphql_resolve(doc, schema, &json, &len);
    C_REST_FREE(json);
    json = NULL;
  }
  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

  failed += (c_rest_graphql_node_free(doc) != C_REST_OK);
  failed += (c_rest_graphql_schema_free(schema) != C_REST_OK);

  return failed;
}

int test_graphql(void) {
  int failed = 0;
  c_rest_error_t rc;
  const char *msgs[2];
  printf("Running graphql tests...\n");

  failed += test_graphql_basic_parse();
  failed += test_graphql_parse_alias();
  failed += test_graphql_resolve_dummy();
  failed += test_graphql_malformed();

  {
    struct c_rest_graphql_node *manual_node = NULL;
    struct c_rest_graphql_node *op;
    struct c_rest_graphql_node *field;
    c_rest_graphql_parse("{ a }", 5, &manual_node);
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
    c_rest_graphql_parse("{ b }", 5, &field->arguments->nodes[0]);
    c_rest_graphql_node_free(manual_node);
  }

  {
    struct c_rest_graphql_node *doc = NULL;
    const char *mut_query = "mutation createUser { user { id } }";
    const char *mut_anon = "mutation { user { id } }";
    rc = c_rest_graphql_parse(mut_anon, strlen(mut_anon), &doc);
    failed += (rc != C_REST_OK);
    c_rest_graphql_node_free(doc);
    doc = NULL;
    rc = c_rest_graphql_parse(mut_query, strlen(mut_query), &doc);
    failed += (rc != C_REST_OK);
    c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  c_rest_graphql_schema_init(NULL);
  c_rest_graphql_schema_free(NULL);

  {
    struct c_rest_graphql_node *doc = NULL;
    const char *long_query = "query { f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 }";
    const char *eof_ws = "query {  ";
    const char *missing_brace = "query  ";
    const char *missing_alias = "query { alias: }";

    printf("test long_query\n");
    rc = c_rest_graphql_parse(long_query, strlen(long_query), &doc);
    failed += (rc != C_REST_OK);
    c_rest_graphql_node_free(doc);
    doc = NULL;

    printf("test eof_ws\n");
    rc = c_rest_graphql_parse(eof_ws, strlen(eof_ws), &doc);
    failed += (rc == C_REST_OK);
    c_rest_graphql_node_free(doc);
    doc = NULL;

    printf("test missing_brace\n");
    rc = c_rest_graphql_parse(missing_brace, strlen(missing_brace), &doc);
    failed += (rc == C_REST_OK);
    c_rest_graphql_node_free(doc);
    doc = NULL;

    printf("test missing_alias\n");
    rc = c_rest_graphql_parse(missing_alias, strlen(missing_alias), &doc);
    failed += (rc == C_REST_OK);
    c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  printf("Entering test_graphql_router\n");
  failed += test_graphql_router();
  failed += test_graphql_errors();

  msgs[0] = "test_graphql passed\n";
  msgs[1] = "test_graphql failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}
