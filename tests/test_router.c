/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
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
static int g_router_fail_malloc_at = 0;

static c_rest_error_t short_circuit_cb(struct c_rest_request *req,
                                       struct c_rest_response *res,
                                       void *user_data) {
  (void)req;
  (void)user_data;
  res->status_code = 204;
  return C_REST_SHORT_CIRCUIT;
}

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
  return C_REST_OK;
}

static int dummy_verify(const char *token, void **out_auth_context) {
  if (strcmp(token, "valid-token") == 0) {
    *out_auth_context = (void *)(size_t)0xDEADBEEF;
    return 0;
  }
  return 1;
}

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  if (g_router_fail_malloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_router_fail_malloc_at) {
    alloc_count = 0;
    g_router_fail_malloc_at = 0;
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

TEST test_router_oauth2_middleware(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header auth_hdr;
  char auth_key[] = "Authorization";
  char invalid_val[] = "Bearer invalid-token";
  char valid_val[] = "Bearer valid-token";
  union {
    void *ptr;
    int (*fn)(const char *, void **);
  } verify_u;

  verify_u.fn = dummy_verify;
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* Test 1: No auth header */
  ASSERT_NEQ(C_REST_OK, c_rest_oauth2_middleware(&req, &res, verify_u.ptr));
  ASSERT_EQ(401, res.status_code);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  /* Test 2: Invalid token */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  auth_hdr.key = auth_key;
  auth_hdr.value = invalid_val;
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  ASSERT_NEQ(C_REST_OK, c_rest_oauth2_middleware(&req, &res, verify_u.ptr));
  ASSERT_EQ(401, res.status_code);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  /* Test 3: Valid token */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  auth_hdr.key = auth_key;
  auth_hdr.value = valid_val;
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  ASSERT_EQ(C_REST_OK, c_rest_oauth2_middleware(&req, &res, verify_u.ptr));
  ASSERT_EQ((void *)(size_t)0xDEADBEEF, req.auth_context);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  PASS();
}

TEST test_router_null_and_oom_branches(void) {
  c_rest_router *r = NULL;
  int i;
  c_rest_error_t rc;
  struct c_rest_openapi_spec *spec = NULL;
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
  struct c_rest_template_context dummy_ctx;
  ASSERT_EQ(C_REST_OK, c_rest_template_init(&dummy_ctx, "Hello"));
#endif

  ASSERT_NEQ(C_REST_OK, c_rest_router_init(NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_init(&r));
  ASSERT_NEQ(C_REST_OK,
             c_rest_router_add(NULL, "GET", "/a", test_handler, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add(r, NULL, "/a", test_handler, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add(r, "GET", NULL, test_handler, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add(r, "GET", "/a", NULL, NULL));

  ASSERT_NEQ(C_REST_OK, c_rest_router_use(NULL, "/a", test_middleware, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_use(r, "/a", NULL, NULL));

  ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(NULL, NULL, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r, NULL, NULL));
  {
    struct c_rest_request req;
    memset(&req, 0, sizeof(req));
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r, &req, NULL));
  }
  {
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add(r, "GET", "/var/:id", test_handler, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r, NULL, NULL));
  }

  ASSERT_NEQ(C_REST_OK, c_rest_router_add_openapi(NULL, "GET", "/a",
                                                  test_handler, NULL, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(r, "GET", "/a", test_handler,
                                                 NULL, NULL));

  ASSERT_NEQ(C_REST_OK,
             c_rest_router_use_post(NULL, "/a", test_middleware, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_use_post(r, NULL, test_middleware, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_use_post(r, "/a", NULL, NULL));

  ASSERT_NEQ(C_REST_OK, c_rest_router_get_openapi_spec(NULL, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_get_openapi_spec(r, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_get_openapi_spec(r, &spec));
  g_mock_openapi_fail = 7;
  ASSERT_NEQ(C_REST_OK, c_rest_router_get_openapi_spec(r, &spec));
  g_mock_openapi_fail = 0;

  ASSERT_NEQ(C_REST_OK,
             c_rest_router_add_websocket(NULL, "/ws", NULL, NULL, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add_websocket(r, NULL, NULL, NULL, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_add_websocket(r, "/ws", NULL, NULL, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add_websocket_openapi(NULL, "/ws", NULL,
                                                            NULL, NULL, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_add_websocket_openapi(r, "/ws", NULL, NULL,
                                                           NULL, NULL));

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
  ASSERT_NEQ(C_REST_OK, c_rest_router_add_sse(NULL, "/sse", NULL, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add_sse(r, NULL, NULL, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_add_sse(r, "/sse", NULL, NULL));
  ASSERT_NEQ(C_REST_OK,
             c_rest_router_add_sse_openapi(NULL, "/sse", NULL, NULL, NULL));
  ASSERT_EQ(C_REST_OK,
            c_rest_router_add_sse_openapi(r, "/sse", NULL, NULL, NULL));
#endif

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
  ASSERT_NEQ(C_REST_OK, c_rest_router_add_graphql(NULL, "/gql", NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add_graphql(r, NULL, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add_graphql(r, "/gql", NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_add_graphql(
                           r, "/gql", (struct c_rest_graphql_schema *)1));
  ASSERT_NEQ(C_REST_OK,
             c_rest_router_add_graphql_openapi(NULL, "/gql", NULL, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_add_graphql_openapi(
                           r, "/gql", (struct c_rest_graphql_schema *)1, NULL));
#endif

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
  ASSERT_NEQ(C_REST_OK,
             c_rest_router_add_template(NULL, "GET", "/a", NULL, NULL, NULL));
  ASSERT_NEQ(C_REST_OK,
             c_rest_router_add_template(r, "GET", "/a", NULL, NULL, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add_template(r, "GET", "/a", &dummy_ctx,
                                                   NULL, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_add_template(r, "GET", "/a", &dummy_ctx,
                                                  dummy_tpl_fn, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add_template(r, "GET", NULL, &dummy_ctx,
                                                   dummy_tpl_fn, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_router_add_template_openapi(
                            NULL, "GET", "/a", NULL, NULL, NULL, NULL));
  ASSERT_EQ(C_REST_OK,
            c_rest_router_add_template_openapi(r, "GET", "/a", &dummy_ctx,
                                               dummy_tpl_fn, NULL, NULL));
#endif
  ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r));

  for (i = 1; i <= 60; i++) {
    struct c_rest_request req;
    struct c_rest_response res;
    c_rest_router *router = NULL;
    g_router_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_router_fail_malloc_at = i;

    if (c_rest_router_init(&router) == C_REST_OK) {
      rc = c_rest_router_use(router, "/api", test_middleware, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_use(router, NULL, test_middleware, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_use(router, "/api2", test_middleware, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);

      rc = c_rest_router_add(router, "GET", "/api/users/:id", test_handler,
                             NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_add(router, "POST", "/api/data", test_handler, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_add(router, "GET", "/*", test_handler, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);

      rc = c_rest_router_use_post(router, "/api", test_middleware, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_use_post(router, NULL, test_middleware, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_use_post(router, "/api2", test_middleware, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_add_openapi(router, "GET", "/api/openapi",
                                     test_handler, NULL, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_add_websocket(router, "/ws", dummy_ws_fn, NULL, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_add_websocket_openapi(router, "/ws_api", dummy_ws_fn,
                                               NULL, NULL, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
      rc = c_rest_router_add_sse(router, "/sse", test_handler, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_add_sse_openapi(router, "/sse_api", test_handler, NULL,
                                         NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
#endif
#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
      rc = c_rest_router_add_graphql(router, "/gql",
                                     (struct c_rest_graphql_schema *)1);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_add_graphql_openapi(
          router, "/gql_api", (struct c_rest_graphql_schema *)1, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
#endif
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
      rc = c_rest_router_add_template(router, "GET", "/tpl", &dummy_ctx,
                                      dummy_tpl_fn, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_router_add_template_openapi(
          router, "GET", "/tpl_api", &dummy_ctx, dummy_tpl_fn, NULL, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
#endif

      rc = c_rest_router_add_websocket(router, "/ws", dummy_ws_fn, NULL, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
      rc = c_rest_router_add_sse(router, "/sse", test_handler, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
#endif
#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
      rc = c_rest_router_add_graphql(router, "/gql",
                                     (struct c_rest_graphql_schema *)1);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
#endif
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
      rc = c_rest_router_add_template(router, "GET", "/api/tpl", &dummy_ctx,
                                      dummy_tpl_fn, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
#endif
      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      req.method = "GET";
      req.path = "/api/users/123";
      rc = c_rest_router_dispatch(router, &req, &res);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      req.method = "POST";
      req.path = "/api/users/123";
      rc = c_rest_router_dispatch(router, &req, &res);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

      ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router));
    }

    g_crf_malloc_hook = NULL;
    g_router_fail_malloc_at = 0;
  }

  {
    struct c_rest_openapi_operation op_meta;
    struct c_rest_openapi_spec *spec2 = NULL;
    c_rest_router *r2 = NULL;

    memset(&op_meta, 0, sizeof(op_meta));
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&r2));
    ASSERT_EQ(C_REST_OK, c_rest_router_get_openapi_spec(r2, &spec2));

    g_router_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_router_fail_malloc_at = 1;
    ASSERT_NEQ(C_REST_OK,
               c_rest_router_add_openapi(r2, "GET", "/api/a", test_handler,
                                         NULL, &op_meta));
    g_crf_malloc_hook = NULL;
    g_router_fail_malloc_at = 0;

    ASSERT_EQ(C_REST_OK, c_rest_router_add_websocket_openapi(
                             r2, "/ws_api", dummy_ws_fn, NULL, NULL, &op_meta));
    g_router_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_router_fail_malloc_at = 1;
    ASSERT_NEQ(C_REST_OK,
               c_rest_router_add_websocket_openapi(r2, "/ws_api2", dummy_ws_fn,
                                                   NULL, NULL, &op_meta));
    g_crf_malloc_hook = NULL;
    g_router_fail_malloc_at = 0;

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
    ASSERT_EQ(C_REST_OK, c_rest_router_add_sse_openapi(
                             r2, "/sse_api", test_handler, NULL, &op_meta));
    g_router_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_router_fail_malloc_at = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_router_add_sse_openapi(
                              r2, "/sse_api2", test_handler, NULL, &op_meta));
    g_crf_malloc_hook = NULL;
    g_router_fail_malloc_at = 0;
#endif

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
    ASSERT_EQ(C_REST_OK, c_rest_router_add_template_openapi(
                             r2, "GET", "/tpl_api", &dummy_ctx, dummy_tpl_fn,
                             NULL, &op_meta));
    g_router_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_router_fail_malloc_at = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_router_add_template_openapi(
                              r2, "GET", "/tpl_api2", &dummy_ctx, dummy_tpl_fn,
                              NULL, &op_meta));
    g_crf_malloc_hook = NULL;
    g_router_fail_malloc_at = 0;
#endif
    ASSERT_EQ(C_REST_OK, dummy_ws_fn(NULL, NULL, 0, 0, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r2));
  }

  {
    c_rest_router *r3 = NULL;
    struct c_rest_request req3;
    struct c_rest_response res3;
    memset(&req3, 0, sizeof(req3));
    memset(&res3, 0, sizeof(res3));

    ASSERT_EQ(C_REST_OK, c_rest_router_init(&r3));
    req3.method = "GET";
    req3.path = "/test";
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add(r3, "GET", "/test", fail_handler, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r3));

    memset(&res3, 0, sizeof(res3));
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&r3));
    req3.path = "/test2";
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add(r3, "GET", "/test2", test_handler, NULL));
    ASSERT_EQ(C_REST_OK,
              c_rest_router_use(r3, "/test2", fail_middleware, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r3));

    memset(&res3, 0, sizeof(res3));
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&r3));
    req3.path = "/test3";
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add(r3, "GET", "/test3", test_handler, NULL));
    ASSERT_EQ(C_REST_OK,
              c_rest_router_use_post(r3, "/test3", fail_middleware, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r3));

    /* Short-circuit in pre-middleware skips handler, runs post-middleware,
     * returns OK */
    memset(&res3, 0, sizeof(res3));
    handler_called = 0;
    mw_called = 0;
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&r3));
    req3.path = "/sc_pre";
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add(r3, "GET", "/sc_pre", test_handler, NULL));
    ASSERT_EQ(C_REST_OK,
              c_rest_router_use(r3, "/sc_pre", short_circuit_cb, NULL));
    ASSERT_EQ(C_REST_OK,
              c_rest_router_use_post(r3, "/sc_pre", test_middleware, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
    ASSERT_EQ(0, handler_called);
    ASSERT_EQ(1, mw_called);
    ASSERT_EQ(204, res3.status_code);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r3));

    /* Short-circuit in route handler and post-middleware */
    memset(&res3, 0, sizeof(res3));
    mw_called = 0;
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&r3));
    req3.path = "/sc_hdl";
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add(r3, "GET", "/sc_hdl", short_circuit_cb, NULL));
    ASSERT_EQ(C_REST_OK,
              c_rest_router_use_post(r3, "/sc_hdl", short_circuit_cb, NULL));
    ASSERT_EQ(C_REST_OK,
              c_rest_router_use_post(r3, "/sc_hdl", test_middleware, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
    ASSERT_EQ(0, mw_called);
    ASSERT_EQ(204, res3.status_code);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r3));

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
    memset(&res3, 0, sizeof(res3));
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&r3));
    req3.path = "/tpl";
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add_template(r3, "GET", "/tpl", &dummy_ctx,
                                         dummy_tpl_fn, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    memset(&res3, 0, sizeof(res3));

    req3.path = "/tpl2";
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add_template(r3, "GET", "/tpl2", &dummy_ctx,
                                         fail_tpl_fn, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
    ASSERT_EQ(500, res3.status_code);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    memset(&res3, 0, sizeof(res3));

    req3.path = "/tpl3";
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add_template(r3, "GET", "/tpl3", &dummy_ctx,
                                         dummy_tpl_fn, NULL));
    g_router_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_router_fail_malloc_at = 1;
    rc = c_rest_router_dispatch(r3, &req3, &res3);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    g_crf_malloc_hook = NULL;
    g_router_fail_malloc_at = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    memset(&res3, 0, sizeof(res3));

    /* template data_provider fails and status failure */
    req3.path = "/tpl2";
    g_mock_res_status_fail = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
    g_mock_res_status_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    memset(&res3, 0, sizeof(res3));

    /* template render fails and status failure */
    req3.path = "/tpl3";
    g_router_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_router_fail_malloc_at = 1;
    g_mock_res_status_fail = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
    g_crf_malloc_hook = NULL;
    g_router_fail_malloc_at = 0;
    g_mock_res_status_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    memset(&res3, 0, sizeof(res3));

    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r3));
#endif

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
    {
      char bad_gql[] = "{";
      char ok_gql[] = "query { a }";
      memset(&res3, 0, sizeof(res3));
      ASSERT_EQ(C_REST_OK, c_rest_router_init(&r3));
      req3.method = "POST";
      req3.path = "/gql";
      ASSERT_EQ(C_REST_OK, c_rest_router_add_graphql(
                               r3, "/gql", (struct c_rest_graphql_schema *)1));
      req3.body = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
      ASSERT_EQ(400, res3.status_code);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
      memset(&res3, 0, sizeof(res3));

      /* body NULL and status fail */
      g_mock_res_status_fail = 1;
      ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
      g_mock_res_status_fail = 0;

      req3.body = bad_gql;
      req3.body_len = 1;
      ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
      ASSERT_EQ(400, res3.status_code);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
      memset(&res3, 0, sizeof(res3));

      /* parse error and status fail */
      g_mock_res_status_fail = 1;
      ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r3, &req3, &res3));
      g_mock_res_status_fail = 0;

      ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r3));

      /* GraphQL with NULL schema and status failure */
      {
        c_rest_router *r_gql = NULL;
        struct c_rest_request req_gql;
        struct c_rest_response res_gql;
        ASSERT_EQ(C_REST_OK, c_rest_router_init(&r_gql));
        ASSERT_NEQ(C_REST_OK,
                   c_rest_router_add_graphql(r_gql, "/gql_null", NULL));
        memset(&req_gql, 0, sizeof(req_gql));
        memset(&res_gql, 0, sizeof(res_gql));
        req_gql.method = "POST";
        req_gql.path = "/gql_null";
        req_gql.body = bad_gql;
        req_gql.body_len = 1;
        g_mock_res_status_fail = 1;
        ASSERT_NEQ(C_REST_OK,
                   c_rest_router_dispatch(r_gql, &req_gql, &res_gql));
        g_mock_res_status_fail = 0;
        ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_gql));
        ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r_gql));
      }

      /* GraphQL resolve success, set_status 200 fails and node_free fails */
      {
        c_rest_router *r_gql_ok = NULL;
        struct c_rest_graphql_schema *schema_ok = NULL;
        ASSERT_EQ(C_REST_OK, c_rest_graphql_schema_init(&schema_ok));
        ASSERT_EQ(C_REST_OK, c_rest_router_init(&r_gql_ok));
        ASSERT_EQ(C_REST_OK,
                  c_rest_router_add_graphql(r_gql_ok, "/gql_ok", schema_ok));
        req3.body = ok_gql;
        req3.body_len = strlen(ok_gql);
        req3.path = "/gql_ok";
        g_mock_res_status_fail = 1;
        ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r_gql_ok, &req3, &res3));
        g_mock_res_status_fail = 0;
        ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
        memset(&res3, 0, sizeof(res3));

        g_mock_graphql_free_countdown = 0;
        ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r_gql_ok, &req3, &res3));
        g_mock_graphql_free_countdown = -1;
        ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
        memset(&res3, 0, sizeof(res3));

        ASSERT_EQ(C_REST_OK, c_rest_graphql_schema_free(schema_ok));
        ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r_gql_ok));
        req3.body = NULL;
      }
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
      ASSERT_EQ(C_REST_OK, c_rest_router_init(&r_sse));
      ASSERT_EQ(C_REST_OK, c_rest_router_add_sse(r_sse, "/events", NULL, NULL));
      memset(&req_sse, 0, sizeof(req_sse));
      memset(&res_sse, 0, sizeof(res_sse));
      res_sse.context = &fake_ctx;
      req_sse.method = "GET";
      req_sse.path = "/events";
      g_mock_socket_fail = 201;
      ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r_sse, &req_sse, &res_sse));
      g_mock_socket_fail = 0;
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_sse));
      ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r_sse));
    }
#endif

    /* Test 404 with status failure */
    {
      c_rest_router *r404 = NULL;
      struct c_rest_request req_nf;
      struct c_rest_response res_nf;
      ASSERT_EQ(C_REST_OK, c_rest_router_init(&r404));
      memset(&req_nf, 0, sizeof(req_nf));
      memset(&res_nf, 0, sizeof(res_nf));
      req_nf.method = "GET";
      req_nf.path = "/notfound";
      g_mock_res_status_fail = 1;
      ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r404, &req_nf, &res_nf));
      g_mock_res_status_fail = 0;
      ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r404));
    }

    /* Test handler returning error with status failure */
    {
      c_rest_router *rh = NULL;
      struct c_rest_request req_err;
      struct c_rest_response res_err;
      ASSERT_EQ(C_REST_OK, c_rest_router_init(&rh));
      ASSERT_EQ(C_REST_OK,
                c_rest_router_add(rh, "GET", "/err", fail_handler, NULL));
      memset(&req_err, 0, sizeof(req_err));
      memset(&res_err, 0, sizeof(res_err));
      req_err.method = "GET";
      req_err.path = "/err";
      g_mock_res_status_fail = 1;
      ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(rh, &req_err, &res_err));
      g_mock_res_status_fail = 0;
      ASSERT_EQ(C_REST_OK, c_rest_router_destroy(rh));
    }

    /* Test free_node and destroy mock branches */
    {
      c_rest_router *r_mock = NULL;
      struct test_router *tr;

      /* test free_node(NULL) */
      ASSERT_EQ(C_REST_OK, c_rest_router_init(&r_mock));
      tr = (struct test_router *)r_mock;
      free(tr->root->segment);
      free(tr->root);
      tr->root = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r_mock));

      /* test router->openapi_spec fail */
      ASSERT_EQ(C_REST_OK, c_rest_router_init(&r_mock));
      tr = (struct test_router *)r_mock;
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(
                               (struct c_rest_openapi_spec *)tr->openapi_spec));
      tr->openapi_spec = (void *)0x1234;
      ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_router_destroy(r_mock));
      free(r_mock);

      /* test router->openapi_spec is NULL */
      ASSERT_EQ(C_REST_OK, c_rest_router_init(&r_mock));
      tr = (struct test_router *)r_mock;
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(
                               (struct c_rest_openapi_spec *)tr->openapi_spec));
      tr->openapi_spec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r_mock));
    }
  }

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
  ASSERT_EQ(C_REST_OK, c_rest_template_destroy(&dummy_ctx));
#endif
  PASS();
}

TEST test_router_dispatch_and_matching(void) {
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  c_rest_error_t ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  handler_called = 0;
  mw_called = 0;

  ASSERT_EQ(C_REST_OK, c_rest_router_init(&router));
  ASSERT_EQ(C_REST_OK,
            c_rest_router_use(router, "/api", test_middleware, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_add(router, "GET", "/api/users/:id",
                                         test_handler, NULL));

  req.method = "GET";
  req.path = "/api/users/123";
  res.status_code = 200;

  {
    int mf_idx;
    for (mf_idx = 1; mf_idx <= 5; mf_idx++) {
      g_router_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_router_fail_malloc_at = mf_idx;
      ret = c_rest_router_dispatch(router, &req, &res);
      ASSERT(ret == C_REST_OK || ret == C_REST_ERROR_OOM ||
             ret == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
      req.path_vars = NULL;
    }
    g_router_fail_malloc_at = 0;
    g_crf_malloc_hook = NULL;
  }

  ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router, &req, &res));
  ASSERT_NEQ(0, mw_called);
  ASSERT_NEQ(0, handler_called);
  ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  /* Test 404 */
  req.path = "/api/unknown";
  res.status_code = 200;
  ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router, &req, &res));
  ASSERT_EQ(404, res.status_code);
  ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  /* Test 405 */
  req.path = "/api/users/123";
  req.method = "POST";
  res.status_code = 200;
  ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router, &req, &res));
  ASSERT_EQ(405, res.status_code);
  ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  /* Test wildcard */
  ASSERT_EQ(C_REST_OK,
            c_rest_router_add(router, "GET", "/*", test_handler, NULL));
  req.method = "GET";
  req.path = "/something/else";
  res.status_code = 200;
  ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router, &req, &res));
  ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  /* Test path variations */
  ASSERT_EQ(C_REST_OK, c_rest_router_add(router, "GET", "api/no-slash",
                                         test_handler, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_add(router, "GET", "//double-slash",
                                         test_handler, NULL));
  ASSERT_EQ(C_REST_OK,
            c_rest_router_add(router, "GET", "/", test_handler, NULL));

#ifdef C_REST_ENABLE_SERVER_SENT_EVENTS_SSE
  ASSERT_EQ(C_REST_OK,
            c_rest_router_add_sse(router, "/sse", test_handler, NULL));
  req.path = "/sse";
  res.status_code = 200;
  ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router, &req, &res));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_OK, c_rest_router_add_sse(router, "/sse_null", NULL, NULL));
  req.path = "/sse_null";
  res.status_code = 200;
  ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router, &req, &res));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
#endif

  ASSERT_NEQ(C_REST_OK, c_rest_router_destroy(NULL));
  ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router));

  PASS();
}

SUITE(router_suite) {
  RUN_TEST(test_router_null_and_oom_branches);
  RUN_TEST(test_router_dispatch_and_matching);
  RUN_TEST(test_router_oauth2_middleware);
}
