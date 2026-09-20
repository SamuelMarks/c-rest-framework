/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_middleware.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

static int mock_verify_bearer_ok(const char *token, void **ctx) {
  int is_good = (strcmp(token, "good_token") == 0);
  *ctx = (void *)0x123;
  return is_good ? C_REST_OK : C_REST_ERROR_GENERIC;
}

static int mock_verify_basic_ok(const char *user, const char *pass,
                                void **ctx) {
  int is_good = (strcmp(user, "alice") == 0 && strcmp(pass, "wonderland") == 0);
  *ctx = (void *)0x456;
  return is_good ? C_REST_OK : C_REST_ERROR_GENERIC;
}

static int mock_verify_oauth2_ok(const char *token, void **ctx) {
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

static void test_coverage(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_auth_verifier v = {0, 0};
  int i;
  union {
    void *ptr;
    int (*func)(const char *, void **);
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
    req.method = "GET";
    c_rest_cors_middleware(&req, &res, NULL);
    c_rest_response_cleanup(&res);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    c_rest_hsts_middleware(&req, &res, NULL);
    c_rest_response_cleanup(&res);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    req.scheme = "http";
    req.path = "/hello";
    c_rest_https_redirect_middleware(&req, &res, NULL);
    c_rest_response_cleanup(&res);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    c_rest_auth_middleware(&req, &res, NULL);
    c_rest_response_cleanup(&res);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Basic YWxpY2U6d29uZGVybGFuZA==");
    c_rest_auth_middleware(&req, &res, &v);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    c_rest_auth_middleware(&req, &res, &v);
    c_rest_response_cleanup(&res);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Basic Ym9iOmJhZA=="); /* bob:bad */
    c_rest_auth_middleware(&req, &res, &v);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Bearer good_token");
    c_rest_auth_middleware(&req, &res, &v);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Bearer bad_token");
    c_rest_auth_middleware(&req, &res, &v);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);

    {
      struct c_rest_auth_verifier empty_v = {0, 0};
      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      mock_add_header(&req, "Authorization", "Basic YWxpY2U6d29uZGVybGFuZA==");
      c_rest_auth_middleware(&req, &res, &empty_v);
      c_rest_response_cleanup(&res);
      free_mock_headers(&req);

      memset(&req, 0, sizeof(req));
      memset(&res, 0, sizeof(res));
      mock_add_header(&req, "Authorization", "Bearer good_token");
      c_rest_auth_middleware(&req, &res, &empty_v);
      c_rest_response_cleanup(&res);
      free_mock_headers(&req);
    }

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Bearer valid_oauth");
    c_rest_oauth2_middleware(&req, &res, u.ptr);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    mock_add_header(&req, "Authorization", "Bearer bad");
    c_rest_oauth2_middleware(&req, &res, u.ptr);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    c_rest_oauth2_middleware(&req, &res, NULL);
    c_rest_response_cleanup(&res);

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    c_rest_oauth2_middleware(&req, &res, u.ptr);
    c_rest_response_cleanup(&res);

    g_crf_malloc_hook = NULL;
  }
}

int test_middleware_suite(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  c_rest_error_t rc;
  int failed = 0;

  printf("Testing cors middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_cors_middleware(NULL, &res, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);
  rc = c_rest_cors_middleware(&req, NULL, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  req.method = "GET";
  rc = c_rest_cors_middleware(&req, &res, NULL);
  failed += (rc != C_REST_OK);

  req.method = "OPTIONS";
  rc = c_rest_cors_middleware(&req, &res, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);
  failed += (res.status_code != 204);
  c_rest_response_cleanup(&res);
  memset(&req, 0, sizeof(req));

  printf("Testing logger middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_logger_middleware(NULL, &res, NULL);
  failed += (rc != C_REST_OK);
  req.method = "GET";
  req.path = "/test";
  rc = c_rest_logger_middleware(&req, &res, NULL);
  failed += (rc != C_REST_OK);
  req.method = "GET";
  req.path = NULL;
  rc = c_rest_logger_middleware(&req, &res, NULL);
  failed += (rc != C_REST_OK);
  req.method = NULL;
  rc = c_rest_logger_middleware(&req, &res, NULL);
  failed += (rc != C_REST_OK);
  c_rest_response_cleanup(&res);
  memset(&req, 0, sizeof(req));

  printf("Testing static middleware...\n");
  rc = c_rest_static_middleware(NULL, NULL, NULL);
  failed += (rc != C_REST_OK);

  printf("Testing hsts middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_hsts_middleware(&req, NULL, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);
  rc = c_rest_hsts_middleware(NULL, &res, NULL);
  failed += (rc != C_REST_OK);
  rc = c_rest_hsts_middleware(&req, &res, NULL);
  failed += (rc != C_REST_OK);
  c_rest_response_cleanup(&res);
  memset(&req, 0, sizeof(req));

  printf("Testing https redirect middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_https_redirect_middleware(NULL, &res, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);
  rc = c_rest_https_redirect_middleware(&req, NULL, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);
  rc = c_rest_https_redirect_middleware(&req, &res, NULL);
  failed += (rc != C_REST_OK);

  req.scheme = "https";
  req.path = "/hello";
  rc = c_rest_https_redirect_middleware(&req, &res, NULL);
  failed += (rc != C_REST_OK);

  req.scheme = "http";
  req.path = "/hello";
  rc = c_rest_https_redirect_middleware(&req, &res, NULL);
  failed += (res.status_code != 301);
  c_rest_response_cleanup(&res);

  req.scheme = "http";
  req.path = NULL;
  mock_add_header(&req, "Host", "example.com");
  rc = c_rest_https_redirect_middleware(&req, &res, NULL);
  failed += (res.status_code != 301);
  c_rest_response_cleanup(&res);
  free_mock_headers(&req);
  memset(&req, 0, sizeof(req));

  req.scheme = "http";
  req.path = NULL;
  mock_add_header(&req, "Host", NULL);
  rc = c_rest_https_redirect_middleware(&req, &res, NULL);
  failed += (res.status_code != 301);
  c_rest_response_cleanup(&res);
  free_mock_headers(&req);
  memset(&req, 0, sizeof(req));

  printf("Testing auth middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_auth_middleware(NULL, &res, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);
  rc = c_rest_auth_middleware(&req, NULL, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_auth_middleware(&req, &res, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);
  failed += (res.status_code != 500);
  c_rest_response_cleanup(&res);

  {
    struct c_rest_auth_verifier v = {0, 0};

    /* Missing auth header entirely */
    rc = c_rest_auth_middleware(&req, &res, &v);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 401);
    c_rest_response_cleanup(&res);

    /* Bearer provided but verifier missing */
    mock_add_header(&req, "Authorization", "Bearer token");
    rc = c_rest_auth_middleware(&req, &res, &v);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 500);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    /* Basic provided but verifier missing */
    mock_add_header(&req, "Authorization", "Basic YWxpY2U6d29uZGVybGFuZA==");
    rc = c_rest_auth_middleware(&req, &res, &v);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 500);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    v.verify_bearer = mock_verify_bearer_ok;
    v.verify_basic = mock_verify_basic_ok;

    /* Basic provided and valid */
    mock_add_header(&req, "Authorization", "Basic YWxpY2U6d29uZGVybGFuZA==");
    rc = c_rest_auth_middleware(&req, &res, &v);
    failed += (rc != C_REST_OK);
    failed += (req.auth_context != (void *)0x456);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    /* Basic provided and invalid */
    mock_add_header(&req, "Authorization", "Basic Ym9iOmJhZA==");
    rc = c_rest_auth_middleware(&req, &res, &v);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 401);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    /* Bearer provided and valid */
    mock_add_header(&req, "Authorization", "Bearer good_token");
    rc = c_rest_auth_middleware(&req, &res, &v);
    failed += (rc != C_REST_OK);
    failed += (req.auth_context != (void *)0x123);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    /* Bearer provided and invalid */
    mock_add_header(&req, "Authorization", "Bearer bad_token");
    rc = c_rest_auth_middleware(&req, &res, &v);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 401);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));
  }

  printf("Testing oauth2 middleware...\n");
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  rc = c_rest_oauth2_middleware(NULL, &res, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);
  rc = c_rest_oauth2_middleware(&req, NULL, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_oauth2_middleware(&req, &res, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);
  failed += (res.status_code != 500);
  c_rest_response_cleanup(&res);

  {
    union {
      void *ptr;
      int (*func)(const char *, void **);
    } u;
    u.func = mock_verify_oauth2_ok;

    /* Missing header */
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 401);
    c_rest_response_cleanup(&res);
    memset(&req, 0, sizeof(req));

    /* Invalid header */
    mock_add_header(&req, "Authorization", "Bearer bad");
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 401);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));

    /* Valid header */
    mock_add_header(&req, "Authorization", "Bearer valid_oauth");
    rc = c_rest_oauth2_middleware(&req, &res, u.ptr);
    failed += (rc != C_REST_OK);
    failed += (req.auth_context != (void *)0x789);
    c_rest_response_cleanup(&res);
    free_mock_headers(&req);
    memset(&req, 0, sizeof(req));
  }

  test_coverage();

  {
    const char *msgs[2];
    msgs[0] = "test_middleware_suite passed\n";
    msgs[1] = "test_middleware_suite failed\n";
    printf("%s", msgs[failed != 0]);
  }

  return failed;
}
