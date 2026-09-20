/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>
#include <stdio.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_jwt_middleware.h"
#include "c_rest_response.h"
#include "c_rest_request.h"
#include "c_rest_crypto.h"
#include "c_rest_mem.h"

static int g_mock_res_status_fail = 0;
static int g_mock_res_html_fail = 0;
static int g_mock_res_header_fail = 0;

static c_rest_error_t mock_c_rest_response_set_status(struct c_rest_response *res, int status_code) {
    (void)res;
    (void)status_code;
    if (g_mock_res_status_fail) {
        g_mock_res_status_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return C_REST_OK;
}

static c_rest_error_t mock_c_rest_response_html(struct c_rest_response *res, const char *html_str) {
    (void)res;
    (void)html_str;
    if (g_mock_res_html_fail) {
        g_mock_res_html_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return C_REST_OK;
}

static c_rest_error_t mock_c_rest_response_set_header(struct c_rest_response *res, const char *name, const char *value) {
    (void)res;
    (void)name;
    (void)value;
    if (g_mock_res_header_fail) {
        g_mock_res_header_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return C_REST_OK;
}

#define c_rest_response_set_status mock_c_rest_response_set_status
#define c_rest_response_html mock_c_rest_response_html
#define c_rest_response_set_header mock_c_rest_response_set_header

#include "../src/c_rest_jwt_middleware.c"

#undef c_rest_response_set_status
#undef c_rest_response_html
#undef c_rest_response_set_header

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_res_status_fail = 0;
  g_mock_res_html_fail = 0;
  g_mock_res_header_fail = 0;
}

static c_rest_error_t mock_payload_fail(const char *payload, void **out_ctx) {
  (void)payload;
  (void)out_ctx;
  return C_REST_ERROR_GENERIC;
}

static c_rest_error_t mock_payload_ok(const char *payload, void **out_ctx) {
  (void)payload;
  if (out_ctx)
    *out_ctx = (void *)0x1234;
  return C_REST_OK;
}

TEST test_jwt_middleware_error_branches(void) {
  struct c_rest_request req = {0};
  struct c_rest_response res = {0};
  struct c_rest_jwt_middleware_config cfg = {0};
  struct c_rest_header auth_hdr = {0};
  const unsigned char secret[] = "testsecret";
  char *valid_token = NULL;
  char bearer_val[512];

  /* Test config init NULL checks */
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware_config_init(
                                      NULL, secret, sizeof(secret) - 1, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware_config_init(
                                      &cfg, NULL, sizeof(secret) - 1, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_jwt_middleware_config_init(&cfg, secret, 0, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_jwt_middleware_config_init(
                           &cfg, secret, sizeof(secret) - 1, NULL));

  /* Test req/res NULL checks */
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(NULL, &res, &cfg));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, NULL, &cfg));

  /* Test user_data NULL branches */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, NULL));

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, NULL));

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, NULL));

  /* Test missing bearer token branches (req.headers == NULL) */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  /* Set up invalid bearer token */
  auth_hdr.key = "Authorization";
  auth_hdr.value = "Bearer invalid.token.value";
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  /* Test invalid token signature branches */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  /* Sign a valid token */
  ASSERT_EQ(C_REST_OK, c_rest_jwt_sign_hs256("{\"sub\":\"user123\"}", secret,
                                             sizeof(secret) - 1, &valid_token));

#if defined(_MSC_VER)
  sprintf_s(bearer_val, sizeof(bearer_val), "Bearer %s", valid_token);
#else
  sprintf(bearer_val, "Bearer %s", valid_token);
#endif
  auth_hdr.value = bearer_val;

  /* Test verify_payload failure branches */
  cfg.verify_payload = mock_payload_fail;

  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_jwt_middleware(&req, &res, &cfg));

  /* Test success with verify_payload == NULL */
  cfg.verify_payload = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_jwt_middleware(&req, &res, &cfg));

  /* Test success with verify_payload != NULL */
  cfg.verify_payload = mock_payload_ok;
  mock_payload_ok(NULL, NULL);
  ASSERT_EQ(C_REST_OK, c_rest_jwt_middleware(&req, &res, &cfg));
  ASSERT_EQ((void *)0x1234, req.auth_context);

  C_REST_FREE(valid_token);
  PASS();
}

SUITE_EXTERN(jwt_middleware_mock_suite);
SUITE(jwt_middleware_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_jwt_middleware_error_branches);
}
