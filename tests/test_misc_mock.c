/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_middleware.h"
#include "c_rest_response.h"
#include "c_rest_request.h"

static int g_mock_res_redirect_fail = 0;
static int g_mock_res_header_fail = 0;
static int g_mock_res_status_fail = 0;
static int g_mock_res_html_fail = 0;

extern c_rest_error_t c_rest_response_redirect(struct c_rest_response *res, const char *url, int status_code);
extern c_rest_error_t c_rest_response_set_header(struct c_rest_response *res, const char *key, const char *val);
extern c_rest_error_t c_rest_response_set_status(struct c_rest_response *res, int status_code);
extern c_rest_error_t c_rest_response_html(struct c_rest_response *res, const char *html);

static c_rest_error_t mock_c_rest_response_redirect(struct c_rest_response *res, const char *url, int status_code) {
  if (g_mock_res_redirect_fail) return C_REST_ERROR_GENERIC;
  return c_rest_response_redirect(res, url, status_code);
}

static c_rest_error_t mock_c_rest_response_set_header(struct c_rest_response *res, const char *key, const char *val) {
  if (g_mock_res_header_fail) return C_REST_ERROR_GENERIC;
  return c_rest_response_set_header(res, key, val);
}

static c_rest_error_t mock_c_rest_response_set_status(struct c_rest_response *res, int status_code) {
  if (g_mock_res_status_fail) return C_REST_ERROR_GENERIC;
  return c_rest_response_set_status(res, status_code);
}

static c_rest_error_t mock_c_rest_response_html(struct c_rest_response *res, const char *html) {
  if (g_mock_res_html_fail) return C_REST_ERROR_GENERIC;
  return c_rest_response_html(res, html);
}

#define c_rest_response_redirect mock_c_rest_response_redirect
#define c_rest_response_set_header mock_c_rest_response_set_header
#define c_rest_response_set_status mock_c_rest_response_set_status
#define c_rest_response_html mock_c_rest_response_html

#include "../src/middleware.c"

#undef c_rest_response_redirect
#undef c_rest_response_set_header
#undef c_rest_response_set_status
#undef c_rest_response_html

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_res_redirect_fail = 0;
  g_mock_res_header_fail = 0;
  g_mock_res_status_fail = 0;
  g_mock_res_html_fail = 0;
}

static int dummy_verify_bearer_ok(const char *token, void **out_ctx) {
  (void)token;
  *out_ctx = (void *)0x123;
  return 0;
}
static int dummy_verify_bearer_fail(const char *token, void **out_ctx) {
  (void)token;
  (void)out_ctx;
  return -1;
}
static int dummy_verify_basic_ok(const char *user, const char *pass,
                                 void **out_ctx) {
  (void)user;
  (void)pass;
  *out_ctx = (void *)0x456;
  return 0;
}
static int dummy_verify_basic_fail(const char *user, const char *pass,
                                   void **out_ctx) {
  (void)user;
  (void)pass;
  (void)out_ctx;
  return -1;
}
static int dummy_oauth2_verify_ok(const char *token, void **out_ctx) {
  (void)token;
  *out_ctx = (void *)0x789;
  return 0;
}
static int dummy_oauth2_verify_fail(const char *token, void **out_ctx) {
  (void)token;
  (void)out_ctx;
  return -1;
}

TEST test_misc_error_branches(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header hdr;
  struct c_rest_auth_verifier verifier;
  union {
    c_rest_oauth2_verify_fn fn;
    void *ptr;
  } ok_fn, fail_fn;
  ok_fn.fn = dummy_oauth2_verify_ok;
  fail_fn.fn = dummy_oauth2_verify_fail;

  /* 1. CORS middleware */
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_cors_middleware(NULL, &res, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_cors_middleware(&req, NULL, NULL));

  req.method = "GET";
  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_cors_middleware(&req, &res, NULL));
  g_mock_res_header_fail = 0;

  ASSERT_EQ(C_REST_OK, c_rest_cors_middleware(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  req.method = "OPTIONS";
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_cors_middleware(&req, &res, NULL));
  ASSERT_EQ(204, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 2. HSTS middleware */
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hsts_middleware(&req, NULL, NULL));
  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hsts_middleware(&req, &res, NULL));
  g_mock_res_header_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_hsts_middleware(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 3. HTTPS redirect middleware */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_https_redirect_middleware(NULL, &res, NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_https_redirect_middleware(&req, NULL, NULL));

  /* scheme is NULL */
  req.scheme = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_https_redirect_middleware(&req, &res, NULL));

  /* scheme is https */
  req.scheme = "https";
  ASSERT_EQ(C_REST_OK, c_rest_https_redirect_middleware(&req, &res, NULL));

  /* scheme is http, redirect fails */
  req.scheme = "http";
  req.path = "/test";
  g_mock_res_redirect_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_https_redirect_middleware(&req, &res, NULL));
  g_mock_res_redirect_fail = 0;

  /* scheme is http, redirect succeeds */
  ASSERT_EQ(C_REST_OK, c_rest_https_redirect_middleware(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 4. Auth middleware */
  memset(&verifier, 0, sizeof(verifier));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(NULL, &res, &verifier));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, NULL, &verifier));

  /* Missing user_data */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_auth_middleware(&req, &res, NULL));
  g_mock_res_status_fail = 0;

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_auth_middleware(&req, &res, NULL));
  g_mock_res_html_fail = 0;

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_auth_middleware(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing authentication (no headers) */
  req.headers = NULL;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_status_fail = 0;

  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_header_fail = 0;

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_html_fail = 0;

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Bearer auth */
  memset(&hdr, 0, sizeof(hdr));
  hdr.key = "Authorization";
  hdr.value = "Bearer mytoken";
  req.headers = &hdr;

  /* Bearer not supported by verifier */
  verifier.verify_bearer = NULL;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_status_fail = 0;

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_html_fail = 0;

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Bearer verification failure */
  verifier.verify_bearer = dummy_verify_bearer_fail;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_status_fail = 0;

  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_header_fail = 0;

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_html_fail = 0;

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Bearer verification success */
  verifier.verify_bearer = dummy_verify_bearer_ok;
  ASSERT_EQ(C_REST_OK, c_rest_auth_middleware(&req, &res, &verifier));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Basic auth */
  hdr.value = "Basic dXNlcjpwYXNz"; /* base64 of user:pass */

  /* Basic not supported by verifier */
  verifier.verify_basic = NULL;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_status_fail = 0;

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_html_fail = 0;

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Basic verification failure */
  verifier.verify_basic = dummy_verify_basic_fail;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_status_fail = 0;

  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_header_fail = 0;

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  g_mock_res_html_fail = 0;

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_auth_middleware(&req, &res, &verifier));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Basic verification success */
  verifier.verify_basic = dummy_verify_basic_ok;
  ASSERT_EQ(C_REST_OK, c_rest_auth_middleware(&req, &res, &verifier));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 5. OAuth2 middleware */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_oauth2_middleware(NULL, &res, ok_fn.ptr));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_oauth2_middleware(&req, NULL, ok_fn.ptr));

  /* Missing user_data */
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_oauth2_middleware(&req, &res, NULL));
  g_mock_res_status_fail = 0;

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_oauth2_middleware(&req, &res, NULL));
  g_mock_res_html_fail = 0;

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_oauth2_middleware(&req, &res, NULL));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing Bearer token */
  req.headers = NULL;
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_oauth2_middleware(&req, &res, ok_fn.ptr));
  g_mock_res_status_fail = 0;

  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_oauth2_middleware(&req, &res, ok_fn.ptr));
  g_mock_res_header_fail = 0;

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_oauth2_middleware(&req, &res, ok_fn.ptr));
  g_mock_res_html_fail = 0;

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_oauth2_middleware(&req, &res, ok_fn.ptr));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Bearer present, verification fails */
  req.headers = &hdr;
  hdr.value = "Bearer badtoken";
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_oauth2_middleware(&req, &res, fail_fn.ptr));
  g_mock_res_status_fail = 0;

  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_oauth2_middleware(&req, &res, fail_fn.ptr));
  g_mock_res_header_fail = 0;

  g_mock_res_html_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_oauth2_middleware(&req, &res, fail_fn.ptr));
  g_mock_res_html_fail = 0;

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_oauth2_middleware(&req, &res, fail_fn.ptr));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Bearer present, verification succeeds */
  hdr.value = "Bearer goodtoken";
  ASSERT_EQ(C_REST_OK, c_rest_oauth2_middleware(&req, &res, ok_fn.ptr));
  c_rest_response_cleanup(&res);

  PASS();
}

SUITE_EXTERN(misc_mock_suite);
SUITE(misc_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_misc_error_branches);
}
