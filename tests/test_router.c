/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_template.h"
#include "c_rest_openapi.h"
#include "c_rest_middleware.h"

#include <stdio.h>
#include <string.h>
/* clang-format on */

static int handler_called = 0;
static int mw_called = 0;

static c_rest_error_t test_handler(struct c_rest_request *req,
                                   struct c_rest_response *res,
                                   void *user_data) {
  (void)res;
  (void)user_data;
  handler_called = 1;

  if (req->path_vars) {
    if (req->path_vars->name && req->path_vars->value &&
        strcmp(req->path_vars->name, "id") == 0 &&
        strcmp(req->path_vars->value, "123") == 0) {
      /* Matched var */
    } else {
      printf("Path var mismatch: %s = %s\n", req->path_vars->name,
             req->path_vars->value);
      handler_called = 0; /* Fail test if var is wrong */
    }
  } else {
    printf("No path vars extracted\n");
    handler_called = 0;
  }
  return 0;
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

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  printf("Testing OAuth2 Middleware...\n");

  /* Test 1: No auth header */
  ret = c_rest_oauth2_middleware(&req, &res, (void *)(size_t)dummy_verify);
  if (ret == 0 || res.status_code != 401) {
    printf("Expected 401 for no auth header\n");
    return 1;
  }
  (void)!c_rest_response_cleanup(&res);

  /* Test 2: Invalid token */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  auth_hdr.key = "Authorization";
  auth_hdr.value = "Bearer invalid-token";
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  ret = c_rest_oauth2_middleware(&req, &res, (void *)(size_t)dummy_verify);
  if (ret == 0 || res.status_code != 401) {
    printf("Expected 401 for invalid token\n");
    return 1;
  }
  (void)!c_rest_response_cleanup(&res);

  /* Test 3: Valid token */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  auth_hdr.key = "Authorization";
  auth_hdr.value = "Bearer valid-token";
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  ret = c_rest_oauth2_middleware(&req, &res, (void *)(size_t)dummy_verify);
  if (ret != 0 || req.auth_context != (void *)(size_t)0xDEADBEEF) {
    printf("Expected success and valid auth_context\n");
    return 1;
  }
  (void)!c_rest_response_cleanup(&res);

  return 0;
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

static void *fail_realloc_n(void *ptr, size_t size) {
  static int alloc_count = 0;
  extern int g_fail_realloc_at;
  if (g_fail_realloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_realloc_at) {
    alloc_count = 0;
    g_fail_realloc_at = 0;
    return NULL;
  }
  return realloc(ptr, size);
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
    c_rest_router_destroy(r3);
#endif

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
    c_rest_router_init(&r3);
    req3.method = "POST";
    req3.path = "/gql";
    c_rest_router_add_graphql(r3, "/gql", (void *)1);
    req3.body = NULL;
    c_rest_router_dispatch(r3, &req3, &res3);
    req3.body = (void *)"{";
    req3.body_len = 1;
    c_rest_router_dispatch(r3, &req3, &res3);
    c_rest_router_destroy(r3);
#endif
  }

  for (i = 1; i <= 60; i++) {
    c_rest_router *router = NULL;
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 1);
    fail_realloc_n(NULL, 1);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    if (c_rest_router_init(&router) == C_REST_OK) {
      c_rest_router_add(router, "GET", "/api/users/:id", test_handler, NULL);
      c_rest_router_add(router, "POST", "/api/users/:id", test_handler, NULL);
      c_rest_router_add(router, "PUT", "/api/users/:id", test_handler, NULL);
      c_rest_router_destroy(router);
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
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
  test_coverage();

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  printf("Running router tests...\n");

  ret = c_rest_router_init(&router);
  if (ret != 0 || !router) {
    printf("Failed to init router\n");
    return 1;
  }

  ret = c_rest_router_use(router, "/api", test_middleware, NULL);
  if (ret != 0) {
    printf("Failed to add middleware\n");
    return 1;
  }

  ret = c_rest_router_add(router, "GET", "/api/users/:id", test_handler, NULL);
  if (ret != 0) {
    printf("Failed to add route\n");
    return 1;
  }

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
  if (ret != 0) {
    printf("Dispatch failed\n");
    return 1;
  }

  if (!mw_called) {
    printf("Middleware was not called\n");
    return 1;
  }

  if (!handler_called) {
    printf("Handler was not called\n");
    return 1;
  }

  (void)!c_rest_request_cleanup(&req);
  (void)!c_rest_response_cleanup(&res);

  /* Test 404 */
  req.path = "/api/unknown";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0 || res.status_code != 404) {
    printf("Expected 404\n");
    return 1;
  }

  (void)!c_rest_request_cleanup(&req);
  (void)!c_rest_response_cleanup(&res);

  /* Test 405 */
  req.path = "/api/users/123";
  req.method = "POST";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0 || res.status_code != 405) {
    printf("Expected 405\n");
    return 1;
  }

  (void)!c_rest_request_cleanup(&req);
  (void)!c_rest_response_cleanup(&res);
  /* Test wildcard */
  c_rest_router_add(router, "GET", "/*", test_handler, NULL);
  req.method = "GET";
  req.path = "/something/else";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0) {
    printf("Expected wildcard match\n");
    return 1;
  }
  (void)!c_rest_request_cleanup(&req);
  (void)!c_rest_response_cleanup(&res);

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
  (void)!ret;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Test SSE wrapper with null handler */
  c_rest_router_add_sse(router, "/sse_null", NULL, NULL);
  req.path = "/sse_null";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  (void)!ret;
  c_rest_response_cleanup(&res);
#endif

  (void)!c_rest_router_destroy(NULL);
  (void)!c_rest_router_destroy(router);

  if (test_oauth2_middleware_func() != 0)
    return 1;

  return 0;
}
