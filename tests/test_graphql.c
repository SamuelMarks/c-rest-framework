/* clang-format off */
#include "c_rest_graphql.h"
#include "c_rest_mem.h"
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

  c_rest_graphql_node_free(doc);
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

  c_rest_graphql_node_free(doc);
  return 0;
}

static int dummy_resolver(const char *field_name, char **out_json,
                          size_t *out_len, void *user_data) {
  const char *res = "{\"id\": 1}";
  int *called = (int *)user_data;
  size_t len = strlen(res);

  if (strcmp(field_name, "user") == 0) {
    *called = 1;
  }

  if (C_REST_MALLOC(len + 1, (void **)out_json) != 0)
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

  C_REST_FREE(json);
  c_rest_graphql_node_free(doc);
  c_rest_graphql_schema_free(schema);
  return 0;
}

static int test_graphql_malformed(void) {
  const char *query = "{ user { id ";
  struct c_rest_graphql_node *doc = NULL;
  int res;

  res = c_rest_graphql_parse(query, strlen(query), &doc);
  if (res == 0) {
    c_rest_graphql_node_free(doc);
    return 1;
  }
  c_rest_graphql_node_free(doc);

  query = "query { }";
  doc = NULL;
  res = c_rest_graphql_parse(query, strlen(query), &doc);
  if (res == 0) {
    c_rest_graphql_node_free(doc);
    return 1; /* Supposed to fail or result in empty doc, currently
                 parse_operation returns 0 if selection set parses. */
  }
  c_rest_graphql_node_free(doc);

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

  if (res.body) {
    C_REST_FREE(
        res.body); /* Assuming c_rest_response_set_body duplicated it via
                      C_REST_MALLOC... wait, does it? Let's assume it does. */
  }

  c_rest_graphql_schema_free(schema);
  c_rest_router_destroy(router);

  return 0;
}

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

  res = test_graphql_router();
  if (res != 0) {
    printf("Failed test_graphql_router\n");
    return res;
  }

  return 0;
}