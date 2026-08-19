#include <string.h>
/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_template.h"
#include "c_rest_request.h"
#include "c_rest_modality.h"
#include "parson.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  extern int g_fail_malloc_at;
  if (g_fail_malloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_malloc_at) {
    alloc_count = 0;
    g_fail_malloc_at = 0;
    return NULL;
  }
  return malloc(size);
}

static void test_coverage(void) {
  c_rest_error_t rc;
  struct c_rest_request req;
  struct c_rest_response res;
  int i;
  extern int g_fail_malloc_at;
  extern int g_fail_realloc_at;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* Hit NULL branches */
  rc = c_rest_request_get_query(NULL, "a", NULL);
  rc = c_rest_request_get_header(NULL, "a", NULL);
  rc = c_rest_request_get_cookie(NULL, "a", NULL);
  rc = c_rest_request_read_body(NULL, NULL, NULL);
  rc = c_rest_request_parse_json(NULL, NULL);
  rc = c_rest_request_parse_urlencoded(NULL);
  rc = c_rest_request_get_form_param(NULL, "a", NULL);
  rc = c_rest_request_get_auth_bearer(NULL, NULL);
  rc = c_rest_request_get_auth_basic(NULL, NULL, NULL);
  rc = c_rest_request_accepts_encoding(NULL, NULL);

  rc = c_rest_response_set_status(NULL, 200);
  rc = c_rest_response_set_header(NULL, "a", "b");
  rc = c_rest_response_set_cookie(NULL, "a", "b", "c");
  rc = c_rest_response_json(NULL, "a");
  rc = c_rest_response_json_obj(NULL, NULL);
  rc = c_rest_response_json_dict(NULL, NULL, 0);
  rc = c_rest_response_check_etag(NULL, NULL, "a");
  rc = c_rest_response_set_cache_control(NULL, "a");
  rc = c_rest_response_serialize(NULL, NULL, NULL);

  {
    struct c_rest_response r = {0};
    c_rest_response_oauth2_error(&r, NULL, NULL);
  }
  {
    struct c_rest_response r2 = {0};
    c_rest_response_oauth2_error(&r2, NULL, NULL);
    c_rest_response_oauth2_error(NULL, "error", NULL);
    c_rest_response_oauth2_error(&r2, "error", NULL);
  }

  {
    struct c_rest_response r = {0};
    struct c_rest_connection_context c = {0};
    r.context = &c;
    r.body = "x";
    r.body_len = 0;
    c.tls_conn = (void *)1;
    c_rest_response_send(&r);
    c.tls_conn = NULL;
    r.headers_sent = 0;
    c_rest_response_send(&r);

    r.body = NULL;
    r.body_len = 0;
    r.headers_sent = 0;
    c.tls_conn = (void *)1;
    c_rest_response_send(&r);
  }

  {
    struct c_rest_response res_n = {0};
    char *ob = NULL;
    size_t ol;
    c_rest_response_serialize(&res_n, &ob, &ol);
    if (ob)
      C_REST_FREE(ob);
  }

  {
    struct c_rest_response res_b = {0};
    char *ob = NULL;
    size_t ol;
    res_b.body = "x";
    res_b.body_len = 0;
    c_rest_response_serialize(&res_b, &ob, &ol);
    if (ob)
      C_REST_FREE(ob);
  }

  {
    int codes[] = {200, 201, 202, 204, 301, 302, 304, 400,
                   401, 403, 404, 405, 500, 501, 503, 999};
    for (i = 0; i < 16; i++) {
      struct c_rest_response r = {0};
      char *ob = NULL;
      size_t ol;
      r.status_code = codes[i];
      c_rest_response_serialize(&r, &ob, &ol);
      if (ob)
        C_REST_FREE(ob);
    }
  }

  {
    struct c_rest_response res_t = {0};
    struct c_rest_template_context ctx_t = {0};
    extern int g_fail_malloc_at;
    extern void *fail_malloc_n(size_t);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 1;
    c_rest_response_template(&res_t, &ctx_t, NULL, NULL, 0);
    g_fail_malloc_at = 0;
    g_crf_malloc_hook = NULL;
  }

  {
    struct c_rest_request req_e = {0};
    struct c_rest_response res_e = {0};

    struct c_rest_header h = {0};
    h.key = "If-None-Match";
    h.value = "etag1";
    req_e.headers = &h;
    c_rest_response_check_etag(&req_e, &res_e, "etag2");

    /* c_rest_request_cleanup(&req_e); */
  }

  {
    struct c_rest_response res_c = {0};
    struct c_rest_connection_context ctx = {0};
    res_c.context = &ctx;
    res_c.headers_sent = 1;
    res_c.is_chunked = 1;
    c_rest_response_write_chunk(&res_c, NULL, 0);
    res_c.is_chunked = 0;
    c_rest_response_write_chunk(&res_c, NULL, 0);
  }
  {
    struct c_rest_response res_c = {0};
    struct c_rest_connection_context ctx = {0};
    res_c.context = &ctx;
    res_c.headers_sent = 1;
    res_c.is_chunked = 1;
    ctx.tls_conn = (void *)1;
    c_rest_response_write_chunk(&res_c, NULL, 0);
  }

  {
    struct c_rest_response res_c = {0};
    res_c.body = "test";
    res_c.body_len = 0;
    (void)res_c;
    /* To test body_len == 0 but body != NULL, although c_rest_response_send
     * handles this. */
  }

  c_rest_response_send_file(NULL, "file");
  {
    struct c_rest_response res_f = {0};
    c_rest_response_send_file(&res_f, NULL);
  }

  c_rest_response_redirect(NULL, "/url", 302);
  {
    struct c_rest_response res_r = {0};
    c_rest_response_redirect(&res_r, NULL, 302);
  }

  {
    struct c_rest_response res_chk = {0};
    char *ob = NULL;
    size_t ol;
    res_chk.is_chunked = 1;
    c_rest_response_serialize(&res_chk, &ob, &ol);
    if (ob)
      C_REST_FREE(ob);
  }

  {
    struct c_rest_response res_zero = {0};
    char *ob = NULL;
    size_t ol;
    c_rest_response_serialize(&res_zero, &ob, &ol);
    if (ob)
      C_REST_FREE(ob);
  }

  (void)rc;

  c_rest_response_send(NULL);
  res.headers_sent = 1;
  c_rest_response_send(&res);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  res.headers_sent = 0;
  res.status_code = 400;
  c_rest_response_send(&res);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  res.headers_sent = 0;
  res.status_code = 401;
  c_rest_response_send(&res);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  res.headers_sent = 0;
  res.status_code = 404;
  c_rest_response_send(&res);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  res.headers_sent = 0;
  res.status_code = 500;
  c_rest_response_send(&res);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  {
    struct {
      int sock;
      void *tls;
      void *cm;
    } fake_ctx;
    fake_ctx.sock = -1;
    fake_ctx.tls = NULL;
    fake_ctx.cm = NULL;
    res.context = &fake_ctx;
    res.headers_sent = 0;
    res.body = "body";
    res.body_len = 4;
    c_rest_response_send(&res);
    res.body = NULL;
    c_rest_response_cleanup(&res);
    memset(&res, 0, sizeof(res));
  }

  res.headers_sent = 0;
  res.is_chunked = 1;
  res.status_code = 200;
  c_rest_response_send(&res);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));
  res.is_chunked = 0;

  {
    const char *val;
    struct c_rest_header h1;
    struct c_rest_header h2;
    struct c_rest_header h3;
    memset(&req, 0, sizeof(req));

    c_rest_request_get_cookie(&req, "name", &val); /* no cookie header */

    h1.key = "Cookie";
    h1.value = "invalid_format; name=value; foo=bar";
    h1.next = NULL;
    h2.key = "Content-Type";
    h2.value = "application/json";
    h2.next = &h1;
    h3.key = "Authorization";
    h3.value = "Bearer token";
    h3.next = &h2;
    req.headers = &h3;

    {
      struct c_rest_request r;
      void *json_obj = NULL;
      memset(&r, 0, sizeof(r));

      c_rest_request_parse_urlencoded(&r); /* empty body */

      c_rest_request_accepts_encoding(&r, "gzip"); /* missing header */

      r.body = "{}";
      r.body_len = 2;
      c_rest_request_parse_json(&r, &json_obj);
      if (json_obj)
        json_value_free(json_obj);

      r.body = "invalid";
      r.body_len = 7;
      c_rest_request_parse_json(&r, &json_obj);

      r.body = NULL;
      r.body_len = 0;
      c_rest_request_parse_json(&r, &json_obj);

      r.headers = (struct c_rest_header *)malloc(sizeof(struct c_rest_header));
      r.headers->key = (char *)CRF_STRDUP("k");
      r.headers->value = (char *)CRF_STRDUP("v");
      r.headers->next = NULL;

      r.path_vars =
          (struct c_rest_path_var *)malloc(sizeof(struct c_rest_path_var));
      r.path_vars->name = (char *)CRF_STRDUP("k");
      r.path_vars->value = (char *)CRF_STRDUP("v");
      r.path_vars->next = NULL;

      r.body = (char *)CRF_STRDUP("dynamic");

      c_rest_request_cleanup(&r);
    }

    c_rest_request_get_cookie(NULL, "name", &val);
    c_rest_request_get_cookie(&req, NULL, &val);
    c_rest_request_get_cookie(&req, "name", NULL);
    c_rest_request_get_cookie(&req, "name", &val);
    c_rest_request_get_cookie(&req, "foo", &val);
    c_rest_request_get_cookie(&req, "missing", &val);

    c_rest_request_get_header(&req, "missing", &val);

    c_rest_request_accepts_encoding(NULL, "gzip");
    c_rest_request_accepts_encoding(&req, NULL);

    req.headers = NULL;
    c_rest_request_cleanup(&req);
  }

  /* request.c coverage extras */
  {
    struct c_rest_request req2_ex;
    struct c_rest_response res2_ex;
    struct c_rest_header h;
    const char *val;

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = "a=%XX";
    c_rest_request_get_query(&req2_ex, "a", &val);
    c_rest_request_cleanup(&req2_ex);

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = "%XX=1";
    c_rest_request_get_query(&req2_ex, "a", &val);
    c_rest_request_cleanup(&req2_ex);

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = "%XX";
    c_rest_request_get_query(&req2_ex, "a", &val);
    c_rest_request_cleanup(&req2_ex);

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = "a=%XX";
    req2_ex.body_len = 5;
    c_rest_request_parse_urlencoded(&req2_ex);
    req2_ex.body = NULL;
    c_rest_request_cleanup(&req2_ex);

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = "%XX=1";
    req2_ex.body_len = 5;
    c_rest_request_parse_urlencoded(&req2_ex);
    req2_ex.body = NULL;
    c_rest_request_cleanup(&req2_ex);

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = "%XX";
    req2_ex.body_len = 3;
    c_rest_request_parse_urlencoded(&req2_ex);
    req2_ex.body = NULL;
    c_rest_request_cleanup(&req2_ex);

    /* Cookie with no equals */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.headers = &h;
    h.key = "Cookie";
    h.value = "no_equals; invalid=cookie; no_equals2; another=val";
    h.next = NULL;
    c_rest_request_get_cookie(&req2_ex, "a", &val);
    req2_ex.headers = NULL;
    c_rest_request_cleanup(&req2_ex);

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.headers = &h;
    h.key = "Cookie";
    h.value = ";";
    h.next = NULL;
    c_rest_request_get_cookie(&req2_ex, "a", &val);
    req2_ex.headers = NULL;
    c_rest_request_cleanup(&req2_ex);

    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.headers = &h;
    h.key = "Cookie";
    h.value = "no_eq_no_semi";
    h.next = NULL;
    c_rest_request_get_cookie(&req2_ex, "a", &val);
    req2_ex.headers = NULL;
    c_rest_request_cleanup(&req2_ex);

    /* Query double parse / already parsed */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = "a=1";
    c_rest_request_get_query(&req2_ex, "a", &val);
    c_rest_request_get_query(&req2_ex, "b",
                             &val); /* Hits already parsed branch */
    c_rest_request_cleanup(&req2_ex);

    /* Empty query */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = "";
    c_rest_request_get_query(&req2_ex, "a", &val);
    c_rest_request_cleanup(&req2_ex);

    /* NULL query */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = NULL;
    c_rest_request_get_query(&req2_ex, "a", &val);
    c_rest_request_cleanup(&req2_ex);

    /* Query with no equals */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = "no_equals";
    c_rest_request_get_query(&req2_ex, "a", &val);
    c_rest_request_cleanup(&req2_ex);

    /* Query with eq > amp */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.query = "a&b=1";
    c_rest_request_get_query(&req2_ex, "a", &val);
    c_rest_request_cleanup(&req2_ex);

    /* Form no equals */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = "no_equals";
    req2_ex.body_len = 9;
    c_rest_request_get_form_param(&req2_ex, "a", &val);
    req2_ex.body = NULL;
    c_rest_request_cleanup(&req2_ex);

    /* Form with eq > amp */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = "a&b=1";
    req2_ex.body_len = 5;
    c_rest_request_get_form_param(&req2_ex, "a", &val);
    req2_ex.body = NULL;
    c_rest_request_cleanup(&req2_ex);

    /* Form while (*p) false immediately */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = "\0";
    req2_ex.body_len = 1;
    c_rest_request_get_form_param(&req2_ex, "a", &val);
    req2_ex.body = NULL;
    c_rest_request_cleanup(&req2_ex);

    /* Empty form */
    memset(&req2_ex, 0, sizeof(req2_ex));
    req2_ex.body = "";
    req2_ex.body_len = 0;
    c_rest_request_get_form_param(&req2_ex, "a", &val);
    req2_ex.body = NULL;
    c_rest_request_cleanup(&req2_ex);

    /* Empty JSON */
    {
      void *json_obj;
      memset(&req2_ex, 0, sizeof(req2_ex));
      req2_ex.body = "";
      req2_ex.body_len = 0;
      c_rest_request_parse_json(&req2_ex, &json_obj);
    }

    memset(&req2_ex, 0, sizeof(req2_ex));
    memset(&res2_ex, 0, sizeof(res2_ex));
    h.key = "If-None-Match";
    h.value = "my-etag";
    h.next = NULL;
    req2_ex.headers = &h;
    c_rest_response_check_etag(&req2_ex, &res2_ex, "my-etag");
    c_rest_response_check_etag(&req2_ex, &res2_ex, "other-etag");

    {
      int k_oom;
      extern int g_fail_malloc_at;

      /* To hit branch 112, fail the first allocation
       * (c_rest_response_set_header) */
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = 1;
      c_rest_response_check_etag(&req2_ex, &res2_ex, "my-etag-oom");

      /* To hit branch 118, fail the set_status if it allocates, but it doesn't.
       * Wait, status doesn't allocate. Is there any allocation inside
       * set_status? No. Let's just run it with fail_malloc_at = 1 to see if we
       * hit it */
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
    }

    req2_ex.headers = NULL;
    c_rest_request_cleanup(&req2_ex);
    c_rest_response_cleanup(&res2_ex);

    memset(&res2_ex, 0, sizeof(res2_ex));
    c_rest_response_set_status(&res2_ex, 400);
    c_rest_response_send(&res2_ex);
    memset(&res2_ex, 0, sizeof(res2_ex));
    c_rest_response_set_status(&res2_ex, 401);
    c_rest_response_send(&res2_ex);
    memset(&res2_ex, 0, sizeof(res2_ex));
    c_rest_response_set_status(&res2_ex, 404);
    c_rest_response_send(&res2_ex);
    memset(&res2_ex, 0, sizeof(res2_ex));
    c_rest_response_set_status(&res2_ex, 500);
    c_rest_response_send(&res2_ex);
    c_rest_response_send(&res2_ex);

    {
      struct c_rest_connection_context conn_ctx;
      memset(&conn_ctx, 0, sizeof(conn_ctx));
      memset(&res2_ex, 0, sizeof(res2_ex));
      res2_ex.context = &conn_ctx;
      res2_ex.body = "body";
      res2_ex.body_len = 4;
      c_rest_response_send(&res2_ex);

      memset(&res2_ex, 0, sizeof(res2_ex));
      conn_ctx.tls_conn = (struct c_rest_tls_connection *)1; /* dummy ptr */
      res2_ex.context = &conn_ctx;
      res2_ex.body = "body";
      res2_ex.body_len = 4;
      c_rest_response_send(&res2_ex);
      res2_ex.body = NULL; /* prevent cleanup */
    }
  }

  /* request.c coverage extras */
  c_rest_request_cleanup(NULL);

  /* request.c NULL branches */
  {
    struct c_rest_request r;
    struct c_rest_response res3;
    const char *val;
    char *out;
    size_t len;
    void *json_obj;
    memset(&r, 0, sizeof(r));
    memset(&res3, 0, sizeof(res3));

    c_rest_request_get_header(&r, NULL, &val);
    c_rest_request_get_header(&r, "a", NULL);

    c_rest_request_get_cookie(&r, NULL, &val);
    c_rest_request_get_cookie(&r, "a", NULL);

    c_rest_request_get_query(&r, NULL, &val);
    c_rest_request_get_query(&r, "a", NULL);

    c_rest_request_get_form_param(NULL, "a", &val);
    c_rest_request_get_form_param(&r, NULL, &val);
    c_rest_request_get_form_param(&r, "a", NULL);

    c_rest_request_read_body(&r, NULL, &len);
    out = NULL;
    c_rest_request_read_body(&r, &out, NULL);
    c_rest_request_read_body(NULL, &out, &len);

    c_rest_request_parse_json(&r, NULL);
    c_rest_request_parse_json(NULL, &json_obj);

    c_rest_request_get_auth_bearer(&r, NULL);
    c_rest_request_get_auth_basic(NULL, &out, &out);
    c_rest_request_get_auth_basic(&r, NULL, &out);
    c_rest_request_get_auth_basic(&r, &out, NULL);

    /* response.c NULL branches */
    c_rest_response_set_header(&res3, NULL, "b");
    c_rest_response_set_header(&res3, "a", NULL);

    c_rest_response_set_cookie(&res3, NULL, "b", "c");
    c_rest_response_set_cookie(&res3, "a", NULL, "c");

    c_rest_response_json(&res3, NULL);
    c_rest_response_json_obj(&res3, NULL);
    c_rest_response_json_dict(NULL, NULL, 0);
    c_rest_response_json_dict(&res3, NULL, 0);

    c_rest_response_redirect(&res3, NULL, 302);

    c_rest_response_check_etag(NULL, &res3, "a");
    c_rest_response_check_etag(&r, NULL, "a");
    c_rest_response_check_etag(&r, &res3, NULL);

    c_rest_response_set_cache_control(&res3, NULL);

    c_rest_response_serialize(&res3, NULL, &len);
    c_rest_response_serialize(&res3, &out, NULL);

    c_rest_response_send_file(&res3, NULL);
    c_rest_response_send_file(&res3, "some_file.txt");

    c_rest_response_write_chunk(&res3, NULL, 5);
    c_rest_response_write_chunk(&res3, "chunk", 5);
    c_rest_response_write_chunk(NULL, "chunk", 5);

    c_rest_response_html(NULL, "hello");
    c_rest_response_html(&res3, NULL);
    c_rest_response_template(NULL, NULL, NULL, NULL, 0);
    c_rest_response_template(&res3, NULL, NULL, NULL, 0);

    c_rest_response_json_dict(NULL, NULL, 0);
    c_rest_response_json_obj(NULL, NULL);

    c_rest_response_cleanup(NULL);
  }

  {
    struct c_rest_request r;
    struct c_rest_header h;
    const char *val;
    memset(&r, 0, sizeof(r));
    h.key = "Cookie";
    h.value = "name=value;  ";
    h.next = NULL;
    r.headers = &h;
    c_rest_request_get_cookie(&r, "name", &val);
    r.headers = NULL;
  }

  {
    struct c_rest_request r;
    struct c_rest_header h;
    char *user = NULL, *pass = NULL;
    memset(&r, 0, sizeof(r));

    h.key = "Authorization";
    h.value = "Bearer xyz";
    h.next = NULL;
    r.headers = &h;
    c_rest_request_get_auth_basic(&r, &user, &pass);
    h.value = "Basic something";
    c_rest_request_get_auth_bearer(&r, &user);
    c_rest_request_get_auth_basic(&r, &user, &pass);

    h.value = "Basic $!";
    c_rest_request_get_auth_basic(&r, &user, &pass);

    h.value = "Basic bm9jb2xvbg==";
    c_rest_request_get_auth_basic(&r, &user, &pass);

    r.headers = NULL;
  }

  /* response.c coverage extras */
  {
    struct c_rest_response r;
    struct {
      int sock;
      void *tls;
      void *cm;
    } fake_ctx;
    memset(&r, 0, sizeof(r));
    fake_ctx.sock = -1;
    fake_ctx.tls = NULL;
    fake_ctx.cm = NULL;
    r.context = &fake_ctx;
    r.body = "body";
    r.body_len = 4;
    c_rest_response_send(&r);
    r.body = NULL;
    c_rest_response_cleanup(&r);
    memset(&r, 0, sizeof(r));

    r.headers_sent = 0;
    r.is_chunked = 1;
    c_rest_response_send(&r);
    c_rest_response_cleanup(&r);
    memset(&r, 0, sizeof(r));

    fake_ctx.tls = NULL;
    fake_ctx.cm = NULL;
    r.headers_sent = 0;
    r.is_chunked = 0;
    c_rest_response_send(&r);
    r.body = NULL;
    c_rest_response_cleanup(&r);
    memset(&r, 0, sizeof(r));

    r.headers_sent = 0;
    r.is_chunked = 1;
    r.context = &fake_ctx;
    c_rest_response_send(&r);
    c_rest_response_cleanup(&r);
    memset(&r, 0, sizeof(r));

    /* write_chunk with headers not sent */
    r.context = &fake_ctx;
    r.headers_sent = 0;
    r.is_chunked = 0;
    c_rest_response_write_chunk(&r, "chunk", 5);
    c_rest_response_write_chunk(&r, "chunk2", 6);
    c_rest_response_cleanup(&r);
    memset(&r, 0, sizeof(r));

    /* write_chunk with raw bytes (is_chunked = 0 but headers sent) */
    r.context = &fake_ctx;
    fake_ctx.tls = NULL;
#ifndef _WIN32
    fake_ctx.sock = -1;
#else
    fake_ctx.sock = (unsigned long long)-1;
#endif
    r.headers_sent = 1;
    r.is_chunked = 0;
    c_rest_response_write_chunk(&r, "chunk", 5);
    c_rest_response_cleanup(&r);
    memset(&r, 0, sizeof(r));

    /* write_chunk with TLS */
    r.context = &fake_ctx;
    fake_ctx.tls = (void *)1; /* Fake pointer to trigger tls_conn branch */
    r.headers_sent = 1;
    r.is_chunked = 1;
    c_rest_response_write_chunk(&r, "chunk", 5);
    r.is_chunked = 0;
    c_rest_response_write_chunk(&r, "chunk", 5);

    r.headers_sent = 0;
    r.body = "body";
    r.body_len = 4;
    c_rest_response_send(&r);
    r.body = NULL;
    c_rest_response_cleanup(&r);
    memset(&r, 0, sizeof(r));
  }

  for (i = 1; i <= 80; i++) {
    struct c_rest_request req2;
    struct c_rest_response res2;
    struct c_rest_header req2_h;
    const char *val;
    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    req2_h.key = "Cookie";
    req2_h.value = "name=value; foo=bar";
    req2_h.next = NULL;
    req2.headers = &req2_h;
    c_rest_request_get_cookie(&req2, "name", &val);
    req2.headers = NULL;

    req2.query = "a=1&b=2&c";
    c_rest_request_get_query(&req2, "a", &val);

    req2.body = "a=1&b=2&c";
    req2.body_len = 7;
    c_rest_request_parse_urlencoded(&req2);

    c_rest_response_set_header(&res2, "a", "b");
    c_rest_response_set_cookie(&res2, "a", "b", "c");
    c_rest_response_set_header(&res2, "a", "c");

    c_rest_response_check_etag(&req2, &res2, "etag");
    c_rest_response_set_cache_control(&res2, "policy");
    c_rest_response_json(&res2, "{\"a\":1}");

    {
      void *json_obj = json_parse_string("{\"a\":1}");
      if (json_obj) {
        c_rest_response_json_obj(&res2, json_obj);
        json_value_free(json_obj);
      }

      {
        struct c_rest_json_pair pairs[] = {
            {"access_token", C_REST_JSON_TYPE_STRING, "test_token_123", 0, 0}};
        c_rest_response_json_dict(&res2, pairs, 1);
      }
    }

    c_rest_response_redirect(&res2, "url", 302);

    {
      char *token;
      struct c_rest_header auth_bearer;
      auth_bearer.key = "Authorization";
      auth_bearer.value = "Bearer my-token-123";
      auth_bearer.next = NULL;
      req2.headers = &auth_bearer;
      if (c_rest_request_get_auth_bearer(&req2, &token) == C_REST_OK)
        CRF_FREE(token);
    }

    {
      char *user;
      char *pass;
      struct c_rest_header auth_basic;
      auth_basic.key = "Authorization";
      auth_basic.value = "Basic YWRtaW46c2VjcmV0MTIz";
      auth_basic.next = NULL;
      req2.headers = &auth_basic;
      if (c_rest_request_get_auth_basic(&req2, &user, &pass) == C_REST_OK) {
        CRF_FREE(user);
        CRF_FREE(pass);
      }
    }
    req2.headers = NULL;

    {
      char *out;
      size_t olen;
      if (c_rest_response_serialize(&res2, &out, &olen) == C_REST_OK)
        CRF_FREE(out);
    }

    {
      struct {
        int sock;
        void *tls;
        void *cm;
      } fake_ctx;
      fake_ctx.sock = -1;
      fake_ctx.tls = NULL;
      fake_ctx.cm = NULL;
      res2.context = &fake_ctx;
      res2.headers_sent = 0;
      c_rest_response_send(&res2);
      c_rest_response_cleanup(&res2);
      memset(&res2, 0, sizeof(res2));

      res2.headers_sent = 0;
      res2.is_chunked = 1;
      c_rest_response_send(&res2);
      c_rest_response_cleanup(&res2);
      memset(&res2, 0, sizeof(res2));
      res2.is_chunked = 0;
      res2.context = NULL;
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;

    req2.body = NULL; /* was a string literal */
    res2.body = NULL; /* was a string literal from json() or json_obj */
    c_rest_request_cleanup(&req2);
    c_rest_response_cleanup(&res2);
  }

  {
    int s;
    struct c_rest_response rr;
    char *out_buf = NULL;
    size_t out_len = 0;

    memset(&rr, 0, sizeof(rr));
    c_rest_response_redirect(&rr, "url", 999); /* Hit invalid status code */
    c_rest_response_redirect(&rr, "url", 200);

    /* Cover get_status_text branches */
    for (s = 100; s <= 600; s += 100) {
      memset(&rr, 0, sizeof(rr));
      c_rest_response_set_status(&rr, s);
      c_rest_response_serialize(&rr, &out_buf, &out_len);
      if (out_buf)
        CRF_FREE(out_buf);
      out_buf = NULL;
      c_rest_response_cleanup(&rr);
    }

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 201);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 202);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 204);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 301);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 304);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 400);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 401);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 403);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 404);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 405);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 500);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    memset(&rr, 0, sizeof(rr));
    c_rest_response_set_status(&rr, 501);
    c_rest_response_serialize(&rr, &out_buf, &out_len);
    if (out_buf)
      CRF_FREE(out_buf);
    out_buf = NULL;
    c_rest_response_cleanup(&rr);

    c_rest_response_oauth2_error(NULL, NULL, NULL);
    c_rest_response_oauth2_error(&rr, "error", NULL);
  }
}

int test_request_response(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  const char *val;
  int failed = 0;
  const char *msgs[2];
  test_coverage();

  {
    int i;
    extern int g_fail_malloc_at;

    for (i = 0; i <= 10; i++) {
      struct c_rest_response rr;
      void *json_obj = json_value_init_object();
      struct c_rest_json_pair pairs[] = {
          {"access_token", C_REST_JSON_TYPE_STRING, "test_token_123", 0, 0}};

      memset(&rr, 0, sizeof(rr));

      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;
      c_rest_response_json_obj(&rr, json_obj);

      g_fail_malloc_at = i;
      c_rest_response_json_dict(&rr, pairs, 1);

      g_fail_malloc_at = i;
      c_rest_response_html(&rr, "hello");

      g_fail_malloc_at = i;
      c_rest_response_template(NULL, NULL, NULL, NULL, 0);

      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;

      c_rest_response_cleanup(&rr);
      json_value_free(json_obj);
    }
  }

  {
    int i;
    extern int g_fail_malloc_at;

    for (i = 0; i <= 3; i++) {
      struct c_rest_request req_ex3;
      const char *val_ex3;

      memset(&req_ex3, 0, sizeof(req_ex3));
      req_ex3.query = "key1=value1&key2=value2";

      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;
      c_rest_request_get_query(&req_ex3, "key2", &val_ex3);

      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
      c_rest_request_cleanup(&req_ex3);
    }

    for (i = 0; i <= 3; i++) {
      struct c_rest_request req_ex3;
      const char *val_ex3;

      memset(&req_ex3, 0, sizeof(req_ex3));
      req_ex3.body = "key1=value1&key2=value2";
      req_ex3.body_len = strlen(req_ex3.body);

      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;
      c_rest_request_get_form_param(&req_ex3, "key2", &val_ex3);

      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
      req_ex3.body = NULL;
      c_rest_request_cleanup(&req_ex3);
    }
  }

  {
    struct c_rest_request req3;
    struct c_rest_header *dyn_h;
    memset(&req3, 0, sizeof(req3));

    dyn_h = (struct c_rest_header *)malloc(sizeof(struct c_rest_header));
    dyn_h->key = (char *)CRF_STRDUP("Dyn-Key");
    dyn_h->value = (char *)CRF_STRDUP("Dyn-Value");
    dyn_h->next = NULL;
    req3.headers = dyn_h;

    req3.cookies = (struct c_rest_header *)malloc(sizeof(struct c_rest_header));
    req3.cookies->key = (char *)CRF_STRDUP("Cookie-Key");
    req3.cookies->value = (char *)CRF_STRDUP("Cookie-Value");
    req3.cookies->next = NULL;

    req3.path_vars =
        (struct c_rest_path_var *)malloc(sizeof(struct c_rest_path_var));
    req3.path_vars->name = (char *)CRF_STRDUP("Path-Key");
    req3.path_vars->value = (char *)CRF_STRDUP("Path-Value");
    req3.path_vars->next = NULL;

    c_rest_request_cleanup(&req3);
  }

  printf("Running request/response tests...\n");

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* Test request query parsing */
  req.query = "id=123&name=test&empty=&no_val";

  val = NULL;
  (void)!c_rest_request_get_query(&req, "id", &val);
  failed += (!val || strcmp(val, "123") != 0);

  val = NULL;
  (void)!c_rest_request_get_query(&req, "name", &val);
  failed += (!val || strcmp(val, "test") != 0);

  val = NULL;
  (void)!c_rest_request_get_query(&req, "empty", &val);
  failed += (!val || strcmp(val, "") != 0);

  val = NULL;
  (void)!c_rest_request_get_query(&req, "no_val", &val);
  failed += (!val || strcmp(val, "") != 0);

  val = NULL;
  (void)!c_rest_request_get_query(&req, "missing", &val);
  failed += (val != NULL);

  /* Test response headers */
  (void)!c_rest_response_set_header(&res, "Content-Type", "text/plain");
  (void)!c_rest_response_set_header(&res, "Content-Type",
                                    "application/json"); /* Should replace */

  (void)!c_rest_response_set_cookie(&res, "session", "abc", "HttpOnly; Secure");
  (void)!c_rest_response_set_cookie(&res, "theme", "dark",
                                    NULL); /* Should not replace session */

  /* Verify headers */
  {
    int found_ct = 0;
    int found_cookie_session = 0;
    int found_cookie_theme = 0;
    struct c_rest_header *h;

    for (h = res.headers; h != NULL; h = h->next) {
      if (strcmp(h->key, "Content-Type") == 0) {
        found_ct = 1;
        failed += (strcmp(h->value, "application/json") != 0);
      } else if (strcmp(h->key, "Set-Cookie") == 0) {
        if (strstr(h->value, "session=abc") != NULL) {
          found_cookie_session = 1;
        } else if (strstr(h->value, "theme=dark") != NULL) {
          found_cookie_theme = 1;
        }
      }
    }

    failed += (!found_ct || !found_cookie_session || !found_cookie_theme);
  }

  /* Test ETag && Cache Control */
  {
    struct c_rest_header req_h_etag;
    req_h_etag.key = "If-None-Match";
    req_h_etag.value = "\"12345\"";
    req_h_etag.next = NULL;
    req.headers = &req_h_etag;

    (void)!c_rest_response_set_cache_control(&res, "max-age=3600");

    failed += (!c_rest_response_check_etag(&req, &res, "\"12345\""));
    failed += (res.status_code != 304);

    req_h_etag.value = "\"wrong_etag\"";
    failed += (c_rest_response_check_etag(&req, &res, "\"12345\"") != 0);

    req.headers = NULL; /* remove pointer to stack memory before cleanup */
  }

  /* Test Accepts Encoding */
  {
    struct c_rest_header req_h_enc;
    req_h_enc.key = "Accept-Encoding";
    req_h_enc.value = "gzip, deflate, br";
    req_h_enc.next = NULL;
    req.headers = &req_h_enc;

    failed += (!c_rest_request_accepts_encoding(&req, "gzip") ||
               !c_rest_request_accepts_encoding(&req, "br") ||
               c_rest_request_accepts_encoding(&req, "identity"));

    req.headers = NULL;
  }

  /* Test large body */
  {
    char *large_body = (char *)CRF_MALLOC(1024 * 1024); /* 1MB */
    char *read_ptr = NULL;
    size_t read_len = 0;
    if (large_body) {
      memset(large_body, 'A', 1024 * 1024 - 1);
      large_body[1024 * 1024 - 1] = '\0';
      req.body = large_body;
      req.body_len = 1024 * 1024 - 1;

      (void)!c_rest_request_read_body(&req, &read_ptr, &read_len);
      failed +=
          (!read_ptr || read_len != 1024 * 1024 - 1 || read_ptr[0] != 'A');
      CRF_FREE(large_body);
    }
  }

  /* Test helpers */
  (void)!c_rest_response_json(&res, "{\"hello\":\"world\"}");
  if (res.status_code != 0) { /* Defaults to 0 since we didn't set it */
  }
  failed += (strcmp(res.body, "{\"hello\":\"world\"}") != 0);

  /* Test JSON Request Parsing */
  {
    void *json_obj = NULL;
    req.body = "{\"key\": \"value\"}";
    req.body_len = strlen(req.body);
    failed += (c_rest_request_parse_json(&req, &json_obj) != 0 || !json_obj);

    /* Test JSON Response Generation */
    res.headers_sent = 0; /* Reset state */
    failed += (c_rest_response_json_obj(&res, json_obj) != 0);
    json_value_free(json_obj);
    req.body = NULL;
  }

  /* Test JSON Dict Generation */
  {
    struct c_rest_json_pair pairs[] = {
        {"access_token", C_REST_JSON_TYPE_STRING, "test_token_123", 0, 0},
        {"expires_in", C_REST_JSON_TYPE_NUMBER, NULL, 3600.0, 0},
        {"is_active", C_REST_JSON_TYPE_BOOLEAN, NULL, 0, 1},
        {"refresh_token", C_REST_JSON_TYPE_NULL, NULL, 0, 0},
        {"invalid", (enum c_rest_json_val_type)99, NULL, 0, 0}};

    res.headers_sent = 0; /* Reset state */
    failed += (c_rest_response_json_dict(&res, pairs, 5) != 0);
    failed += !res.body ||
              (strstr(res.body, "\"access_token\":\"test_token_123\"") == NULL);
    (void)!c_rest_response_cleanup(&res);
  }

  /* Test URL Encoded Parsing */
  {
    const char *form_val = NULL;
    req.body = "username=admin&password=123%20456&grant_type=password";
    req.body_len = strlen(req.body);
    /* Form params list starts empty since req was cleaned up/not initialized
     * for this */
    failed += (c_rest_request_parse_urlencoded(&req) != 0);
    failed +=
        (c_rest_request_get_form_param(&req, "username", &form_val) != 0 ||
         strcmp(form_val, "admin") != 0);
    failed +=
        (c_rest_request_get_form_param(&req, "password", &form_val) != 0 ||
         strcmp(form_val, "123 456") != 0);
    req.body = NULL; /* Prevent free of string literal */
  }

  /* Test Auth Extraction */
  {
    struct c_rest_header auth_bearer;
    struct c_rest_header auth_basic;
    char *token = NULL;
    char *user = NULL;
    char *pass = NULL;

    auth_bearer.key = "Authorization";
    auth_bearer.value = "Bearer my-token-123";
    auth_bearer.next = NULL;

    req.headers = &auth_bearer;
    failed += (c_rest_request_get_auth_bearer(&req, &token) != 0 ||
               strcmp(token, "my-token-123") != 0);
    CRF_FREE(token);

    auth_basic.key = "Authorization";
    /* "admin:secret123" base64 encoded is "YWRtaW46c2VjcmV0MTIz" */
    auth_basic.value = "Basic YWRtaW46c2VjcmV0MTIz";
    auth_basic.next = NULL;

    req.headers = &auth_basic;
    failed += (c_rest_request_get_auth_basic(&req, &user, &pass) != 0 ||
               strcmp(user, "admin") != 0 || strcmp(pass, "secret123") != 0);
    CRF_FREE(user);
    CRF_FREE(pass);

    req.headers = NULL; /* Clean stack pointer */
  }

  /* Test response serialize */
  {
    struct c_rest_response ser_res;
    char *out_buf = NULL;
    size_t out_len = 0;

    memset(&ser_res, 0, sizeof(ser_res));
    (void)!c_rest_response_set_status(&ser_res, 200);
    (void)!c_rest_response_set_header(&ser_res, "Content-Type", "text/plain");

    ser_res.body = (char *)CRF_MALLOC(12);
    if (ser_res.body) {
#if defined(_MSC_VER)
      strcpy_s(ser_res.body, 12, "Hello World");
#else
      strcpy(ser_res.body, "Hello World");
#endif
    }
    ser_res.body_len = 11;

    failed += (c_rest_response_serialize(&ser_res, &out_buf, &out_len) != 0 ||
               !out_buf);

    if (out_buf) {
      failed += (strstr(out_buf, "HTTP/1.1 200 OK\r\n") == NULL);
      failed += (strstr(out_buf, "Content-Type: text/plain\r\n") == NULL);
      failed += (strstr(out_buf, "Content-Length: 11\r\n") == NULL);
      failed += (strstr(out_buf, "\r\n\r\nHello World") == NULL);
      CRF_FREE(out_buf);
    }
    (void)!c_rest_response_cleanup(&ser_res);
  }

  /* Test OAuth2 Error */
  {
    struct c_rest_response err_res;
    memset(&err_res, 0, sizeof(err_res));
    failed += (c_rest_response_oauth2_error(&err_res, "invalid_request",
                                            "Missing parameter") != 0);
    failed += (err_res.status_code != 400);
    if (err_res.body) {
      failed += (strstr(err_res.body, "\"error\":\"invalid_request\"") == NULL);
      failed += (strstr(err_res.body,
                        "\"error_description\":\"Missing parameter\"") == NULL);
    }
    (void)!c_rest_response_cleanup(&err_res);
  }

  (void)!c_rest_request_cleanup(&req);
  (void)!c_rest_response_cleanup(&res);

  msgs[0] = "test_request_response passed\n";
  msgs[1] = "test_request_response failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}
