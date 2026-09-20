/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>
#include "c_rest_mem.h"

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_openapi.h"
#include "c_rest_response.h"
#include "c_rest_router.h"

static int g_mock_res_status_fail = 0;
static int g_mock_res_json_fail = 0;
static int g_mock_res_html_fail = 0;
static int g_mock_router_get_fail = 0;
static int g_mock_router_get_spec_null = 0;

extern c_rest_error_t c_rest_router_get_openapi_spec(struct c_rest_router *router, struct c_rest_openapi_spec **out_spec);

static c_rest_error_t mock_c_rest_response_set_status(struct c_rest_response *res, int status) {
    (void)res;
    (void)status;
    if (g_mock_res_status_fail) {
        g_mock_res_status_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return C_REST_OK;
}

static c_rest_error_t mock_c_rest_response_json(struct c_rest_response *res, const char *json) {
    (void)res;
    (void)json;
    if (g_mock_res_json_fail) {
        g_mock_res_json_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return C_REST_OK;
}

static c_rest_error_t mock_c_rest_response_html(struct c_rest_response *res, const char *html) {
    (void)res;
    (void)html;
    if (g_mock_res_html_fail) {
        g_mock_res_html_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return C_REST_OK;
}

static c_rest_error_t mock_c_rest_router_get_openapi_spec(struct c_rest_router *router, struct c_rest_openapi_spec **out_spec) {
    if (g_mock_router_get_spec_null) {
        if (out_spec)
            *out_spec = NULL;
        return C_REST_OK;
    }
    if (g_mock_router_get_fail) {
        g_mock_router_get_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return c_rest_router_get_openapi_spec(router, out_spec);
}

static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

#define c_rest_response_set_status mock_c_rest_response_set_status
#define c_rest_response_json mock_c_rest_response_json
#define c_rest_response_html mock_c_rest_response_html
#define c_rest_router_get_openapi_spec mock_c_rest_router_get_openapi_spec

#include "../src/c_rest_openapi.c"

#undef c_rest_response_set_status
#undef c_rest_response_json
#undef c_rest_response_html
#undef c_rest_router_get_openapi_spec

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_res_status_fail = 0;
  g_mock_res_json_fail = 0;
  g_mock_res_html_fail = 0;
  g_mock_router_get_fail = 0;
  g_mock_router_get_spec_null = 0;
  g_crf_malloc_hook = NULL;
}

TEST test_openapi_error_branches(void) {
  struct c_rest_request req = {0};
  struct c_rest_response res = {0};
  struct c_rest_router *router = NULL;

  c_rest_router_init(&router);

  /* openapi_handler: c_rest_router_get_openapi_spec failure */
  g_mock_router_get_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, openapi_handler(&req, &res, router));

  /* openapi_handler: c_rest_openapi_spec_to_json failure */
  g_mock_router_get_spec_null = 1;
  mock_c_rest_router_get_openapi_spec(router, NULL);
  ASSERT_EQ(C_REST_ERROR_GENERIC, openapi_handler(&req, &res, router));
  g_mock_router_get_spec_null = 0;

  /* openapi_handler: set_status 200 failure */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, openapi_handler(&req, &res, router));

  /* openapi_handler: response_json failure */
  g_mock_res_json_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, openapi_handler(&req, &res, router));

  /* openapi_handler: normal success path */
  ASSERT_EQ(C_REST_OK, openapi_handler(&req, &res, router));

  /* swagger_ui_handler: c_rest_router_get_openapi_spec failure */
  g_mock_router_get_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, swagger_ui_handler(&req, &res, router));

  /* swagger_ui_handler: set_status failure */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, swagger_ui_handler(&req, &res, router));

  /* swagger_ui_handler: response_html failure */
  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, swagger_ui_handler(&req, &res, router));

  /* swagger_ui_handler: normal success path with default url */
  ASSERT_EQ(C_REST_OK, swagger_ui_handler(&req, &res, router));

  /* swagger_ui_handler: OOM on html_buf */
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(C_REST_ERROR_OOM, swagger_ui_handler(&req, &res, router));
  g_crf_malloc_hook = NULL;

  /* enable_swagger_ui: NULL checks */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_enable_swagger_ui(NULL, "/docs", "/openapi.json"));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_enable_swagger_ui(router, NULL, "/openapi.json"));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_enable_swagger_ui(router, "/docs", NULL));

  /* enable_swagger_ui: router_get_openapi_spec failure */
  g_mock_router_get_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_enable_swagger_ui(router, "/docs", "/openapi.json"));

  /* enable_swagger_ui: malloc OOM on swagger_openapi_url */
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_enable_swagger_ui(router, "/docs", "/openapi.json"));
  g_crf_malloc_hook = NULL;

  /* enable_swagger_ui: normal success */
  ASSERT_EQ(C_REST_OK,
            c_rest_enable_swagger_ui(router, "/docs", "/custom_openapi.json"));

  /* swagger_ui_handler: success with custom swagger_openapi_url */
  ASSERT_EQ(C_REST_OK, swagger_ui_handler(&req, &res, router));

  c_rest_router_destroy(router);
  PASS();
}

SUITE_EXTERN(openapi_mock_suite);
SUITE(openapi_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_openapi_error_branches);
}
