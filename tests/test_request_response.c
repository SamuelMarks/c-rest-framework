/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_template.h"
#include "c_rest_modality.h"
#include "parson.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static int g_rr_fail_malloc_at = 0;

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  if (g_rr_fail_malloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_rr_fail_malloc_at) {
    alloc_count = 0;
    g_rr_fail_malloc_at = 0;
    return NULL;
  }
  return malloc(size);
}

static char *test_strdup(const char *s) {
  char *out = NULL;
  if (c_rest_internal_strdup(s, &out) != C_REST_OK)
    return NULL;
  return out;
}

TEST test_request_response_null_and_edge_cases(void) {
  c_rest_error_t rc;
  struct c_rest_request req;
  struct c_rest_response res;
  int i;
  char s_x[] = "x";
  char s_body[] = "body";

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* Hit NULL branches */
  ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(NULL, "a", NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_request_get_header(NULL, "a", NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(NULL, "a", NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_request_read_body(NULL, NULL, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_request_parse_json(NULL, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_request_get_form_param(NULL, "a", NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_bearer(NULL, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_basic(NULL, NULL, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_request_accepts_encoding(NULL, NULL));

  ASSERT_NEQ(C_REST_OK, c_rest_response_set_status(NULL, 200));
  ASSERT_NEQ(C_REST_OK, c_rest_response_set_header(NULL, "a", "b"));
  ASSERT_NEQ(C_REST_OK, c_rest_response_set_cookie(NULL, "a", "b", "c"));
  ASSERT_NEQ(C_REST_OK, c_rest_response_json(NULL, "a"));
  ASSERT_NEQ(C_REST_OK, c_rest_response_json_obj(NULL, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_response_json_dict(NULL, NULL, 0));
  ASSERT_EQ(C_REST_OK, c_rest_response_check_etag(NULL, NULL, "a"));
  ASSERT_NEQ(C_REST_OK, c_rest_response_set_cache_control(NULL, "a"));
  ASSERT_NEQ(C_REST_OK, c_rest_response_serialize(NULL, NULL, NULL));

  {
    struct c_rest_response r;
    struct c_rest_response r2;
    memset(&r, 0, sizeof(r));
    memset(&r2, 0, sizeof(r2));
    ASSERT_NEQ(C_REST_OK, c_rest_response_oauth2_error(&r, NULL, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_response_oauth2_error(&r2, NULL, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_response_oauth2_error(NULL, "error", NULL));
    ASSERT_EQ(C_REST_OK, c_rest_response_oauth2_error(&r2, "error", NULL));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r2));
  }

  {
    struct c_rest_response r;
    struct c_rest_connection_context c;
    memset(&r, 0, sizeof(r));
    memset(&c, 0, sizeof(c));
    c.sock = C_REST_INVALID_SOCKET;
    r.context = &c;
    r.body = s_x;
    r.body_len = 0;
    c.tls_conn = (struct c_rest_tls_connection *)1;
    rc = c_rest_response_send(&r);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_TLS);
    c.tls_conn = NULL;
    r.headers_sent = 0;
    rc = c_rest_response_send(&r);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);

    r.body = NULL;
    r.body_len = 0;
    r.headers_sent = 0;
    c.tls_conn = (struct c_rest_tls_connection *)1;
    rc = c_rest_response_send(&r);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
           rc == C_REST_ERROR_TLS);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r));
  }

  {
    struct c_rest_response res_n;
    char *ob = NULL;
    size_t ol = 0;
    memset(&res_n, 0, sizeof(res_n));
    ASSERT_EQ(C_REST_OK, c_rest_response_serialize(&res_n, &ob, &ol));
    C_REST_FREE(ob);
  }

  {
    struct c_rest_response res_b;
    char *ob = NULL;
    size_t ol = 0;
    memset(&res_b, 0, sizeof(res_b));
    res_b.body = s_x;
    res_b.body_len = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_serialize(&res_b, &ob, &ol));
    C_REST_FREE(ob);
  }

  {
    static const int codes[16] = {200, 201, 202, 204, 301, 302, 304, 400,
                                  401, 403, 404, 405, 500, 501, 503, 999};
    for (i = 0; i < 16; i++) {
      struct c_rest_response r;
      char *ob = NULL;
      size_t ol = 0;
      memset(&r, 0, sizeof(r));
      r.status_code = codes[i];
      rc = c_rest_response_serialize(&r, &ob, &ol);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
      C_REST_FREE(ob);
    }
  }

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
  {
    struct c_rest_response res_t;
    struct c_rest_template_context ctx_t;
    memset(&res_t, 0, sizeof(res_t));
    memset(&ctx_t, 0, sizeof(ctx_t));
    ASSERT_EQ(C_REST_OK, c_rest_template_init(&ctx_t, "Hello"));
    g_rr_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_rr_fail_malloc_at = 1;
    ASSERT_NEQ(C_REST_OK,
               c_rest_response_template(&res_t, &ctx_t, NULL, NULL, 0));
    g_rr_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_rr_fail_malloc_at = 2;
    ASSERT_NEQ(C_REST_OK,
               c_rest_response_template(&res_t, &ctx_t, NULL, NULL, 0));
    g_rr_fail_malloc_at = 0;
    g_crf_malloc_hook = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_template_destroy(&ctx_t));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_t));
  }
#endif

  {
    struct c_rest_request req_e;
    struct c_rest_response res_e;
    struct c_rest_header h;
    char k_inm[] = "If-None-Match";
    char v_etag1[] = "etag1";
    memset(&req_e, 0, sizeof(req_e));
    memset(&res_e, 0, sizeof(res_e));
    memset(&h, 0, sizeof(h));
    h.key = k_inm;
    h.value = v_etag1;
    req_e.headers = &h;
    ASSERT_EQ(C_REST_OK, c_rest_response_check_etag(&req_e, &res_e, "etag2"));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_e));
  }

  {
    struct c_rest_response res_c;
    struct c_rest_connection_context ctx;
    memset(&res_c, 0, sizeof(res_c));
    memset(&ctx, 0, sizeof(ctx));
    ctx.sock = C_REST_INVALID_SOCKET;
    res_c.context = &ctx;
    res_c.headers_sent = 1;
    res_c.is_chunked = 1;
    rc = c_rest_response_write_chunk(&res_c, NULL, 0);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    res_c.is_chunked = 0;
    rc = c_rest_response_write_chunk(&res_c, NULL, 0);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
  }
  {
    struct c_rest_response res_c;
    struct c_rest_connection_context ctx;
    memset(&res_c, 0, sizeof(res_c));
    memset(&ctx, 0, sizeof(ctx));
    res_c.context = &ctx;
    res_c.headers_sent = 1;
    res_c.is_chunked = 1;
    ctx.tls_conn = (struct c_rest_tls_connection *)1;
    rc = c_rest_response_write_chunk(&res_c, NULL, 0);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
  }

  ASSERT_NEQ(C_REST_OK, c_rest_response_send_file(NULL, "file"));
  {
    struct c_rest_response res_f;
    memset(&res_f, 0, sizeof(res_f));
    ASSERT_NEQ(C_REST_OK, c_rest_response_send_file(&res_f, NULL));
  }

  ASSERT_NEQ(C_REST_OK, c_rest_response_redirect(NULL, "/url", 302));
  {
    struct c_rest_response res_r;
    memset(&res_r, 0, sizeof(res_r));
    ASSERT_NEQ(C_REST_OK, c_rest_response_redirect(&res_r, NULL, 302));
  }

  {
    struct c_rest_response res_chk;
    char *ob = NULL;
    size_t ol = 0;
    memset(&res_chk, 0, sizeof(res_chk));
    res_chk.is_chunked = 1;
    ASSERT_EQ(C_REST_OK, c_rest_response_serialize(&res_chk, &ob, &ol));
    C_REST_FREE(ob);
  }

  {
    struct c_rest_response res_zero;
    char *ob = NULL;
    size_t ol = 0;
    memset(&res_zero, 0, sizeof(res_zero));
    ASSERT_EQ(C_REST_OK, c_rest_response_serialize(&res_zero, &ob, &ol));
    C_REST_FREE(ob);
  }

  ASSERT_NEQ(C_REST_OK, c_rest_response_send(NULL));
  res.headers_sent = 1;
  ASSERT_NEQ(C_REST_OK, c_rest_response_send(&res));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  {
    static const int err_codes[4] = {400, 401, 404, 500};
    for (i = 0; i < 4; i++) {
      res.headers_sent = 0;
      res.status_code = err_codes[i];
      ASSERT_EQ(C_REST_OK, c_rest_response_send(&res));
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
      memset(&res, 0, sizeof(res));
    }
  }

  {
    struct c_rest_connection_context fake_ctx;
    memset(&fake_ctx, 0, sizeof(fake_ctx));
    fake_ctx.sock = C_REST_INVALID_SOCKET;
    res.context = &fake_ctx;
    res.headers_sent = 0;
    res.body = s_body;
    res.body_len = 4;
    rc = c_rest_response_send(&res);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    res.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
    memset(&res, 0, sizeof(res));
  }

  res.headers_sent = 0;
  res.is_chunked = 1;
  res.status_code = 200;
  ASSERT_EQ(C_REST_OK, c_rest_response_send(&res));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  memset(&res, 0, sizeof(res));

  {
    const char *val = NULL;
    struct c_rest_header h1;
    struct c_rest_header h2;
    struct c_rest_header h3;
    char k_cookie[] = "Cookie";
    char v_cookie[] = "invalid_format; name=value; foo=bar";
    char k_ct[] = "Content-Type";
    char v_ct[] = "application/json";
    char k_auth[] = "Authorization";
    char v_auth[] = "Bearer token";
    memset(&req, 0, sizeof(req));

    ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(&req, "name", &val));

    h1.key = k_cookie;
    h1.value = v_cookie;
    h1.next = NULL;
    h2.key = k_ct;
    h2.value = v_ct;
    h2.next = &h1;
    h3.key = k_auth;
    h3.value = v_auth;
    h3.next = &h2;
    req.headers = &h3;

    {
      struct c_rest_request r;
      void *json_obj = NULL;
      char b_valid[] = "{}";
      char b_invalid[] = "invalid";
      memset(&r, 0, sizeof(r));

      ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&r));
      ASSERT_NEQ(C_REST_OK, c_rest_request_accepts_encoding(&r, "gzip"));

      r.body = b_valid;
      r.body_len = 2;
      ASSERT_EQ(C_REST_OK, c_rest_request_parse_json(&r, &json_obj));
      json_value_free((JSON_Value *)json_obj);

      r.body = b_invalid;
      r.body_len = 7;
      ASSERT_NEQ(C_REST_OK, c_rest_request_parse_json(&r, &json_obj));

      r.body = NULL;
      r.body_len = 0;
      ASSERT_NEQ(C_REST_OK, c_rest_request_parse_json(&r, &json_obj));

      r.headers = (struct c_rest_header *)malloc(sizeof(struct c_rest_header));
      r.headers->key = test_strdup("k");
      r.headers->value = test_strdup("v");
      r.headers->next = NULL;

      r.path_vars =
          (struct c_rest_path_var *)malloc(sizeof(struct c_rest_path_var));
      r.path_vars->name = test_strdup("k");
      r.path_vars->value = test_strdup("v");
      r.path_vars->next = NULL;

      r.body = test_strdup("dynamic");

      ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&r));
    }

    ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(NULL, "name", &val));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(&req, NULL, &val));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(&req, "name", NULL));
    ASSERT_EQ(C_REST_OK, c_rest_request_get_cookie(&req, "name", &val));
    ASSERT_EQ(C_REST_OK, c_rest_request_get_cookie(&req, "foo", &val));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(&req, "missing", &val));

    ASSERT_NEQ(C_REST_OK, c_rest_request_get_header(&req, "missing", &val));

    ASSERT_EQ(C_REST_OK, c_rest_request_accepts_encoding(NULL, "gzip"));
    ASSERT_EQ(C_REST_OK, c_rest_request_accepts_encoding(&req, NULL));

    req.headers = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
  }

  PASS();
}

TEST test_request_response_extra_branches_and_mocks(void) {
  c_rest_error_t rc;
  int i;
  char s_body[] = "body";

  {
    struct c_rest_request req2_ex;
    struct c_rest_response res2_ex;
    struct c_rest_header h;
    const char *val = NULL;
    char v_val[] = "val";
    char q_bad1[] = "a=%XX";
    char q_bad2[] = "%XX=1";
    char q_bad3[] = "%XX";
    char k_cookie[] = "Cookie";
    char v_ck1[] = "no_equals; invalid=cookie; no_equals2; another=val";
    char v_ck2[] = ";";
    char v_ck3[] = "no_eq_no_semi";
    char q_ok[] = "a=1";
    char q_empty[] = "";
    char q_noeq[] = "no_equals";
    char q_eqamp[] = "a&b=1";
    char b_nul[2] = {'\0', '\0'};
    char k_inm[] = "If-None-Match";
    char v_etag[] = "my-etag";

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.headers = &h;
    h.key = NULL;
    h.value = v_val;
    h.next = NULL;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_header(&req2_ex, "a", &val));
    req2_ex.headers = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = q_bad1;
    ASSERT_EQ(C_REST_OK, c_rest_request_get_query(&req2_ex, "a", &val));
    req2_ex.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = q_bad2;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&req2_ex, "a", &val));
    req2_ex.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = q_bad3;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&req2_ex, "a", &val));
    req2_ex.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = q_bad1;
    req2_ex.body_len = 5;
    ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&req2_ex));
    req2_ex.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = q_bad2;
    req2_ex.body_len = 5;
    ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&req2_ex));
    req2_ex.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = q_bad3;
    req2_ex.body_len = 3;
    ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&req2_ex));
    req2_ex.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* Cookie with no equals */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.headers = &h;
    h.key = k_cookie;
    h.value = v_ck1;
    h.next = NULL;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(&req2_ex, "a", &val));
    req2_ex.headers = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.headers = &h;
    h.key = k_cookie;
    h.value = v_ck2;
    h.next = NULL;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(&req2_ex, "a", &val));
    req2_ex.headers = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.headers = &h;
    h.key = k_cookie;
    h.value = v_ck3;
    h.next = NULL;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(&req2_ex, "a", &val));
    req2_ex.headers = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* Query double parse / already parsed */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = q_ok;
    ASSERT_EQ(C_REST_OK, c_rest_request_get_query(&req2_ex, "a", &val));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&req2_ex, "b", &val));
    req2_ex.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* Empty query */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = q_empty;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&req2_ex, "a", &val));
    req2_ex.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* NULL query */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = NULL;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&req2_ex, "a", &val));
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* Query with no equals */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = q_noeq;
    ASSERT_EQ(C_REST_OK, c_rest_request_get_query(&req2_ex, "no_equals", &val));
    req2_ex.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* Query with eq > amp */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = q_eqamp;
    ASSERT_EQ(C_REST_OK, c_rest_request_get_query(&req2_ex, "a", &val));
    req2_ex.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* Form no equals */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = q_noeq;
    req2_ex.body_len = 9;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_form_param(&req2_ex, "a", &val));
    req2_ex.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* Form with eq > amp */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = q_eqamp;
    req2_ex.body_len = 5;
    ASSERT_EQ(C_REST_OK, c_rest_request_get_form_param(&req2_ex, "a", &val));
    req2_ex.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* Form while (*p) false immediately */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = b_nul;
    req2_ex.body_len = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_form_param(&req2_ex, "a", &val));
    req2_ex.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* Empty form */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = q_empty;
    req2_ex.body_len = 0;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_form_param(&req2_ex, "a", &val));
    req2_ex.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));

    /* Empty JSON */
    {
      void *json_obj = NULL;
      memset(&req2_ex, 0, sizeof(req2_ex));
      req2_ex.body = q_empty;
      req2_ex.body_len = 0;
      ASSERT_NEQ(C_REST_OK, c_rest_request_parse_json(&req2_ex, &json_obj));
    }

    memset(&req2_ex, 0, sizeof(req2_ex));
    memset(&res2_ex, 0, sizeof(res2_ex));
    h.key = k_inm;
    h.value = v_etag;
    h.next = NULL;
    req2_ex.headers = &h;
    ASSERT_EQ(C_REST_SHORT_CIRCUIT,
              c_rest_response_check_etag(&req2_ex, &res2_ex, "my-etag"));
    ASSERT_EQ(C_REST_OK,
              c_rest_response_check_etag(&req2_ex, &res2_ex, "other-etag"));

    g_rr_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_rr_fail_malloc_at = 1;
    ASSERT_NEQ(C_REST_OK,
               c_rest_response_check_etag(&req2_ex, &res2_ex, "my-etag-oom"));
    g_crf_malloc_hook = NULL;
    g_rr_fail_malloc_at = -1;

    req2_ex.headers = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2_ex));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2_ex));

    memset(&res2_ex, 0, sizeof(res2_ex));
    ASSERT_EQ(C_REST_OK, c_rest_response_set_status(&res2_ex, 400));
    ASSERT_EQ(C_REST_OK, c_rest_response_send(&res2_ex));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2_ex));
    memset(&res2_ex, 0, sizeof(res2_ex));
    ASSERT_EQ(C_REST_OK, c_rest_response_set_status(&res2_ex, 401));
    ASSERT_EQ(C_REST_OK, c_rest_response_send(&res2_ex));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2_ex));
    memset(&res2_ex, 0, sizeof(res2_ex));
    ASSERT_EQ(C_REST_OK, c_rest_response_set_status(&res2_ex, 404));
    ASSERT_EQ(C_REST_OK, c_rest_response_send(&res2_ex));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2_ex));
    memset(&res2_ex, 0, sizeof(res2_ex));
    ASSERT_EQ(C_REST_OK, c_rest_response_set_status(&res2_ex, 500));
    ASSERT_EQ(C_REST_OK, c_rest_response_send(&res2_ex));
    ASSERT_NEQ(C_REST_OK, c_rest_response_send(&res2_ex));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2_ex));

    {
      struct c_rest_connection_context conn_ctx;
      memset(&conn_ctx, 0, sizeof(conn_ctx));
      conn_ctx.sock = C_REST_INVALID_SOCKET;
      memset(&res2_ex, 0, sizeof(res2_ex));
      res2_ex.context = &conn_ctx;
      res2_ex.body = s_body;
      res2_ex.body_len = 4;
      rc = c_rest_response_send(&res2_ex);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);

      memset(&res2_ex, 0, sizeof(res2_ex));
      conn_ctx.tls_conn = (struct c_rest_tls_connection *)1;
      res2_ex.context = &conn_ctx;
      res2_ex.body = s_body;
      res2_ex.body_len = 4;
      rc = c_rest_response_send(&res2_ex);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC ||
             rc == C_REST_ERROR_TLS);
      res2_ex.body = NULL;
    }
  }

  ASSERT_NEQ(C_REST_OK, c_rest_request_cleanup(NULL));

  {
    struct c_rest_request r;
    struct c_rest_response res3;
    const char *val = NULL;
    char *out = NULL;
    size_t len = 0;
    void *json_obj = NULL;
    memset(&r, 0, sizeof(r));
    memset(&res3, 0, sizeof(res3));

    ASSERT_NEQ(C_REST_OK, c_rest_request_get_header(&r, NULL, &val));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_header(&r, "a", NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(&r, NULL, &val));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_cookie(&r, "a", NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&r, NULL, &val));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&r, "a", NULL));

    ASSERT_NEQ(C_REST_OK, c_rest_request_get_form_param(NULL, "a", &val));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_form_param(&r, NULL, &val));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_form_param(&r, "a", NULL));

    ASSERT_NEQ(C_REST_OK, c_rest_request_read_body(&r, NULL, &len));
    ASSERT_NEQ(C_REST_OK, c_rest_request_read_body(&r, &out, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_request_read_body(NULL, &out, &len));

    ASSERT_NEQ(C_REST_OK, c_rest_request_parse_json(&r, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_request_parse_json(NULL, &json_obj));

    ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_bearer(&r, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_basic(NULL, &out, &out));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_basic(&r, NULL, &out));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_basic(&r, &out, NULL));

    ASSERT_NEQ(C_REST_OK, c_rest_response_set_header(&res3, NULL, "b"));
    ASSERT_NEQ(C_REST_OK, c_rest_response_set_header(&res3, "a", NULL));

    ASSERT_NEQ(C_REST_OK, c_rest_response_set_cookie(&res3, NULL, "b", "c"));
    ASSERT_NEQ(C_REST_OK, c_rest_response_set_cookie(&res3, "a", NULL, "c"));

    ASSERT_NEQ(C_REST_OK, c_rest_response_json(&res3, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_response_json_obj(&res3, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_response_json_dict(NULL, NULL, 0));
    ASSERT_EQ(C_REST_OK, c_rest_response_json_dict(&res3, NULL, 0));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res3));
    memset(&res3, 0, sizeof(res3));

    ASSERT_NEQ(C_REST_OK, c_rest_response_redirect(&res3, NULL, 302));

    ASSERT_EQ(C_REST_OK, c_rest_response_check_etag(NULL, &res3, "a"));
    ASSERT_EQ(C_REST_OK, c_rest_response_check_etag(&r, NULL, "a"));
    ASSERT_EQ(C_REST_OK, c_rest_response_check_etag(&r, &res3, NULL));

    ASSERT_NEQ(C_REST_OK, c_rest_response_set_cache_control(&res3, NULL));

    ASSERT_NEQ(C_REST_OK, c_rest_response_serialize(&res3, NULL, &len));
    ASSERT_NEQ(C_REST_OK, c_rest_response_serialize(&res3, &out, NULL));

    ASSERT_NEQ(C_REST_OK, c_rest_response_send_file(&res3, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_response_send_file(&res3, "some_file.txt"));

    ASSERT_NEQ(C_REST_OK, c_rest_response_write_chunk(&res3, NULL, 5));
    ASSERT_NEQ(C_REST_OK, c_rest_response_write_chunk(&res3, "chunk", 5));
    ASSERT_NEQ(C_REST_OK, c_rest_response_write_chunk(NULL, "chunk", 5));

    ASSERT_NEQ(C_REST_OK, c_rest_response_html(NULL, "hello"));
    ASSERT_NEQ(C_REST_OK, c_rest_response_html(&res3, NULL));
#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
    ASSERT_NEQ(C_REST_OK, c_rest_response_template(NULL, NULL, NULL, NULL, 0));
    ASSERT_NEQ(C_REST_OK, c_rest_response_template(&res3, NULL, NULL, NULL, 0));
#endif

    ASSERT_NEQ(C_REST_OK, c_rest_response_cleanup(NULL));
  }

  {
    struct c_rest_request r;
    struct c_rest_header h;
    const char *val = NULL;
    char k_ck[] = "Cookie";
    char v_ck[] = "name=value;  ";
    memset(&r, 0, sizeof(r));
    h.key = k_ck;
    h.value = v_ck;
    h.next = NULL;
    r.headers = &h;
    ASSERT_EQ(C_REST_OK, c_rest_request_get_cookie(&r, "name", &val));
    r.headers = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&r));
  }

  {
    struct c_rest_request r;
    struct c_rest_header h;
    char *user = NULL;
    char *pass = NULL;
    char k_auth[] = "Authorization";
    char v_bearer[] = "Bearer xyz";
    char v_basic1[] = "Basic something";
    char v_basic2[] = "Basic $!";
    char v_basic3[] = "Basic bm9jb2xvbg==";
    char v_basic4[] = "Basic TEST_MOCK_BASE64_DECODE_FAIL";
    memset(&r, 0, sizeof(r));

    h.key = k_auth;
    h.value = v_bearer;
    h.next = NULL;
    r.headers = &h;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_basic(&r, &user, &pass));
    h.value = v_basic1;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_bearer(&r, &user));
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_basic(&r, &user, &pass));
    h.value = v_basic2;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_basic(&r, &user, &pass));
    h.value = v_basic3;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_basic(&r, &user, &pass));
    h.value = v_basic4;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_auth_basic(&r, &user, &pass));
    r.headers = NULL;
  }

  /* Test URL decode error in query params and urlencoded body */
  {
    struct c_rest_request req_q;
    const char *qval = NULL;
    char q1[] = "TEST_MOCK_URL_DECODE_FAIL=1";
    char q2[] = "k=TEST_MOCK_URL_DECODE_FAIL";
    char q3[] = "TEST_MOCK_URL_DECODE_FAIL";

    memset(&req_q, 0, sizeof(req_q));
    req_q.query = q1;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&req_q, "k", &qval));
    req_q.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req_q));

    memset(&req_q, 0, sizeof(req_q));
    req_q.query = q2;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&req_q, "k", &qval));
    req_q.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req_q));

    memset(&req_q, 0, sizeof(req_q));
    req_q.query = q3;
    ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&req_q, "k", &qval));
    req_q.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req_q));

    memset(&req_q, 0, sizeof(req_q));
    req_q.body = q1;
    req_q.body_len = strlen(q1);
    ASSERT_NEQ(C_REST_OK, c_rest_request_parse_urlencoded(&req_q));
    req_q.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req_q));

    memset(&req_q, 0, sizeof(req_q));
    req_q.body = q2;
    req_q.body_len = strlen(q2);
    ASSERT_NEQ(C_REST_OK, c_rest_request_parse_urlencoded(&req_q));
    req_q.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req_q));

    memset(&req_q, 0, sizeof(req_q));
    req_q.body = q3;
    req_q.body_len = strlen(q3);
    ASSERT_NEQ(C_REST_OK, c_rest_request_parse_urlencoded(&req_q));
    req_q.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req_q));
  }

  /* response.c coverage extras */
  {
    struct c_rest_response r;
    struct c_rest_connection_context fake_ctx;
    struct c_rest_response res_null_h;
    struct c_rest_header h_null;
    char *ser_buf = NULL;
    size_t ser_len = 0;
    char v_val[] = "val";

    memset(&res_null_h, 0, sizeof(res_null_h));
    memset(&h_null, 0, sizeof(h_null));
    res_null_h.headers = &h_null;
    h_null.key = NULL;
    h_null.value = v_val;
    h_null.next = NULL;
    ASSERT_NEQ(C_REST_OK, c_rest_response_set_header(&res_null_h, "a", "val"));
    ASSERT_NEQ(C_REST_OK, c_rest_response_set_header(
                              &res_null_h, "TEST_MOCK_STRCASECMP_FAIL", "val"));
    rc = c_rest_response_serialize(&res_null_h, &ser_buf, &ser_len);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    C_REST_FREE(ser_buf);
    res_null_h.headers = h_null.next;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_null_h));

    memset(&r, 0, sizeof(r));
    memset(&fake_ctx, 0, sizeof(fake_ctx));
    fake_ctx.sock = C_REST_INVALID_SOCKET;
    r.context = &fake_ctx;
    r.body = s_body;
    r.body_len = 4;
    rc = c_rest_response_send(&r);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    r.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r));
    memset(&r, 0, sizeof(r));

    r.headers_sent = 0;
    r.is_chunked = 1;
    ASSERT_EQ(C_REST_OK, c_rest_response_send(&r));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r));
    memset(&r, 0, sizeof(r));

    r.headers_sent = 0;
    r.is_chunked = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_send(&r));
    r.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r));
    memset(&r, 0, sizeof(r));

    r.headers_sent = 0;
    r.is_chunked = 1;
    r.context = &fake_ctx;
    rc = c_rest_response_send(&r);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r));
    memset(&r, 0, sizeof(r));

    /* write_chunk with headers not sent */
    r.context = &fake_ctx;
    r.headers_sent = 0;
    r.is_chunked = 0;
    rc = c_rest_response_write_chunk(&r, "chunk", 5);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    rc = c_rest_response_write_chunk(&r, "chunk2", 6);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r));
    memset(&r, 0, sizeof(r));

    /* write_chunk with raw bytes (is_chunked = 0 but headers sent) */
    r.context = &fake_ctx;
    r.headers_sent = 1;
    r.is_chunked = 0;
    rc = c_rest_response_write_chunk(&r, "chunk", 5);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r));
    memset(&r, 0, sizeof(r));

#ifdef C_REST_TESTING_MALLOC_HOOK
    memset(&r, 0, sizeof(r));
    memset(&fake_ctx, 0, sizeof(fake_ctx));
    r.context = &fake_ctx;
    r.headers_sent = 1;
    r.is_chunked = 1;

    g_mock_socket_fail = 200;
    ASSERT_EQ(C_REST_OK, c_rest_response_write_chunk(&r, "chunk", 5));
    ASSERT_EQ(C_REST_OK, c_rest_response_write_chunk(&r, NULL, 0));

    g_mock_socket_fail = 201;
    ASSERT_NEQ(C_REST_OK, c_rest_response_write_chunk(&r, "chunk", 5));

    g_mock_socket_fail = 202;
    ASSERT_NEQ(C_REST_OK, c_rest_response_write_chunk(&r, "chunk", 5));

    g_mock_socket_fail = 203;
    ASSERT_NEQ(C_REST_OK, c_rest_response_write_chunk(&r, "chunk", 5));

    r.is_chunked = 0;
    g_mock_socket_fail = 200;
    ASSERT_EQ(C_REST_OK, c_rest_response_write_chunk(&r, "chunk", 5));
    ASSERT_EQ(C_REST_OK, c_rest_response_write_chunk(&r, NULL, 0));

    g_mock_socket_fail = 201;
    ASSERT_NEQ(C_REST_OK, c_rest_response_write_chunk(&r, "chunk", 5));
    g_mock_socket_fail = 0;

    r.is_chunked = 1;
    fake_ctx.tls_conn = (struct c_rest_tls_connection *)1;

    g_mock_tls_fail = 3;
    rc = c_rest_response_write_chunk(&r, "chunk", 5);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    rc = c_rest_response_write_chunk(&r, NULL, 0);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);

    g_mock_tls_fail = 0;
    rc = c_rest_response_write_chunk(&r, "chunk", 5);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);

    g_mock_tls_fail = 4;
    ASSERT_NEQ(C_REST_OK, c_rest_response_write_chunk(&r, "chunk", 5));

    g_mock_tls_fail = 5;
    ASSERT_NEQ(C_REST_OK, c_rest_response_write_chunk(&r, "chunk", 5));

    r.is_chunked = 0;
    g_mock_tls_fail = 3;
    rc = c_rest_response_write_chunk(&r, "chunk", 5);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_response_write_chunk(&r, NULL, 0));

    g_mock_tls_fail = 0;
    rc = c_rest_response_write_chunk(&r, "chunk", 5);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
    fake_ctx.tls_conn = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r));
    memset(&r, 0, sizeof(r));
#endif

    r.headers_sent = 0;
    r.body = s_body;
    r.body_len = 4;
    ASSERT_EQ(C_REST_OK, c_rest_response_send(&r));
    r.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&r));
  }

  for (i = 1; i <= 80; i++) {
    struct c_rest_request req2;
    struct c_rest_response res2;
    struct c_rest_header req2_h;
    const char *val = NULL;
    char k_ck[] = "Cookie";
    char v_ck[] = "name=value; foo=bar";
    char q_str[] = "a=1&b=2&c";
    char k_auth[] = "Authorization";
    char v_bearer[] = "Bearer my-token-123";
    char v_basic[] = "Basic YWRtaW46c2VjcmV0MTIz";

    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));

    g_rr_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_rr_fail_malloc_at = i;

    req2_h.key = k_ck;
    req2_h.value = v_ck;
    req2_h.next = NULL;
    req2.headers = &req2_h;
    rc = c_rest_request_get_cookie(&req2, "name", &val);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    req2.headers = NULL;

    req2.query = q_str;
    rc = c_rest_request_get_query(&req2, "a", &val);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    req2.query = NULL;

    req2.body = q_str;
    req2.body_len = 7;
    rc = c_rest_request_parse_urlencoded(&req2);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    rc = c_rest_response_set_header(&res2, "a", "b");
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    rc = c_rest_response_set_cookie(&res2, "a", "b", "c");
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    rc = c_rest_response_set_header(&res2, "a", "c");
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    rc = c_rest_response_check_etag(&req2, &res2, "etag");
    ASSERT(rc == C_REST_OK || rc == C_REST_SHORT_CIRCUIT ||
           rc == C_REST_ERROR_OOM || rc == C_REST_ERROR_GENERIC);
    rc = c_rest_response_set_cache_control(&res2, "policy");
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    rc = c_rest_response_json(&res2, "{\"a\":1}");
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    {
      JSON_Value *json_obj = json_parse_string("{\"a\":1}");
      struct c_rest_json_pair pairs[1];
      if (json_obj) {
        rc = c_rest_response_json_obj(&res2, json_obj);
        ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
               rc == C_REST_ERROR_GENERIC);
        json_value_free(json_obj);
      }

      pairs[0].key = "access_token";
      pairs[0].type = C_REST_JSON_TYPE_STRING;
      pairs[0].str_val = "test_token_123";
      pairs[0].num_val = 0;
      pairs[0].bool_val = 0;
      rc = c_rest_response_json_dict(&res2, pairs, 1);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
    }

    rc = c_rest_response_redirect(&res2, "url", 302);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    {
      char *token = NULL;
      struct c_rest_header auth_bearer;
      auth_bearer.key = k_auth;
      auth_bearer.value = v_bearer;
      auth_bearer.next = NULL;
      req2.headers = &auth_bearer;
      if (c_rest_request_get_auth_bearer(&req2, &token) == C_REST_OK)
        C_REST_FREE(token);
    }

    {
      char *user = NULL;
      char *pass = NULL;
      struct c_rest_header auth_basic;
      auth_basic.key = k_auth;
      auth_basic.value = v_basic;
      auth_basic.next = NULL;
      req2.headers = &auth_basic;
      if (c_rest_request_get_auth_basic(&req2, &user, &pass) == C_REST_OK) {
        C_REST_FREE(user);
        C_REST_FREE(pass);
      }
    }
    req2.headers = NULL;

    {
      char *out = NULL;
      size_t olen = 0;
      if (c_rest_response_serialize(&res2, &out, &olen) == C_REST_OK)
        C_REST_FREE(out);
    }

    {
      struct c_rest_connection_context fake_ctx;
      memset(&fake_ctx, 0, sizeof(fake_ctx));
      fake_ctx.sock = C_REST_INVALID_SOCKET;
      res2.context = &fake_ctx;
      res2.headers_sent = 0;
      rc = c_rest_response_send(&res2);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2));
      memset(&res2, 0, sizeof(res2));

      res2.headers_sent = 0;
      res2.is_chunked = 1;
      rc = c_rest_response_send(&res2);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2));
      memset(&res2, 0, sizeof(res2));
    }

    g_crf_malloc_hook = NULL;
    g_rr_fail_malloc_at = 0;

    req2.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req2));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2));
  }

  {
    int s;
    static const int extra_statuses[11] = {201, 202, 204, 301, 304, 400,
                                           401, 403, 404, 405, 500};
    struct c_rest_response rr;
    char *out_buf = NULL;
    size_t out_len = 0;

    memset(&rr, 0, sizeof(rr));
    ASSERT_EQ(C_REST_OK, c_rest_response_redirect(&rr, "url", 999));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&rr));
    memset(&rr, 0, sizeof(rr));
    ASSERT_EQ(C_REST_OK, c_rest_response_redirect(&rr, "url", 200));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&rr));

    for (s = 100; s <= 600; s += 100) {
      memset(&rr, 0, sizeof(rr));
      ASSERT_EQ(C_REST_OK, c_rest_response_set_status(&rr, s));
      rc = c_rest_response_serialize(&rr, &out_buf, &out_len);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
      C_REST_FREE(out_buf);
      out_buf = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&rr));
    }

    for (i = 0; i < 11; i++) {
      memset(&rr, 0, sizeof(rr));
      ASSERT_EQ(C_REST_OK, c_rest_response_set_status(&rr, extra_statuses[i]));
      ASSERT_EQ(C_REST_OK, c_rest_response_serialize(&rr, &out_buf, &out_len));
      C_REST_FREE(out_buf);
      out_buf = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&rr));
    }

    memset(&rr, 0, sizeof(rr));
    ASSERT_EQ(C_REST_OK, c_rest_response_set_status(&rr, 501));
    ASSERT_EQ(C_REST_OK, c_rest_response_serialize(&rr, &out_buf, &out_len));
    C_REST_FREE(out_buf);
    out_buf = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&rr));

    ASSERT_NEQ(C_REST_OK, c_rest_response_oauth2_error(NULL, NULL, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_response_oauth2_error(&rr, "error", NULL));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&rr));
  }

  for (i = 0; i <= 10; i++) {
    struct c_rest_response rr;
    JSON_Value *json_obj = json_value_init_object();
    struct c_rest_json_pair pairs[1];

    pairs[0].key = "access_token";
    pairs[0].type = C_REST_JSON_TYPE_STRING;
    pairs[0].str_val = "test_token_123";
    pairs[0].num_val = 0;
    pairs[0].bool_val = 0;

    memset(&rr, 0, sizeof(rr));

    g_rr_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_rr_fail_malloc_at = i;
    rc = c_rest_response_json_obj(&rr, json_obj);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    g_rr_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_rr_fail_malloc_at = i;
    rc = c_rest_response_json_dict(&rr, pairs, 1);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    g_rr_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_rr_fail_malloc_at = i;
    rc = c_rest_response_html(&rr, "hello");
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
    g_rr_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_rr_fail_malloc_at = i;
    ASSERT_NEQ(C_REST_OK, c_rest_response_template(NULL, NULL, NULL, NULL, 0));
#endif

    g_crf_malloc_hook = NULL;
    g_rr_fail_malloc_at = -1;

    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&rr));
    json_value_free(json_obj);
  }

  for (i = 0; i <= 3; i++) {
    struct c_rest_request req_ex3;
    const char *val_ex3 = NULL;
    char q_kv[] = "key1=value1&key2=value2";

    memset(&req_ex3, 0, sizeof(req_ex3));
    req_ex3.query = q_kv;

    g_rr_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_rr_fail_malloc_at = i;
    rc = c_rest_request_get_query(&req_ex3, "key2", &val_ex3);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    g_crf_malloc_hook = NULL;
    g_rr_fail_malloc_at = -1;
    req_ex3.query = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req_ex3));
  }

  for (i = 0; i <= 3; i++) {
    struct c_rest_request req_ex3;
    const char *val_ex3 = NULL;
    char b_kv[] = "key1=value1&key2=value2";

    memset(&req_ex3, 0, sizeof(req_ex3));
    req_ex3.body = b_kv;
    req_ex3.body_len = strlen(b_kv);

    g_rr_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_rr_fail_malloc_at = i;
    rc = c_rest_request_get_form_param(&req_ex3, "key2", &val_ex3);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    g_crf_malloc_hook = NULL;
    g_rr_fail_malloc_at = -1;
    req_ex3.body = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req_ex3));
  }

  {
    struct c_rest_request req3;
    struct c_rest_header *dyn_h;
    memset(&req3, 0, sizeof(req3));

    dyn_h = (struct c_rest_header *)malloc(sizeof(struct c_rest_header));
    dyn_h->key = test_strdup("Dyn-Key");
    dyn_h->value = test_strdup("Dyn-Value");
    dyn_h->next = NULL;
    req3.headers = dyn_h;

    req3.cookies = (struct c_rest_header *)malloc(sizeof(struct c_rest_header));
    req3.cookies->key = test_strdup("Cookie-Key");
    req3.cookies->value = test_strdup("Cookie-Value");
    req3.cookies->next = NULL;

    req3.path_vars =
        (struct c_rest_path_var *)malloc(sizeof(struct c_rest_path_var));
    req3.path_vars->name = test_strdup("Path-Key");
    req3.path_vars->value = test_strdup("Path-Value");
    req3.path_vars->next = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req3));
  }

  PASS();
}

TEST test_request_response_functional(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  const char *val = NULL;
  char q_main[] = "id=123&name=test&empty=&no_val";

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* Test request query parsing */
  req.query = q_main;

  ASSERT_EQ(C_REST_OK, c_rest_request_get_query(&req, "id", &val));
  ASSERT_NEQ(NULL, val);
  ASSERT_STR_EQ("123", val);

  val = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_request_get_query(&req, "name", &val));
  ASSERT_NEQ(NULL, val);
  ASSERT_STR_EQ("test", val);

  val = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_request_get_query(&req, "empty", &val));
  ASSERT_NEQ(NULL, val);
  ASSERT_STR_EQ("", val);

  val = NULL;
  ASSERT_EQ(C_REST_OK, c_rest_request_get_query(&req, "no_val", &val));
  ASSERT_NEQ(NULL, val);
  ASSERT_STR_EQ("", val);

  val = NULL;
  ASSERT_NEQ(C_REST_OK, c_rest_request_get_query(&req, "missing", &val));
  ASSERT_EQ(NULL, val);
  req.query = NULL;

  /* Test response headers */
  ASSERT_EQ(C_REST_OK,
            c_rest_response_set_header(&res, "Content-Type", "text/plain"));
  ASSERT_EQ(C_REST_OK, c_rest_response_set_header(&res, "Content-Type",
                                                  "application/json"));

  ASSERT_EQ(C_REST_OK, c_rest_response_set_cookie(&res, "session", "abc",
                                                  "HttpOnly; Secure"));
  ASSERT_EQ(C_REST_OK, c_rest_response_set_cookie(&res, "theme", "dark", NULL));

  /* Verify headers */
  {
    int found_ct = 0;
    int found_cookie_session = 0;
    int found_cookie_theme = 0;
    struct c_rest_header *h;

    for (h = res.headers; h != NULL; h = h->next) {
      if (h->key && strcmp(h->key, "Content-Type") == 0) {
        found_ct = 1;
        ASSERT_STR_EQ("application/json", h->value);
      }
      if (h->key && strcmp(h->key, "Set-Cookie") == 0) {
        if (strstr(h->value, "session=abc") != NULL)
          found_cookie_session = 1;
        if (strstr(h->value, "theme=dark") != NULL)
          found_cookie_theme = 1;
      }
    }

    ASSERT_EQ(1, found_ct);
    ASSERT_EQ(1, found_cookie_session);
    ASSERT_EQ(1, found_cookie_theme);
  }

  /* Test ETag && Cache Control */
  {
    struct c_rest_header req_h_etag;
    char k_inm[] = "If-None-Match";
    char v_etag_ok[] = "\"12345\"";
    char v_etag_bad[] = "\"wrong_etag\"";
    req_h_etag.key = k_inm;
    req_h_etag.value = v_etag_ok;
    req_h_etag.next = NULL;
    req.headers = &req_h_etag;

    ASSERT_EQ(C_REST_OK,
              c_rest_response_set_cache_control(&res, "max-age=3600"));
    ASSERT_EQ(C_REST_SHORT_CIRCUIT,
              c_rest_response_check_etag(&req, &res, "\"12345\""));
    ASSERT_EQ(304, res.status_code);

    req_h_etag.value = v_etag_bad;
    ASSERT_EQ(C_REST_OK, c_rest_response_check_etag(&req, &res, "\"12345\""));
    req.headers = NULL;
  }

  /* Test Accepts Encoding */
  {
    struct c_rest_header req_h_enc;
    char k_ae[] = "Accept-Encoding";
    char v_ae[] = "gzip, deflate, br";
    req_h_enc.key = k_ae;
    req_h_enc.value = v_ae;
    req_h_enc.next = NULL;
    req.headers = &req_h_enc;

    ASSERT_NEQ(C_REST_OK, c_rest_request_accepts_encoding(&req, "gzip"));
    ASSERT_NEQ(C_REST_OK, c_rest_request_accepts_encoding(&req, "br"));
    ASSERT_EQ(C_REST_OK, c_rest_request_accepts_encoding(&req, "identity"));

    req.headers = NULL;
  }

  /* Test large body */
  {
    char *large_body = NULL;
    char *read_ptr = NULL;
    size_t read_len = 0;
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(1024 * 1024, &large_body));
    memset(large_body, 'A', 1024 * 1024 - 1);
    large_body[1024 * 1024 - 1] = '\0';
    req.body = large_body;
    req.body_len = 1024 * 1024 - 1;

    ASSERT_EQ(C_REST_OK, c_rest_request_read_body(&req, &read_ptr, &read_len));
    ASSERT_NEQ(NULL, read_ptr);
    ASSERT_EQ((size_t)(1024 * 1024 - 1), read_len);
    ASSERT_EQ('A', read_ptr[0]);
    C_REST_FREE(large_body);
    req.body = NULL;
  }

  /* Test helpers */
  ASSERT_EQ(C_REST_OK, c_rest_response_json(&res, "{\"hello\":\"world\"}"));
  ASSERT_STR_EQ("{\"hello\":\"world\"}", res.body);

  /* Test JSON Request Parsing */
  {
    void *json_obj = NULL;
    char b_json[] = "{\"key\": \"value\"}";
    req.body = b_json;
    req.body_len = strlen(b_json);
    ASSERT_EQ(C_REST_OK, c_rest_request_parse_json(&req, &json_obj));
    ASSERT_NEQ(NULL, json_obj);

    res.headers_sent = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_json_obj(&res, json_obj));
    json_value_free((JSON_Value *)json_obj);
    req.body = NULL;
  }

  /* Test JSON Dict Generation */
  {
    struct c_rest_json_pair pairs[5];
    pairs[0].key = "access_token";
    pairs[0].type = C_REST_JSON_TYPE_STRING;
    pairs[0].str_val = "test_token_123";
    pairs[0].num_val = 0;
    pairs[0].bool_val = 0;

    pairs[1].key = "expires_in";
    pairs[1].type = C_REST_JSON_TYPE_NUMBER;
    pairs[1].str_val = NULL;
    pairs[1].num_val = 3600.0;
    pairs[1].bool_val = 0;

    pairs[2].key = "is_active";
    pairs[2].type = C_REST_JSON_TYPE_BOOLEAN;
    pairs[2].str_val = NULL;
    pairs[2].num_val = 0;
    pairs[2].bool_val = 1;

    pairs[3].key = "refresh_token";
    pairs[3].type = C_REST_JSON_TYPE_NULL;
    pairs[3].str_val = NULL;
    pairs[3].num_val = 0;
    pairs[3].bool_val = 0;

    pairs[4].key = "invalid";
    pairs[4].type = (enum c_rest_json_val_type)99;
    pairs[4].str_val = NULL;
    pairs[4].num_val = 0;
    pairs[4].bool_val = 0;

    res.headers_sent = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_json_dict(&res, pairs, 5));
    ASSERT_NEQ(NULL, res.body);
    ASSERT_NEQ(NULL, strstr(res.body, "\"access_token\":\"test_token_123\""));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  }

  /* Test URL Encoded Parsing */
  {
    const char *form_val = NULL;
    char b_form[] = "username=admin&password=123%20456&grant_type=password";
    req.body = b_form;
    req.body_len = strlen(b_form);
    ASSERT_EQ(C_REST_OK, c_rest_request_parse_urlencoded(&req));
    ASSERT_EQ(C_REST_OK,
              c_rest_request_get_form_param(&req, "username", &form_val));
    ASSERT_STR_EQ("admin", form_val);
    ASSERT_EQ(C_REST_OK,
              c_rest_request_get_form_param(&req, "password", &form_val));
    ASSERT_STR_EQ("123 456", form_val);
    req.body = NULL;
  }

  /* Test Auth Extraction */
  {
    struct c_rest_header auth_bearer;
    struct c_rest_header auth_basic;
    char *token = NULL;
    char *user = NULL;
    char *pass = NULL;
    char k_auth[] = "Authorization";
    char v_bearer[] = "Bearer my-token-123";
    char v_basic[] = "Basic YWRtaW46c2VjcmV0MTIz";

    auth_bearer.key = k_auth;
    auth_bearer.value = v_bearer;
    auth_bearer.next = NULL;

    req.headers = &auth_bearer;
    ASSERT_EQ(C_REST_OK, c_rest_request_get_auth_bearer(&req, &token));
    ASSERT_STR_EQ("my-token-123", token);
    C_REST_FREE(token);

    auth_basic.key = k_auth;
    auth_basic.value = v_basic;
    auth_basic.next = NULL;

    req.headers = &auth_basic;
    ASSERT_EQ(C_REST_OK, c_rest_request_get_auth_basic(&req, &user, &pass));
    ASSERT_STR_EQ("admin", user);
    ASSERT_STR_EQ("secret123", pass);
    C_REST_FREE(user);
    C_REST_FREE(pass);

    req.headers = NULL;
  }

  /* Test response serialize */
  {
    struct c_rest_response ser_res;
    char *out_buf = NULL;
    size_t out_len = 0;

    memset(&ser_res, 0, sizeof(ser_res));
    ASSERT_EQ(C_REST_OK, c_rest_response_set_status(&ser_res, 200));
    ASSERT_EQ(C_REST_OK, c_rest_response_set_header(&ser_res, "Content-Type",
                                                    "text/plain"));

    ser_res.body = NULL;
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(12, &ser_res.body));
#if defined(_MSC_VER)
    strcpy_s(ser_res.body, 12, "Hello World");
#else
    strcpy(ser_res.body, "Hello World");
#endif
    ser_res.body_len = 11;

    ASSERT_EQ(C_REST_OK,
              c_rest_response_serialize(&ser_res, &out_buf, &out_len));
    ASSERT_NEQ(NULL, out_buf);
    ASSERT_NEQ(NULL, strstr(out_buf, "HTTP/1.1 200 OK\r\n"));
    ASSERT_NEQ(NULL, strstr(out_buf, "Content-Type: text/plain\r\n"));
    ASSERT_NEQ(NULL, strstr(out_buf, "Content-Length: 11\r\n"));
    ASSERT_NEQ(NULL, strstr(out_buf, "\r\n\r\nHello World"));
    C_REST_FREE(out_buf);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&ser_res));
  }

  /* Test OAuth2 Error */
  {
    struct c_rest_response err_res;
    memset(&err_res, 0, sizeof(err_res));
    ASSERT_EQ(C_REST_OK, c_rest_response_oauth2_error(
                             &err_res, "invalid_request", "Missing parameter"));
    ASSERT_EQ(400, err_res.status_code);
    ASSERT_NEQ(NULL, strstr(err_res.body, "\"error\":\"invalid_request\""));
    ASSERT_NEQ(NULL, strstr(err_res.body,
                            "\"error_description\":\"Missing parameter\""));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&err_res));
  }

  ASSERT_EQ(C_REST_OK, c_rest_request_cleanup(&req));
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  PASS();
}

SUITE(request_response_suite) {
  RUN_TEST(test_request_response_null_and_edge_cases);
  RUN_TEST(test_request_response_extra_branches_and_mocks);
  RUN_TEST(test_request_response_functional);
}
