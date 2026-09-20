/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_template.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_testing_mocks.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING

static int g_tpl_realloc_fail_at = 0;
static int g_tpl_realloc_count = 0;

static void *fail_realloc_at_n(void *ptr, size_t size) {
  g_tpl_realloc_count++;
  if (g_tpl_realloc_count == g_tpl_realloc_fail_at) {
    return NULL;
  }
  return realloc(ptr, size);
}

static void test_fail_realloc_nonfail(void) {
  void *p = malloc(10);
  g_tpl_realloc_count = 0;
  g_tpl_realloc_fail_at = 2;
  p = fail_realloc_at_n(p, 20);
  free(p);
  g_tpl_realloc_fail_at = 0;
  g_tpl_realloc_count = 0;
}

static void *fail_malloc_always(size_t size) {
  (void)size;
  return NULL;
}

static int check_str_eq(const char *actual, const char *expected) {
  return strcmp(actual, expected) != 0;
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
  return C_REST_OK;
}

/**
 * @brief Test runner for template engine.
 * @return 0 on success, non-zero on failure.
 */
int test_template(void) {
  int failed = 0;
  c_rest_error_t rc;
  const char *msgs[2];
  struct c_rest_template_context ctx;
  char *result = NULL;

  test_fail_realloc_nonfail();

  /* 1. Init & Destroy & Null checks */
  rc = c_rest_template_init(&ctx, "Hello {{name}}");
  failed += (rc != C_REST_OK);
  failed += (ctx.template_str == NULL);
  failed += (ctx.template_len != 14);

  rc = c_rest_template_destroy(&ctx);
  failed += (rc != C_REST_OK);
  failed += (ctx.template_str != NULL);
  failed += (ctx.template_len != 0);

  rc = c_rest_template_init(NULL, "test");
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_template_init(&ctx, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_template_destroy(NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  memset(&ctx, 0, sizeof(ctx));
  rc = c_rest_template_destroy(&ctx);
  failed += (rc != C_REST_OK);

  /* Init OOM */
  g_crf_malloc_hook = fail_malloc_always;
  rc = c_rest_template_init(&ctx, "Hello");
  failed += (rc != C_REST_ERROR_GENERIC);
  g_crf_malloc_hook = NULL;

  /* 2. Basic Render & Null checks */
  {
    const char *keys[] = {"name", "title"};
    const char *values[] = {"World", "Mr"};

    rc = c_rest_template_init(&ctx, "Hello {{name}}, I am {{title}}!");
    failed += (rc != C_REST_OK);

    rc = c_rest_template_render(NULL, keys, values, 2, &result);
    failed += (rc != C_REST_ERROR_GENERIC);

    rc = c_rest_template_render(&ctx, keys, values, 2, NULL);
    failed += (rc != C_REST_ERROR_GENERIC);

    /* Null template_str in ctx */
    {
      char *orig = ctx.template_str;
      ctx.template_str = NULL;
      rc = c_rest_template_render(&ctx, keys, values, 2, &result);
      failed += (rc != C_REST_ERROR_GENERIC);
      ctx.template_str = orig;
    }

    /* Render OOM on initial malloc */
    g_crf_malloc_hook = fail_malloc_always;
    rc = c_rest_template_render(&ctx, keys, values, 2, &result);
    failed += (rc != C_REST_ERROR_GENERIC);
    g_crf_malloc_hook = NULL;

    /* Successful render */
    rc = c_rest_template_render(&ctx, keys, values, 2, &result);
    failed += (rc != C_REST_OK);
    failed += (result == NULL);
    failed += check_str_eq(result, "Hello World, I am Mr!");
    CRF_FREE(result);
    result = NULL;

    rc = c_rest_template_destroy(&ctx);
    failed += (rc != C_REST_OK);
  }

  /* 3. Render missing key */
  {
    const char *keys[] = {"name"};
    const char *values[] = {"World"};

    rc = c_rest_template_init(&ctx, "Hello {{name}}, {{title}}");
    failed += (rc != C_REST_OK);

    rc = c_rest_template_render(&ctx, keys, values, 1, &result);
    failed += (rc != C_REST_OK);
    failed += (result == NULL);
    failed += check_str_eq(result, "Hello World, {{title}}");
    CRF_FREE(result);
    result = NULL;

    rc = c_rest_template_destroy(&ctx);
    failed += (rc != C_REST_OK);
  }

  /* 4. Render edge cases: incomplete braces, missing 2nd brace, null key, null
   * value */
  {
    const char *keys[] = {"a", "b", "c"};
    const char *values[] = {"b", NULL, "test_val"};

    rc = c_rest_template_init(&ctx, "{{a}}{{a}}");
    failed += (rc != C_REST_OK);
    rc = c_rest_template_render(&ctx, keys, values, 1, &result);
    failed += (rc != C_REST_OK);
    failed += (result == NULL);
    failed += check_str_eq(result, "bb");
    CRF_FREE(result);
    result = NULL;
    rc = c_rest_template_destroy(&ctx);
    failed += (rc != C_REST_OK);

    /* Incomplete braces */
    rc = c_rest_template_init(&ctx, "Hello {{name");
    failed += (rc != C_REST_OK);
    rc = c_rest_template_render(&ctx, keys, values, 0, &result);
    failed += (rc != C_REST_OK);
    failed += (result == NULL);
    failed += check_str_eq(result, "Hello {{name");
    CRF_FREE(result);
    result = NULL;
    rc = c_rest_template_destroy(&ctx);
    failed += (rc != C_REST_OK);

    /* Missing second brace */
    rc = c_rest_template_init(&ctx, "Hello {{name}foo");
    failed += (rc != C_REST_OK);
    rc = c_rest_template_render(&ctx, keys, values, 0, &result);
    failed += (rc != C_REST_OK);
    failed += (result == NULL);
    failed += check_str_eq(result, "Hello {{name}foo");
    CRF_FREE(result);
    result = NULL;
    rc = c_rest_template_destroy(&ctx);
    failed += (rc != C_REST_OK);

    /* Null key entry in keys array */
    {
      const char *null_keys[] = {NULL, "a"};
      const char *null_values[] = {"1", "2"};
      rc = c_rest_template_init(&ctx, "Hello {{a}}");
      failed += (rc != C_REST_OK);
      rc = c_rest_template_render(&ctx, null_keys, null_values, 2, &result);
      failed += (rc != C_REST_OK);
      failed += (result == NULL);
      failed += check_str_eq(result, "Hello 2");
      CRF_FREE(result);
      result = NULL;
      rc = c_rest_template_destroy(&ctx);
      failed += (rc != C_REST_OK);
    }

    /* Null value for matched key */
    rc = c_rest_template_init(&ctx, "Null {{b}} test");
    failed += (rc != C_REST_OK);
    rc = c_rest_template_render(&ctx, keys, values, 2, &result);
    failed += (rc != C_REST_OK);
    failed += (result == NULL);
    failed += check_str_eq(result, "Null  test");
    CRF_FREE(result);
    result = NULL;
    rc = c_rest_template_destroy(&ctx);
    failed += (rc != C_REST_OK);
  }

  /* 5. Force reallocations and test realloc failures */
  {
    int i;
    char large_template[1024];
    const char *keys[] = {"c"};
    const char *values[] = {
        "long_replacement_value_exceeding_initial_buffer_size_1234567890"};

    /* Reallocation in matched key replacement */
    large_template[0] = '\0';
    for (i = 0; i < 10; i++) {
#if defined(_MSC_VER)
      strcat_s(large_template, sizeof(large_template), "{{c}}");
#else
      strcat(large_template, "{{c}}");
#endif
    }
    rc = c_rest_template_init(&ctx, large_template);
    failed += (rc != C_REST_OK);
    ctx.template_len = 1;
    rc = c_rest_template_render(&ctx, keys, values, 1, &result);
    failed += (rc != C_REST_OK);
    failed += (result == NULL);
    CRF_FREE(result);
    result = NULL;

    /* Realloc failure in matched key replacement */
    ctx.template_len = 1;
    g_tpl_realloc_count = 0;
    g_tpl_realloc_fail_at = 1;
    g_crf_realloc_hook = fail_realloc_at_n;
    rc = c_rest_template_render(&ctx, keys, values, 1, &result);
    failed += (rc != C_REST_ERROR_GENERIC);
    g_crf_realloc_hook = NULL;
    g_tpl_realloc_fail_at = 0;

    rc = c_rest_template_destroy(&ctx);
    failed += (rc != C_REST_OK);

    /* Reallocation in unmatched key replacement */
    large_template[0] = '\0';
    for (i = 0; i < 20; i++) {
#if defined(_MSC_VER)
      strcat_s(large_template, sizeof(large_template), "{{unknown_key}}");
#else
      strcat(large_template, "{{unknown_key}}");
#endif
    }
    rc = c_rest_template_init(&ctx, large_template);
    failed += (rc != C_REST_OK);
    ctx.template_len = 1;
    rc = c_rest_template_render(&ctx, keys, values, 1, &result);
    failed += (rc != C_REST_OK);
    failed += (result == NULL);
    CRF_FREE(result);
    result = NULL;

    /* Realloc failure in unmatched key replacement */
    ctx.template_len = 1;
    g_tpl_realloc_count = 0;
    g_tpl_realloc_fail_at = 1;
    g_crf_realloc_hook = fail_realloc_at_n;
    rc = c_rest_template_render(&ctx, keys, values, 1, &result);
    failed += (rc != C_REST_ERROR_GENERIC);
    g_crf_realloc_hook = NULL;
    g_tpl_realloc_fail_at = 0;

    rc = c_rest_template_destroy(&ctx);
    failed += (rc != C_REST_OK);

    /* Reallocation in regular char copy */
    large_template[0] = '\0';
    for (i = 0; i < 500; i++) {
#if defined(_MSC_VER)
      strcat_s(large_template, sizeof(large_template), "a");
#else
      strcat(large_template, "a");
#endif
    }
    rc = c_rest_template_init(&ctx, large_template);
    failed += (rc != C_REST_OK);
    ctx.template_len = 1;
    rc = c_rest_template_render(&ctx, keys, values, 0, &result);
    failed += (rc != C_REST_OK);
    failed += (result == NULL);
    CRF_FREE(result);
    result = NULL;

    /* Realloc failure in regular char copy */
    ctx.template_len = 1;
    g_tpl_realloc_count = 0;
    g_tpl_realloc_fail_at = 1;
    g_crf_realloc_hook = fail_realloc_at_n;
    rc = c_rest_template_render(&ctx, keys, values, 0, &result);
    failed += (rc != C_REST_ERROR_GENERIC);
    g_crf_realloc_hook = NULL;
    g_tpl_realloc_fail_at = 0;

    rc = c_rest_template_destroy(&ctx);
    failed += (rc != C_REST_OK);
  }

  /* 6. Integration test with c_rest_router */
  {
    c_rest_router *router = NULL;
    struct c_rest_request req;
    struct c_rest_response res;

    rc = c_rest_template_init(&ctx, "Welcome {{user}}! Role: {{role}}");
    failed += (rc != C_REST_OK);

    rc = c_rest_router_init(&router);
    failed += (rc != C_REST_OK);

    rc = c_rest_router_add_template(router, "GET", "/profile", &ctx,
                                    dummy_template_data_provider, NULL);
    failed += (rc != C_REST_OK);

    memset(&req, 0, sizeof(req));
    req.method = "GET";
    req.path = "/profile";

    memset(&res, 0, sizeof(res));

    rc = c_rest_router_dispatch(router, &req, &res);
    failed += (rc != C_REST_OK);
    failed += (res.status_code != 200);
    failed += (res.body == NULL);
    failed += check_str_eq(res.body, "Welcome Alice! Role: Admin");

    rc = c_rest_response_cleanup(&res);
    failed += (rc != C_REST_OK);
    rc = c_rest_router_destroy(router);
    failed += (rc != C_REST_OK);
    rc = c_rest_template_destroy(&ctx);
    failed += (rc != C_REST_OK);
  }

  msgs[0] = "test_template passed\n";
  msgs[1] = "test_template failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}

#endif /* C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING */
