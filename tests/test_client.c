/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_client.h"
#include "c_rest_client_hooks.h"
#include "c_rest_tls.h"
#include "c_rest_platform.h"
#include <parson.h>
#include <c_abstract_http/http_types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static int g_client_fail_malloc_at = 0;
static int g_client_fail_realloc_at = 0;
static int g_client_fail_calloc_at = 0;

static int async_called = 0;

static void async_callback(struct c_rest_client_response *res, void *data) {
  (void)res;
  (void)data;
  async_called = 1;
}

static c_rest_error_t proxy_mock_server_thread(void *arg) {
  c_rest_socket_t srv = *(c_rest_socket_t *)arg;
  c_rest_socket_t cli = C_REST_INVALID_SOCKET;
  char req_buf[512];
  size_t rd = 0;
  size_t wr = 0;
  static const char resp[] =
      "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}";

  if (c_rest_socket_accept(srv, &cli) == C_REST_OK) {
    if (c_rest_socket_recv(cli, req_buf, sizeof(req_buf), &rd) == C_REST_OK) {
      if (c_rest_socket_send(cli, resp, sizeof(resp) - 1, &wr) != C_REST_OK) {
        wr = 0;
      }
    }
    if (c_rest_socket_close(cli) != C_REST_OK) {
      cli = C_REST_INVALID_SOCKET;
    }
  }
  return C_REST_OK;
}

struct c_rest_client_context {
  struct HttpClient client;
};

static int g_mock_send_return_null = 0;
static int g_mock_send_no_body = 0;
static int g_mock_send_null_headers = 0;
static int g_mock_send_body_len_only = 0;

static c_abstract_http_error_t mock_send_full(struct HttpTransportContext *ctx,
                                              const struct HttpRequest *req,
                                              struct HttpResponse **res) {
  struct HttpResponse *r;
  (void)ctx;
  (void)req;

  if (g_mock_send_return_null) {
    *res = NULL;
    return 0;
  }

  r = (struct HttpResponse *)malloc(sizeof(struct HttpResponse));
  memset(r, 0, sizeof(*r));
  r->status_code = 200;

  if (!g_mock_send_null_headers) {
    char *kbuf = (char *)malloc(13);
    char *vbuf = (char *)malloc(17);
    r->headers.count = 1;
    r->headers.headers = (struct HttpHeader *)malloc(sizeof(struct HttpHeader));
    memcpy(kbuf, "Content-Type", 13);
    memcpy(vbuf, "application/json", 17);
    r->headers.headers[0].key = kbuf;
    r->headers.headers[0].value = vbuf;
  } else {
    r->headers.count = 1;
    r->headers.headers = (struct HttpHeader *)malloc(sizeof(struct HttpHeader));
    r->headers.headers[0].key = NULL;
    r->headers.headers[0].value = NULL;
  }

  if (g_mock_send_body_len_only) {
    r->body_len = 2;
    r->body = NULL;
  } else if (!g_mock_send_no_body) {
    r->body_len = 2;
    r->body = malloc(2);
    memcpy(r->body, "{}", 2);
  } else {
    r->body_len = 0;
    r->body = NULL;
  }

  *res = r;
  return 0;
}

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  if (g_client_fail_malloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_client_fail_malloc_at) {
    alloc_count = 0;
    g_client_fail_malloc_at = 0;
    return NULL;
  }
  return malloc(size);
}

static void *fail_realloc_n(void *ptr, size_t size) {
  static int alloc_count = 0;
  if (g_client_fail_realloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_client_fail_realloc_at) {
    alloc_count = 0;
    g_client_fail_realloc_at = 0;
    return NULL;
  }
  return realloc(ptr, size);
}

static void *fail_calloc_n(size_t nmemb, size_t size) {
  static int alloc_count = 0;
  if (g_client_fail_calloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_client_fail_calloc_at) {
    alloc_count = 0;
    g_client_fail_calloc_at = 0;
    return NULL;
  }
  return calloc(nmemb, size);
}

static void test_safe_free(void **ptr) {
  if (ptr && *ptr) {
    C_REST_FREE(*ptr);
    *ptr = NULL;
  }
}

static NO_DISCARD c_rest_error_t
test_safe_response_free(struct c_rest_client_response **res) {
  if (res && *res) {
    c_rest_error_t rc = c_rest_client_response_free(*res);
    *res = NULL;
    return rc;
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
test_safe_fields_free(struct c_rest_client_form_field **fields, size_t *count) {
  if (fields && *fields) {
    c_rest_error_t rc =
        c_rest_client_form_fields_free(*fields, count ? *count : 0);
    *fields = NULL;
    if (count)
      *count = 0;
    return rc;
  }
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t
test_safe_headers_free(struct c_rest_client_header **headers, size_t *count) {
  if (headers && *headers) {
    c_rest_error_t rc =
        c_rest_client_headers_free(*headers, count ? *count : 0);
    *headers = NULL;
    if (count)
      *count = 0;
    return rc;
  }
  return C_REST_OK;
}

static void test_safe_json_free(void **json) {
  if (json && *json) {
    json_value_free((JSON_Value *)*json);
    *json = NULL;
  }
}

TEST test_client_mocks_and_null_guards(void) {
  void *p_helper = malloc(8);
  struct c_rest_client_response *r_helper =
      (struct c_rest_client_response *)malloc(sizeof(*r_helper));
  struct c_rest_client_form_field *f_helper = NULL;
  size_t c_f_helper = 0;
  struct c_rest_client_header *h_helper = NULL;
  size_t c_h_helper = 0;
  JSON_Value *j_helper = json_value_init_object();
  void *v_helper = (void *)j_helper;

  test_safe_free(NULL);
  test_safe_free(&p_helper);
  test_safe_free(&p_helper);

  memset(r_helper, 0, sizeof(*r_helper));
  ASSERT_EQ(C_REST_OK, test_safe_response_free(NULL));
  ASSERT_EQ(C_REST_OK, test_safe_response_free(&r_helper));
  ASSERT_EQ(C_REST_OK, test_safe_response_free(&r_helper));

  ASSERT_EQ(C_REST_OK, test_safe_fields_free(NULL, NULL));
  ASSERT_EQ(C_REST_OK, test_safe_fields_free(&f_helper, NULL));
  ASSERT_EQ(C_REST_OK,
            c_rest_client_parse_form_urlencoded("a=b", &f_helper, &c_f_helper));
  ASSERT_EQ(C_REST_OK, test_safe_fields_free(&f_helper, &c_f_helper));
  ASSERT_EQ(C_REST_OK, test_safe_fields_free(&f_helper, &c_f_helper));
  f_helper = (struct c_rest_client_form_field *)malloc(sizeof(*f_helper));
  memset(f_helper, 0, sizeof(*f_helper));
  ASSERT_EQ(C_REST_OK, test_safe_fields_free(&f_helper, NULL));

  ASSERT_EQ(C_REST_OK, test_safe_headers_free(NULL, NULL));
  ASSERT_EQ(C_REST_OK, test_safe_headers_free(&h_helper, NULL));
  ASSERT_EQ(C_REST_OK,
            c_rest_client_header_set(&h_helper, &c_h_helper, "a", "b"));
  ASSERT_EQ(C_REST_OK, test_safe_headers_free(&h_helper, &c_h_helper));
  ASSERT_EQ(C_REST_OK, test_safe_headers_free(&h_helper, &c_h_helper));
  h_helper = (struct c_rest_client_header *)malloc(sizeof(*h_helper));
  memset(h_helper, 0, sizeof(*h_helper));
  ASSERT_EQ(C_REST_OK, test_safe_headers_free(&h_helper, NULL));

  test_safe_json_free(NULL);
  test_safe_json_free(&v_helper);
  test_safe_json_free(&v_helper);

  ASSERT_NEQ(C_REST_OK, c_rest_client_init(NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_client_destroy(NULL));

  ASSERT_NEQ(C_REST_OK, c_rest_client_request_sync(NULL, "http://a", "GET",
                                                   NULL, 0, NULL, 0, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_client_request_sync(NULL, "http://a", NULL, NULL,
                                                   0, NULL, 0, NULL));
  ASSERT_NEQ(C_REST_OK,
             c_rest_client_request_async(NULL, "http://a", "GET", NULL, 0, NULL,
                                         0, NULL, NULL));

  /* Edge cases for urlencode/decode */
  {
    char *out_enc = NULL;
    char *out_dec = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_client_url_encode("-_.!~*'()", &out_enc));
    C_REST_FREE(out_enc);
    out_enc = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%2", &out_dec));
    C_REST_FREE(out_dec);
    out_dec = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%2G", &out_dec));
    C_REST_FREE(out_dec);
    out_dec = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%G2", &out_dec));
    C_REST_FREE(out_dec);
    out_dec = NULL;

    ASSERT_NEQ(C_REST_OK, c_rest_client_url_encode(NULL, &out_enc));
    ASSERT_NEQ(C_REST_OK, c_rest_client_url_encode("test", NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_client_url_decode(NULL, &out_dec));
    ASSERT_NEQ(C_REST_OK, c_rest_client_url_decode("test", NULL));

    /* Test g_mock_client_fail branches */
    {
      c_rest_client_context *tc = NULL;
      char *t_enc = NULL;
      char *t_hdr = NULL;
      struct c_rest_client_response *t_sr = NULL;
      struct c_rest_client_header t_hdrs[1];
      struct c_rest_client_form_field t_ff[1];

      g_mock_client_fail = 1;
      ASSERT_NEQ(C_REST_OK, c_rest_client_init(&tc));
      g_mock_client_fail = 2;
      ASSERT_NEQ(C_REST_OK, c_rest_client_init(&tc));
      g_mock_client_fail = 0;

      ASSERT_EQ(C_REST_OK, c_rest_client_init(&tc));
      g_mock_client_fail = 11;
      ASSERT_NEQ(C_REST_OK, c_rest_client_destroy(tc));
      g_mock_client_fail = 14;
      ASSERT_NEQ(C_REST_OK, c_rest_client_destroy(tc));
      {
        struct HttpTransportContext *saved_tr = tc->client.transport;
        tc->client.transport = NULL;
        g_mock_client_fail = 15;
        ASSERT_NEQ(C_REST_OK, c_rest_client_destroy(tc));
        tc->client.transport = saved_tr;
      }
      g_mock_client_fail = 0;

      g_mock_client_fail = 12;
      ASSERT_NEQ(C_REST_OK,
                 c_rest_client_request_sync(tc, "http://a", "GET", NULL, 0,
                                            NULL, 0, &t_sr));
      g_mock_client_fail = 0;

      {
        http_send_fn old_send = tc->client.send;
        tc->client.send = mock_send_full;
        g_mock_client_fail = 16;
        ASSERT_NEQ(C_REST_OK,
                   c_rest_client_request_sync(tc, "http://a", "GET", NULL, 0,
                                              NULL, 0, &t_sr));
        ASSERT_EQ(C_REST_OK, test_safe_response_free(&t_sr));
        g_mock_client_fail = 17;
        ASSERT_NEQ(C_REST_OK,
                   c_rest_client_request_sync(tc, "http://a", "GET", NULL, 0,
                                              NULL, 0, &t_sr));
        ASSERT_EQ(C_REST_OK, test_safe_response_free(&t_sr));
        g_mock_client_fail = 0;
        tc->client.send = old_send;
      }

      g_mock_client_fail = 20;
      ASSERT_NEQ(C_REST_OK, c_rest_client_url_encode("@", &t_enc));
      g_mock_client_fail = 21;
      ASSERT_NEQ(C_REST_OK, c_rest_client_url_encode("@", &t_enc));
      g_mock_client_fail = 0;

      g_mock_client_fail = 30;
      ASSERT_NEQ(C_REST_OK, c_rest_client_build_auth_basic("u", "p", &t_hdr));
      g_mock_client_fail = 31;
      ASSERT_NEQ(C_REST_OK, c_rest_client_build_auth_basic("u", "p", &t_hdr));
      g_mock_client_fail = 0;

      t_hdrs[0].key = "k";
      t_hdrs[0].value = "v";
      g_mock_client_fail = 40;
      ASSERT_NEQ(C_REST_OK, c_rest_client_headers_free(t_hdrs, 1));
      g_mock_client_fail = 0;

      t_ff[0].key = "k";
      t_ff[0].value = "v";
      g_mock_client_fail = 40;
      ASSERT_NEQ(C_REST_OK, c_rest_client_post_form_sync(tc, "http://a", t_hdrs,
                                                         1, t_ff, 1, &t_sr));
      g_mock_client_fail = 0;

      ASSERT_EQ(C_REST_OK, c_rest_client_url_encode("\xBB", &t_enc));
      C_REST_FREE(t_enc);
      t_enc = NULL;
      g_mock_client_fail = 21;
      ASSERT_NEQ(C_REST_OK, c_rest_client_url_encode("\xBB", &t_enc));
      g_mock_client_fail = 0;
      C_REST_FREE(t_enc);
      t_enc = NULL;

      /* Header with NULL key in request_sync */
      {
        http_send_fn old_send = tc->client.send;
        tc->client.send = mock_send_full;
        t_hdrs[0].key = NULL;
        t_hdrs[0].value = "v";
        ASSERT_NEQ(C_REST_OK,
                   c_rest_client_request_sync(tc, "http://a", "GET", t_hdrs, 1,
                                              NULL, 0, &t_sr));
        ASSERT_EQ(C_REST_OK, test_safe_response_free(&t_sr));
        tc->client.send = old_send;
      }

      /* NULL client.send */
      {
        http_send_fn old_send = tc->client.send;
        tc->client.send = NULL;
        ASSERT_NEQ(C_REST_OK,
                   c_rest_client_request_sync(tc, "http://a", "GET", NULL, 0,
                                              NULL, 0, &t_sr));
        tc->client.send = old_send;
      }

      /* Async response free failure */
      {
        http_send_fn old_send = tc->client.send;
        tc->client.send = mock_send_full;
        g_mock_client_fail = 10;
        ASSERT_NEQ(C_REST_OK,
                   c_rest_client_request_async(tc, "http://a", "GET", NULL, 0,
                                               NULL, 0, async_callback, NULL));
        g_mock_client_fail = 0;
        tc->client.send = old_send;
      }

      /* Form urlencode value encode failure with non-null and null ekey */
      {
        char *b_out = NULL;
        size_t b_out_len = 0;
        t_ff[0].key = "k";
        t_ff[0].value = "@";
        g_mock_client_fail = 20;
        ASSERT_NEQ(C_REST_OK, c_rest_client_build_form_urlencoded(
                                  t_ff, 1, &b_out, &b_out_len));
        t_ff[0].key = NULL;
        ASSERT_NEQ(C_REST_OK, c_rest_client_build_form_urlencoded(
                                  t_ff, 1, &b_out, &b_out_len));
        g_mock_client_fail = 0;
      }

      /* Post form sync with headers_free fail and request_sync success */
      {
        http_send_fn old_send = tc->client.send;
        tc->client.send = mock_send_full;
        t_ff[0].key = "k";
        t_ff[0].value = "v";
        g_mock_client_fail = 40;
        ASSERT_NEQ(C_REST_OK, c_rest_client_post_form_sync(tc, "http://a", NULL,
                                                           0, t_ff, 1, &t_sr));
        g_mock_client_fail = 0;
        tc->client.send = old_send;
        ASSERT_EQ(C_REST_OK, test_safe_response_free(&t_sr));
      }

      ASSERT_NEQ(C_REST_OK, c_rest_proxy_request("://invalid", NULL, NULL));
      g_mock_client_fail = 10;
      ASSERT_NEQ(C_REST_OK, c_rest_proxy_request("://invalid", NULL, NULL));
      {
        int ci;
        for (ci = 1; ci <= 4; ci++) {
          g_client_fail_calloc_at = -1;
          fail_calloc_n(0, 0);
          g_crf_calloc_hook = fail_calloc_n;
          g_client_fail_calloc_at = ci;
          ASSERT_NEQ(C_REST_OK, c_rest_proxy_request("://invalid", NULL, NULL));
          g_crf_calloc_hook = NULL;
          g_client_fail_calloc_at = 0;
        }
      }
      g_mock_client_fail = 11;
      ASSERT_NEQ(C_REST_OK, c_rest_proxy_request("://invalid", NULL, NULL));
      g_mock_client_fail = 13;
      ASSERT_NEQ(C_REST_OK, c_rest_proxy_request("://invalid", NULL, NULL));
      g_mock_client_fail = 0;

      {
        struct c_rest_client_form_field *bad_ff = NULL;
        size_t bad_count = 0;
        ASSERT_NEQ(C_REST_OK, c_rest_client_form_fields_free(NULL, 0));
        g_mock_client_fail = 41;
        ASSERT_NEQ(C_REST_OK,
                   c_rest_client_form_fields_free(
                       tc ? (struct c_rest_client_form_field *)1 : NULL, 0));
        g_mock_client_fail = 43;
        ASSERT_NEQ(C_REST_OK, c_rest_client_parse_form_urlencoded(
                                  "a=1", &bad_ff, &bad_count));
        g_mock_client_fail = 45;
        ASSERT_NEQ(C_REST_OK, c_rest_client_parse_form_urlencoded(
                                  "a=1", &bad_ff, &bad_count));
        g_mock_client_fail = 44;
        ASSERT_NEQ(C_REST_OK, c_rest_client_parse_form_urlencoded(
                                  "a=1", &bad_ff, &bad_count));
        g_mock_client_fail = 46;
        ASSERT_NEQ(C_REST_OK, c_rest_client_parse_form_urlencoded(
                                  "a=1", &bad_ff, &bad_count));
        g_mock_client_fail = 0;
      }

      {
        http_send_fn old_send = tc->client.send;
        tc->client.send = mock_send_full;
        t_hdrs[0].key = "k";
        t_hdrs[0].value = "v";
        t_ff[0].key = "k";
        t_ff[0].value = "v";
        g_mock_client_fail = 42;
        ASSERT_NEQ(C_REST_OK, c_rest_client_post_form_sync(
                                  tc, "http://a", t_hdrs, 1, t_ff, 1, &t_sr));
        ASSERT_NEQ(C_REST_OK, c_rest_client_post_form_sync(tc, "http://a", NULL,
                                                           0, t_ff, 1, &t_sr));
        g_mock_client_fail = 0;
        tc->client.send = old_send;
      }

      ASSERT_EQ(C_REST_OK, c_rest_client_destroy(tc));
    }
  }

  PASS();
}

TEST test_client_oom_and_edge_branches(void) {
  int i;
  c_rest_error_t rc;

  /* mock_send_full malloc testing */
  {
    c_rest_client_context *c = NULL;
    struct c_rest_client_response *sr = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_client_init(&c));
    c->client.send = mock_send_full;

    ASSERT_NEQ(C_REST_OK, c_rest_client_request_sync(c, "http://a", NULL, NULL,
                                                     0, NULL, 0, &sr));
    ASSERT_NEQ(C_REST_OK, c_rest_client_request_sync(c, NULL, "GET", NULL, 0,
                                                     NULL, 0, &sr));
    ASSERT_NEQ(C_REST_OK, c_rest_client_request_sync(NULL, "http://a", "GET",
                                                     NULL, 0, NULL, 0, &sr));

    /* Missing transport */
    {
      struct HttpTransportContext *tmp = c->client.transport;
      c->client.transport = NULL;
      rc = c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0,
                                      &sr);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_INVALID_ARG ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));
      c->client.transport = tmp;
    }

    /* Async NULL callback */
    ASSERT_EQ(C_REST_OK, c_rest_client_request_async(c, "http://a", "GET", NULL,
                                                     0, NULL, 0, NULL, NULL));

    for (i = 1; i <= 20; i++) {
      g_client_fail_calloc_at = -1;
      fail_calloc_n(0, 0);
      g_crf_calloc_hook = fail_calloc_n;
      g_client_fail_calloc_at = i;

      rc = c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0,
                                      &sr);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));

      g_crf_calloc_hook = NULL;
      g_client_fail_calloc_at = 0;

      g_client_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_client_fail_malloc_at = i;

      rc = c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0,
                                      &sr);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));

      g_crf_malloc_hook = NULL;
      g_client_fail_malloc_at = 0;
    }

    ASSERT_EQ(C_REST_OK, c_rest_client_destroy(c));
  }

  /* Further edge case tests for client.c */
  {
    c_rest_client_context *c = NULL;
    struct c_rest_client_response *sr = NULL;
    struct c_rest_client_form_field fields[1];
    struct c_rest_client_header hdrs[1];
    ASSERT_EQ(C_REST_OK, c_rest_client_init(&c));
    hdrs[0].key = "k";
    hdrs[0].value = "v";
    fields[0].key = "k";
    fields[0].value = "v";
    c->client.send = mock_send_full;

    ASSERT_NEQ(C_REST_OK, c_rest_client_post_form_sync(NULL, "http://a", hdrs,
                                                       1, fields, 1, &sr));
    ASSERT_NEQ(C_REST_OK,
               c_rest_client_post_form_sync(c, NULL, hdrs, 1, fields, 1, &sr));
    ASSERT_EQ(C_REST_OK, c_rest_client_post_form_sync(c, "http://a", NULL, 1,
                                                      fields, 1, &sr));
    ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));
    ASSERT_EQ(C_REST_OK, c_rest_client_post_form_sync(c, "http://a", hdrs, 0,
                                                      fields, 1, &sr));
    ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));
    ASSERT_EQ(C_REST_OK, c_rest_client_post_form_sync(c, "http://a", hdrs, 1,
                                                      NULL, 0, &sr));
    ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));

    ASSERT_EQ(C_REST_OK, c_rest_client_destroy(c));
  }

  {
    struct c_rest_client_header *hdr = NULL;
    size_t hc = 0;
    char *hdr_val = NULL;

    ASSERT_NEQ(C_REST_OK, c_rest_client_header_set(&hdr, NULL, "a", "b"));
    ASSERT_NEQ(C_REST_OK, c_rest_client_header_set(&hdr, &hc, NULL, "b"));
    ASSERT_NEQ(C_REST_OK, c_rest_client_header_set(&hdr, &hc, "a", NULL));

    ASSERT_EQ(C_REST_OK, c_rest_client_header_set(&hdr, &hc, "a", "b"));
    C_REST_FREE((void *)hdr[0].key);
    hdr[0].key = NULL;
    C_REST_FREE((void *)hdr[0].value);
    hdr[0].value = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_client_headers_free(hdr, hc));

    ASSERT_NEQ(C_REST_OK,
               c_rest_client_build_auth_basic("user", NULL, &hdr_val));
    ASSERT_NEQ(C_REST_OK, c_rest_client_build_auth_basic("user", "pass", NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_client_build_auth_bearer("token", NULL));
  }

  {
    struct c_rest_client_response res_obj;
    void *json_out = NULL;
    char empty_obj[] = "{}";
    memset(&res_obj, 0, sizeof(res_obj));
    res_obj.body = empty_obj;
    res_obj.body_len = 0;
    ASSERT_NEQ(C_REST_OK,
               c_rest_client_response_parse_json(&res_obj, &json_out));

    res_obj.body = NULL;
    res_obj.body_len = 2;
    ASSERT_NEQ(C_REST_OK,
               c_rest_client_response_parse_json(&res_obj, &json_out));
  }

  for (i = 1; i <= 5; i++) {
    g_client_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_client_fail_malloc_at = i;
    ASSERT_NEQ(C_REST_OK, c_rest_proxy_request("://invalid", NULL, NULL));
    g_crf_malloc_hook = NULL;
    g_client_fail_malloc_at = 0;
  }

  {
    c_rest_client_context *c = NULL;
    struct c_rest_client_response *sr = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_client_init(&c));
    c->client.send = mock_send_full;

    g_mock_send_body_len_only = 1;
    ASSERT_EQ(C_REST_OK, c_rest_client_request_sync(c, "http://a", "GET", NULL,
                                                    0, NULL, 0, &sr));
    ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));
    g_mock_send_body_len_only = 0;

    g_client_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_client_fail_malloc_at = 1;
    rc = c_rest_client_request_async(c, "http://a", "GET", NULL, 0, NULL, 0,
                                     NULL, NULL);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    g_crf_malloc_hook = NULL;
    g_client_fail_malloc_at = 0;

    ASSERT_EQ(C_REST_OK, c_rest_client_destroy(c));
  }

  {
    c_rest_client_context *c = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_client_init(&c));
    ASSERT_NEQ(C_REST_OK,
               c_rest_client_request_async(c, "://invalid", "GET", NULL, 0,
                                           NULL, 0, NULL, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_client_request_async(c, "http://a", NULL, NULL,
                                                      0, NULL, 0, NULL, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_client_request_async(c, NULL, "GET", NULL, 0,
                                                      NULL, 0, NULL, NULL));
    {
      struct HttpTransportContext *tmp = c->client.transport;
      c->client.transport = NULL;
      ASSERT_NEQ(C_REST_OK,
                 c_rest_client_request_async(c, "http://a", "GET", NULL, 0,
                                             NULL, 0, NULL, NULL));
      c->client.transport = tmp;
    }
    ASSERT_EQ(C_REST_OK, c_rest_client_destroy(c));
  }

  {
    char *enc = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_client_url_encode(" ", &enc));
    C_REST_FREE(enc);
    enc = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_client_url_encode("\xFF", &enc));
    C_REST_FREE(enc);
    enc = NULL;
  }

  {
    c_rest_client_context *c = NULL;
    struct c_rest_client_form_field fields[1];
    ASSERT_EQ(C_REST_OK, c_rest_client_init(&c));
    fields[0].key = "a";
    fields[0].value = "b";
    c->client.send = mock_send_full;
    ASSERT_EQ(C_REST_OK, c_rest_client_post_form_sync(c, "http://a", NULL, 0,
                                                      fields, 1, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_client_destroy(c));
  }

  {
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;

    for (i = 1; i <= 20; i++) {
      g_client_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_client_fail_malloc_at = i;

      rc = c_rest_client_parse_form_urlencoded("a&b", &pf, &pc);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, test_safe_fields_free(&pf, &pc));

      g_crf_malloc_hook = NULL;
      g_client_fail_malloc_at = 0;
    }

    {
      char *dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%0", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%2+", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%+2", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%0A", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%1a", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%1A", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%g1", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%1g", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%20", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%09", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%A1", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("%a1", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("a+%ZZ", &dec));
      C_REST_FREE(dec);
      dec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_client_url_decode("a+%6a", &dec));
      C_REST_FREE(dec);
      dec = NULL;
    }

    ASSERT_EQ(C_REST_OK, c_rest_client_parse_form_urlencoded("a=b", &pf, &pc));
    ASSERT_EQ(C_REST_OK, c_rest_client_form_fields_free(pf, pc));
    pf = NULL;

    ASSERT_EQ(C_REST_OK,
              c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc));
    ASSERT_EQ(C_REST_OK, c_rest_client_form_fields_free(pf, pc));
    pf = NULL;

    ASSERT_EQ(C_REST_OK,
              c_rest_client_parse_form_urlencoded("ab&c=d", &pf, &pc));
    ASSERT_EQ(C_REST_OK, c_rest_client_form_fields_free(pf, pc));
    pf = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_client_parse_form_urlencoded("", &pf, &pc));
    ASSERT_EQ(C_REST_OK, test_safe_fields_free(&pf, &pc));

    ASSERT_EQ(C_REST_OK, c_rest_client_parse_form_urlencoded("a", &pf, &pc));
    ASSERT_EQ(C_REST_OK, test_safe_fields_free(&pf, &pc));

    for (i = 1; i <= 20; i++) {
      g_client_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_client_fail_malloc_at = i;

      rc = c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, test_safe_fields_free(&pf, &pc));

      g_crf_malloc_hook = NULL;
      g_client_fail_malloc_at = 0;
    }

    for (i = 1; i <= 10; i++) {
      g_client_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_client_fail_malloc_at = i;

      rc = c_rest_client_parse_form_urlencoded("a=b", &pf, &pc);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, test_safe_fields_free(&pf, &pc));

      g_crf_malloc_hook = NULL;
      g_client_fail_malloc_at = 0;
    }
  }

  {
    c_rest_client_context *c = NULL;
    struct c_rest_client_response *sr = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_client_init(&c));
    c->client.send = mock_send_full;

    ASSERT_EQ(C_REST_OK, c_rest_client_request_sync(c, "http://a", "GET", NULL,
                                                    0, "b", 0, &sr));
    ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));

    ASSERT_EQ(C_REST_OK, c_rest_client_request_async(c, "http://a", "GET", NULL,
                                                     0, "b", 0, NULL, NULL));

    /* Null out_res */
    ASSERT_EQ(C_REST_OK, c_rest_client_request_sync(c, "http://a", "GET", NULL,
                                                    0, NULL, 0, NULL));

    /* mock_send returns NULL */
    g_mock_send_return_null = 1;
    ASSERT_EQ(C_REST_OK, c_rest_client_request_sync(c, "http://a", "GET", NULL,
                                                    0, NULL, 0, &sr));
    sr = NULL;
    g_mock_send_return_null = 0;

    /* mock_send returns no body */
    g_mock_send_no_body = 1;
    ASSERT_EQ(C_REST_OK, c_rest_client_request_sync(c, "http://a", "GET", NULL,
                                                    0, NULL, 0, &sr));
    ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));
    g_mock_send_no_body = 0;

    /* mock_send returns null keys/values in headers */
    g_mock_send_null_headers = 1;
    ASSERT_EQ(C_REST_OK, c_rest_client_request_sync(c, "http://a", "GET", NULL,
                                                    0, NULL, 0, &sr));
    ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));
    g_mock_send_null_headers = 0;

    ASSERT_EQ(C_REST_OK, c_rest_client_request_sync(c, "http://a", "GET", NULL,
                                                    0, "test", 4, &sr));
    ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));

    ASSERT_EQ(C_REST_OK, c_rest_client_destroy(c));
  }

  /* Form build/parse edge cases */
  {
    char *body = NULL;
    size_t body_len = 0;
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;
    struct c_rest_client_form_field fields[1];
    fields[0].key = "a";
    fields[0].value = "b";

    ASSERT_EQ(C_REST_OK,
              c_rest_client_build_form_urlencoded(fields, 0, &body, &body_len));
    test_safe_free((void **)&body);
    ASSERT_EQ(C_REST_OK,
              c_rest_client_build_form_urlencoded(NULL, 1, &body, &body_len));
    test_safe_free((void **)&body);
    ASSERT_NEQ(C_REST_OK,
               c_rest_client_build_form_urlencoded(fields, 1, NULL, &body_len));
    ASSERT_NEQ(C_REST_OK,
               c_rest_client_build_form_urlencoded(fields, 1, &body, NULL));
    test_safe_free((void **)&body);

    ASSERT_NEQ(C_REST_OK,
               c_rest_client_parse_form_urlencoded("a=b", NULL, &pc));
    ASSERT_NEQ(C_REST_OK,
               c_rest_client_parse_form_urlencoded("a=b", &pf, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_client_parse_form_urlencoded(NULL, &pf, &pc));

    fields[0].key = NULL;
    fields[0].value = NULL;
    ASSERT_EQ(C_REST_OK,
              c_rest_client_build_form_urlencoded(fields, 1, &body, &body_len));
    test_safe_free((void **)&body);
  }

  /* Trigger method_from_str failure */
  {
    struct c_rest_client_context dummy_client;
    memset(&dummy_client, 0, sizeof(dummy_client));
    ASSERT_NEQ(C_REST_OK,
               c_rest_client_request_sync(&dummy_client, "http://a", "UNKNOWN",
                                          NULL, 0, NULL, 0, NULL));
  }

  {
    struct c_rest_client_response c_res;
    void *json_obj = NULL;
    char valid_json[] = "{}";
    char invalid_json[] = "invalid";
    memset(&c_res, 0, sizeof(c_res));

    ASSERT_NEQ(C_REST_OK, c_rest_client_response_parse_json(NULL, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_client_response_parse_json(&c_res, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_client_response_parse_json(NULL, &json_obj));
    ASSERT_NEQ(C_REST_OK, c_rest_client_response_parse_json(&c_res, &json_obj));

    ASSERT_NEQ(C_REST_OK, c_rest_client_response_free(NULL));

    c_res.body = valid_json;
    c_res.body_len = 2;
    ASSERT_EQ(C_REST_OK, c_rest_client_response_parse_json(&c_res, &json_obj));
    json_value_free((JSON_Value *)json_obj);
    json_obj = NULL;

    c_res.body = invalid_json;
    c_res.body_len = 7;
    ASSERT_NEQ(C_REST_OK, c_rest_client_response_parse_json(&c_res, &json_obj));

    g_client_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_client_fail_malloc_at = 1;
    c_res.body = valid_json;
    c_res.body_len = 2;
    ASSERT_NEQ(C_REST_OK, c_rest_client_response_parse_json(&c_res, &json_obj));
    g_crf_malloc_hook = NULL;
    g_client_fail_malloc_at = 0;
  }

  for (i = 1; i <= 150; i++) {
    c_rest_client_context *c = NULL;
    char *enc = NULL;
    char *dec = NULL;
    char *body = NULL;
    size_t blen = 0;
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;
    struct c_rest_client_header *hdr = NULL;
    size_t hc = 0;
    char *auth = NULL;
    struct c_rest_client_form_field fields[1];

    g_client_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_client_fail_malloc_at = i;

    rc = c_rest_client_url_encode("a b", &enc);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    C_REST_FREE(enc);
    enc = NULL;

    rc = c_rest_client_url_decode("a+b", &dec);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    C_REST_FREE(dec);
    dec = NULL;

    rc = c_rest_client_url_decode("a%2X", &dec);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    C_REST_FREE(dec);
    dec = NULL;

    rc = c_rest_client_url_decode("a%X2", &dec);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    C_REST_FREE(dec);
    dec = NULL;

    fields[0].key = "a";
    fields[0].value = "b";
    rc = c_rest_client_build_form_urlencoded(fields, 1, &body, &blen);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    rc = c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    ASSERT_NEQ(C_REST_OK, c_rest_client_header_set(NULL, NULL, NULL, NULL));
    rc = c_rest_client_header_set(&hdr, &hc, "a", "b");
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    ASSERT_NEQ(C_REST_OK, c_rest_client_build_auth_basic(NULL, NULL, NULL));
    rc = c_rest_client_build_auth_basic("a", "b", &auth);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    test_safe_free((void **)&auth);
    ASSERT_NEQ(C_REST_OK, c_rest_client_build_auth_bearer(NULL, NULL));
    rc = c_rest_client_build_auth_bearer("a", &auth);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    if (c_rest_client_init(&c) == C_REST_OK) {
      struct c_rest_client_header h[1];
      struct c_rest_client_response *sr = NULL;
      static const char *const methods[8] = {"POST",  "PUT",    "DELETE",
                                             "PATCH", "HEAD",   "OPTIONS",
                                             "TRACE", "CONNECT"};
      int mi;
      c->client.send = mock_send_full;
      h[0].key = "a";
      h[0].value = "b";
      ASSERT_NEQ(C_REST_OK, c_rest_client_post_form_sync(NULL, NULL, NULL, 0,
                                                         NULL, 0, NULL));
      rc = c_rest_client_post_form_sync(c, "http://a", h, 1, fields, 1, NULL);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      rc = c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0,
                                      &sr);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, test_safe_response_free(&sr));
      for (mi = 0; mi < 8; mi++) {
        rc = c_rest_client_request_sync(c, "http://a", methods[mi], NULL, 0,
                                        NULL, 0, NULL);
        ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
               rc == C_REST_ERROR_GENERIC);
      }
      ASSERT_EQ(C_REST_OK, c_rest_client_destroy(c));
    }

    ASSERT_NEQ(C_REST_OK, c_rest_proxy_request(NULL, NULL, NULL));

    g_crf_malloc_hook = NULL;
    g_client_fail_malloc_at = 0;

    if (body)
      C_REST_FREE(body);
    ASSERT_EQ(C_REST_OK, test_safe_fields_free(&pf, &pc));
    ASSERT_NEQ(C_REST_OK, c_rest_client_form_fields_free(NULL, 0));
    ASSERT_EQ(C_REST_OK, test_safe_headers_free(&hdr, &hc));
    ASSERT_NEQ(C_REST_OK, c_rest_client_headers_free(NULL, 0));
    test_safe_free((void **)&auth);
  }

  for (i = 1; i <= 20; i++) {
    struct c_rest_client_header *hdr = NULL;
    size_t hc = 0;

    g_client_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_client_fail_realloc_at = i;

    rc = c_rest_client_header_set(&hdr, &hc, "a", "b");
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);
    rc = c_rest_client_header_set(&hdr, &hc, "c", "d");
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    g_crf_realloc_hook = NULL;
    g_client_fail_realloc_at = 0;

    ASSERT_EQ(C_REST_OK, test_safe_headers_free(&hdr, &hc));
  }

  for (i = 1; i <= 5; i++) {
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;

    g_client_fail_calloc_at = -1;
    fail_calloc_n(0, 0);
    g_crf_calloc_hook = fail_calloc_n;
    g_client_fail_calloc_at = i;

    rc = c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
    ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
           rc == C_REST_ERROR_GENERIC);

    g_crf_calloc_hook = NULL;
    g_client_fail_calloc_at = 0;

    ASSERT_EQ(C_REST_OK, test_safe_fields_free(&pf, &pc));
  }

  PASS();
}

TEST test_client_end_to_end(void) {
  c_rest_client_context *client = NULL;
  struct c_rest_client_response *sync_res = NULL;
  struct c_rest_client_header headers[1];
  c_rest_error_t rc;
  char *encoded = NULL;
  char *decoded = NULL;
  struct c_rest_client_form_field fields[2];
  char *body = NULL;
  size_t body_len = 0;
  struct c_rest_client_form_field *parsed_fields = NULL;
  size_t parsed_count = 0;
  struct c_rest_client_header *custom_headers = NULL;
  size_t custom_headers_count = 0;
  char *auth_basic = NULL;
  char *auth_bearer = NULL;
  void *json = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_tls_init());
  ASSERT_EQ(C_REST_OK, c_rest_client_init(&client));
  ASSERT_NEQ(NULL, client);

  headers[0].key = "X-Test-Header";
  headers[0].value = "TestValue";

  rc = c_rest_client_request_sync(client, "://invalid", "GET", headers, 1, NULL,
                                  0, &sync_res);
  ASSERT_NEQ(C_REST_OK, rc);
  ASSERT_EQ(C_REST_OK, test_safe_response_free(&sync_res));

  rc = c_rest_client_request_async(client, "://invalid", "POST", NULL, 0,
                                   "test", 4, async_callback, NULL);
  ASSERT_NEQ(C_REST_OK, rc);

  ASSERT_NEQ(C_REST_OK, c_rest_proxy_request("://invalid", NULL, NULL));

#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
  if (c_rest_platform_init() == C_REST_OK) {
    c_rest_socket_t proxy_srv = C_REST_INVALID_SOCKET;
    unsigned short p_port;
    int bound = 0;
    if (c_rest_socket_create(&proxy_srv) == C_REST_OK) {
      for (p_port = 46785; p_port < 46805; p_port++) {
        if (c_rest_socket_bind(proxy_srv, "127.0.0.1", p_port) == C_REST_OK) {
          bound = 1;
          break;
        }
      }
      if (bound && c_rest_socket_listen(proxy_srv, 4) == C_REST_OK) {
        c_rest_thread_t srv_tid = (c_rest_thread_t)0;
        char url_buf[64];
#if defined(_MSC_VER)
        sprintf_s(url_buf, sizeof(url_buf), "http://127.0.0.1:%u/",
                  (unsigned int)p_port);
#else
        sprintf(url_buf, "http://127.0.0.1:%u/", (unsigned int)p_port);
#endif
        if (c_rest_thread_create(&srv_tid, proxy_mock_server_thread,
                                 &proxy_srv) == C_REST_OK) {
          ASSERT_EQ(C_REST_OK, c_rest_proxy_request(url_buf, NULL, NULL));
          ASSERT_EQ(C_REST_OK, c_rest_thread_join(srv_tid));
        }
      }
      ASSERT_EQ(C_REST_OK, c_rest_socket_close(proxy_srv));
    }
  }
#endif

  /* Test URL encoding/decoding */
  ASSERT_EQ(C_REST_OK, c_rest_client_url_encode("test + && = ?", &encoded));
  ASSERT_STR_EQ("test+%2B+%26%26+%3D+%3F", encoded);

  ASSERT_EQ(C_REST_OK, c_rest_client_url_decode(encoded, &decoded));
  ASSERT_STR_EQ("test + && = ?", decoded);

  C_REST_FREE(encoded);
  C_REST_FREE(decoded);

  /* Test Form URL encoded builder */
  fields[0].key = "grant_type";
  fields[0].value = "password";
  fields[1].key = "username";
  fields[1].value = "test user";

  ASSERT_EQ(C_REST_OK,
            c_rest_client_build_form_urlencoded(fields, 2, &body, &body_len));
  ASSERT_STR_EQ("grant_type=password&username=test+user", body);
  ASSERT_EQ(strlen("grant_type=password&username=test+user"), body_len);

  /* Test Form URL encoded parser */
  ASSERT_EQ(C_REST_OK, c_rest_client_parse_form_urlencoded(body, &parsed_fields,
                                                           &parsed_count));
  ASSERT_NEQ(NULL, parsed_fields);
  ASSERT_EQ(2, (int)parsed_count);
  ASSERT_STR_EQ("grant_type", parsed_fields[0].key);
  ASSERT_STR_EQ("password", parsed_fields[0].value);
  ASSERT_STR_EQ("username", parsed_fields[1].key);
  ASSERT_STR_EQ("test user", parsed_fields[1].value);
  ASSERT_EQ(C_REST_OK, test_safe_fields_free(&parsed_fields, &parsed_count));
  C_REST_FREE(body);

  /* Test Header Builders */
  ASSERT_EQ(C_REST_OK,
            c_rest_client_header_set(&custom_headers, &custom_headers_count,
                                     "Accept", "application/json"));
  ASSERT_EQ(C_REST_OK,
            c_rest_client_header_set(&custom_headers, &custom_headers_count,
                                     "Custom-Key", "Custom-Val"));
  ASSERT_EQ(2, (int)custom_headers_count);
  ASSERT_STR_EQ("Accept", custom_headers[0].key);
  ASSERT_EQ(C_REST_OK,
            c_rest_client_headers_free(custom_headers, custom_headers_count));

  /* Test Auth basic/bearer */
  ASSERT_EQ(C_REST_OK, c_rest_client_build_auth_basic("Aladdin", "open sesame",
                                                      &auth_basic));
  ASSERT_STR_EQ("Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==", auth_basic);
  C_REST_FREE(auth_basic);

  ASSERT_EQ(C_REST_OK,
            c_rest_client_build_auth_bearer("some_token", &auth_bearer));
  ASSERT_STR_EQ("Bearer some_token", auth_bearer);
  C_REST_FREE(auth_bearer);

  /* Test c_rest_client_post_form_sync */
  ASSERT_NEQ(C_REST_OK,
             c_rest_client_post_form_sync(client, "://invalid", headers, 1,
                                          fields, 2, &sync_res));
  ASSERT_EQ(C_REST_OK, test_safe_response_free(&sync_res));

  /* Test JSON parsing on dummy response */
  {
    struct c_rest_client_response dummy_res;
    char json_str[] = "{\"key\":\"value\"}";
    memset(&dummy_res, 0, sizeof(dummy_res));
    dummy_res.body = json_str;
    dummy_res.body_len = strlen(json_str);
    ASSERT_EQ(C_REST_OK, c_rest_client_response_parse_json(&dummy_res, &json));
    ASSERT_NEQ(NULL, json);
    test_safe_json_free(&json);
  }

  ASSERT_EQ(C_REST_OK, c_rest_client_set_hooks(NULL));
  ASSERT_EQ(C_REST_OK, c_rest_client_destroy(client));

  PASS();
}

SUITE(client_suite) {
  RUN_TEST(test_client_mocks_and_null_guards);
  RUN_TEST(test_client_oom_and_edge_branches);
  RUN_TEST(test_client_end_to_end);
}
