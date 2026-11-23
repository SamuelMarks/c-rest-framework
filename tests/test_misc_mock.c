/* clang-format off */
#include "c_rest_export.h"
#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <cfs/cfs.h>
#include <stdio.h>
#include <string.h>

#include "c_rest_log.h"
#include "c_rest_middleware.h"
#include "c_rest_response.h"
#include "c_rest_request.h"
#include "c_rest_str_utils.h"

static int g_mock_res_redirect_fail = 0;
static int g_mock_res_header_fail = 0;
static int g_mock_res_status_fail = 0;
static int g_mock_res_html_fail = 0;
static int g_mock_log_debug_fail = 0;
static int g_mock_strcasecmp_fail = 0;
static int g_mock_mw_cfs_init_fail = 0;
static int g_mock_mw_cfs_append_fail = 0;
static int g_mock_mw_cfs_exists_fail = 0;
static int g_mock_mw_cfs_status_fail = 0;
static int g_mock_mw_cfs_isreg_fail = 0;
static int g_mock_mw_cfs_cstr_fail = 0;
static int g_mock_mw_fopen_fail = 0;
static int g_mock_mw_fseek_fail = 0;
static int g_mock_mw_ftell_fail = 0;

static c_rest_error_t mock_c_rest_response_redirect(struct c_rest_response *res, const char *url, int status_code) {
  if (g_mock_res_redirect_fail) return C_REST_ERROR_GENERIC;
  return c_rest_response_redirect(res, url, status_code);
}

static c_rest_error_t mock_c_rest_response_set_header(struct c_rest_response *res, const char *key, const char *val) {
  if (g_mock_res_header_fail == 1) return C_REST_ERROR_GENERIC;
  if (g_mock_res_header_fail > 1) g_mock_res_header_fail--;
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

static c_rest_error_t mock_log_debug(const char *fmt, ...) {
  (void)fmt;
  if (g_mock_log_debug_fail) return C_REST_ERROR_IO;
  return C_REST_OK;
}

static c_rest_error_t mock_strcasecmp(const char *s1, const char *s2, int *out_cmp) {
  if (g_mock_strcasecmp_fail) return C_REST_ERROR_INVALID_ARG;
  return c_rest_strcasecmp(s1, s2, out_cmp);
}

static cfs_errc mock_mw_cfs_path_init_str(cfs_path *p, const cfs_char_t *src) {
  if (g_mock_mw_cfs_init_fail) return cfs_errc_not_enough_memory;
  return cfs_path_init_str(p, src);
}

static cfs_errc mock_mw_cfs_path_append(cfs_path *p, const cfs_char_t *src) {
  if (g_mock_mw_cfs_append_fail) return cfs_errc_not_enough_memory;
  return cfs_path_append(p, src);
}

static cfs_errc mock_mw_cfs_exists_path(const cfs_path *p, cfs_bool *out, cfs_error_code *ec) {
  if (g_mock_mw_cfs_exists_fail) return cfs_errc_io_error;
  return cfs_exists_path(p, out, ec);
}

static cfs_errc mock_mw_cfs_status(const cfs_path *p, cfs_file_status *out, cfs_error_code *ec) {
  if (g_mock_mw_cfs_status_fail) return cfs_errc_permission_denied;
  return cfs_status(p, out, ec);
}

static cfs_errc mock_mw_cfs_is_regular_file(cfs_file_status s, cfs_bool *out) {
  if (g_mock_mw_cfs_isreg_fail) return cfs_errc_invalid_argument;
  return cfs_is_regular_file(s, out);
}

static cfs_errc mock_mw_cfs_path_c_str(const cfs_path *p, const cfs_char_t **out) {
  if (g_mock_mw_cfs_cstr_fail) return cfs_errc_invalid_argument;
  return cfs_path_c_str(p, out);
}

#if defined(_MSC_VER)
static errno_t mock_mw_fopen_s(FILE **pFile, const char *filename, const char *mode) {
  if (g_mock_mw_fopen_fail) {
    *pFile = NULL;
    return 13;
  }
  return fopen_s(pFile, filename, mode);
}
#else
static FILE *mock_mw_fopen(const char *filename, const char *mode) {
  if (g_mock_mw_fopen_fail) return NULL;
  return fopen(filename, mode);
}
#endif

static int mock_mw_fseek(FILE *fp, long offset, int origin) {
  if (g_mock_mw_fseek_fail == 1 && origin == SEEK_END) return -1;
  if (g_mock_mw_fseek_fail == 2 && origin == SEEK_SET) return -1;
  return fseek(fp, offset, origin);
}

static long mock_mw_ftell(FILE *fp) {
  if (g_mock_mw_ftell_fail) return -1L;
  return ftell(fp);
}

#define c_rest_response_redirect mock_c_rest_response_redirect
#define c_rest_response_set_header mock_c_rest_response_set_header
#define c_rest_response_set_status mock_c_rest_response_set_status
#define c_rest_response_html mock_c_rest_response_html
#define c_rest_framework_log_debug mock_log_debug
#define c_rest_strcasecmp mock_strcasecmp
#define cfs_path_init_str mock_mw_cfs_path_init_str
#define cfs_path_append mock_mw_cfs_path_append
#define cfs_exists_path mock_mw_cfs_exists_path
#define cfs_status mock_mw_cfs_status
#define cfs_is_regular_file mock_mw_cfs_is_regular_file
#define cfs_path_c_str mock_mw_cfs_path_c_str
#if defined(_MSC_VER)
#define fopen_s mock_mw_fopen_s
#else
#define fopen mock_mw_fopen
#endif
#define fseek mock_mw_fseek
#define ftell mock_mw_ftell

#include "../src/middleware.c"

#undef ftell
#undef fseek
#if defined(_MSC_VER)
#undef fopen_s
#else
#undef fopen
#endif
#undef cfs_path_c_str
#undef cfs_is_regular_file
#undef cfs_status
#undef cfs_exists_path
#undef cfs_path_append
#undef cfs_path_init_str
#undef c_rest_strcasecmp
#undef c_rest_framework_log_debug
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
  g_mock_log_debug_fail = 0;
  g_mock_strcasecmp_fail = 0;
  g_mock_mw_cfs_init_fail = 0;
  g_mock_mw_cfs_append_fail = 0;
  g_mock_mw_cfs_exists_fail = 0;
  g_mock_mw_cfs_status_fail = 0;
  g_mock_mw_cfs_isreg_fail = 0;
  g_mock_mw_cfs_cstr_fail = 0;
  g_mock_mw_fopen_fail = 0;
  g_mock_mw_fseek_fail = 0;
  g_mock_mw_ftell_fail = 0;
}

static c_rest_error_t dummy_verify_bearer_ok(const char *token,
                                             void **out_ctx) {
  (void)token;
  *out_ctx = (void *)0x1234;
  return C_REST_OK;
}
static c_rest_error_t dummy_verify_bearer_fail(const char *token,
                                               void **out_ctx) {
  (void)token;
  *out_ctx = NULL;
  return C_REST_ERROR_GENERIC;
}
static c_rest_error_t dummy_verify_basic_ok(const char *user, const char *pass,
                                            void **out_ctx) {
  (void)user;
  (void)pass;
  *out_ctx = (void *)0x5678;
  return C_REST_OK;
}
static c_rest_error_t
dummy_verify_basic_fail(const char *user, const char *pass, void **out_ctx) {
  (void)user;
  (void)pass;
  *out_ctx = NULL;
  return C_REST_ERROR_GENERIC;
}
static c_rest_error_t dummy_oauth2_verify_ok(const char *token,
                                             void **out_ctx) {
  (void)token;
  *out_ctx = (void *)0x789;
  return C_REST_OK;
}
static c_rest_error_t dummy_oauth2_verify_fail(const char *token,
                                               void **out_ctx) {
  (void)token;
  (void)out_ctx;
  return C_REST_ERROR_GENERIC;
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
  g_mock_res_header_fail = 2;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_cors_middleware(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));
  g_mock_res_header_fail = 3;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_cors_middleware(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));
  g_mock_res_header_fail = 0;

  ASSERT_EQ(C_REST_OK, c_rest_cors_middleware(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  req.method = "OPTIONS";
  g_mock_res_status_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_cors_middleware(&req, &res, NULL));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  ASSERT_EQ(C_REST_SHORT_CIRCUIT, c_rest_cors_middleware(&req, &res, NULL));
  ASSERT_EQ(204, res.status_code);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* 1b. Logger & Static middleware fault injection */
  req.method = "GET";
  req.path = "/CMakeLists.txt";
  g_mock_log_debug_fail = 1;
  ASSERT_EQ(C_REST_ERROR_IO, c_rest_logger_middleware(&req, &res, NULL));
  g_mock_log_debug_fail = 0;

  /* Traversal status / html fail */
  {
    char root_dot[] = ".";
    req.path = "/../bad";
    g_mock_res_status_fail = 1;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_static_middleware(&req, &res, root_dot));
    g_mock_res_status_fail = 0;
    g_mock_res_html_fail = 1;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_static_middleware(&req, &res, root_dot));
    g_mock_res_html_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    memset(&res, 0, sizeof(res));
  }

  /* cfs / stdio / header / status failures on valid existing file */
  {
    char root_dot[] = ".";
    FILE *tfp = NULL;
#if defined(_MSC_VER)
    fopen_s(&tfp, "test_misc_static_tmp.html", "wb");
#else
    tfp = fopen("test_misc_static_tmp.html", "wb");
#endif
    if (tfp) {
      fputs("<h1>hi</h1>", tfp);
      fclose(tfp);
    }
    req.path = "/test_misc_static_tmp.html";

    g_mock_mw_cfs_init_fail = 1;
    ASSERT_EQ(C_REST_ERROR_OOM, c_rest_static_middleware(&req, &res, root_dot));
    g_mock_mw_cfs_init_fail = 0;

    g_mock_mw_cfs_append_fail = 1;
    ASSERT_EQ(C_REST_ERROR_OOM, c_rest_static_middleware(&req, &res, root_dot));
    g_mock_mw_cfs_append_fail = 0;

    g_mock_mw_cfs_exists_fail = 1;
    ASSERT_EQ(C_REST_ERROR_IO, c_rest_static_middleware(&req, &res, root_dot));
    g_mock_mw_cfs_exists_fail = 0;

    g_mock_mw_cfs_status_fail = 1;
    ASSERT_EQ(C_REST_ERROR_UNAUTHORIZED,
              c_rest_static_middleware(&req, &res, root_dot));
    g_mock_mw_cfs_status_fail = 0;

    g_mock_mw_cfs_isreg_fail = 1;
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
              c_rest_static_middleware(&req, &res, root_dot));
    g_mock_mw_cfs_isreg_fail = 0;

    g_mock_mw_cfs_cstr_fail = 1;
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
              c_rest_static_middleware(&req, &res, root_dot));
    g_mock_mw_cfs_cstr_fail = 0;

    g_mock_mw_fopen_fail = 1;
    ASSERT_EQ(C_REST_ERROR_IO, c_rest_static_middleware(&req, &res, root_dot));
    g_mock_mw_fopen_fail = 0;

    g_mock_mw_fseek_fail = 1;
    ASSERT_EQ(C_REST_ERROR_IO, c_rest_static_middleware(&req, &res, root_dot));
    g_mock_mw_fseek_fail = 2;
    ASSERT_EQ(C_REST_ERROR_IO, c_rest_static_middleware(&req, &res, root_dot));
    g_mock_mw_fseek_fail = 0;

    g_mock_mw_ftell_fail = 1;
    ASSERT_EQ(C_REST_ERROR_IO, c_rest_static_middleware(&req, &res, root_dot));
    g_mock_mw_ftell_fail = 0;

    g_mock_strcasecmp_fail = 1;
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
              c_rest_static_middleware(&req, &res, root_dot));
    g_mock_strcasecmp_fail = 0;

    g_mock_res_status_fail = 1;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_static_middleware(&req, &res, root_dot));
    g_mock_res_status_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    memset(&res, 0, sizeof(res));

    g_mock_res_header_fail = 1;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_static_middleware(&req, &res, root_dot));
    g_mock_res_header_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    memset(&res, 0, sizeof(res));

    req.method = "HEAD";
    g_mock_res_header_fail = 1;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_static_middleware(&req, &res, root_dot));
    g_mock_res_header_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    memset(&res, 0, sizeof(res));
    req.method = "GET";

    remove("test_misc_static_tmp.html");
  }

  /* 2. HSTS middleware */
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hsts_middleware(&req, NULL, NULL));
  g_mock_res_header_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_hsts_middleware(&req, &res, NULL));
  g_mock_res_header_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_hsts_middleware(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
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
  ASSERT_EQ(C_REST_SHORT_CIRCUIT,
            c_rest_https_redirect_middleware(&req, &res, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
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
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
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
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* Bearer auth */
  {
    static char k_auth[] = "Authorization";
    static char v_bearer[] = "Bearer mytoken";
    memset(&hdr, 0, sizeof(hdr));
    hdr.key = k_auth;
    hdr.value = v_bearer;
  }
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
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
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
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* Bearer verification success */
  verifier.verify_bearer = dummy_verify_bearer_ok;
  ASSERT_EQ(C_REST_OK, c_rest_auth_middleware(&req, &res, &verifier));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* Basic auth */
  {
    static char v_basic[] = "Basic dXNlcjpwYXNz";
    hdr.value = v_basic;
  }

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
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
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
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* Basic verification success */
  verifier.verify_basic = dummy_verify_basic_ok;
  ASSERT_EQ(C_REST_OK, c_rest_auth_middleware(&req, &res, &verifier));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
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
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
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
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* Bearer present, verification fails */
  req.headers = &hdr;
  {
    static char v_bad[] = "Bearer badtoken";
    hdr.value = v_bad;
  }
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
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  /* Bearer present, verification succeeds */
  {
    static char v_good[] = "Bearer goodtoken";
    hdr.value = v_good;
  }
  ASSERT_EQ(C_REST_OK, c_rest_oauth2_middleware(&req, &res, ok_fn.ptr));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  PASS();
}

SUITE(misc_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_misc_error_branches);
}
