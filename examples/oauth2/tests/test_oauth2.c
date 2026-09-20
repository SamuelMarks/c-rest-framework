#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "greatest.h"
#include "greatest_clean.h"
#include "oauth2_client.h"
#include "oauth2_server.h"
#include "c_orm_db.h"
#include "c_orm_sqlite.h"
#include "c_orm_oauth2.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_modality.h"
#include "c_rest_openapi.h"
#include "c_rest_tls.h"
#include "c_rest_testing_mocks.h"
#include <c_abstract_http/http_types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
/* clang-format on */

struct c_rest_route_node;
struct c_rest_middleware_chain;
struct c_rest_router {
  struct c_rest_route_node *root;
  struct c_rest_middleware_chain *middlewares;
  struct c_rest_middleware_chain *post_middlewares;
  struct c_rest_openapi_spec *openapi_spec;
};

struct c_rest_client_context {
  struct HttpClient client;
};

static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

static int g_test_alloc_count = 0;
static int g_test_fail_at = 0;

static void *fail_malloc_at_n(size_t size) {
  g_test_alloc_count++;
  if (g_test_alloc_count == g_test_fail_at) {
    g_test_fail_at = 0;
    return NULL;
  }
  return malloc(size);
}

static size_t g_fail_size = 0;
static void *fail_malloc_size(size_t size) {
  if (g_fail_size != 0 && size == g_fail_size) {
    return NULL;
  }
  return malloc(size);
}

static size_t g_target_size = 0;
static int g_fail_count_for_size = 0;
static int g_current_count_for_size = 0;

static void *fail_malloc_size_n(size_t size) {
  if (g_target_size != 0 && size == g_target_size) {
    g_current_count_for_size++;
    if (g_current_count_for_size == g_fail_count_for_size) {
      return NULL;
    }
  }
  return malloc(size);
}

static int g_mock_client_send_null = 0;
static int g_mock_client_send_err = 0;
static int g_mock_client_send_err_with_res = 0;
static int g_mock_client_status = 200;
static const char *g_mock_client_body =
    "{\"access_token\":\"token_abc\",\"expires_in\":3600}";

static c_abstract_http_error_t
mock_oauth2_http_send(struct HttpTransportContext *ctx,
                      const struct HttpRequest *req,
                      struct HttpResponse **res) {
  struct HttpResponse *r;
  (void)ctx;
  (void)req;

  if (g_mock_client_send_err) {
    return 1;
  }
  if (g_mock_client_send_err_with_res) {
    r = (struct HttpResponse *)malloc(sizeof(struct HttpResponse));
    memset(r, 0, sizeof(*r));
    *res = r;
    return 1;
  }
  if (g_mock_client_send_null) {
    *res = NULL;
    return 0;
  }

  r = (struct HttpResponse *)malloc(sizeof(struct HttpResponse));
  memset(r, 0, sizeof(*r));
  r->status_code = g_mock_client_status;

  if (g_mock_client_body) {
    size_t blen = strlen(g_mock_client_body);
    r->body_len = blen;
    r->body = malloc(blen + 1);
    memcpy(r->body, g_mock_client_body, blen + 1);
  }

  *res = r;
  return 0;
}

static c_rest_error_t set_test_env(const char *key, const char *val) {
#if defined(_WIN32)
  char buf[256];
#if defined(_MSC_VER)
  sprintf_s(buf, sizeof(buf), "%s=%s", key, val ? val : "");
#else
  sprintf(buf, "%s=%s", key, val ? val : "");
#endif
  _putenv(buf);
#else
  if (val) {
    setenv(key, val, 1);
  } else {
    unsetenv(key);
  }
#endif
  return C_REST_OK;
}

/* Mock declarations for main.c */
int oauth2_app_main(int argc, char **argv);

static void dummy_exit(int s) { (void)s; }

static int g_intercept_run = 0;
static c_rest_error_t mock_main_c_rest_run(struct c_rest_context *ctx) {
  (void)ctx;
  if (g_intercept_run == 1) {
    return C_REST_ERROR_GENERIC;
  }
  return C_REST_OK;
}

static int g_mock_main_tls_init_fail = 0;
static c_rest_error_t mock_main_c_rest_tls_init(void) {
  if (g_mock_main_tls_init_fail) {
    return C_REST_ERROR_GENERIC;
  }
  return c_rest_tls_init();
}

static int g_mock_main_tls_ctx_init_fail = 0;
static c_rest_error_t
mock_main_c_rest_tls_context_init(struct c_rest_tls_context **ctx) {
  if (g_mock_main_tls_ctx_init_fail) {
    return C_REST_ERROR_GENERIC;
  }
  return c_rest_tls_context_init(ctx);
}

static int g_mock_main_router_init_fail = 0;
static c_rest_error_t mock_main_c_rest_router_init(c_rest_router **r) {
  if (g_mock_main_router_init_fail) {
    return C_REST_ERROR_GENERIC;
  }
  return c_rest_router_init(r);
}

static int g_mock_main_server_init_fail = 0;
static c_rest_error_t mock_main_oauth2_server_init(c_rest_router *r,
                                                   c_orm_db_t *d) {
  if (g_mock_main_server_init_fail) {
    return 1;
  }
  return oauth2_server_init(r, d);
}

static int g_mock_main_tls_load_cert_fail = 0;
static c_rest_error_t
mock_main_c_rest_tls_load_cert(struct c_rest_tls_context *ctx,
                               const char *cert_path) {
  if (g_mock_main_tls_load_cert_fail) {
    return C_REST_ERROR_GENERIC;
  }
  return c_rest_tls_load_cert(ctx, cert_path);
}

static int g_mock_main_tls_load_key_fail = 0;
static c_rest_error_t
mock_main_c_rest_tls_load_key(struct c_rest_tls_context *ctx,
                              const char *key_path) {
  if (g_mock_main_tls_load_key_fail) {
    return C_REST_ERROR_GENERIC;
  }
  return c_rest_tls_load_key(ctx, key_path);
}

static int g_mock_main_enable_openapi_fail = 0;
static c_rest_error_t mock_main_c_rest_enable_openapi(c_rest_router *r,
                                                      const char *p) {
  if (g_mock_main_enable_openapi_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_enable_openapi(r, p);
}

static int g_mock_main_enable_swagger_ui_fail = 0;
static c_rest_error_t mock_main_c_rest_enable_swagger_ui(c_rest_router *r,
                                                         const char *p,
                                                         const char *u) {
  if (g_mock_main_enable_swagger_ui_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_enable_swagger_ui(r, p, u);
}

static int g_mock_main_set_router_fail = 0;
static c_rest_error_t mock_main_c_rest_set_router(struct c_rest_context *c,
                                                  c_rest_router *r) {
  if (g_mock_main_set_router_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_set_router(c, r);
}

static int g_mock_main_tls_ctx_destroy_fail = 0;
static c_rest_error_t
mock_main_c_rest_tls_context_destroy(struct c_rest_tls_context *ctx) {
  if (g_mock_main_tls_ctx_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_tls_context_destroy(ctx);
}

static int g_mock_main_router_destroy_fail = 0;
static c_rest_error_t mock_main_c_rest_router_destroy(c_rest_router *r) {
  if (g_mock_main_router_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_destroy(r);
}

static int g_mock_main_ctx_destroy_fail = 0;
static c_rest_error_t mock_main_c_rest_destroy(struct c_rest_context *c) {
  if (g_mock_main_ctx_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_destroy(c);
}

#define exit(s) dummy_exit(s)
#define c_rest_run mock_main_c_rest_run
#define c_rest_tls_init mock_main_c_rest_tls_init
#define c_rest_tls_context_init mock_main_c_rest_tls_context_init
#define c_rest_tls_load_cert mock_main_c_rest_tls_load_cert
#define c_rest_tls_load_key mock_main_c_rest_tls_load_key
#define c_rest_router_init mock_main_c_rest_router_init
#define oauth2_server_init mock_main_oauth2_server_init
#define c_rest_enable_openapi mock_main_c_rest_enable_openapi
#define c_rest_enable_swagger_ui mock_main_c_rest_enable_swagger_ui
#define c_rest_set_router mock_main_c_rest_set_router
#define c_rest_tls_context_destroy mock_main_c_rest_tls_context_destroy
#define c_rest_router_destroy mock_main_c_rest_router_destroy
#define c_rest_destroy mock_main_c_rest_destroy
#define main oauth2_app_main
#include "../src/main.c"
#undef main
#undef c_rest_destroy
#undef c_rest_router_destroy
#undef c_rest_tls_context_destroy
#undef c_rest_set_router
#undef c_rest_enable_swagger_ui
#undef c_rest_enable_openapi
#undef oauth2_server_init
#undef c_rest_router_init
#undef c_rest_tls_load_key
#undef c_rest_tls_load_cert
#undef c_rest_tls_context_init
#undef c_rest_tls_init
#undef c_rest_run
#undef exit

SUITE(oauth2_suite);

TEST test_client_init_and_cleanup(void) {
  c_orm_db_t *db = NULL;
  c_rest_client_context *ctx = NULL;

  c_orm_sqlite_connect(":memory:", &db);

  /* NULL arguments */
  ASSERT_EQ(1, oauth2_client_init(NULL, "cid", "sec", db));
  ASSERT_EQ(1,
            oauth2_client_init("http://127.0.0.1:8080/token", NULL, "sec", db));
  ASSERT_EQ(1,
            oauth2_client_init("http://127.0.0.1:8080/token", "cid", NULL, db));

  /* Cleanup when uninitialized */
  ASSERT_EQ(0, oauth2_client_cleanup());

  /* Context getter NULL */
  ASSERT_EQ(1, oauth2_client_get_context(NULL));

  /* OOM branches in oauth2_client_init */
  g_test_alloc_count = 0;
  g_test_fail_at = 1;
  g_crf_malloc_hook = fail_malloc_at_n;
  ASSERT_EQ(
      1, oauth2_client_init("http://127.0.0.1:8080/token", "cid", "sec", db));
  g_crf_malloc_hook = NULL;

  g_test_alloc_count = 0;
  g_test_fail_at = 2;
  g_crf_malloc_hook = fail_malloc_at_n;
  ASSERT_EQ(
      1, oauth2_client_init("http://127.0.0.1:8080/token", "cid", "sec", db));
  g_crf_malloc_hook = NULL;

  g_test_alloc_count = 0;
  g_test_fail_at = 3;
  g_crf_malloc_hook = fail_malloc_at_n;
  ASSERT_EQ(
      1, oauth2_client_init("http://127.0.0.1:8080/token", "cid", "sec", db));
  g_crf_malloc_hook = NULL;

  g_test_alloc_count = 0;
  g_test_fail_at = 4;
  g_crf_malloc_hook = fail_malloc_at_n;
  ASSERT_EQ(
      1, oauth2_client_init("http://127.0.0.1:8080/token", "cid", "sec", db));
  g_crf_malloc_hook = NULL;

  /* Normal init */
  ASSERT_EQ(
      0, oauth2_client_init("http://127.0.0.1:8080/token", "cid", "sec", db));
  ASSERT_EQ(0, oauth2_client_get_context(&ctx));
  ASSERT_NEQ(NULL, ctx);

  /* Normal cleanup */
  ASSERT_EQ(0, oauth2_client_cleanup());

  {
    void *p;
    g_fail_size = 0;
    p = fail_malloc_size(10);
    free(p);
    g_fail_size = 10;
    p = fail_malloc_size(5);
    free(p);
    g_fail_size = 0;

    g_target_size = 0;
    p = fail_malloc_size_n(10);
    free(p);
    g_target_size = 10;
    g_fail_count_for_size = 2;
    p = fail_malloc_size_n(5);
    free(p);
    p = fail_malloc_size_n(10);
    free(p);
    p = fail_malloc_size_n(10);
    free(p);
    g_target_size = 0;
  }

  db->vtable->disconnect(db);
  PASS();
}

TEST test_client_password_grant(void) {
  char *access_token = NULL;
  int expires_in = 0;
  c_rest_client_context *ctx = NULL;

  /* Invalid arguments */
  ASSERT_EQ(1, oauth2_client_password_grant(NULL, "pass", &access_token,
                                            &expires_in));
  ASSERT_EQ(1, oauth2_client_password_grant("user", NULL, &access_token,
                                            &expires_in));
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", NULL, &expires_in));
  ASSERT_EQ(1,
            oauth2_client_password_grant("user", "pass", &access_token, NULL));

  /* Uninitialized client */
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));

  /* Initialize client */
  ASSERT_EQ(
      0, oauth2_client_init("http://127.0.0.1:8080/token", "cid", "sec", NULL));
  ASSERT_EQ(0, oauth2_client_get_context(&ctx));
  ASSERT_NEQ(NULL, ctx);

  /* Hook transport send function */
  ctx->client.send = mock_oauth2_http_send;

  /* Test c_rest_client_build_auth_basic OOM failure */
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  g_crf_malloc_hook = NULL;

  /* Test mock_send returns error */
  g_mock_client_send_err = 1;
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  g_mock_client_send_err = 0;

  /* Test mock_send returns error with res non-null */
  g_mock_client_send_err_with_res = 1;
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  /* Test mock_send returns error with res non-null and free fails */
  g_mock_client_fail = 10;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      oauth2_client_password_grant("user", "pass", &access_token, &expires_in));
  g_mock_client_fail = 0;
  g_mock_client_send_err_with_res = 0;

  /* Test mock_send returns NULL */
  g_mock_client_send_null = 1;
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  g_mock_client_send_null = 0;

  /* Test non-200 status code */
  g_mock_client_status = 401;
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  /* Test non-200 status code and free fails */
  g_mock_client_fail = 10;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      oauth2_client_password_grant("user", "pass", &access_token, &expires_in));
  g_mock_client_fail = 0;
  g_mock_client_status = 200;

  /* Test invalid JSON response */
  g_mock_client_body = "invalid-json";
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  /* Test invalid JSON response and free fails */
  g_mock_client_fail = 10;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      oauth2_client_password_grant("user", "pass", &access_token, &expires_in));
  g_mock_client_fail = 0;

  /* Test non-object JSON response */
  g_mock_client_body = "\"not-an-object\"";
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  /* Test non-object JSON response and free fails */
  g_mock_client_fail = 10;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      oauth2_client_password_grant("user", "pass", &access_token, &expires_in));
  g_mock_client_fail = 0;

  /* Test JSON response missing access_token */
  g_mock_client_body = "{\"expires_in\":3600}";
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  /* Test JSON response missing access_token and free fails */
  g_mock_client_fail = 10;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      oauth2_client_password_grant("user", "pass", &access_token, &expires_in));
  g_mock_client_fail = 0;

  /* Test OOM when allocating out_access_token */
  g_mock_client_body = "{\"access_token\":\"unique_token_77_bytes_"
                       "12345678901234567890123456789012345678901234567890\","
                       "\"expires_in\":3600}";
  g_fail_size = strlen("unique_token_77_bytes_"
                       "12345678901234567890123456789012345678901234567890") +
                1;
  g_crf_malloc_hook = fail_malloc_size;
  ASSERT_EQ(1, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  /* Test OOM when allocating out_access_token and free fails */
  g_mock_client_fail = 10;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      oauth2_client_password_grant("user", "pass", &access_token, &expires_in));
  g_mock_client_fail = 0;
  g_crf_malloc_hook = NULL;
  g_fail_size = 0;

  /* Test success path but free fails */
  g_mock_client_body =
      "{\"access_token\":\"valid_tok_123\",\"expires_in\":3600}";
  g_mock_client_fail = 10;
  ASSERT_EQ(
      C_REST_ERROR_GENERIC,
      oauth2_client_password_grant("user", "pass", &access_token, &expires_in));
  g_mock_client_fail = 0;
  CRF_FREE(access_token);
  access_token = NULL;

  /* Test success path */
  g_mock_client_body =
      "{\"access_token\":\"valid_tok_123\",\"expires_in\":3600}";
  ASSERT_EQ(0, oauth2_client_password_grant("user", "pass", &access_token,
                                            &expires_in));
  ASSERT_NEQ(NULL, access_token);
  ASSERT_STR_EQ("valid_tok_123", access_token);
  ASSERT_EQ(3600, expires_in);
  CRF_FREE(access_token);
  access_token = NULL;

  g_mock_client_fail = 11;
  ASSERT_EQ(C_REST_ERROR_GENERIC, oauth2_client_cleanup());
  g_mock_client_fail = 0;
  ASSERT_EQ(0, oauth2_client_cleanup());

  /* Cover mock_oauth2_http_send with g_mock_client_body == NULL */
  {
    struct HttpResponse *mres = NULL;
    g_mock_client_body = NULL;
    mock_oauth2_http_send(NULL, NULL, &mres);
    free(mres);
    g_mock_client_body =
        "{\"access_token\":\"valid_tok_123\",\"expires_in\":3600}";
  }

  PASS();
}

TEST test_server_init(void) {
  c_orm_db_t *db = NULL;
  c_rest_router *router = NULL;
  c_rest_router *router_with_openapi = NULL;
  struct c_rest_openapi_spec *saved_spec = NULL;

  c_orm_sqlite_connect(":memory:", &db);
  ASSERT_EQ(C_REST_OK, c_rest_router_init(&router));
  ASSERT_EQ(C_REST_OK, c_rest_router_init(&router_with_openapi));
  ASSERT_EQ(C_REST_OK,
            c_rest_enable_openapi(router_with_openapi, "/api/v0/openapi.json"));

  /* Null arguments */
  ASSERT_EQ(1, oauth2_server_init(NULL, db));
  ASSERT_EQ(1, oauth2_server_init(router, NULL));

  /* Server init with spec == NULL */
  saved_spec = router->openapi_spec;
  router->openapi_spec = NULL;
  ASSERT_EQ(0, oauth2_server_init(router, db));
  router->openapi_spec = saved_spec;

  /* Server init WITH OpenAPI spec initialized on router */
  ASSERT_EQ(0, oauth2_server_init(router_with_openapi, db));

  ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router));
  ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router_with_openapi));
  db->vtable->disconnect(db);
  PASS();
}

static c_orm_driver_vtable_t g_real_vtable;
static int g_mock_user_verify_fail = 0;

static c_orm_error_t mock_db_prepare_fail(c_orm_db_t *db, const char *sql,
                                          c_orm_query_t **query) {
  if (g_mock_user_verify_fail) {
    if (sql && strstr(sql, "users")) {
      return 1;
    }
    return g_real_vtable.prepare(db, sql, query);
  }
  (void)db;
  (void)sql;
  (void)query;
  return 1;
}

static c_rest_error_t clear_test_req(struct c_rest_request *req) {
  if (req) {
    c_rest_error_t rc;
    req->headers = NULL;
    req->body = NULL;
    rc = c_rest_request_cleanup(req);
    memset(req, 0, sizeof(*req));
    return rc;
  }
  return C_REST_OK;
}

TEST test_server_handlers_complete(void) {
  c_orm_db_t *db = NULL;
  c_orm_db_t mock_fail_db;
  c_orm_driver_vtable_t fail_vtable;
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header hdr;
  struct c_rest_header auth_hdr;

  c_orm_sqlite_connect(":memory:", &db);
  c_orm_oauth2_create_tables(db);

  memset(&mock_fail_db, 0, sizeof(mock_fail_db));
  memset(&fail_vtable, 0, sizeof(fail_vtable));
  g_real_vtable = *db->vtable;
  fail_vtable = *db->vtable;
  fail_vtable.prepare = mock_db_prepare_fail;
  mock_fail_db = *db;
  mock_fail_db.vtable = &fail_vtable;

  clear_test_req(NULL);
  mock_db_prepare_fail(NULL, NULL, NULL);
  g_mock_user_verify_fail = 1;
  mock_db_prepare_fail(db, NULL, NULL);
  mock_db_prepare_fail(db, "SELECT * FROM clients", NULL);
  g_mock_user_verify_fail = 0;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* ------------------- Register Client Handler ------------------- */
  ASSERT_EQ(1, oauth2_register_client_handler(NULL, &res, db));
  ASSERT_EQ(1, oauth2_register_client_handler(&req, NULL, db));
  ASSERT_EQ(1, oauth2_register_client_handler(&req, &res, NULL));

  /* URL-encoded parse failure */
  clear_test_req(&req);
  req.body = "client_id=myid";
  req.body_len = strlen(req.body);
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(0, oauth2_register_client_handler(&req, &res, db));
  g_crf_malloc_hook = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing client_secret only */
  clear_test_req(&req);
  req.body = "client_id=myid";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_client_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing client_id only */
  clear_test_req(&req);
  req.body = "client_secret=mysecret";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_client_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Prepare failure */
  clear_test_req(&req);
  req.body = "client_id=myid&client_secret=mysecret";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_client_handler(&req, &res, &mock_fail_db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Successful client registration */
  clear_test_req(&req);
  req.body = "client_id=myid&client_secret=mysecret";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_client_handler(&req, &res, db));
  ASSERT_EQ(201, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Duplicate client registration -> step fails */
  clear_test_req(&req);
  req.body = "client_id=myid&client_secret=mysecret";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_client_handler(&req, &res, db));
  ASSERT_EQ(400, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* ------------------- Register User Handler ------------------- */
  ASSERT_EQ(1, oauth2_register_user_handler(NULL, &res, db));
  ASSERT_EQ(1, oauth2_register_user_handler(&req, NULL, db));
  ASSERT_EQ(1, oauth2_register_user_handler(&req, &res, NULL));

  /* URL-encoded parse failure */
  clear_test_req(&req);
  req.body = "username=myuser";
  req.body_len = strlen(req.body);
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(0, oauth2_register_user_handler(&req, &res, db));
  g_crf_malloc_hook = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing password only */
  clear_test_req(&req);
  req.body = "username=myuser";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_user_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing username only */
  clear_test_req(&req);
  req.body = "password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_user_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Prepare failure */
  clear_test_req(&req);
  req.body = "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_user_handler(&req, &res, &mock_fail_db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Successful user registration */
  clear_test_req(&req);
  req.body = "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_user_handler(&req, &res, db));
  ASSERT_EQ(201, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Duplicate user registration -> step fails */
  clear_test_req(&req);
  req.body = "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_register_user_handler(&req, &res, db));
  ASSERT_EQ(400, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* ------------------- Token Handler ------------------- */
  ASSERT_EQ(1, oauth2_token_handler(NULL, &res, db));
  ASSERT_EQ(1, oauth2_token_handler(&req, NULL, db));
  ASSERT_EQ(1, oauth2_token_handler(&req, &res, NULL));

  /* URL-encoded parse failure */
  clear_test_req(&req);
  req.body = "grant_type=password";
  req.body_len = strlen(req.body);
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  g_crf_malloc_hook = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing grant_type */
  clear_test_req(&req);
  req.body = "client_id=myid";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Unsupported grant_type */
  clear_test_req(&req);
  req.body = "grant_type=client_credentials";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing client credentials (no body params, no auth header) */
  clear_test_req(&req);
  req.body = "grant_type=password&username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing client_secret in body */
  clear_test_req(&req);
  req.body = "grant_type=password&client_id=myid";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing client_id in body */
  clear_test_req(&req);
  req.body = "grant_type=password&client_secret=mysecret";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Invalid client credentials in body */
  clear_test_req(&req);
  req.body = "grant_type=password&client_id=bad&client_secret=bad&username="
             "myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* c_orm_oauth2_verify_client fails with error (err != 0) */
  clear_test_req(&req);
  req.body = "grant_type=password&client_id=myid&client_secret=mysecret&"
             "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, &mock_fail_db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Invalid client credentials via Basic Auth header */
  clear_test_req(&req);
  auth_hdr.key = "Authorization";
  auth_hdr.value = "Basic YmFkOmJhZA=="; /* bad:bad */
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;
  req.body = "grant_type=password&username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.headers = NULL;

  /* Valid client credentials, missing username in body */
  clear_test_req(&req);
  req.body = "grant_type=password&client_id=myid&client_secret=mysecret&"
             "password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Valid client credentials, missing password in body */
  clear_test_req(&req);
  req.body = "grant_type=password&client_id=myid&client_secret=mysecret&"
             "username=myuser";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Valid client credentials via Basic auth, missing username */
  clear_test_req(&req);
  auth_hdr.value = "Basic bXlpZDpteXNlY3JldA=="; /* myid:mysecret */
  req.headers = &auth_hdr;
  req.body = "grant_type=password";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.headers = NULL;

  /* Valid client credentials, invalid user credentials */
  clear_test_req(&req);
  req.body = "grant_type=password&client_id=myid&client_secret=mysecret&"
             "username=baduser&password=badpass";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Valid client credentials via Basic auth, invalid user */
  clear_test_req(&req);
  req.headers = &auth_hdr;
  req.body = "grant_type=password&username=baduser&password=badpass";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.headers = NULL;

  /* c_orm_user_verify_credentials fails with error (err != 0) */
  clear_test_req(&req);
  req.body = "grant_type=password&client_id=myid&client_secret=mysecret&"
             "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  g_mock_user_verify_fail = 1;
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, &mock_fail_db));
  g_mock_user_verify_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* c_orm_user_verify_credentials fails with error (err != 0) via Basic Auth */
  clear_test_req(&req);
  auth_hdr.value = "Basic bXlpZDpteXNlY3JldA=="; /* myid:mysecret */
  req.headers = &auth_hdr;
  req.body = "grant_type=password&username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  g_mock_user_verify_fail = 1;
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, &mock_fail_db));
  g_mock_user_verify_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.headers = NULL;

  /* Valid client and user, OOM on token_str */
  clear_test_req(&req);
  req.body = "grant_type=password&client_id=myid&client_secret=mysecret&"
             "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&req));
  g_target_size = 64;
  g_fail_count_for_size = 1;
  g_current_count_for_size = 0;
  g_crf_malloc_hook = fail_malloc_size_n;
  ASSERT_EQ(1, oauth2_token_handler(&req, &res, db));
  g_crf_malloc_hook = NULL;
  g_target_size = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Valid client and user via Basic auth, OOM on token_str */
  clear_test_req(&req);
  auth_hdr.value = "Basic bXlpZDpteXNlY3JldA=="; /* myid:mysecret */
  req.headers = &auth_hdr;
  req.body = "grant_type=password&username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&req));
  g_target_size = 64;
  g_fail_count_for_size = 1;
  g_current_count_for_size = 0;
  g_crf_malloc_hook = fail_malloc_size_n;
  ASSERT_EQ(1, oauth2_token_handler(&req, &res, db));
  g_crf_malloc_hook = NULL;
  g_target_size = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.headers = NULL;

  /* Valid client and user, OOM on refresh_str */
  clear_test_req(&req);
  req.body = "grant_type=password&client_id=myid&client_secret=mysecret&"
             "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&req));
  g_target_size = 64;
  g_fail_count_for_size = 2;
  g_current_count_for_size = 0;
  g_crf_malloc_hook = fail_malloc_size_n;
  ASSERT_EQ(1, oauth2_token_handler(&req, &res, db));
  g_crf_malloc_hook = NULL;
  g_target_size = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Valid client and user via Basic auth, OOM on refresh_str */
  clear_test_req(&req);
  auth_hdr.value = "Basic bXlpZDpteXNlY3JldA=="; /* myid:mysecret */
  req.headers = &auth_hdr;
  req.body = "grant_type=password&username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&req));
  g_target_size = 64;
  g_fail_count_for_size = 2;
  g_current_count_for_size = 0;
  g_crf_malloc_hook = fail_malloc_size_n;
  ASSERT_EQ(1, oauth2_token_handler(&req, &res, db));
  g_crf_malloc_hook = NULL;
  g_target_size = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.headers = NULL;

  /* Valid client and user, SUCCESS via body params */
  clear_test_req(&req);
  req.body = "grant_type=password&client_id=myid&client_secret=mysecret&"
             "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  ASSERT_EQ(200, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Valid client via Basic Auth, SUCCESS */
  clear_test_req(&req);
  auth_hdr.value = "Basic bXlpZDpteXNlY3JldA=="; /* myid:mysecret */
  req.headers = &auth_hdr;
  req.body = "grant_type=password&username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_token_handler(&req, &res, db));
  ASSERT_EQ(200, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  req.headers = NULL;

  /* ------------------- Login Handler ------------------- */
  ASSERT_EQ(1, oauth2_login_handler(NULL, &res, db));
  ASSERT_EQ(1, oauth2_login_handler(&req, NULL, db));
  ASSERT_EQ(1, oauth2_login_handler(&req, &res, NULL));

  /* URL-encoded parse failure */
  clear_test_req(&req);
  req.body = "username=myuser";
  req.body_len = strlen(req.body);
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(0, oauth2_login_handler(&req, &res, db));
  g_crf_malloc_hook = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing password only */
  clear_test_req(&req);
  req.body = "username=myuser";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_login_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Missing username only */
  clear_test_req(&req);
  req.body = "password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_login_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Invalid credentials */
  clear_test_req(&req);
  req.body = "username=myuser&password=wrongpass";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_login_handler(&req, &res, db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* c_orm_user_verify_credentials fails with error (err != 0) */
  clear_test_req(&req);
  req.body = "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_login_handler(&req, &res, &mock_fail_db));
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* OOM on token_str */
  clear_test_req(&req);
  req.body = "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&req));
  g_target_size = 64;
  g_fail_count_for_size = 1;
  g_current_count_for_size = 0;
  g_crf_malloc_hook = fail_malloc_size_n;
  ASSERT_EQ(1, oauth2_login_handler(&req, &res, db));
  g_crf_malloc_hook = NULL;
  g_target_size = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* OOM on refresh_str */
  clear_test_req(&req);
  req.body = "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&req));
  g_target_size = 64;
  g_fail_count_for_size = 2;
  g_current_count_for_size = 0;
  g_crf_malloc_hook = fail_malloc_size_n;
  ASSERT_EQ(1, oauth2_login_handler(&req, &res, db));
  g_crf_malloc_hook = NULL;
  g_target_size = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Success login */
  clear_test_req(&req);
  req.body = "username=myuser&password=mypassword";
  req.body_len = strlen(req.body);
  ASSERT_EQ(0, oauth2_login_handler(&req, &res, db));
  ASSERT_EQ(200, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* ------------------- Secret Handler ------------------- */
  /* Missing bearer token */
  req.headers = NULL;
  ASSERT_EQ(0, oauth2_secret_handler(&req, &res, db));
  ASSERT_EQ(401, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Invalid bearer token */
  hdr.key = "Authorization";
  hdr.value = "Bearer nonexistent_token";
  hdr.next = NULL;
  req.headers = &hdr;
  ASSERT_EQ(0, oauth2_secret_handler(&req, &res, db));
  ASSERT_EQ(401, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Valid bearer token (from previous login/token handler) */
  hdr.value = "Bearer access_token_demo_123";
  ASSERT_EQ(0, oauth2_secret_handler(&req, &res, db));
  ASSERT_EQ(200, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Test secret handler with token containing user_id and scopes */
  {
    c_orm_oauth2_token_t custom_tok;
    memset(&custom_tok, 0, sizeof(custom_tok));
    custom_tok.access_token = "tok_with_user_and_scopes";
    custom_tok.refresh_token = "ref_custom";
    custom_tok.token_type = "bearer";
    custom_tok.expires_in = 3600;
    custom_tok.created_at = (int64_t)time(NULL);
    custom_tok.user_id = "myuser";
    custom_tok.scopes = "read write";
    c_orm_oauth2_save_token(db, &custom_tok);

    clear_test_req(&req);
    hdr.value = "Bearer tok_with_user_and_scopes";
    req.headers = &hdr;
    ASSERT_EQ(0, oauth2_secret_handler(&req, &res, db));
    ASSERT_EQ(200, res.status_code);
    c_rest_response_cleanup(&res);
    memset(&res, 0, sizeof(res));
  }

  /* Test secret handler with minimal token (NULL refresh_token, token_type,
   * user_id, scopes) */
  {
    c_orm_oauth2_token_t min_tok;
    memset(&min_tok, 0, sizeof(min_tok));
    min_tok.access_token = "min_token_abc";
    min_tok.expires_in = 3600;
    min_tok.created_at = (int64_t)time(NULL);
    c_orm_oauth2_save_token(db, &min_tok);

    clear_test_req(&req);
    hdr.value = "Bearer min_token_abc";
    req.headers = &hdr;
    ASSERT_EQ(0, oauth2_secret_handler(&req, &res, db));
    ASSERT_EQ(200, res.status_code);
    c_rest_response_cleanup(&res);
    memset(&res, 0, sizeof(res));
  }

  /* ------------------- Logout Handler ------------------- */
  /* Missing bearer */
  req.headers = NULL;
  ASSERT_EQ(0, oauth2_logout_handler(&req, &res, db));
  ASSERT_EQ(401, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Valid logout */
  hdr.value = "Bearer access_token_demo_123";
  req.headers = &hdr;
  ASSERT_EQ(0, oauth2_logout_handler(&req, &res, db));
  ASSERT_EQ(200, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Secret handler now fails after logout revocation */
  ASSERT_EQ(0, oauth2_secret_handler(&req, &res, db));
  ASSERT_EQ(401, res.status_code);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  db->vtable->disconnect(db);
  PASS();
}

TEST test_main_cli_complete(void) {
  const char *cert_path = "../../../tests/certs/server.crt";
  const char *key_path = "../../../tests/certs/server.key";

  char *args_help1[] = {"oauth2_cli", "--help"};
  char *args_help2[] = {"oauth2_cli", "-h"};
  char *args_help3[] = {"oauth2_cli", "-?"};
  char *args_help4[] = {"oauth2_cli", "/?"};

  char *args_t1[] = {"oauth2_cli", "--db-url"};
  char *args_t2[] = {"oauth2_cli", "--listen-addr"};
  char *args_t3[] = {"oauth2_cli", "--listen-port"};
  char *args_t4[] = {"oauth2_cli", "--tls-cert"};
  char *args_t5[] = {"oauth2_cli", "--tls-key"};

  char *args_full[11];
  char *args_tls_only[5];
  char *args_bad_db_tls[7];
  char *args_cert_only[3];
  char *args_key_only[3];

  char *args_bad_tls[] = {"oauth2_cli", "--tls-cert", "nonexistent.crt",
                          "--tls-key", "nonexistent.key"};

  char *args_bad_db[] = {"oauth2_cli", "--db-url", "/bad_dir/bad_file.db"};

  char *args_base[] = {"oauth2_cli"};

  args_full[0] = "oauth2_cli";
  args_full[1] = "--db-url";
  args_full[2] = ":memory:";
  args_full[3] = "--listen-addr";
  args_full[4] = "127.0.0.1";
  args_full[5] = "--listen-port";
  args_full[6] = "8080";
  args_full[7] = "--tls-cert";
  args_full[8] = (char *)cert_path;
  args_full[9] = "--tls-key";
  args_full[10] = (char *)key_path;

  args_tls_only[0] = "oauth2_cli";
  args_tls_only[1] = "--tls-cert";
  args_tls_only[2] = (char *)cert_path;
  args_tls_only[3] = "--tls-key";
  args_tls_only[4] = (char *)key_path;

  args_bad_db_tls[0] = "oauth2_cli";
  args_bad_db_tls[1] = "--db-url";
  args_bad_db_tls[2] = "/bad_dir/bad_file.db";
  args_bad_db_tls[3] = "--tls-cert";
  args_bad_db_tls[4] = (char *)cert_path;
  args_bad_db_tls[5] = "--tls-key";
  args_bad_db_tls[6] = (char *)key_path;

  args_cert_only[0] = "oauth2_cli";
  args_cert_only[1] = "--tls-cert";
  args_cert_only[2] = (char *)cert_path;

  args_key_only[0] = "oauth2_cli";
  args_key_only[1] = "--tls-key";
  args_key_only[2] = (char *)key_path;

  /* Signals */
  sig_handler(SIGINT);
  sig_handler(SIGTERM);
  handle_sigint(SIGINT);

  /* Test handle_sigint when g_ctx is non-NULL */
  {
    struct c_rest_context *sig_ctx = NULL;
    c_rest_init(C_REST_MODALITY_SINGLE_THREAD, &sig_ctx);
    g_ctx = sig_ctx;
    handle_sigint(SIGINT);
    ASSERT_EQ(C_REST_OK, c_rest_destroy(sig_ctx));
    g_ctx = NULL;
  }

  /* Help options */
  ASSERT_EQ(0, oauth2_app_main(2, args_help1));
  ASSERT_EQ(0, oauth2_app_main(2, args_help2));
  ASSERT_EQ(0, oauth2_app_main(2, args_help3));
  ASSERT_EQ(0, oauth2_app_main(2, args_help4));

  /* Trailing CLI flags missing value */
  g_intercept_run = 2;
  ASSERT_EQ(0, oauth2_app_main(2, args_t1));
  ASSERT_EQ(0, oauth2_app_main(2, args_t2));
  ASSERT_EQ(0, oauth2_app_main(2, args_t3));
  ASSERT_EQ(0, oauth2_app_main(2, args_t4));
  ASSERT_EQ(0, oauth2_app_main(2, args_t5));
  g_intercept_run = 0;

  /* Full CLI args, successful run */
  g_intercept_run = 2;
  ASSERT_EQ(0, oauth2_app_main(11, args_full));
  g_intercept_run = 0;

  /* Run failure */
  g_intercept_run = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_intercept_run = 0;

  /* Only tls_cert provided without tls_key */
  g_intercept_run = 2;
  ASSERT_EQ(0, oauth2_app_main(3, args_cert_only));
  g_intercept_run = 0;

  /* Only tls_key provided without tls_cert */
  g_intercept_run = 2;
  ASSERT_EQ(0, oauth2_app_main(3, args_key_only));
  g_intercept_run = 0;

  /* Bad TLS certs */
  g_mock_main_tls_load_cert_fail = 1;
  g_intercept_run = 2;
  ASSERT_EQ(0, oauth2_app_main(5, args_bad_tls));
  g_mock_main_tls_load_cert_fail = 0;
  g_intercept_run = 0;

  /* TLS key load failure */
  g_mock_main_tls_load_key_fail = 1;
  g_intercept_run = 2;
  ASSERT_EQ(0, oauth2_app_main(5, args_tls_only));
  g_mock_main_tls_load_key_fail = 0;
  g_intercept_run = 0;

  /* TLS init failure */
  g_mock_main_tls_init_fail = 1;
  g_intercept_run = 2;
  ASSERT_EQ(0, oauth2_app_main(5, args_tls_only));
  g_mock_main_tls_init_fail = 0;
  g_intercept_run = 0;

  /* TLS context init failure */
  g_mock_main_tls_ctx_init_fail = 1;
  g_intercept_run = 2;
  ASSERT_EQ(0, oauth2_app_main(5, args_tls_only));
  g_mock_main_tls_ctx_init_fail = 0;
  g_intercept_run = 0;

  /* Environment variables */
  set_test_env("OAUTH2_DB_URL", ":memory:");
  set_test_env("OAUTH2_LISTEN_ADDR", "127.0.0.1");
  set_test_env("OAUTH2_LISTEN_PORT", "8081");
  set_test_env("OAUTH2_TLS_CERT", "nonexistent.crt");
  set_test_env("OAUTH2_TLS_KEY", "nonexistent.key");
  g_intercept_run = 2;
  ASSERT_EQ(0, oauth2_app_main(1, args_base));
  g_intercept_run = 0;

  set_test_env("OAUTH2_DB_URL", NULL);
  set_test_env("OAUTH2_LISTEN_ADDR", NULL);
  set_test_env("OAUTH2_LISTEN_PORT", NULL);
  set_test_env("OAUTH2_TLS_CERT", NULL);
  set_test_env("OAUTH2_TLS_KEY", NULL);

  /* Fail c_rest_init */
  g_crf_malloc_hook = fail_malloc;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_crf_malloc_hook = NULL;

  /* Fail c_rest_router_init without TLS */
  g_mock_main_router_init_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_mock_main_router_init_fail = 0;

  /* Fail c_rest_router_init with TLS */
  g_mock_main_router_init_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(5, args_tls_only));
  g_mock_main_router_init_fail = 0;

  /* Fail c_orm_sqlite_connect with TLS */
  ASSERT_EQ(1, oauth2_app_main(7, args_bad_db_tls));

  /* Fail c_orm_sqlite_connect without TLS */
  ASSERT_EQ(1, oauth2_app_main(3, args_bad_db));

  /* Fail oauth2_server_init with TLS */
  g_mock_main_server_init_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(5, args_tls_only));
  g_mock_main_server_init_fail = 0;

  /* Fail oauth2_server_init without TLS */
  g_mock_main_server_init_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_mock_main_server_init_fail = 0;

  /* Fail cert load + tls_context_destroy fail (line 166) */
  g_mock_main_tls_load_cert_fail = 1;
  g_mock_main_tls_ctx_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(5, args_tls_only));
  g_mock_main_tls_ctx_destroy_fail = 0;
  g_mock_main_tls_load_cert_fail = 0;

  /* Fail router_init with TLS + tls_context_destroy fail (line 179) */
  g_mock_main_router_init_fail = 1;
  g_mock_main_tls_ctx_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(5, args_tls_only));
  g_mock_main_tls_ctx_destroy_fail = 0;
  g_mock_main_router_init_fail = 0;

  /* Fail router_init + ctx destroy fail (line 183) */
  g_mock_main_router_init_fail = 1;
  g_mock_main_ctx_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_mock_main_ctx_destroy_fail = 0;
  g_mock_main_router_init_fail = 0;

  /* Fail sqlite connect + router_destroy fail (line 193) */
  g_mock_main_router_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(3, args_bad_db));
  g_mock_main_router_destroy_fail = 0;

  /* Fail sqlite connect with TLS + tls_ctx_destroy fail (line 197) */
  g_mock_main_tls_ctx_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(7, args_bad_db_tls));
  g_mock_main_tls_ctx_destroy_fail = 0;

  /* Fail sqlite connect + ctx_destroy fail (line 201) */
  g_mock_main_ctx_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(3, args_bad_db));
  g_mock_main_ctx_destroy_fail = 0;

  /* Fail server_init + router_destroy fail (line 212) */
  g_mock_main_server_init_fail = 1;
  g_mock_main_router_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_mock_main_router_destroy_fail = 0;
  g_mock_main_server_init_fail = 0;

  /* Fail server_init with TLS + tls_ctx_destroy fail (line 216) */
  g_mock_main_server_init_fail = 1;
  g_mock_main_tls_ctx_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(5, args_tls_only));
  g_mock_main_tls_ctx_destroy_fail = 0;
  g_mock_main_server_init_fail = 0;

  /* Fail server_init + ctx_destroy fail (line 220) */
  g_mock_main_server_init_fail = 1;
  g_mock_main_ctx_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_mock_main_ctx_destroy_fail = 0;
  g_mock_main_server_init_fail = 0;

  /* Fail enable_openapi (lines 226-229) */
  g_mock_main_enable_openapi_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_mock_main_enable_openapi_fail = 0;

  /* Fail enable_swagger_ui (lines 232-235) */
  g_mock_main_enable_swagger_ui_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_mock_main_enable_swagger_ui_fail = 0;

  /* Fail set_router (lines 239-242) */
  g_mock_main_set_router_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_mock_main_set_router_fail = 0;

  /* Normal exit + router_destroy fail (line 260) */
  g_intercept_run = 2;
  g_mock_main_router_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_mock_main_router_destroy_fail = 0;
  g_intercept_run = 0;

  /* Normal exit with TLS + tls_ctx_destroy fail (line 264) */
  g_intercept_run = 2;
  g_mock_main_tls_ctx_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(5, args_tls_only));
  g_mock_main_tls_ctx_destroy_fail = 0;
  g_intercept_run = 0;

  /* Normal exit + ctx_destroy fail (line 268) */
  g_intercept_run = 2;
  g_mock_main_ctx_destroy_fail = 1;
  ASSERT_EQ(1, oauth2_app_main(1, args_base));
  g_mock_main_ctx_destroy_fail = 0;
  g_intercept_run = 0;

  PASS();
}

SUITE_EXTERN(oauth2_suite);
SUITE(oauth2_suite) {
  RUN_TEST(test_client_init_and_cleanup);
  RUN_TEST(test_client_password_grant);
  RUN_TEST(test_server_init);
  RUN_TEST(test_server_handlers_complete);
  RUN_TEST(test_main_cli_complete);
}
