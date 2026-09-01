/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_openapi.h"
#include "c_rest_response.h"
#include "c_rest_router.h"

static int g_mock_res_status_countdown = -1;
static int g_mock_res_json_countdown = -1;
static int g_mock_res_html_countdown = -1;
static int g_mock_router_get_countdown = -1;

extern c_rest_error_t c_rest_response_set_status(struct c_rest_response *res, int status);
extern c_rest_error_t c_rest_response_json(struct c_rest_response *res, const char *json);
extern c_rest_error_t c_rest_response_html(struct c_rest_response *res, const char *html);
extern c_rest_error_t c_rest_router_get_openapi_spec(struct c_rest_router *router, struct c_rest_openapi_spec **out_spec);

static c_rest_error_t mock_c_rest_response_set_status(struct c_rest_response *res, int status) {
    if (g_mock_res_status_countdown >= 0) {
        if (g_mock_res_status_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_status_countdown--;
    }
    return c_rest_response_set_status(res, status);
}

static c_rest_error_t mock_c_rest_response_json(struct c_rest_response *res, const char *json) {
    if (g_mock_res_json_countdown >= 0) {
        if (g_mock_res_json_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_json_countdown--;
    }
    return c_rest_response_json(res, json);
}

static c_rest_error_t mock_c_rest_response_html(struct c_rest_response *res, const char *html) {
    if (g_mock_res_html_countdown >= 0) {
        if (g_mock_res_html_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_html_countdown--;
    }
    return c_rest_response_html(res, html);
}

static c_rest_error_t mock_c_rest_router_get_openapi_spec(struct c_rest_router *router, struct c_rest_openapi_spec **out_spec) {
    if (g_mock_router_get_countdown >= 0) {
        if (g_mock_router_get_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_router_get_countdown--;
    }
    return c_rest_router_get_openapi_spec(router, out_spec);
}

#define c_rest_response_set_status mock_c_rest_response_set_status
#define c_rest_response_json mock_c_rest_response_json
#define c_rest_response_html mock_c_rest_response_html
#define c_rest_router_get_openapi_spec mock_c_rest_router_get_openapi_spec

#define openapi_handler test_openapi_handler
#define swagger_ui_handler test_swagger_ui_handler

#include "../src/c_rest_openapi.c"

#undef c_rest_response_set_status
#undef c_rest_response_json
#undef c_rest_response_html
#undef c_rest_router_get_openapi_spec

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_res_status_countdown = -1;
  g_mock_res_json_countdown = -1;
  g_mock_res_html_countdown = -1;
  g_mock_router_get_countdown = -1;
}

TEST test_openapi_error_branches(void) {
  struct c_rest_request req = {0};
  struct c_rest_response res = {0};

  /* openapi_handler: c_rest_router_get_openapi_spec */
  g_mock_router_get_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_openapi_handler(&req, &res, NULL));

  /* openapi_handler: c_rest_response_set_status */
  g_mock_res_status_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_openapi_handler(&req, &res, NULL));

  /* openapi_handler: c_rest_response_json */
  g_mock_res_json_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_openapi_handler(&req, &res, NULL));

  /* swagger_ui_handler: c_rest_router_get_openapi_spec */
  g_mock_router_get_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_swagger_ui_handler(&req, &res, NULL));

  /* swagger_ui_handler: c_rest_response_set_status */
  g_mock_res_status_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_swagger_ui_handler(&req, &res, NULL));

  /* swagger_ui_handler: c_rest_response_html */
  g_mock_res_html_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_swagger_ui_handler(&req, &res, NULL));

  PASS();
}

SUITE(openapi_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_openapi_error_branches);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(openapi_mock_suite);
  GREATEST_MAIN_END();
}
