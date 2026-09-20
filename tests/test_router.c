/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_template.h"
#include "c_rest_openapi.h"
#include "c_rest_middleware.h"
#include "c_rest_modality.h"
#include "c_rest_graphql.h"
#include "c_rest_testing_mocks.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* clang-format on */

struct test_route_node {
  char *segment;
  int is_var;
  int is_wildcard;
  char *var_name;
  struct test_route_node *parent;
  struct test_route_node *children;
  struct test_route_node *next;
  void *handlers;
};

struct test_router {
  struct test_route_node *root;
  void *middlewares;
  void *post_middlewares;
  void *openapi_spec;
};

static int handler_called = 0;
static int mw_called = 0;

static c_rest_error_t test_handler(struct c_rest_request *req,
                                   struct c_rest_response *res,
                                   void *user_data) {
  (void)req;
  (void)res;
  (void)user_data;
  handler_called = 1;
  return C_REST_OK;
}

static c_rest_error_t test_middleware(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  (void)req;
  (void)res;
  (void)user_data;
  mw_called = 1;
  return 0; /* Continue */
}

static int dummy_verify(const char *token, void **out_auth_context) {
  if (strcmp(token, "valid-token") == 0) {
    *out_auth_context = (void *)(size_t)0xDEADBEEF;
    return 0;
  }
  return 1;
}

static int test_oauth2_middleware_func(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header auth_hdr;
  c_rest_error_t ret;
  int failed = 0;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  printf("Testing OAuth2 Middleware...\n");

  /* Test 1: No auth header */
  ret = c_rest_oauth2_middleware(&req, &res, (void *)(size_t)dummy_verify);
  failed += (ret == 0);
  failed += (res.status_code != 401);
  failed += (c_rest_response_cleanup(&res) != C_REST_OK);

  /* Test 2: Invalid token */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  auth_hdr.key = "Authorization";
  auth_hdr.value = "Bearer invalid-token";
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  ret = c_rest_oauth2_middleware(&req, &res, (void *)(size_t)dummy_verify);
  failed += (ret == 0);
  failed += (res.status_code != 401);
  failed += (c_rest_response_cleanup(&res) != C_REST_OK);

  /* Test 3: Valid token */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  auth_hdr.key = "Authorization";
  auth_hdr.value = "Bearer valid-token";
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  ret = c_rest_oauth2_middleware(&req, &res, (void *)(size_t)dummy_verify);
  failed += (ret != 0);
  failed += (req.auth_context != (void *)(size_t)0xDEADBEEF);
  failed += (c_rest_response_cleanup(&res) != C_REST_OK);

  return failed;
}

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  extern int g_fail_malloc_at;
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

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
static c_rest_error_t dummy_tpl_fn(struct c_rest_request *req,
                                   const char ***keys, const char ***values,
                                   size_t *count, void *user_data) {
  (void)req;
  (void)keys;
  (void)values;
  (void)count;
  (void)user_data;
  return C_REST_OK;
}
static c_rest_error_t fail_tpl_fn(struct c_rest_request *req,
                                  const char ***keys, const char ***values,
                                  size_t *count, void *user_data) {
  (void)req;
  (void)keys;
  (void)values;
  (void)count;
  (void)user_data;
  return C_REST_ERROR_GENERIC;
}
#endif

static c_rest_error_t fail_handler(struct c_rest_request *req,
                                   struct c_rest_response *res, void *user) {
  (void)req;
  (void)res;
  (void)user;
  return C_REST_ERROR_GENERIC;
}
static c_rest_error_t fail_middleware(struct c_rest_request *req,
                                      struct c_rest_response *res, void *user) {
  (void)req;
  (void)res;
  (void)user;
  return C_REST_ERROR_GENERIC;
}
static c_rest_error_t dummy_ws_fn(struct c_rest_request *req,
                                  const unsigned char *msg, size_t len,
                                  int type, void *user) {
  (void)req;
  (void)msg;
  (void)len;
  (void)type;
  (void)user;
  return C_REST_OK;
}

static void test_coverage(void) {
  c_rest_router *r;
  int i;
  extern int g_fail_malloc_at;
  extern int g_fail_realloc_at;
  struct c_rest_openapi_spec *spec;
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
  struct c_rest_template_context dummy_ctx;
  c_rest_template_init(&dummy_ctx, "Hello");
#endif
  (void)g_fail_realloc_at;

  c_rest_router_init(NULL);
  c_rest_router_init(&r);
  c_rest_router_add(NULL, "GET", "/a", test_handler, NULL);
  c_rest_router_add(r, NULL, "/a", test_handler, NULL);
  c_rest_router_add(r, "GET", NULL, test_handler, NULL);
  c_rest_router_add(r, "GET", "/a", NULL, NULL);

  c_rest_router_use(NULL, "/a", test_middleware, NULL);
  c_rest_router_use(r, "/a", NULL, NULL);

  c_rest_router_dispatch(NULL, NULL, NULL);
  c_rest_router_dispatch(r, NULL, NULL);
  {
    struct c_rest_request req;
    c_rest_router_dispatch(r, &req, NULL);
  }
  {
    c_rest_router_add(r, "GET", "/var/:id", test_handler, NULL);
    c_rest_router_dispatch(r, NULL, NULL);
  }

  c_rest_router_add_openapi(NULL, "GET", "/a", test_handler, NULL, NULL);
  c_rest_router_add_openapi(r, "GET", "/a", test_handler, NULL, NULL);

  c_rest_router_use_post(NULL, "/a", test_middleware, NULL);
  c_rest_router_use_post(r, NULL, test_middleware, NULL);
  c_rest_router_use_post(r, "/a", NULL, NULL);

  c_rest_router_get_openapi_spec(NULL, NULL);
  c_rest_router_get_openapi_spec(r, NULL);
  c_rest_router_get_openapi_spec(r, &spec);

  c_rest_router_add_websocket(NULL, "/ws", NULL, NULL, NULL);
  c_rest_router_add_websocket(r, NULL, NULL, NULL, NULL);
  c_rest_router_add_websocket(r, "/ws", NULL, NULL, NULL);
  c_rest_router_add_websocket_openapi(NULL, "/ws", NULL, NULL, NULL, NULL);
  c_rest_router_add_websocket_openapi(r, "/ws", NULL, NULL, NULL, NULL);

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
  c_rest_router_add_sse(NULL, "/sse", NULL, NULL);
  c_rest_router_add_sse(r, NULL, NULL, NULL);
  c_rest_router_add_sse(r, "/sse", NULL, NULL);
  c_rest_router_add_sse_openapi(NULL, "/sse", NULL, NULL, NULL);
  c_rest_router_add_sse_openapi(r, "/sse", NULL, NULL, NULL);
#endif

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
  c_rest_router_add_graphql(NULL, "/gql", NULL);
  c_rest_router_add_graphql(r, NULL, NULL);
  c_rest_router_add_graphql(r, "/gql", NULL);
  c_rest_router_add_graphql(r, "/gql", (void *)1);
  c_rest_router_add_graphql_openapi(NULL, "/gql", NULL, NULL);
  c_rest_router_add_graphql_openapi(r, "/gql", (void *)1, NULL);
#endif

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
  c_rest_router_add_template(NULL, "GET", "/a", NULL, NULL, NULL);
  c_rest_router_add_template(r, "GET", "/a", NULL, NULL, NULL);
  c_rest_router_add_template(r, "GET", "/a", &dummy_ctx, NULL, NULL);
  c_rest_router_add_template(r, "GET", "/a", &dummy_ctx, dummy_tpl_fn, NULL);
  c_rest_router_add_template(r, "GET", NULL, &dummy_ctx, dummy_tpl_fn, NULL);
  c_rest_router_add_template_openapi(NULL, "GET", "/a", NULL, NULL, NULL, NULL);
  c_rest_router_add_template_openapi(r, "GET", "/a", &dummy_ctx, dummy_tpl_fn,
                                     NULL, NULL);
#endif
  c_rest_router_destroy(r);

  for (i = 1; i <= 60; i++) {
    struct c_rest_request req;
    struct c_rest_response res;
    c_rest_router *router = NULL;
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    if (c_rest_router_init(&router) == C_REST_OK) {
      c_rest_router_use(router, "/api", test_middleware, NULL);
      c_rest_router_use(router, NULL, test_middleware, NULL);
      c_rest_router_use(router, "/api2", test_middleware, NULL);

      c_rest_router_add(router, "GET", "/api/users/:id", test_handler, NULL);
      c_rest_router_add(router, "POST", "/api/data", test_handler, NULL);
      c_rest_router_add(router, "GET", "/*", test_handler, NULL);

      c_rest_router_use_post(router, "/api", test_middleware, NULL);
      c_rest_router_use_post(router, NULL, test_middleware, NULL);
      c_rest_router_use_post(router, "/api2", test_middleware, NULL);
      c_rest_router_add_openapi(router, "GET", "/api/openapi", test_handler,
                                NULL, NULL);
      c_rest_router_add_websocket(router, "/ws", dummy_ws_fn, NULL, NULL);
      c_rest_router_add_websocket_openapi(router, "/ws_api", dummy_ws_fn, NULL,
                                          NULL, NULL);
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
      c_rest_router_add_sse(router, "/sse", test_handler, NULL);
      c_rest_router_add_sse_openapi(router, "/sse_api", test_handler, NULL,
                                    NULL);
#endif
#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
      c_rest_router_add_graphql(router, "/gql", (void *)1);
      c_rest_router_add_graphql_openapi(router, "/gql_api", (void *)1, NULL);
#endif
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
      c_rest_router_add_template(router, "GET", "/tpl", &dummy_ctx,
                                 dummy_tpl_fn, NULL);
      c_rest_router_add_template_openapi(router, "GET", "/tpl_api", &dummy_ctx,
                                         dummy_tpl_fn, NULL, NULL);
#endif

      c_rest_router_add_websocket(router, "/ws", dummy_ws_fn, NULL, NULL);
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
      c_rest_router_add_sse(router, "/sse", test_handler, NULL);
#endif
#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
      c_rest_router_add_graphql(router, "/gql", (void *)1);
#endif
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
      c_rest_router_add_template(router, "GET", "/api/tpl", &dummy_ctx,
                                 dummy_tpl_fn, NULL);
#endif
      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      req.method = "GET";
      req.path = "/api/users/123";
      c_rest_router_dispatch(router, &req, &res);
      c_rest_request_cleanup(&req);

      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      req.method = "POST";
      req.path = "/api/users/123";
      c_rest_router_dispatch(router, &req, &res);
      c_rest_request_cleanup(&req);

      c_rest_router_destroy(router);
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
  }

  {
    struct c_rest_openapi_operation op_meta;
    struct c_rest_openapi_spec *spec2 = NULL;
    c_rest_router *r2 = NULL;

    memset(&op_meta, 0, sizeof(op_meta));
    c_rest_router_init(&r2);
    c_rest_router_get_openapi_spec(r2, &spec2);

    g_fail_malloc_at = 1;
    c_rest_router_add_openapi(r2, "GET", "/api/a", test_handler, NULL,
                              &op_meta);
    g_fail_malloc_at = 0;

    c_rest_router_add_websocket_openapi(r2, "/ws_api", dummy_ws_fn, NULL, NULL,
                                        &op_meta);
    g_fail_malloc_at = 1;
    c_rest_router_add_websocket_openapi(r2, "/ws_api2", dummy_ws_fn, NULL, NULL,
                                        &op_meta);
    g_fail_malloc_at = 0;

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
    c_rest_router_add_sse_openapi(r2, "/sse_api", test_handler, NULL, &op_meta);
    g_fail_malloc_at = 1;
    c_rest_router_add_sse_openapi(r2, "/sse_api2", test_handler, NULL,
                                  &op_meta);
    g_fail_malloc_at = 0;
#endif

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
    c_rest_router_add_template_openapi(r2, "GET", "/tpl_api", &dummy_ctx,
                                       dummy_tpl_fn, NULL, &op_meta);
    g_fail_malloc_at = 1;
    c_rest_router_add_template_openapi(r2, "GET", "/tpl_api2", &dummy_ctx,
                                       dummy_tpl_fn, NULL, &op_meta);
    g_fail_malloc_at = 0;
#endif
    dummy_ws_fn(NULL, NULL, 0, 0, NULL);
    c_rest_router_destroy(r2);
  }

  {
    c_rest_router *r3 = NULL;
    struct c_rest_request req3;
    struct c_rest_response res3;
    memset(&req3, 0, sizeof(req3));
    memset(&res3, 0, sizeof(res3));

    c_rest_router_init(&r3);
    req3.method = "GET";
    req3.path = "/test";
    c_rest_router_add(r3, "GET", "/test", fail_handler, NULL);
    c_rest_router_dispatch(r3, &req3, &res3);
    c_rest_router_destroy(r3);

    c_rest_router_init(&r3);
    req3.path = "/test2";
    c_rest_router_add(r3, "GET", "/test2", test_handler, NULL);
    c_rest_router_use(r3, "/test2", fail_middleware, NULL);
    c_rest_router_dispatch(r3, &req3, &res3);
    c_rest_router_destroy(r3);

    c_rest_router_init(&r3);
    req3.path = "/test3";
    c_rest_router_add(r3, "GET", "/test3", test_handler, NULL);
    c_rest_router_use_post(r3, "/test3", fail_middleware, NULL);
    c_rest_router_dispatch(r3, &req3, &res3);
    c_rest_router_destroy(r3);

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
    c_rest_router_init(&r3);
    req3.path = "/tpl";
    c_rest_router_add_template(r3, "GET", "/tpl", &dummy_ctx, dummy_tpl_fn,
                               NULL);
    c_rest_router_dispatch(r3, &req3, &res3);

    req3.path = "/tpl2";
    c_rest_router_add_template(r3, "GET", "/tpl2", &dummy_ctx, fail_tpl_fn,
                               NULL);
    c_rest_router_dispatch(r3, &req3, &res3);

    req3.path = "/tpl3";
    c_rest_router_add_template(r3, "GET", "/tpl3", &dummy_ctx, dummy_tpl_fn,
                               NULL);
    g_fail_malloc_at = 1;
    c_rest_router_dispatch(r3, &req3, &res3);
    g_fail_malloc_at = 0;

    /* template data_provider fails and status failure */
    req3.path = "/tpl2";
    g_mock_res_status_fail = 1;
    c_rest_router_dispatch(r3, &req3, &res3);
    g_mock_res_status_fail = 0;

    /* template render fails and status failure */
    req3.path = "/tpl3";
    g_fail_malloc_at = 1;
    g_mock_res_status_fail = 1;
    c_rest_router_dispatch(r3, &req3, &res3);
    g_fail_malloc_at = 0;
    g_mock_res_status_fail = 0;

    c_rest_router_destroy(r3);
#endif

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
    c_rest_router_init(&r3);
    req3.method = "POST";
    req3.path = "/gql";
    c_rest_router_add_graphql(r3, "/gql", (void *)1);
    req3.body = NULL;
    c_rest_router_dispatch(r3, &req3, &res3);

    /* body NULL and status fail */
    g_mock_res_status_fail = 1;
    c_rest_router_dispatch(r3, &req3, &res3);
    g_mock_res_status_fail = 0;

    req3.body = (void *)"{";
    req3.body_len = 1;
    c_rest_router_dispatch(r3, &req3, &res3);

    /* parse error and status fail */
    g_mock_res_status_fail = 1;
    c_rest_router_dispatch(r3, &req3, &res3);
    g_mock_res_status_fail = 0;

    c_rest_router_destroy(r3);

    /* GraphQL with NULL schema and status failure */
    {
      c_rest_router *r_gql = NULL;
      struct c_rest_request req_gql;
      struct c_rest_response res_gql;
      c_rest_router_init(&r_gql);
      c_rest_router_add_graphql(r_gql, "/gql_null", NULL);
      memset(&req_gql, 0, sizeof(req_gql));
      memset(&res_gql, 0, sizeof(res_gql));
      req_gql.method = "POST";
      req_gql.path = "/gql_null";
      req_gql.body = (void *)"{";
      req_gql.body_len = 1;
      g_mock_res_status_fail = 1;
      c_rest_router_dispatch(r_gql, &req_gql, &res_gql);
      g_mock_res_status_fail = 0;
      c_rest_router_destroy(r_gql);
    }

    /* GraphQL resolve success, set_status 200 fails and node_free fails */
    {
      c_rest_router *r_gql_ok = NULL;
      struct c_rest_graphql_schema schema_ok;
      memset(&schema_ok, 0, sizeof(schema_ok));
      c_rest_router_init(&r_gql_ok);
      c_rest_router_add_graphql(r_gql_ok, "/gql_ok", &schema_ok);
      req3.body = (void *)"query { a }";
      req3.body_len = strlen((const char *)req3.body);
      req3.path = "/gql_ok";
      g_mock_res_status_fail = 1;
      c_rest_router_dispatch(r_gql_ok, &req3, &res3);
      g_mock_res_status_fail = 0;

      g_mock_graphql_free_countdown = 0;
      c_rest_router_dispatch(r_gql_ok, &req3, &res3);
      g_mock_graphql_free_countdown = -1;

      c_rest_router_destroy(r_gql_ok);
    }
#endif

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
    {
      c_rest_router *r_sse = NULL;
      struct c_rest_request req_sse;
      struct c_rest_response res_sse;
      struct c_rest_connection_context fake_ctx;
      memset(&fake_ctx, 0, sizeof(fake_ctx));
      fake_ctx.sock = 123;
      c_rest_router_init(&r_sse);
      c_rest_router_add_sse(r_sse, "/events", NULL, NULL);
      memset(&req_sse, 0, sizeof(req_sse));
      memset(&res_sse, 0, sizeof(res_sse));
      res_sse.context = &fake_ctx;
      req_sse.method = "GET";
      req_sse.path = "/events";
      g_mock_socket_fail = 201;
      c_rest_router_dispatch(r_sse, &req_sse, &res_sse);
      g_mock_socket_fail = 0;
      c_rest_router_destroy(r_sse);
    }
#endif

    /* Test 404 with status failure */
    {
      c_rest_router *r404 = NULL;
      struct c_rest_request req_nf;
      struct c_rest_response res_nf;
      c_rest_router_init(&r404);
      memset(&req_nf, 0, sizeof(req_nf));
      memset(&res_nf, 0, sizeof(res_nf));
      req_nf.method = "GET";
      req_nf.path = "/notfound";
      g_mock_res_status_fail = 1;
      c_rest_router_dispatch(r404, &req_nf, &res_nf);
      g_mock_res_status_fail = 0;
      c_rest_router_destroy(r404);
    }

    /* Test handler returning error with status failure */
    {
      c_rest_router *rh = NULL;
      struct c_rest_request req_err;
      struct c_rest_response res_err;
      c_rest_router_init(&rh);
      c_rest_router_add(rh, "GET", "/err", fail_handler, NULL);
      memset(&req_err, 0, sizeof(req_err));
      memset(&res_err, 0, sizeof(res_err));
      req_err.method = "GET";
      req_err.path = "/err";
      g_mock_res_status_fail = 1;
      c_rest_router_dispatch(rh, &req_err, &res_err);
      g_mock_res_status_fail = 0;
      c_rest_router_destroy(rh);
    }

    /* Test free_node and destroy mock branches */
    {
      c_rest_router *r_mock = NULL;
      struct test_router *tr;

      /* test free_node(NULL) */
      c_rest_router_init(&r_mock);
      tr = (struct test_router *)r_mock;
      free(tr->root->segment);
      free(tr->root);
      tr->root = NULL;
      c_rest_router_destroy(r_mock);

      /* test router->openapi_spec fail */
      c_rest_router_init(&r_mock);
      tr = (struct test_router *)r_mock;
      c_rest_openapi_spec_destroy(
          (struct c_rest_openapi_spec *)tr->openapi_spec);
      tr->openapi_spec = (void *)0x1234;
      c_rest_router_destroy(r_mock);

      /* test router->openapi_spec is NULL */
      c_rest_router_init(&r_mock);
      tr = (struct test_router *)r_mock;
      c_rest_openapi_spec_destroy(
          (struct c_rest_openapi_spec *)tr->openapi_spec);
      tr->openapi_spec = NULL;
      c_rest_router_destroy(r_mock);

      /* test node->children fail */
      c_rest_router_init(&r_mock);
      tr = (struct test_router *)r_mock;
      tr->root->children = (struct test_route_node *)0x1234;
      c_rest_router_destroy(r_mock);

      /* test node->next fail */
      c_rest_router_init(&r_mock);
      tr = (struct test_router *)r_mock;
      tr->root->next = (struct test_route_node *)0x1234;
      c_rest_router_destroy(r_mock);

      /* test router->root fail */
      c_rest_router_init(&r_mock);
      tr = (struct test_router *)r_mock;
      free(tr->root->segment);
      free(tr->root);
      tr->root = (struct test_route_node *)0x1234;
      c_rest_router_destroy(r_mock);
    }
  }
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
  c_rest_template_destroy(&dummy_ctx);
#endif
}

int test_router(void) {
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  c_rest_error_t ret;
  int failed = 0;
  test_coverage();

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  printf("Running router tests...\n");

  ret = c_rest_router_init(&router);
  failed += (ret != 0);

  ret = c_rest_router_use(router, "/api", test_middleware, NULL);
  failed += (ret != 0);

  ret = c_rest_router_add(router, "GET", "/api/users/:id", test_handler, NULL);
  failed += (ret != 0);

  /* Test dispatch to matching route */
  req.method = "GET";
  req.path = "/api/users/123";
  res.status_code = 200;

  {
    extern int g_fail_malloc_at;
    int mf_idx;
    for (mf_idx = 1; mf_idx <= 5; mf_idx++) {
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = mf_idx;
      c_rest_router_dispatch(router, &req, &res);
      c_rest_request_cleanup(&req);
      req.path_vars = NULL;
    }
    g_fail_malloc_at = 0;
    g_crf_malloc_hook = NULL;
  }

  ret = c_rest_router_dispatch(router, &req, &res);
  failed += (ret != 0);
  failed += (mw_called == 0);
  failed += (handler_called == 0);

  failed += (c_rest_request_cleanup(&req) != C_REST_OK);
  failed += (c_rest_response_cleanup(&res) != C_REST_OK);

  /* Test 404 */
  req.path = "/api/unknown";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  failed += (ret != 0);
  failed += (res.status_code != 404);

  failed += (c_rest_request_cleanup(&req) != C_REST_OK);
  failed += (c_rest_response_cleanup(&res) != C_REST_OK);

  /* Test 405 */
  req.path = "/api/users/123";
  req.method = "POST";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  failed += (ret != 0);
  failed += (res.status_code != 405);

  failed += (c_rest_request_cleanup(&req) != C_REST_OK);
  failed += (c_rest_response_cleanup(&res) != C_REST_OK);
  /* Test wildcard */
  c_rest_router_add(router, "GET", "/*", test_handler, NULL);
  req.method = "GET";
  req.path = "/something/else";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  failed += (ret != 0);
  failed += (c_rest_request_cleanup(&req) != C_REST_OK);
  failed += (c_rest_response_cleanup(&res) != C_REST_OK);

  /* Test path variations */
  c_rest_router_add(router, "GET", "api/no-slash", test_handler, NULL);
  c_rest_router_add(router, "GET", "//double-slash", test_handler, NULL);
  c_rest_router_add(router, "GET", "/", test_handler, NULL);

  /* Test SSE wrapper */
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
  c_rest_router_add_sse(router, "/sse", test_handler, NULL);
  req.path = "/sse";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  failed += (ret != C_REST_OK);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Test SSE wrapper with null handler */
  c_rest_router_add_sse(router, "/sse_null", NULL, NULL);
  req.path = "/sse_null";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  failed += (ret != C_REST_OK);
  c_rest_response_cleanup(&res);
#endif

  failed += (c_rest_router_destroy(NULL) == C_REST_OK);
  failed += (c_rest_router_destroy(router) != C_REST_OK);

  failed += (test_oauth2_middleware_func() != 0);

  {
    const char *msgs[2];
    msgs[0] = "test_router passed\n";
    msgs[1] = "test_router failed\n";
    printf("%s", msgs[failed != 0]);
  }

  return failed;
}
