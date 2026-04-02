/* clang-format off */
#include "test_protos.h"
#include "c_rest_template.h"
#include "c_rest_mem.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING

static int test_c_rest_template_init_destroy(void) {
  struct c_rest_template_context ctx;
  if (c_rest_template_init(&ctx, "Hello {{name}}") != 0)
    return __LINE__;
  if (ctx.template_str == NULL)
    return __LINE__;
  if (ctx.template_len != 14)
    return __LINE__;
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;
  if (ctx.template_str != NULL)
    return __LINE__;
  if (ctx.template_len != 0)
    return __LINE__;

  /* Null checks */
  if (c_rest_template_init(NULL, "test") == 0)
    return __LINE__;
  if (c_rest_template_init(&ctx, NULL) == 0)
    return __LINE__;
  if (c_rest_template_destroy(NULL) == 0)
    return __LINE__;
  return 0;
}

static int test_c_rest_template_render_basic(void) {
  struct c_rest_template_context ctx;
  const char *keys[] = {"name", "title"};
  const char *values[] = {"World", "Mr"};
  char *result = NULL;

  if (c_rest_template_init(&ctx, "Hello {{name}}, I am {{title}}!") != 0)
    return __LINE__;
  if (c_rest_template_render(&ctx, keys, values, 2, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  if (strcmp(result, "Hello World, I am Mr!") != 0)
    return __LINE__;
  C_REST_FREE(result);
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;
  return 0;
}

static int test_c_rest_template_render_missing_key(void) {
  struct c_rest_template_context ctx;
  const char *keys[] = {"name"};
  const char *values[] = {"World"};
  char *result = NULL;

  if (c_rest_template_init(&ctx, "Hello {{name}}, {{title}}") != 0)
    return __LINE__;
  if (c_rest_template_render(&ctx, keys, values, 1, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  if (strcmp(result, "Hello World, {{title}}") != 0)
    return __LINE__;
  C_REST_FREE(result);
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;
  return 0;
}

static int test_c_rest_template_render_edge_cases(void) {
  struct c_rest_template_context ctx;
  const char *keys[] = {"a"};
  const char *values[] = {"b"};
  char *result = NULL;

  if (c_rest_template_init(&ctx, "{{a}}{{a}}") != 0)
    return __LINE__;
  if (c_rest_template_render(&ctx, keys, values, 1, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  if (strcmp(result, "bb") != 0)
    return __LINE__;
  C_REST_FREE(result);
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;

  /* Incomplete braces */
  if (c_rest_template_init(&ctx, "Hello {{name") != 0)
    return __LINE__;
  if (c_rest_template_render(&ctx, keys, values, 0, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  if (strcmp(result, "Hello {{name") != 0)
    return __LINE__;
  C_REST_FREE(result);
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;
  return 0;
}

static int dummy_template_data_provider(struct c_rest_request *req,
                                        const char ***out_keys,
                                        const char ***out_values,
                                        size_t *out_count, void *user_data) {
  static const char *keys[] = {"user", "role"};
  static const char *values[] = {"Alice", "Admin"};
  (void)req;
  (void)user_data;
  *out_keys = keys;
  *out_values = values;
  *out_count = 2;
  return 0;
}

static int test_c_rest_template_integration(void) {
  struct c_rest_template_context ctx;
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;

  if (c_rest_template_init(&ctx, "Welcome {{user}}! Role: {{role}}") != 0)
    return __LINE__;

  if (c_rest_router_init(&router) != 0)
    return __LINE__;

  if (c_rest_router_add_template(router, "GET", "/profile", &ctx,
                                 dummy_template_data_provider, NULL) != 0)
    return __LINE__;

  memset(&req, 0, sizeof(req));
  req.method = "GET";
  req.path = "/profile";

  memset(&res, 0, sizeof(res));

  if (c_rest_router_dispatch(router, &req, &res) != 0) {
    printf("dispatch failed\n");
    return __LINE__;
  }

  if (res.status_code != 200) {
    printf("Status code was %d\n", res.status_code);
    return __LINE__;
  }

  if (res.body == NULL) {
    printf("Body was null\n");
    return __LINE__;
  }

  if (strcmp(res.body, "Welcome Alice! Role: Admin") != 0) {
    printf("Body was %s\n", res.body);
    return __LINE__;
  }

  c_rest_response_cleanup(&res);
  c_rest_router_destroy(router);
  c_rest_template_destroy(&ctx);
  return 0;
}

int test_template(void) {
  int res;

  res = test_c_rest_template_init_destroy();
  if (res != 0) {
    printf("Failed test_c_rest_template_init_destroy\n");
    return res;
  }

  res = test_c_rest_template_render_basic();
  if (res != 0) {
    printf("Failed test_c_rest_template_render_basic\n");
    return res;
  }

  res = test_c_rest_template_render_missing_key();
  if (res != 0) {
    printf("Failed test_c_rest_template_render_missing_key\n");
    return res;
  }

  res = test_c_rest_template_render_edge_cases();
  if (res != 0) {
    printf("Failed test_c_rest_template_render_edge_cases\n");
    return res;
  }

  res = test_c_rest_template_integration();
  if (res != 0) {
    printf("Failed test_c_rest_template_integration at line %d\\n", res);
    return res;
  }

  return 0;
}

#endif /* C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING */
