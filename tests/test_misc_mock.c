/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_middleware.h"
#include "c_rest_response.h"
#include "c_rest_request.h"

static int g_mock_res_redirect_countdown = -1;
static int g_mock_res_header_countdown = -1;

extern c_rest_error_t c_rest_response_redirect(struct c_rest_response *res, const char *url, int status_code);
extern c_rest_error_t c_rest_response_set_header(struct c_rest_response *res, const char *key, const char *val);

static c_rest_error_t mock_c_rest_response_redirect(struct c_rest_response *res, const char *url, int status_code) {
    if (g_mock_res_redirect_countdown >= 0) {
        if (g_mock_res_redirect_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_redirect_countdown--;
    }
    return c_rest_response_redirect(res, url, status_code);
}

static c_rest_error_t mock_c_rest_response_set_header(struct c_rest_response *res, const char *key, const char *val) {
    if (g_mock_res_header_countdown >= 0) {
        if (g_mock_res_header_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_header_countdown--;
    }
    return c_rest_response_set_header(res, key, val);
}

#define c_rest_response_redirect mock_c_rest_response_redirect
#define c_rest_response_set_header mock_c_rest_response_set_header

#define c_rest_https_redirect_middleware test_c_rest_https_redirect_middleware
#define c_rest_cors_middleware test_c_rest_cors_middleware
#define c_rest_hsts_middleware test_c_rest_hsts_middleware

#include "../src/middleware.c"

#undef c_rest_response_redirect
#undef c_rest_response_set_header

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_res_redirect_countdown = -1;
  g_mock_res_header_countdown = -1;
}

TEST test_misc_error_branches(void) {
  struct c_rest_request req = {0};
  struct c_rest_response res = {0};

  /* middleware.c: https_redirect fails on redirect */
  req.scheme = "http";
  req.path = "/";
  g_mock_res_redirect_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_https_redirect_middleware(&req, &res, NULL));

  /* middleware.c: cors fails on set_header */
  req.headers = malloc(sizeof(struct c_rest_header));
  req.headers->key = "Origin";
  req.headers->value = "http://a";
  req.headers->next = NULL;
  g_mock_res_header_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_cors_middleware(&req, &res, NULL));

  /* middleware.c: hsts fails on set_header */
  g_mock_res_header_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            test_c_rest_hsts_middleware(&req, &res, NULL));

  free(req.headers);
  PASS();
}

SUITE(misc_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_misc_error_branches);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(misc_mock_suite);
  GREATEST_MAIN_END();
}
