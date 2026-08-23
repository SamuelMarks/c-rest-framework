/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_graphql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_graphql_basic_parse(void) {
  const char *query = "query getUser { user { id name } }";
  struct c_rest_graphql_node *doc = NULL;
  int res;

  res = c_rest_graphql_parse(query, strlen(query), &doc);
  if (res != 0)
    return 1;
  if (!doc)
    return 1;
  if (doc->type != C_REST_GRAPHQL_NODE_DOCUMENT)
    return 1;

  if (!doc->definitions)
    return 1;
  if (doc->definitions->count != 1)
    return 1;

  if (doc->definitions->count > 0) {
    struct c_rest_graphql_node *op = doc->definitions->nodes[0];
    if (op->type != C_REST_GRAPHQL_NODE_OPERATION)
      return 1;
    if (op->op_type != C_REST_GRAPHQL_OP_QUERY)
      return 1;
    if (strcmp(op->name, "getUser") != 0)
      return 1;

    if (!op->selection_set)
      return 1;
    if (op->selection_set->count != 1)
      return 1;

    if (op->selection_set->count > 0) {
      struct c_rest_graphql_node *field_user = op->selection_set->nodes[0];
      if (field_user->type != C_REST_GRAPHQL_NODE_FIELD)
        return 1;
      if (strcmp(field_user->name, "user") != 0)
        return 1;

      if (!field_user->selection_set)
        return 1;
      if (field_user->selection_set->count != 2)
        return 1;
    }
  }

  (void)!c_rest_graphql_node_free(doc);
  return 0;
}

static int test_graphql_parse_alias(void) {
  const char *query = "{ myUser: user { id } }";
  struct c_rest_graphql_node *doc = NULL;
  int res;

  res = c_rest_graphql_parse(query, strlen(query), &doc);
  if (res != 0)
    return 1;
  if (!doc)
    return 1;

  if (doc->definitions && doc->definitions->count > 0) {
    struct c_rest_graphql_node *op = doc->definitions->nodes[0];
    if (op->selection_set && op->selection_set->count > 0) {
      struct c_rest_graphql_node *field_user = op->selection_set->nodes[0];
      if (strcmp(field_user->name, "user") != 0)
        return 1;
      if (strcmp(field_user->alias, "myUser") != 0)
        return 1;
    }
  }

  (void)!c_rest_graphql_node_free(doc);
  return 0;
}

static c_rest_error_t dummy_resolver(const char *field_name, char **out_json,
                          size_t *out_len, void *user_data) {
  const char *res = "{\"id\": 1}";
  int *called = (int *)user_data;
  size_t len = strlen(res);

  if (strcmp(field_name, "null_json") == 0) {
    *out_json = NULL;
    *out_len = 0;
    return C_REST_OK;
  }

  if (called && strcmp(field_name, "user") == 0) {
    *called = 1;
  }

  if (C_REST_MALLOC(len + 1, out_json) != 0)
    return -1;

  memcpy(*out_json, res, len + 1);
  *out_len = len;
  return 0;
}

static int test_graphql_resolve_dummy(void) {
  const char *query = "{ user { id } }";
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  char *json = NULL;
  size_t len = 0;
  int res;
  int called = 0;

  res = c_rest_graphql_schema_init(&schema);
  if (res != 0)
    return 1;

  res = c_rest_graphql_schema_add_resolver(schema, "user", dummy_resolver,
                                           &called);
  if (res != 0)
    return 1;

  res = c_rest_graphql_schema_add_resolver(schema, "null_json", dummy_resolver,
                                           NULL);
  if (res != 0)
    return 1;

  res = c_rest_graphql_parse(query, strlen(query), &doc);
  if (res != 0)
    return 1;

  res = c_rest_graphql_resolve(doc, schema, &json, &len);
  if (res != 0)
    return 1;
  if (!json)
    return 1;
  if (len == 0)
    return 1;
  if (strcmp(json, "{\"data\": {}}") != 0)
    return 1;
  if (called != 1)
    return 1;

  (void)!C_REST_FREE(json);
  (void)!c_rest_graphql_node_free(doc);

  /* Test null json return */
  doc = NULL;
  json = NULL;
  res = c_rest_graphql_parse("{ null_json }", 13, &doc);
  res = c_rest_graphql_resolve(doc, schema, &json, &len);
  if (json) {
    C_REST_FREE(json);
  }
  c_rest_graphql_node_free(doc);

  /* Test missing fields manually constructed */
  {
    struct c_rest_graphql_node *manual_doc = NULL;
    struct c_rest_graphql_node *op = NULL;
    struct c_rest_graphql_node *field = NULL;

    C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node), (void**)&manual_doc);
    manual_doc->type = C_REST_GRAPHQL_NODE_DOCUMENT;
    c_rest_graphql_resolve(manual_doc, schema, &json, &len);
    if (json) C_REST_FREE(json);

    C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node_list), (void**)&manual_doc->definitions);
    C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node *), (void**)&manual_doc->definitions->nodes);
    manual_doc->definitions->count = 1;

    C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node), (void**)&op);
    manual_doc->definitions->nodes[0] = op;

    op->type = C_REST_GRAPHQL_NODE_FIELD; /* wrong type */
    c_rest_graphql_resolve(manual_doc, schema, &json, &len);
    if (json) C_REST_FREE(json);

    op->type = C_REST_GRAPHQL_NODE_OPERATION;
    c_rest_graphql_resolve(manual_doc, schema, &json, &len);
    if (json) C_REST_FREE(json);

    C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node_list), (void**)&op->selection_set);
    C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node *), (void**)&op->selection_set->nodes);
    op->selection_set->count = 1;

    C_REST_CALLOC(1, sizeof(struct c_rest_graphql_node), (void**)&field);
    op->selection_set->nodes[0] = field;

    field->type = C_REST_GRAPHQL_NODE_DOCUMENT; /* wrong type */
    c_rest_graphql_resolve(manual_doc, schema, &json, &len);
    if (json) C_REST_FREE(json);

    c_rest_graphql_node_free(manual_doc);
  }

  (void)!c_rest_graphql_schema_free(schema);
  return 0;
}

static int test_graphql_malformed(void) {
  const char *query = "# This is a comment\n{ user { id ";
  struct c_rest_graphql_node *doc = NULL;
  int res;

  res = c_rest_graphql_parse(query, strlen(query), &doc);
  if (res == 0) {
    (void)!c_rest_graphql_node_free(doc);
    return 1;
  }
  (void)!c_rest_graphql_node_free(doc);

  query = "query { }";
  doc = NULL;
  res = c_rest_graphql_parse(query, strlen(query), &doc);
  if (res == 0) {
    (void)!c_rest_graphql_node_free(doc);
    return 1; /* Supposed to fail or result in empty doc, currently
                 parse_operation returns 0 if selection set parses. */
  }
  (void)!c_rest_graphql_node_free(doc);

  return 0;
}

#include "c_rest_openapi.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
/* clang-format on */

static int test_graphql_router(void) {
  struct c_rest_router *router = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  int ret;
  int called = 0;
  const char *query = "query { user { id } }";

  struct c_rest_openapi_operation op_meta;

  ret = c_rest_router_init(&router);
  if (ret != 0)
    return 1;

  ret = c_rest_graphql_schema_init(&schema);
  if (ret != 0)
    return 1;

  ret = c_rest_graphql_schema_add_resolver(schema, "user", dummy_resolver,
                                           &called);
  if (ret != 0)
    return 1;

  memset(&op_meta, 0, sizeof(op_meta));
  op_meta.summary = "GraphQL API";

  ret = c_rest_router_add_graphql_openapi(router, "/graphql", schema, &op_meta);
  if (ret != 0)
    return 1;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = (char *)"POST";
  req.path = (char *)"/graphql";
  req.body = (char *)query;
  req.body_len = strlen(query);

  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0)
    return 1;

  if (res.status_code != 200)
    return 1;
  if (called != 1)
    return 1;
  if (!res.body || res.body_len == 0)
    return 1;

  (void)!c_rest_response_cleanup(&res);

  (void)!c_rest_graphql_schema_free(schema);
  (void)!c_rest_router_destroy(router);

  return 0;
}

static int test_graphql_errors(void);
static int test_graphql_errors(void);
int test_graphql(void) {
  int res = 0;
  printf("Running graphql tests...\n");

  res = test_graphql_basic_parse();
  if (res != 0) {
    printf("Failed test_graphql_basic_parse\n");
    return res;
  }

  res = test_graphql_parse_alias();
  if (res != 0) {
    printf("Failed test_graphql_parse_alias\n");
    return res;
  }

  res = test_graphql_resolve_dummy();
  if (res != 0) {
    printf("Failed test_graphql_resolve_dummy\n");
    return res;
  }

  res = test_graphql_malformed();
  if (res != 0) {
    printf("Failed test_graphql_malformed\n");
    return res;
  }

  {
    struct c_rest_graphql_node *manual_node = NULL;
    c_rest_graphql_parse("{ a }", 5, &manual_node);
    if (manual_node && manual_node->definitions &&
        manual_node->definitions->count > 0) {
      struct c_rest_graphql_node *op = manual_node->definitions->nodes[0];
      if (op->selection_set && op->selection_set->count > 0) {
        struct c_rest_graphql_node *field = op->selection_set->nodes[0];
        field->value = malloc(10);
        if (field->value)
          strcpy(field->value, "dummy");
        field->arguments = malloc(sizeof(struct c_rest_graphql_node_list));
        if (field->arguments) {
          field->arguments->count = 1;
          field->arguments->capacity = 1;
          field->arguments->nodes =
              malloc(sizeof(struct c_rest_graphql_node *));
          if (field->arguments->nodes) {
            c_rest_graphql_parse("{ b }", 5, &field->arguments->nodes[0]);
          } else {
            field->arguments->count = 0;
          }
        }
      }
    }
    c_rest_graphql_node_free(manual_node);
  }

  {
    struct c_rest_graphql_node *doc = NULL;
    const char *mut_query = "mutation createUser { user { id } }";
    const char *mut_anon = "mutation { user { id } }";
    if (c_rest_graphql_parse(mut_anon, strlen(mut_anon), &doc) == C_REST_OK)
      c_rest_graphql_node_free(doc);
    if (c_rest_graphql_parse(mut_query, strlen(mut_query), &doc) == C_REST_OK) {
      c_rest_graphql_node_free(doc);
    }
  }

  c_rest_graphql_schema_init(NULL);
  c_rest_graphql_schema_free(NULL);

  {
    struct c_rest_graphql_node *doc = NULL;
    const char *mut_query = "mutation createUser { user { id } }";
    const char *mut_anon = "mutation { user { id } }";
    if (c_rest_graphql_parse(mut_anon, strlen(mut_anon), &doc) == C_REST_OK)
      c_rest_graphql_node_free(doc);
    if (c_rest_graphql_parse(mut_query, strlen(mut_query), &doc) == C_REST_OK) {
      c_rest_graphql_node_free(doc);
    }
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
    if (c_rest_graphql_parse(long_query, strlen(long_query), &doc) ==
        C_REST_OK) {
      c_rest_graphql_node_free(doc);
    } else {
      return 1;
    }
    printf("test eof_ws\n");
    if (c_rest_graphql_parse(eof_ws, strlen(eof_ws), &doc) == C_REST_OK) {
      c_rest_graphql_node_free(doc);
      return 1;
    }
    printf("test missing_brace\n");
    if (c_rest_graphql_parse(missing_brace, strlen(missing_brace), &doc) ==
        C_REST_OK) {
      c_rest_graphql_node_free(doc);
      return 1;
    }
    printf("test missing_alias\n");
    if (c_rest_graphql_parse(missing_alias, strlen(missing_alias), &doc) ==
        C_REST_OK) {
      c_rest_graphql_node_free(doc);
      return 1;
    }
  }

  printf("Entering test_graphql_router\n");
  res = test_graphql_router();
  if (res != 0) {
    printf("Failed test_graphql_router\n");
    return res;
  }

  res = test_graphql_errors();
  if (res != 0) {
    printf("Failed test_graphql_errors\n");
    return res;
  }

  return 0;
}

static int g_malloc_fail_after = -1;
static void *fail_malloc_n(size_t size) {
  if (g_malloc_fail_after == 0) {
    return NULL;
  }
  if (g_malloc_fail_after > 0) {
    g_malloc_fail_after--;
  }
  return malloc(size);
}

static int test_graphql_errors(void) {
  struct c_rest_graphql_node *doc = NULL;
  struct c_rest_graphql_schema *schema = NULL;
  char *json = NULL;
  size_t len = 0;
  int res;
  int i;

  /* c_rest_graphql_parse errors */

  {
    /* no brackets valid start */
    const char *q9 = "query abc";
    c_rest_graphql_parse(q9, strlen(q9), &doc);
    if (doc)
      c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  {
    /* no brackets */
    const char *q3 = "query test";
    c_rest_graphql_parse(q3, strlen(q3), &doc);
    if (doc)
      c_rest_graphql_node_free(doc);
    doc = NULL;

    /* schema null, definitions valid */
    c_rest_graphql_parse("query { a }", 11, &doc);
    c_rest_graphql_resolve(doc, NULL, &json, &len);
    if (json)
      CRF_FREE(json);
    json = NULL;
    c_rest_graphql_node_free(doc);
    doc = NULL;
    c_rest_graphql_parse("query { a }", 11, &doc);
    c_rest_graphql_resolve(doc, NULL, &json, &len);
    if (json)
      CRF_FREE(json);
    json = NULL;
    c_rest_graphql_node_free(doc);
    doc = NULL;
    {
      /* schema valid, definitions null */
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
    if (doc)
      c_rest_graphql_node_free(doc);
    doc = NULL;

    c_rest_graphql_parse(q5, strlen(q5), &doc);
    if (doc)
      c_rest_graphql_node_free(doc);
    doc = NULL;

    c_rest_graphql_parse(q6, strlen(q6), &doc);
    if (doc)
      c_rest_graphql_node_free(doc);
    doc = NULL;

    c_rest_graphql_parse(q7, strlen(q7), &doc);
    if (doc)
      c_rest_graphql_node_free(doc);
    doc = NULL;

    c_rest_graphql_parse(q8, strlen(q8), &doc);
    if (doc)
      c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  {
    /* missing bracket */
    const char *q_mb = "query test X";
    c_rest_graphql_parse(q_mb, strlen(q_mb), &doc);
    if (doc)
      c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  {
    /* schema null */
    c_rest_graphql_parse("query { a }", 11, &doc);
    c_rest_graphql_resolve(doc, NULL, &json, &len);
    if (json)
      CRF_FREE(json);
    json = NULL;
    c_rest_graphql_node_free(doc);
    doc = NULL;
  }

  res = c_rest_graphql_parse(NULL, 10, &doc);
  if (res == C_REST_OK)
    return 1;
  res = c_rest_graphql_parse("query {}", 8, NULL);
  if (res == C_REST_OK)
    return 1;

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = i;
    res = c_rest_graphql_parse("query getUser { user }", 22, &doc);
    if (res == C_REST_OK) {
      c_rest_graphql_node_free(doc);
      doc = NULL;
      break;
    }
  }

  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    res = c_rest_graphql_schema_init(&schema);
    if (res == C_REST_OK) {
      c_rest_graphql_schema_free(schema);
      schema = NULL;
      break;
    }
  }

  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

  /* schema add resolver errors */
  res = c_rest_graphql_schema_init(&schema);
  if (res != C_REST_OK)
    return 1;

  res = c_rest_graphql_schema_add_resolver(NULL, "f", dummy_resolver, NULL);
  if (res == C_REST_OK)
    return 1;
  res = c_rest_graphql_schema_add_resolver(schema, NULL, dummy_resolver, NULL);
  if (res == C_REST_OK)
    return 1;
  res = c_rest_graphql_schema_add_resolver(schema, "f", NULL, NULL);
  if (res == C_REST_OK)
    return 1;

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 5; i++) {
    g_malloc_fail_after = i;
    res = c_rest_graphql_schema_add_resolver(schema, "f", dummy_resolver, NULL);
    if (res == C_REST_OK) {
      break;
    }
  }
  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

  doc = NULL;
  res = c_rest_graphql_resolve(doc, schema, &json, &len); /* doc is NULL here */
  if (res == C_REST_OK)
    return 1;

  g_crf_malloc_hook = NULL;

  res = c_rest_graphql_parse("{ user { id } }", 15, &doc);
  if (res != C_REST_OK)
    return 1;

  /* resolve errors */
  res = c_rest_graphql_resolve(NULL, schema, &json, &len);
  if (res == C_REST_OK)
    return 1;
  res = c_rest_graphql_resolve(doc, schema, NULL, &len);
  if (res == C_REST_OK)
    return 1;
  res = c_rest_graphql_resolve(doc, schema, &json, NULL);
  if (res == C_REST_OK)
    return 1;

  g_crf_malloc_hook = fail_malloc_n;
  for (i = 0; i < 20; i++) {
    g_malloc_fail_after = i;
    res = c_rest_graphql_resolve(doc, schema, &json, &len);
    if (res == C_REST_OK) {
      C_REST_FREE(json);
      break;
    }
  }
  g_crf_malloc_hook = NULL;
  g_malloc_fail_after = -1;

  (void)!c_rest_graphql_node_free(doc);
  (void)!c_rest_graphql_schema_free(schema);

  return 0;
}

/* Need to insert the call to test_graphql_errors inside test_graphql */
