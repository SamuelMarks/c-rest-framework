/* clang-format off */
#include "test_protos.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_middleware.h"

#include <stdio.h>
#include <string.h>
/* clang-format on */

static int handler_called = 0;
static int mw_called = 0;

static int test_handler(struct c_rest_request *req, struct c_rest_response *res,
                        void *user_data) {
  (void)res;
  (void)user_data;
  handler_called = 1;

  if (req->path_vars) {
    if (strcmp(req->path_vars->name, "id") == 0 &&
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

static int test_middleware(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data) {
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
  int ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  printf("Testing OAuth2 Middleware...\n");

  /* Test 1: No auth header */
  ret = c_rest_oauth2_middleware(&req, &res, (void *)(size_t)dummy_verify);
  if (ret == 0 || res.status_code != 401) {
    printf("Expected 401 for no auth header\n");
    return 1;
  }
  c_rest_response_cleanup(&res);

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
  c_rest_response_cleanup(&res);

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
  c_rest_response_cleanup(&res);

  return 0;
}

int test_router(void) {
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  int ret;

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

  c_rest_request_cleanup(&req);
  c_rest_response_cleanup(&res);

  /* Test 404 */
  req.path = "/api/unknown";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0 || res.status_code != 404) {
    printf("Expected 404\n");
    return 1;
  }

  c_rest_request_cleanup(&req);
  c_rest_response_cleanup(&res);

  /* Test 405 */
  req.path = "/api/users/123";
  req.method = "POST";
  res.status_code = 200;
  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0 || res.status_code != 405) {
    printf("Expected 405\n");
    return 1;
  }

  c_rest_request_cleanup(&req);
  c_rest_response_cleanup(&res);

  c_rest_router_destroy(router);

  if (test_oauth2_middleware_func() != 0)
    return 1;

  return 0;
}
