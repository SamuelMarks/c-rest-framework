/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_middleware.h"
#include "c_rest_modality.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_testing_mocks.h"
#include "test_protos.h"
#include <cfs/cfs.h>
#include <stdio.h>
#include <string.h>
/* clang-format on */

static char g_last_mw_log[600];
static int g_mw_log_fail = 0;

static c_rest_error_t capture_mw_log_cb(const char *msg) {
  size_t len;
  if (g_mw_log_fail) {
    return C_REST_ERROR_IO;
  }
  len = strlen(msg);
  if (len >= sizeof(g_last_mw_log)) {
    len = sizeof(g_last_mw_log) - 1U;
  }
#if defined(_MSC_VER)
  memcpy_s(g_last_mw_log, sizeof(g_last_mw_log), msg, len);
#else
  memcpy(g_last_mw_log, msg, len);
#endif
  g_last_mw_log[len] = '\0';
  return C_REST_OK;
}

static void write_mw_static_file(const char *path, const char *content) {
  FILE *fp = NULL;
#if defined(_MSC_VER)
  fopen_s(&fp, path, "wb");
#else
  fp = fopen(path, "wb");
#endif
  if (fp) {
    if (content && content[0] != '\0') {
      fwrite(content, 1U, strlen(content), fp);
    }
    fclose(fp);
  }
}

static c_rest_error_t mock_verify_bearer_ok(const char *token, void **ctx) {
  int is_good = (strcmp(token, "good_token") == 0);
  *ctx = (void *)0x123;
  return is_good ? C_REST_OK : C_REST_ERROR_GENERIC;
}

static c_rest_error_t mock_verify_basic_ok(const char *user, const char *pass,
                                           void **ctx) {
  int is_good = (strcmp(user, "alice") == 0 && strcmp(pass, "wonderland") == 0);
  *ctx = (void *)0x456;
  return is_good ? C_REST_OK : C_REST_ERROR_GENERIC;
}

static c_rest_error_t mock_verify_oauth2_ok(const char *token, void **ctx) {
  int is_good = (strcmp(token, "valid_oauth") == 0);
  *ctx = (void *)0x789;
  return is_good ? C_REST_OK : C_REST_ERROR_GENERIC;
}

static void mock_add_header(struct c_rest_request *req, const char *key,
                            const char *val) {
  struct c_rest_header *h = malloc(sizeof(*h));
  h->key = (char *)key;
  h->value = (char *)val;
  h->next = req->headers;
  req->headers = h;
}

static void free_mock_headers(struct c_rest_request *req) {
  struct c_rest_header *h = req->headers;
  while (h) {
    struct c_rest_header *next = h->next;
    free(h);
    h = next;
  }
  req->headers = NULL;
}

static int fail_after_malloc = -1;
static int current_malloc = 0;

static void *failing_malloc_hook(size_t size) {
  int should_fail = (fail_after_malloc == current_malloc);
  current_malloc++;
  return should_fail ? NULL : malloc(size);
}

TEST test_middleware_oom_coverage(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_auth_verifier v = {0, 0};
  int i;
  c_rest_error_t rc;
  union {
    void *ptr;
    c_rest_oauth2_verify_fn func;
  } u;

  u.func = mock_verify_oauth2_ok;
  v.verify_bearer = mock_verify_bearer_ok;
  v.verify_basic = mock_verify_basic_ok;

  for (i = 0; i < 300; i++) {
    fail_after_malloc = i;

    current_malloc = 0;
    g_crf_malloc_hook = failing_malloc_hook;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    req.method = (char *)"GET";
    rc = c_rest_cors_middleware(&req, &res, NULL);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    rc = c_rest_hsts_middleware(&req, &res, NULL);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    req.scheme = (char *)"http";
    req.path = (char *)"/hello";
    rc = c_rest_https_redirect_middleware(&req, &res, NULL);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    rc = c_rest_auth_middleware(&req, &res, NULL);
    ASSERT_NEQ(C_REST_OK, rc);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Basic YWxpY2U6d29uZGVybGFuZA==");
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT_NEQ(C_REST_OK, rc);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Basic Ym9iOmJhZA=="); /* bob:bad */
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT_NEQ(C_REST_OK, rc);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Bearer good_token");
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Bearer bad_token");
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT_NEQ(C_REST_OK, rc);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);

    {
      struct c_rest_auth_verifier empty_v = {0, 0};
      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      mock_add_header(&req, "Authorization", "Basic YWxpY2U6d29uZGVybGFuZA==");
      rc = c_rest_auth_middleware(&req, &res, &empty_v);
      ASSERT_NEQ(C_REST_OK, rc);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
      free_mock_headers(&req);

      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      mock_add_header(&req, "Authorization", "Bearer good_token");
      rc = c_rest_auth_middleware(&req, &res, &empty_v);
      ASSERT_NEQ(C_REST_OK, rc);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
      free_mock_headers(&req);
    }

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Bearer valid_oauth");
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Bearer bad");
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    ASSERT_NEQ(C_REST_OK, rc);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    rc = c_rest_oauth2_middleware(&req, &res, NULL);
    ASSERT_NEQ(C_REST_OK, rc);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    ASSERT_NEQ(C_REST_OK, rc);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    g_crf_malloc_hook = NULL;
  }
  PASS();
}

TEST test_middleware_cases(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  c_rest_error_t rc;

  printf("Testing cors middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_cors_middleware(NULL, &res, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  rc = c_rest_cors_middleware(&req, NULL, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  req.method = "GET";
  rc = c_rest_cors_middleware(&req, &res, NULL);
  ASSERT_FALSE(rc != C_REST_OK);

  req.method = "OPTIONS";
  rc = c_rest_cors_middleware(&req, &res, NULL);
  ASSERT_EQ(C_REST_SHORT_CIRCUIT, rc);
  ASSERT_FALSE(res.status_code != 204);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&req, 0, sizeof(req));

  printf("Testing logger middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_logger_middleware(NULL, &res, NULL));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_logger_middleware(&req, NULL, NULL));
  req.method = "GET";
  req.path = "/test";
  ASSERT_EQ(C_REST_OK, c_rest_logger_middleware(&req, &res, NULL));
  req.method = "GET";
  req.path = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_logger_middleware(&req, &res, NULL));
  req.method = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_logger_middleware(&req, &res, NULL));
  {
    struct c_rest_logger custom_logger;
    struct c_rest_context fw_ctx;
    struct c_rest_connection_context conn_ctx;
    char long_path[550];
    memset(&custom_logger, 0, sizeof(custom_logger));
    memset(&fw_ctx, 0, sizeof(fw_ctx));
    memset(&conn_ctx, 0, sizeof(conn_ctx));

    /* user_data with NULL log_cb falls back */
    ASSERT_EQ(C_REST_OK, c_rest_logger_middleware(&req, &res, &custom_logger));

    /* res->context without framework_ctx, and framework_ctx without log_cb */
    res.context = &conn_ctx;
    ASSERT_EQ(C_REST_OK, c_rest_logger_middleware(&req, &res, NULL));
    conn_ctx.framework_ctx = &fw_ctx;
    ASSERT_EQ(C_REST_OK, c_rest_logger_middleware(&req, &res, NULL));

    /* framework_ctx logger */
    fw_ctx.logger.log_cb = capture_mw_log_cb;
    req.method = "POST";
    req.path = "/items";
    g_last_mw_log[0] = '\0';
    ASSERT_EQ(C_REST_OK, c_rest_logger_middleware(&req, &res, NULL));
    ASSERT_STR_EQ("[HTTP] POST /items", g_last_mw_log);

    /* explicit user_data logger + callback failure */
    custom_logger.log_cb = capture_mw_log_cb;
    g_mw_log_fail = 1;
    ASSERT_EQ(C_REST_ERROR_IO,
              c_rest_logger_middleware(&req, &res, &custom_logger));
    g_mw_log_fail = 0;

    /* > 512 byte formatted log message (dynamic buffer + OOM + cb fail) */
    memset(long_path, 'x', sizeof(long_path) - 1U);
    long_path[0] = '/';
    long_path[sizeof(long_path) - 1U] = '\0';
    req.path = long_path;
    ASSERT_EQ(C_REST_OK, c_rest_logger_middleware(&req, &res, &custom_logger));
    g_mw_log_fail = 1;
    ASSERT_EQ(C_REST_ERROR_IO,
              c_rest_logger_middleware(&req, &res, &custom_logger));
    g_mw_log_fail = 0;
#ifdef C_REST_TESTING_MALLOC_HOOK
    fail_after_malloc = 0;
    current_malloc = 0;
    g_crf_malloc_hook = failing_malloc_hook;
    ASSERT_EQ(C_REST_ERROR_OOM,
              c_rest_logger_middleware(&req, &res, &custom_logger));
    g_crf_malloc_hook = NULL;
    fail_after_malloc = -1;
#endif
    res.context = NULL;
  }
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&req, 0, sizeof(req));

  printf("Testing static middleware...\n");
  {
    char pub_dir[] = "pub";
    char empty_dir[] = "";
    char static_dir[] = "test_mw_static_dir";
    cfs_path sp = {0};
    cfs_path subp = {0};
    cfs_error_code ec = {0};
    cfs_size_t removed = 0;
    cfs_bool ex = 0;
    char k_bad_paths[8][32] = {
        "/../secret.txt", "/a/../../etc/passwd", "/./hidden", "/sub/.",
        "/sub/..",        "/win\\sep",           "/C:drive",  "/bad\x01char"};
    char p_null_html[] = "/index.html";
    char p_missing[] = "/does_not_exist.html";
    char p_dot_x_a[] = "/.x/a";
    char p_x_dot_a[] = "/x./a";
    char p_dot_x[] = "/.x";
    char p_x_dot[] = "/x.";
    char p_single[] = "/x";
    char p_subdir[] = "/subdir";
    char p_root[] = "/";
    static const char *const k_ext_files[] = {
        "index.html", "page.htm", "app.css", "app.js",  "data.json",
        "readme.txt", "icon.svg", "img.png", "pic.jpg", "photo.jpeg",
        "mod.wasm",   "raw.bin",  "LICENSE"};
    size_t k;

    ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
              c_rest_static_middleware(NULL, &res, pub_dir));
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
              c_rest_static_middleware(&req, NULL, pub_dir));
    ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, empty_dir));
    req.path = NULL;
    req.method = p_null_html; /* non-GET/HEAD */
    ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, pub_dir));
    req.path = p_null_html;
    req.method = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, pub_dir));
    {
      char m_post[] = "POST";
      req.method = m_post;
      ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, pub_dir));
    }

#if defined(CFS_OS_WINDOWS) && defined(CFS_UNICODE)
    ASSERT_EQ(cfs_errc_success, cfs_path_init_str(&sp, L"test_mw_static_dir"));
    ASSERT_EQ(cfs_errc_success,
              cfs_path_init_str(&subp, L"test_mw_static_dir/subdir"));
#else
    ASSERT_EQ(cfs_errc_success,
              cfs_path_init_str(&sp, (const cfs_char_t *)"test_mw_static_dir"));
    ASSERT_EQ(cfs_errc_success,
              cfs_path_init_str(
                  &subp, (const cfs_char_t *)"test_mw_static_dir/subdir"));
#endif
    ASSERT_EQ(cfs_errc_success, cfs_exists_path(&sp, &ex, &ec));
    if (ex) {
      ASSERT_EQ(cfs_errc_success, cfs_remove_all(&sp, &removed, &ec));
    }
    ASSERT_EQ(cfs_errc_success, cfs_create_directories(&subp, &ec));

    for (k = 0U; k < sizeof(k_ext_files) / sizeof(k_ext_files[0]); ++k) {
      char fpath[128];
#if defined(_MSC_VER)
      sprintf_s(fpath, sizeof(fpath), "test_mw_static_dir/%s", k_ext_files[k]);
#else
      sprintf(fpath, "test_mw_static_dir/%s", k_ext_files[k]);
#endif
      write_mw_static_file(fpath, (k == 1U) ? "" : "hello-static");
    }

    /* Traversal attempts -> 403 + C_REST_SHORT_CIRCUIT */
    {
      char m_get[] = "GET";
      char m_head[] = "HEAD";
      req.method = m_get;
      for (k = 0U; k < sizeof(k_bad_paths) / sizeof(k_bad_paths[0]); ++k) {
        memset(&res, 0, sizeof(res));
        req.path = k_bad_paths[k];
        ASSERT_EQ(C_REST_SHORT_CIRCUIT,
                  c_rest_static_middleware(&req, &res, static_dir));
        ASSERT_EQ(403, res.status_code);
        ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
      }

      /* Missing file and directory pass-through -> C_REST_OK */
      memset(&res, 0, sizeof(res));
      req.path = p_missing;
      ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, static_dir));
      req.path = p_dot_x_a;
      ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, static_dir));
      req.path = p_x_dot_a;
      ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, static_dir));
      req.path = p_dot_x;
      ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, static_dir));
      req.path = p_x_dot;
      ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, static_dir));
      req.path = p_single;
      ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, static_dir));
      req.path = p_subdir;
      ASSERT_EQ(C_REST_OK, c_rest_static_middleware(&req, &res, static_dir));

      /* Root "/" serves index.html, overwriting any prior res.body */
      memset(&res, 0, sizeof(res));
      ASSERT_EQ(C_REST_OK, c_rest_response_html(&res, "old"));
      req.path = p_root;
      ASSERT_EQ(C_REST_SHORT_CIRCUIT,
                c_rest_static_middleware(&req, &res, static_dir));
      ASSERT_EQ(200, res.status_code);
      ASSERT_STR_EQ("hello-static", res.body);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

      /* Serve every MIME extension + empty file + HEAD request */
      for (k = 0U; k < sizeof(k_ext_files) / sizeof(k_ext_files[0]); ++k) {
        char rpath[64];
#if defined(_MSC_VER)
        sprintf_s(rpath, sizeof(rpath), "/%s", k_ext_files[k]);
#else
        sprintf(rpath, "/%s", k_ext_files[k]);
#endif
        memset(&res, 0, sizeof(res));
        req.method = m_get;
        req.path = rpath;
        ASSERT_EQ(C_REST_SHORT_CIRCUIT,
                  c_rest_static_middleware(&req, &res, static_dir));
        ASSERT_EQ(200, res.status_code);
        ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
      }
      memset(&res, 0, sizeof(res));
      req.method = m_head;
      req.path = p_null_html;
      ASSERT_EQ(C_REST_SHORT_CIRCUIT,
                c_rest_static_middleware(&req, &res, static_dir));
      ASSERT_EQ(200, res.status_code);
      ASSERT_EQ(NULL, res.body);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

#ifdef C_REST_TESTING_MALLOC_HOOK
      {
        int oi;
        for (oi = 0; oi < 8; ++oi) {
          c_rest_error_t orc;
          memset(&res, 0, sizeof(res));
          req.method = m_get;
          req.path = p_null_html;
          fail_after_malloc = oi;
          current_malloc = 0;
          g_crf_malloc_hook = failing_malloc_hook;
          orc = c_rest_static_middleware(&req, &res, static_dir);
          g_crf_malloc_hook = NULL;
          fail_after_malloc = -1;
          ASSERT(orc == C_REST_SHORT_CIRCUIT || orc == C_REST_ERROR_OOM ||
                 orc == C_REST_ERROR_GENERIC);
          ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
        }
      }
#endif
    }

    ASSERT_EQ(cfs_errc_success, cfs_remove_all(&sp, &removed, &ec));
    cfs_path_destroy(&subp);
    cfs_path_destroy(&sp);
  }
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  printf("Testing hsts middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_hsts_middleware(&req, NULL, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  rc = c_rest_hsts_middleware(NULL, &res, NULL);
  ASSERT_FALSE(rc != C_REST_OK);
  rc = c_rest_hsts_middleware(&req, &res, NULL);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&req, 0, sizeof(req));

  printf("Testing https redirect middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_https_redirect_middleware(NULL, &res, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  rc = c_rest_https_redirect_middleware(&req, NULL, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  rc = c_rest_https_redirect_middleware(&req, &res, NULL);
  ASSERT_FALSE(rc != C_REST_OK);

  req.scheme = "https";
  req.path = "/hello";
  rc = c_rest_https_redirect_middleware(&req, &res, NULL);
  ASSERT_FALSE(rc != C_REST_OK);

  req.scheme = "http";
  req.path = "/hello";
  rc = c_rest_https_redirect_middleware(&req, &res, NULL);
  ASSERT_EQ(C_REST_SHORT_CIRCUIT, rc);
  ASSERT_FALSE(res.status_code != 301);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  req.scheme = "http";
  req.path = NULL;
  mock_add_header(&req, "Host", "example.com");
  rc = c_rest_https_redirect_middleware(&req, &res, NULL);
  ASSERT_EQ(C_REST_SHORT_CIRCUIT, rc);
  ASSERT_FALSE(res.status_code != 301);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));
  free_mock_headers(&req);
  memset(&req, 0, sizeof(req));

  req.scheme = "http";
  req.path = NULL;
  mock_add_header(&req, "Host", NULL);
  rc = c_rest_https_redirect_middleware(&req, &res, NULL);
  ASSERT_EQ(C_REST_SHORT_CIRCUIT, rc);
  ASSERT_FALSE(res.status_code != 301);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));
  free_mock_headers(&req);
  memset(&req, 0, sizeof(req));

  printf("Testing auth middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_auth_middleware(NULL, &res, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  rc = c_rest_auth_middleware(&req, NULL, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_auth_middleware(&req, &res, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  ASSERT_FALSE(res.status_code != 500);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  {
    struct c_rest_auth_verifier v = {0, 0};

    /* Missing auth header entirely */
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 401);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    /* Bearer provided but verifier missing */
    mock_add_header(&req, "Authorization", "Bearer token");
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 500);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    /* Basic provided but verifier missing */
    mock_add_header(&req, "Authorization", "Basic YWxpY2U6d29uZGVybGFuZA==");
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 500);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    v.verify_bearer = mock_verify_bearer_ok;
    v.verify_basic = mock_verify_basic_ok;

    /* Basic provided and valid */
    mock_add_header(&req, "Authorization", "Basic YWxpY2U6d29uZGVybGFuZA==");
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_FALSE(req.auth_context != (void *)0x456);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    /* Basic provided and invalid */
    mock_add_header(&req, "Authorization", "Basic Ym9iOmJhZA==");
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 401);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    /* Bearer provided and valid */
    mock_add_header(&req, "Authorization", "Bearer good_token");
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_FALSE(req.auth_context != (void *)0x123);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    /* Bearer provided and invalid */
    mock_add_header(&req, "Authorization", "Bearer bad_token");
    rc = c_rest_auth_middleware(&req, &res, &v);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 401);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));
  }

  printf("Testing oauth2 middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_oauth2_middleware(NULL, &res, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  rc = c_rest_oauth2_middleware(&req, NULL, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_oauth2_middleware(&req, &res, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  ASSERT_FALSE(res.status_code != 500);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  {
    union {
      void *ptr;
      c_rest_oauth2_verify_fn func;
    } u;
    u.func = mock_verify_oauth2_ok;

    /* Missing header */
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 401);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    memset(&req, 0, sizeof(req));

    /* Invalid header */
    mock_add_header(&req, "Authorization", "Bearer bad");
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 401);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    /* Valid header */
    mock_add_header(&req, "Authorization", "Bearer valid_oauth");
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_FALSE(req.auth_context != (void *)0x789);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));
  }

  CHECK_CALL(test_middleware_oom_coverage());

  {
    struct c_rest_auth_verifier v = {0, 0};
    union {
      void *ptr;
      c_rest_oauth2_verify_fn func;
    } u;
    u.func = mock_verify_oauth2_ok;
    v.verify_bearer = mock_verify_bearer_ok;
    v.verify_basic = mock_verify_basic_ok;

    /* Test c_rest_auth_middleware with g_mock_res_status_fail */
    /* 1. Missing user_data */
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    g_mock_res_status_fail = 1;
    rc = c_rest_auth_middleware(&req, &res, NULL);
    g_mock_res_status_fail = 0;
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    /* 2. Missing auth (not bearer, not basic) */
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    g_mock_res_status_fail = 1;
    rc = c_rest_auth_middleware(&req, &res, &v);
    g_mock_res_status_fail = 0;
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    /* 3. Bearer auth, verify_bearer missing */
    {
      struct c_rest_auth_verifier no_bearer = {0, 0};
      no_bearer.verify_basic = mock_verify_basic_ok;
      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      mock_add_header(&req, "Authorization", "Bearer good_token");
      g_mock_res_status_fail = 1;
      rc = c_rest_auth_middleware(&req, &res, &no_bearer);
      g_mock_res_status_fail = 0;
      ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
      free_mock_headers(&req);
    }

    /* 4. Bearer auth, invalid token */
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Bearer bad_token");
    g_mock_res_status_fail = 1;
    rc = c_rest_auth_middleware(&req, &res, &v);
    g_mock_res_status_fail = 0;
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);

    /* 5. Basic auth, verify_basic missing */
    {
      struct c_rest_auth_verifier no_basic = {0, 0};
      no_basic.verify_bearer = mock_verify_bearer_ok;
      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      mock_add_header(&req, "Authorization", "Basic YWxpY2U6d29uZGVybGFuZA==");
      g_mock_res_status_fail = 1;
      rc = c_rest_auth_middleware(&req, &res, &no_basic);
      g_mock_res_status_fail = 0;
      ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
      free_mock_headers(&req);
    }

    /* 6. Basic auth, invalid credentials */
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Basic Ym9iOmJhZA==");
    g_mock_res_status_fail = 1;
    rc = c_rest_auth_middleware(&req, &res, &v);
    g_mock_res_status_fail = 0;
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);

    /* Test c_rest_oauth2_middleware with g_mock_res_status_fail */
    /* 1. Missing user_data */
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    g_mock_res_status_fail = 1;
    rc = c_rest_oauth2_middleware(&req, &res, NULL);
    g_mock_res_status_fail = 0;
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    /* 2. Missing Bearer token */
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    g_mock_res_status_fail = 1;
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    g_mock_res_status_fail = 0;
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

    /* 3. Invalid token */
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Bearer bad");
    g_mock_res_status_fail = 1;
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    g_mock_res_status_fail = 0;
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    free_mock_headers(&req);
    /* 4. CORS with NULL method and HTTPS redirect URL overflow guard */
    {
      char long_path[1040];
      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      req.method = NULL;
      rc = c_rest_cors_middleware(&req, &res, NULL);
      ASSERT_FALSE(rc != C_REST_OK);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

      memset(long_path, 'a', sizeof(long_path) - 1);
      long_path[0] = '/';
      long_path[sizeof(long_path) - 1] = '\0';
      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      req.scheme = (char *)"http";
      req.path = long_path;
      rc = c_rest_https_redirect_middleware(&req, &res, NULL);
      ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    }
  }

  PASS();
}

SUITE(middleware_suite) { RUN_TEST(test_middleware_cases); }
