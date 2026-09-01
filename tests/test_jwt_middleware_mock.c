/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_jwt_middleware.h"
#include "c_rest_response.h"

static int g_mock_res_status_countdown = -1;
static int g_mock_res_html_countdown = -1;
static int g_mock_res_header_countdown = -1;

extern c_rest_error_t c_rest_response_set_status(struct c_rest_response *res, int status);
extern c_rest_error_t c_rest_response_html(struct c_rest_response *res, const char *html);
extern c_rest_error_t c_rest_response_set_header(struct c_rest_response *res, const char *key, const char *val);

static c_rest_error_t mock_c_rest_response_set_status(struct c_rest_response *res, int status) {
    if (g_mock_res_status_countdown >= 0) {
        if (g_mock_res_status_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_status_countdown--;
    }
    return c_rest_response_set_status(res, status);
}

static c_rest_error_t mock_c_rest_response_html(struct c_rest_response *res, const char *html) {
    if (g_mock_res_html_countdown >= 0) {
        if (g_mock_res_html_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_html_countdown--;
    }
    return c_rest_response_html(res, html);
}

static c_rest_error_t mock_c_rest_response_set_header(struct c_rest_response *res, const char *key, const char *val) {
    if (g_mock_res_header_countdown >= 0) {
        if (g_mock_res_header_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_res_header_countdown--;
    }
    return c_rest_response_set_header(res, key, val);
}

#define c_rest_response_set_status mock_c_rest_response_set_status
#define c_rest_response_html mock_c_rest_response_html
#define c_rest_response_set_header mock_c_rest_response_set_header

#define c_rest_jwt_middleware test_c_rest_jwt_middleware

#include "../src/c_rest_jwt_middleware.c"

#undef c_rest_response_set_status
#undef c_rest_response_html
#undef c_rest_response_set_header

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_res_status_countdown = -1;
  g_mock_res_html_countdown = -1;
  g_mock_res_header_countdown = -1;
}

TEST test_jwt_middleware_error_branches(void) {
  struct c_rest_request req = {0};
  struct c_rest_response res = {0};

  /* Missing auth header: fails set_status */
  g_mock_res_status_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_jwt_middleware(&req, &res, NULL));

  /* Missing auth header: fails html */
  g_mock_res_html_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_jwt_middleware(&req, &res, NULL));

  /* Missing auth header: fails set_header */
  g_mock_res_header_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_jwt_middleware(&req, &res, NULL));

  /* Add Bearer to hit the next branch */
  req.headers = malloc(sizeof(struct c_rest_header));
  req.headers->key = "Authorization";
  req.headers->value = "Bearer INVALID";
  req.headers->next = NULL;

  /* Invalid token: fails set_status */
  g_mock_res_status_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_jwt_middleware(&req, &res, NULL));

  /* Invalid token: fails html */
  g_mock_res_html_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_jwt_middleware(&req, &res, NULL));

  free(req.headers);
  PASS();
}

SUITE(jwt_middleware_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_jwt_middleware_error_branches);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(jwt_middleware_mock_suite);
  GREATEST_MAIN_END();
}
