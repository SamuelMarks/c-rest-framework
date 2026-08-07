/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_template.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING

int g_fail_realloc_at_tpl = 0;
static void *fail_realloc_n_tpl(void *ptr, size_t size) {
  static int alloc_count = 0;
  if (g_fail_realloc_at_tpl <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_realloc_at_tpl) {
    alloc_count = 0;
    g_fail_realloc_at_tpl = 0;
    return NULL;
  }
  return realloc(ptr, size);
}

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
  if (c_rest_template_init(NULL, "test") == C_REST_OK)
    return __LINE__;
  if (c_rest_template_init(&ctx, NULL) == C_REST_OK)
    return __LINE__;
  if (c_rest_template_destroy(NULL) == C_REST_OK)
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

  if (c_rest_template_render(NULL, keys, values, 2, &result) == C_REST_OK)
    return __LINE__;
  if (c_rest_template_render(&ctx, keys, values, 2, NULL) == C_REST_OK)
    return __LINE__;

  /* Null context template_str check for render */
  {
    char *orig = ctx.template_str;
    ctx.template_str = NULL;
    if (c_rest_template_render(&ctx, keys, values, 2, &result) == C_REST_OK)
      return __LINE__;
    if (c_rest_template_destroy(&ctx) != C_REST_OK)
      return __LINE__;
    ctx.template_str = orig;
  }

  if (c_rest_template_render(&ctx, keys, values, 2, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  if (strcmp(result, "Hello World, I am Mr!") != 0)
    return __LINE__;
  (void)!C_REST_FREE(result);
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
  (void)!C_REST_FREE(result);
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;
  return 0;
}

static int test_c_rest_template_render_edge_cases(void) {
  struct c_rest_template_context ctx;
  const char *keys[] = {"a", "b", "c"};
  const char *values[] = {"b", NULL,
                          "really_long_value_that_might_force_reallocation_if_"
                          "we_repeat_it_enough_times_1234567890"};
  char *result = NULL;
  int i;
  char large_template[1024];

  if (c_rest_template_init(&ctx, "{{a}}{{a}}") != 0)
    return __LINE__;
  if (c_rest_template_render(&ctx, keys, values, 1, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  if (strcmp(result, "bb") != 0)
    return __LINE__;
  (void)!C_REST_FREE(result);
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
  (void)!C_REST_FREE(result);
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;

  /* Missing second brace */
  if (c_rest_template_init(&ctx, "Hello {{name}foo") != 0)
    return __LINE__;
  if (c_rest_template_render(&ctx, keys, values, 0, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  if (strcmp(result, "Hello {{name}foo") != 0)
    return __LINE__;
  (void)!C_REST_FREE(result);
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;

  /* Null key test */
  {
    const char *null_keys[] = {NULL, "a"};
    const char *null_values[] = {"1", "2"};
    if (c_rest_template_init(&ctx, "Hello {{a}}") != 0)
      return __LINE__;
    if (c_rest_template_render(&ctx, null_keys, null_values, 2, &result) != 0)
      return __LINE__;
    if (result == NULL)
      return __LINE__;
    if (strcmp(result, "Hello 2") != 0)
      return __LINE__;
    (void)!C_REST_FREE(result);
    if (c_rest_template_destroy(&ctx) != 0)
      return __LINE__;
  }

  /* Null values for matched key */
  if (c_rest_template_init(&ctx, "Null {{b}} test") != 0)
    return __LINE__;
  if (c_rest_template_render(&ctx, keys, values, 2, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  if (strcmp(result, "Null  test") != 0)
    return __LINE__;
  (void)!C_REST_FREE(result);
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;

  /* Force reallocation in replacement */
  large_template[0] = '\0';
  for (i = 0; i < 10; i++) {
#if defined(_MSC_VER)
    strcat_s(large_template, sizeof(large_template), "{{c}}");
#else
    strcat(large_template, "{{c}}");
#endif
  }
  if (c_rest_template_init(&ctx, large_template) != 0)
    return __LINE__;
  ctx.template_len = 1;
  if (c_rest_template_render(&ctx, keys, values, 3, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  (void)!C_REST_FREE(result);
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;

  /* Force reallocation in unmatched key replacement */
  large_template[0] = '\0';
  for (i = 0; i < 20; i++) {
#if defined(_MSC_VER)
    strcat_s(large_template, sizeof(large_template), "{{unknown_key}}");
#else
    strcat(large_template, "{{unknown_key}}");
#endif
  }
  if (c_rest_template_init(&ctx, large_template) != 0)
    return __LINE__;
  ctx.template_len = 1;
  if (c_rest_template_render(&ctx, keys, values, 3, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  (void)!C_REST_FREE(result);
  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;

  /* Force reallocation in normal character copy */
  large_template[0] = '\0';
  for (i = 0; i < 500; i++) {
#if defined(_MSC_VER)
    strcat_s(large_template, sizeof(large_template), "a");
#else
    strcat(large_template, "a");
#endif
  }
  if (c_rest_template_init(&ctx, large_template) != 0)
    return __LINE__;
  /* explicitly force a small initial cap by hacking the struct for testing */
  ctx.template_len = 1;
  if (c_rest_template_render(&ctx, keys, values, 0, &result) != 0)
    return __LINE__;
  if (result == NULL)
    return __LINE__;
  (void)!C_REST_FREE(result);

  /* Test reallocation failure in normal character copy */
  {
    extern void *(*g_crf_realloc_hook)(void *, size_t);

    g_fail_realloc_at_tpl = -1;
    fail_realloc_n_tpl(NULL, 0);
    g_fail_realloc_at_tpl = 1;
    g_crf_realloc_hook = fail_realloc_n_tpl;
    if (c_rest_template_render(&ctx, keys, values, 0, &result) == C_REST_OK)
      return __LINE__;

    g_crf_realloc_hook = NULL;
  }

  if (c_rest_template_destroy(&ctx) != 0)
    return __LINE__;

  /* Test reallocation failure in replacement */
  {
    extern void *(*g_crf_realloc_hook)(void *, size_t);
    large_template[0] = '\0';
    for (i = 0; i < 10; i++) {
#if defined(_MSC_VER)
      strcat_s(large_template, sizeof(large_template), "{{c}}");
#else
      strcat(large_template, "{{c}}");
#endif
    }
    if (c_rest_template_init(&ctx, large_template) != 0)
      return __LINE__;

    ctx.template_len = 1;

    g_fail_realloc_at_tpl = -1;
    fail_realloc_n_tpl(NULL, 0);
    g_fail_realloc_at_tpl = 1;
    g_crf_realloc_hook = fail_realloc_n_tpl;
    if (c_rest_template_render(&ctx, keys, values, 3, &result) == C_REST_OK)
      return __LINE__;

    g_crf_realloc_hook = NULL;
    if (c_rest_template_destroy(&ctx) != 0)
      return __LINE__;
  }

  /* Test reallocation failure in unmatched key replacement */
  {
    extern void *(*g_crf_realloc_hook)(void *, size_t);
    large_template[0] = '\0';
    for (i = 0; i < 20; i++) {
#if defined(_MSC_VER)
      strcat_s(large_template, sizeof(large_template), "{{unknown_key}}");
#else
      strcat(large_template, "{{unknown_key}}");
#endif
    }
    if (c_rest_template_init(&ctx, large_template) != 0)
      return __LINE__;

    ctx.template_len = 1;

    g_fail_realloc_at_tpl = -1;
    fail_realloc_n_tpl(NULL, 0);
    g_fail_realloc_at_tpl = 1;
    g_crf_realloc_hook = fail_realloc_n_tpl;
    if (c_rest_template_render(&ctx, keys, values, 3, &result) == C_REST_OK)
      return __LINE__;

    g_crf_realloc_hook = NULL;
    if (c_rest_template_destroy(&ctx) != 0)
      return __LINE__;
  }

  return 0;
}

static c_rest_error_t dummy_template_data_provider(struct c_rest_request *req,
                                                   const char ***out_keys,
                                                   const char ***out_values,
                                                   size_t *out_count,
                                                   void *user_data) {
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

  (void)!c_rest_response_cleanup(&res);
  (void)!c_rest_router_destroy(router);
  (void)!c_rest_template_destroy(&ctx);
  return 0;
}

#ifdef C_REST_TESTING_MALLOC_HOOK
static int g_malloc_fail_count = -1;
static void *hook_malloc_template(size_t size) {
  if (g_malloc_fail_count == 0) {
    return NULL;
  }
  if (g_malloc_fail_count > 0) {
    g_malloc_fail_count--;
  }
  return malloc(size);
}
static void *hook_realloc_template(void *ptr, size_t size) {
  if (g_malloc_fail_count == 0) {
    return NULL;
  }
  if (g_malloc_fail_count > 0) {
    g_malloc_fail_count--;
  }
  return realloc(ptr, size);
}

static int test_c_rest_template_oom(void) {
  struct c_rest_template_context ctx;
  const char *keys[] = {"name"};
  const char *values[] = {"very_long_value_that_exceeds_initial_capacity"};
  char *result = NULL;
  int i;
  char large_template[1024];

  g_crf_malloc_hook = hook_malloc_template;
  g_crf_realloc_hook = hook_realloc_template;

  g_malloc_fail_count = 0;
  if (c_rest_template_init(&ctx, "Hello {{name}}") == C_REST_OK)
    return __LINE__;

  g_malloc_fail_count = -1;
  if (c_rest_template_init(&ctx, "Hello {{name}}") != C_REST_OK)
    return __LINE__;

  g_malloc_fail_count = 0;
  if (c_rest_template_render(&ctx, keys, values, 1, &result) == C_REST_OK)
    return __LINE__;

  /* Force realloc fail in copy char */
  large_template[0] = '\0';
  for (i = 0; i < 500; i++) {
#if defined(_MSC_VER)
    strcat_s(large_template, sizeof(large_template), "a");
#else
    strcat(large_template, "a");
#endif
  }
  g_malloc_fail_count = -1;
  c_rest_template_destroy(&ctx);
  if (c_rest_template_init(&ctx, large_template) != C_REST_OK)
    return __LINE__;

  ctx.template_len = 1;    /* Force tiny initial capacity */
  g_malloc_fail_count = 1; /* Fail the first realloc */
  if (c_rest_template_render(&ctx, keys, values, 1, &result) == C_REST_OK)
    return __LINE__;

  /* Force realloc fail in unmatched key */
  large_template[0] = '\0';
  for (i = 0; i < 20; i++) {
#if defined(_MSC_VER)
    strcat_s(large_template, sizeof(large_template), "{{unknown_key}}");
#else
    strcat(large_template, "{{unknown_key}}");
#endif
  }
  g_malloc_fail_count = -1;
  c_rest_template_destroy(&ctx);
  if (c_rest_template_init(&ctx, large_template) != C_REST_OK)
    return __LINE__;
  ctx.template_len = 1;
  g_malloc_fail_count = 1;
  if (c_rest_template_render(&ctx, keys, values, 1, &result) == C_REST_OK)
    return __LINE__;

  /* Force realloc fail in matched key */
  large_template[0] = '\0';
  for (i = 0; i < 20; i++) {
#if defined(_MSC_VER)
    strcat_s(large_template, sizeof(large_template), "{{name}}");
#else
    strcat(large_template, "{{name}}");
#endif
  }
  g_malloc_fail_count = -1;
  c_rest_template_destroy(&ctx);
  if (c_rest_template_init(&ctx, large_template) != C_REST_OK)
    return __LINE__;
  ctx.template_len = 1;
  g_malloc_fail_count = 1;
  if (c_rest_template_render(&ctx, keys, values, 1, &result) == C_REST_OK)
    return __LINE__;

  g_malloc_fail_count = -1;
  g_crf_malloc_hook = NULL;
  g_crf_realloc_hook = NULL;
  c_rest_template_destroy(&ctx);
  return 0;
}
#endif

int test_template(void) {
  int res;

  res = test_c_rest_template_init_destroy();
  if (res != 0) {
    printf("Failed test_c_rest_template_init_destroy at line %d\n", res);
    return res;
  }

  res = test_c_rest_template_render_basic();
  if (res != 0) {
    printf("Failed test_c_rest_template_render_basic at line %d\n", res);
    return res;
  }

  res = test_c_rest_template_render_missing_key();
  if (res != 0) {
    printf("Failed test_c_rest_template_render_missing_key at line %d\n", res);
    return res;
  }

  res = test_c_rest_template_render_edge_cases();
  if (res != 0) {
    printf("Failed test_c_rest_template_render_edge_cases at line %d\n", res);
    return res;
  }

  res = test_c_rest_template_integration();
  if (res != 0) {
    printf("Failed test_c_rest_template_integration at line %d\n", res);
    return res;
  }

#ifdef C_REST_TESTING_MALLOC_HOOK
  res = test_c_rest_template_oom();
  if (res != 0) {
    printf("Failed test_c_rest_template_oom at line %d\n", res);
    return res;
  }
#endif

  return 0;
}

#endif /* C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING */
