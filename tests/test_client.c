/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_client.h"
#include "c_rest_tls.h"
#include "c_rest_platform.h"
#include <parson.h>
#include <c_abstract_http/http_types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static int g_fail_malloc_at = 0;
static int g_fail_realloc_at = 0;
static int g_fail_calloc_at = 0;

static int async_called = 0;

static void async_callback(struct c_rest_client_response *res, void *data) {
  (void)res;
  (void)data;
  async_called = 1;
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
    r->headers.count = 1;
    r->headers.headers = (struct HttpHeader *)malloc(sizeof(struct HttpHeader));
    r->headers.headers[0].key = malloc(13);
    memcpy((char *)r->headers.headers[0].key, "Content-Type", 13);
    r->headers.headers[0].value = malloc(17);
    memcpy((char *)r->headers.headers[0].value, "application/json", 17);
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
  return 0; /* C_ABSTRACT_HTTP_OK */
}

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
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

static void *fail_realloc_n(void *ptr, size_t size) {
  static int alloc_count = 0;
  if (g_fail_realloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_realloc_at) {
    alloc_count = 0;
    g_fail_realloc_at = 0;
    return NULL;
  }
  return realloc(ptr, size);
}

static void *fail_calloc_n(size_t nmemb, size_t size) {
  static int alloc_count = 0;
  if (g_fail_calloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_calloc_at) {
    alloc_count = 0;
    g_fail_calloc_at = 0;
    return NULL;
  }
  return calloc(nmemb, size);
}

static void test_safe_free(void **ptr) {
  if (ptr && *ptr) {
    CRF_FREE(*ptr);
    *ptr = NULL;
  }
}

static void test_safe_response_free(struct c_rest_client_response **res) {
  if (res && *res) {
    c_rest_client_response_free(*res);
    *res = NULL;
  }
}

static void test_safe_fields_free(struct c_rest_client_form_field **fields,
                                  size_t *count) {
  if (fields && *fields) {
    c_rest_client_form_fields_free(*fields, count ? *count : 0);
    *fields = NULL;
    if (count)
      *count = 0;
  }
}

static void test_safe_headers_free(struct c_rest_client_header **headers,
                                   size_t *count) {
  if (headers && *headers) {
    c_rest_client_headers_free(*headers, count ? *count : 0);
    *headers = NULL;
    if (count)
      *count = 0;
  }
}

static void test_safe_json_free(void **json) {
  if (json && *json) {
    json_value_free((JSON_Value *)*json);
    *json = NULL;
  }
}

static void test_coverage(void) {
  int i;
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
  test_safe_response_free(NULL);
  test_safe_response_free(&r_helper);
  test_safe_response_free(&r_helper);

  test_safe_fields_free(NULL, NULL);
  test_safe_fields_free(&f_helper, NULL);
  c_rest_client_parse_form_urlencoded("a=b", &f_helper, &c_f_helper);
  test_safe_fields_free(&f_helper, &c_f_helper);
  test_safe_fields_free(&f_helper, &c_f_helper);
  f_helper = (struct c_rest_client_form_field *)malloc(sizeof(*f_helper));
  memset(f_helper, 0, sizeof(*f_helper));
  test_safe_fields_free(&f_helper, NULL);

  test_safe_headers_free(NULL, NULL);
  test_safe_headers_free(&h_helper, NULL);
  c_rest_client_header_set(&h_helper, &c_h_helper, "a", "b");
  test_safe_headers_free(&h_helper, &c_h_helper);
  test_safe_headers_free(&h_helper, &c_h_helper);
  h_helper = (struct c_rest_client_header *)malloc(sizeof(*h_helper));
  memset(h_helper, 0, sizeof(*h_helper));
  test_safe_headers_free(&h_helper, NULL);

  test_safe_json_free(NULL);
  test_safe_json_free(&v_helper);
  test_safe_json_free(&v_helper);

  c_rest_client_init(NULL);
  c_rest_client_destroy(NULL);

  c_rest_client_request_sync(NULL, "http://a", "GET", NULL, 0, NULL, 0, NULL);
  c_rest_client_request_sync(NULL, "http://a", NULL, NULL, 0, NULL, 0, NULL);
  c_rest_client_request_async(NULL, "http://a", "GET", NULL, 0, NULL, 0, NULL,
                              NULL);

  /* Edge cases for urlencode/decode */
  {
    char *out_enc = NULL;
    char *out_dec = NULL;
    c_rest_client_url_encode("-_.!~*'()",
                             &out_enc); /* hit isalnum chars + others */
    CRF_FREE(out_enc);
    out_enc = NULL;

    /* hit decode boundaries */
    c_rest_client_url_decode("%2", &out_dec);
    CRF_FREE(out_dec);
    out_dec = NULL;
    c_rest_client_url_decode("%2G", &out_dec);
    CRF_FREE(out_dec);
    out_dec = NULL;
    c_rest_client_url_decode("%G2", &out_dec);
    CRF_FREE(out_dec);
    out_dec = NULL;

    /* Null arguments */
    c_rest_client_url_encode(NULL, &out_enc);
    c_rest_client_url_encode("test", NULL);
    c_rest_client_url_decode(NULL, &out_dec);
    c_rest_client_url_decode("test", NULL);

    /* Test g_mock_client_fail branches */
    {
      c_rest_client_context *tc = NULL;
      char *t_enc = NULL;
      char *t_hdr = NULL;
      struct c_rest_client_response *t_sr = NULL;
      struct c_rest_client_header t_hdrs[1];
      struct c_rest_client_form_field t_ff[1];

      g_mock_client_fail = 1;
      c_rest_client_init(&tc);
      g_mock_client_fail = 2;
      c_rest_client_init(&tc);
      g_mock_client_fail = 0;

      c_rest_client_init(&tc);
      g_mock_client_fail = 11;
      c_rest_client_destroy(tc);
      g_mock_client_fail = 0;

      g_mock_client_fail = 12;
      c_rest_client_request_sync(tc, "http://a", "GET", NULL, 0, NULL, 0,
                                 &t_sr);
      g_mock_client_fail = 0;

      g_mock_client_fail = 20;
      c_rest_client_url_encode("@", &t_enc);
      g_mock_client_fail = 21;
      c_rest_client_url_encode("@", &t_enc);
      g_mock_client_fail = 0;

      g_mock_client_fail = 30;
      c_rest_client_build_auth_basic("u", "p", &t_hdr);
      g_mock_client_fail = 31;
      c_rest_client_build_auth_basic("u", "p", &t_hdr);
      g_mock_client_fail = 0;

      t_hdrs[0].key = "k";
      t_hdrs[0].value = "v";
      g_mock_client_fail = 40;
      c_rest_client_headers_free(t_hdrs, 1);
      g_mock_client_fail = 0;

      t_ff[0].key = "k";
      t_ff[0].value = "v";
      g_mock_client_fail = 40;
      c_rest_client_post_form_sync(tc, "http://a", t_hdrs, 1, t_ff, 1, &t_sr);
      g_mock_client_fail = 0;

      /* URL encode byte > 160 for >= 10 hex digit branches */
      c_rest_client_url_encode("\xBB", &t_enc);
      CRF_FREE(t_enc);
      t_enc = NULL;
      g_mock_client_fail = 21;
      c_rest_client_url_encode("\xBB", &t_enc);
      g_mock_client_fail = 0;
      CRF_FREE(t_enc);
      t_enc = NULL;

      /* Header with NULL key in request_sync */
      t_hdrs[0].key = NULL;
      t_hdrs[0].value = "v";
      c_rest_client_request_sync(tc, "http://a", "GET", t_hdrs, 1, NULL, 0,
                                 &t_sr);

      /* NULL client.send */
      {
        http_send_fn old_send = tc->client.send;
        tc->client.send = NULL;
        c_rest_client_request_sync(tc, "http://a", "GET", NULL, 0, NULL, 0,
                                   &t_sr);
        tc->client.send = old_send;
      }

      /* Async response free failure */
      {
        http_send_fn old_send = tc->client.send;
        tc->client.send = mock_send_full;
        g_mock_client_fail = 10;
        c_rest_client_request_async(tc, "http://a", "GET", NULL, 0, NULL, 0,
                                    async_callback, NULL);
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
        c_rest_client_build_form_urlencoded(t_ff, 1, &b_out, &b_out_len);
        t_ff[0].key = NULL;
        c_rest_client_build_form_urlencoded(t_ff, 1, &b_out, &b_out_len);
        g_mock_client_fail = 0;
      }

      /* Post form sync with headers_free fail and request_sync success */
      {
        http_send_fn old_send = tc->client.send;
        tc->client.send = mock_send_full;
        t_ff[0].key = "k";
        t_ff[0].value = "v";
        g_mock_client_fail = 40;
        c_rest_client_post_form_sync(tc, "http://a", NULL, 0, t_ff, 1, &t_sr);
        g_mock_client_fail = 0;
        tc->client.send = old_send;
        test_safe_response_free(&t_sr);
      }

      c_rest_proxy_request("http://localhost", NULL, NULL);
      g_mock_client_fail = 10;
      c_rest_proxy_request("http://localhost", NULL, NULL);
      g_mock_client_fail = 11;
      c_rest_proxy_request("http://localhost", NULL, NULL);
      g_mock_client_fail = 13;
      c_rest_proxy_request("http://localhost", NULL, NULL);
      g_mock_client_fail = 0;

      {
        struct c_rest_client_form_field *bad_ff = NULL;
        size_t bad_count = 0;
        c_rest_client_form_fields_free(NULL, 0);
        g_mock_client_fail = 41;
        c_rest_client_form_fields_free(
            tc ? (struct c_rest_client_form_field *)1 : NULL, 0);
        g_mock_client_fail = 43;
        c_rest_client_parse_form_urlencoded("a=1", &bad_ff, &bad_count);
        g_mock_client_fail = 45;
        c_rest_client_parse_form_urlencoded("a=1", &bad_ff, &bad_count);
        g_mock_client_fail = 44;
        c_rest_client_parse_form_urlencoded("a=1", &bad_ff, &bad_count);
        g_mock_client_fail = 46;
        c_rest_client_parse_form_urlencoded("a=1", &bad_ff, &bad_count);
        g_mock_client_fail = 0;
      }

      {
        http_send_fn old_send = tc->client.send;
        tc->client.send = mock_send_full;
        t_ff[0].key = "k";
        t_ff[0].value = "v";
        g_mock_client_fail = 42;
        c_rest_client_post_form_sync(tc, "http://a", t_hdrs, 1, t_ff, 1, &t_sr);
        c_rest_client_post_form_sync(tc, "http://a", NULL, 0, t_ff, 1, &t_sr);
        g_mock_client_fail = 0;
        tc->client.send = old_send;
      }

      c_rest_client_destroy(tc);
    }
  }

  /* mock_send_full malloc testing */
  {
    c_rest_client_context *c = NULL;
    c_rest_client_init(&c);
    {
      struct c_rest_client_response *sr = NULL;
      c->client.send = mock_send_full;

      /* Missing method */
      c_rest_client_request_sync(c, "http://a", NULL, NULL, 0, NULL, 0, &sr);
      c_rest_client_request_sync(c, NULL, "GET", NULL, 0, NULL, 0, &sr);
      c_rest_client_request_sync(NULL, "http://a", "GET", NULL, 0, NULL, 0,
                                 &sr);

      /* Missing transport */
      {
        void *tmp = c->client.transport;
        c->client.transport = NULL;
        c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, &sr);
        c->client.transport = tmp;
      }

      /* Async NULL callback */
      c_rest_client_request_async(c, "http://a", "GET", NULL, 0, NULL, 0, NULL,
                                  NULL);

      /* Trigger allocations via mock_send_full and test failures */
      {
        int mm;
        for (mm = 1; mm <= 20; mm++) {
          g_fail_calloc_at = -1;
          fail_calloc_n(0, 0);
          g_crf_calloc_hook = fail_calloc_n;
          g_fail_calloc_at = mm;

          c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0,
                                     &sr);
          test_safe_response_free(&sr);
          sr = NULL;

          g_crf_calloc_hook = NULL;
          g_fail_calloc_at = 0;

          g_fail_malloc_at = -1;
          fail_malloc_n(0);
          g_crf_malloc_hook = fail_malloc_n;
          g_fail_malloc_at = mm;

          c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0,
                                     &sr);
          test_safe_response_free(&sr);
          sr = NULL;

          g_crf_malloc_hook = NULL;
          g_fail_malloc_at = 0;
        }
      }

      c_rest_client_destroy(c);
    }
  }

  /* Further edge case tests for client.c */
  {
    c_rest_client_context *c = NULL;
    c_rest_client_init(&c);
    {
      struct c_rest_client_response *sr = NULL;
      struct c_rest_client_form_field fields[1];
      struct c_rest_client_header hdrs[1];
      hdrs[0].key = "k";
      hdrs[0].value = "v";
      fields[0].key = "k";
      fields[0].value = "v";

      c->client.send = mock_send_full;

      /* Hit missing branches in c_rest_client_post_form_sync */
      c_rest_client_post_form_sync(NULL, "http://a", hdrs, 1, fields, 1,
                                   &sr); /* !client */
      sr = NULL;
      c_rest_client_post_form_sync(c, NULL, hdrs, 1, fields, 1, &sr); /* !url */
      sr = NULL;
      c_rest_client_post_form_sync(c, "http://a", NULL, 1, fields, 1,
                                   &sr); /* !headers with headers_count > 0 */
      test_safe_response_free(&sr);
      sr = NULL;
      c_rest_client_post_form_sync(c, "http://a", hdrs, 0, fields, 1,
                                   &sr); /* headers_count == 0 */
      test_safe_response_free(&sr);
      sr = NULL;
      c_rest_client_post_form_sync(c, "http://a", hdrs, 1, NULL, 0,
                                   &sr); /* !fields */
      test_safe_response_free(&sr);
      sr = NULL;

      c_rest_client_destroy(c);
    }
  }

  {
    struct c_rest_client_header *hdr = NULL;
    size_t hc = 0;
    char *hdr_val = NULL;

    /* Header setters */
    c_rest_client_header_set(&hdr, NULL, "a", "b"); /* !headers_count */
    c_rest_client_header_set(&hdr, &hc, NULL, "b"); /* !key */
    c_rest_client_header_set(&hdr, &hc, "a", NULL); /* !value */

    /* Header struct with missing keys/values */
    c_rest_client_header_set(&hdr, &hc, "a", "b");
    {
      CRF_FREE((void *)hdr[0].key);
      hdr[0].key = NULL;
      CRF_FREE((void *)hdr[0].value);
      hdr[0].value = NULL;
      c_rest_client_headers_free(hdr, hc);
    }

    c_rest_client_build_auth_basic("user", NULL, &hdr_val);
    c_rest_client_build_auth_basic("user", "pass", NULL);
    hdr_val = NULL;

    c_rest_client_build_auth_bearer("token", NULL);
  }

  {
    struct c_rest_client_response res_obj;
    void *json_out = NULL;
    memset(&res_obj, 0, sizeof(res_obj));
    res_obj.body = (void *)"{}";
    res_obj.body_len = 0; /* hit res->body_len == 0 */
    c_rest_client_response_parse_json(&res_obj, &json_out);

    res_obj.body = NULL; /* hit !res->body */
    res_obj.body_len = 2;
    c_rest_client_response_parse_json(&res_obj, &json_out);
  }

  {
    int mm;
    for (mm = 1; mm <= 5; mm++) {
      g_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = mm;
      c_rest_proxy_request("http://a", NULL, NULL);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = 0;
    }
  }

  {
    /* test out->body_len > 0 && res->body == NULL */
    c_rest_client_context *c = NULL;
    c_rest_client_init(&c);
    {
      struct c_rest_client_response *sr = NULL;
      c->client.send = mock_send_full;

      g_mock_send_body_len_only = 1;
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, &sr);
      test_safe_response_free(&sr);
      sr = NULL;
      g_mock_send_body_len_only = 0;

      c_rest_client_destroy(c);
    }
  }

  {
    /* To hit out=NULL, we need request_sync to return an error AND out=NULL.
     * Let's trigger a failure inside mock_send_full by intercepting malloc
     * during async request */
    c_rest_client_context *c = NULL;
    c_rest_client_init(&c);
    {
      c->client.send = mock_send_full;
      g_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at =
          1; /* Fails request_sync allocation of response struct */
      c_rest_client_request_async(c, "http://a", "GET", NULL, 0, NULL, 0, NULL,
                                  NULL);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = 0;
      c_rest_client_destroy(c);
    }
  }

  {
    /* test out=NULL on async request sync failure */
    c_rest_client_context *c = NULL;
    c_rest_client_init(&c);
    {
      c_rest_client_request_async(c, "http://a", "GET", NULL, 0, NULL, 0, NULL,
                                  NULL);
      c_rest_client_destroy(c);
    }
  }

  {
    char *enc = NULL;
    c_rest_client_url_encode("", &enc); /* v < 10 branch */
    CRF_FREE(enc);
    enc = NULL;
  }
  {
    /* c_rest_client_post_form_sync without out_res */
    c_rest_client_context *c = NULL;
    c_rest_client_init(&c);
    {
      struct c_rest_client_form_field fields[1];
      fields[0].key = "a";
      fields[0].value = "b";
      c->client.send = mock_send_full;
      c_rest_client_post_form_sync(c, "http://a", NULL, 0, fields, 1, NULL);
      c_rest_client_destroy(c);
    }
  }
  {
    /* Mock async missing method */
    c_rest_client_context *c = NULL;
    c_rest_client_init(&c);
    {
      c_rest_client_request_async(c, "http://a", NULL, NULL, 0, NULL, 0, NULL,
                                  NULL);
      c_rest_client_destroy(c);
    }
  }

  {
    char *enc = NULL;
    c_rest_client_url_encode("ÿ", &enc);
    CRF_FREE(enc);
    enc = NULL;
  }

  {
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;

    /* Allocation failures on branch when amp but no eq */
    {
      int mm;
      for (mm = 1; mm <= 20; mm++) {
        g_fail_malloc_at = -1;
        fail_malloc_n(0);
        g_crf_malloc_hook = fail_malloc_n;
        g_fail_malloc_at = mm;

        c_rest_client_parse_form_urlencoded("a&b", &pf, &pc);
        c_rest_client_form_fields_free(pf, pc);
        pf = NULL;
        pf = NULL;

        g_crf_malloc_hook = NULL;
        g_fail_malloc_at = 0;
      }
    }

    /* hex decoding edge case testing */
    {
      char *dec = NULL;
      c_rest_client_url_decode("%0", &dec);
      CRF_FREE(dec);
      dec = NULL;
      c_rest_client_url_decode("%2+", &dec);
      CRF_FREE(dec);
      dec = NULL;
      c_rest_client_url_decode("%+2", &dec);
      CRF_FREE(dec);
      dec = NULL;
    }

    /* Hit early return for request async when missing required params */
    {
      c_rest_client_context *c = NULL;
      c_rest_client_init(&c);
      {
        /* url == NULL */
        c_rest_client_request_async(c, NULL, "GET", NULL, 0, NULL, 0, NULL,
                                    NULL);

        /* mock missing transport */
        {
          void *tmp = c->client.transport;
          c->client.transport = NULL;
          c_rest_client_request_async(c, "http://a", "GET", NULL, 0, NULL, 0,
                                      NULL, NULL);
          c->client.transport = tmp;
        }

        c_rest_client_destroy(c);
      }
    }
  }

  {
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;
    /* Hit loop failure */
    c_rest_client_parse_form_urlencoded("a=b", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    pf = NULL;

    c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    pf = NULL;

    {
      int mm;
      for (mm = 1; mm <= 20; mm++) {
        g_fail_malloc_at = -1;
        fail_malloc_n(0);
        g_crf_malloc_hook = fail_malloc_n;
        g_fail_malloc_at = mm;

        c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
        c_rest_client_form_fields_free(pf, pc);
        pf = NULL;
        pf = NULL;

        g_crf_malloc_hook = NULL;
        g_fail_malloc_at = 0;
      }
    }

    /* Try to decode a hex with some crazy values */
    {
      char *dec = NULL;
      c_rest_client_url_decode("%0A", &dec);
      CRF_FREE(dec);
      dec = NULL;
      c_rest_client_url_decode("%1a", &dec);
      CRF_FREE(dec);
      dec = NULL;
      c_rest_client_url_decode("%1A", &dec);
      CRF_FREE(dec);
      dec = NULL;
      c_rest_client_url_decode("%g1", &dec);
      CRF_FREE(dec);
      dec = NULL;
      c_rest_client_url_decode("%1g", &dec);
      CRF_FREE(dec);
      dec = NULL;
    }
  }

  {
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;
    /* Hit early parsing empty string body correctly to branch 0 on loop */
    c_rest_client_parse_form_urlencoded("", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    pf = NULL;

    /* Try a body with no && and only one side to test out loop */
    c_rest_client_parse_form_urlencoded("a", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    pf = NULL;

    /* Allocation failures on parsing */
    {
      int mm;
      for (mm = 1; mm <= 10; mm++) {
        g_fail_malloc_at = -1;
        fail_malloc_n(0);
        g_crf_malloc_hook = fail_malloc_n;
        g_fail_malloc_at = mm;

        c_rest_client_parse_form_urlencoded("a=b", &pf, &pc);
        c_rest_client_form_fields_free(pf, pc);
        pf = NULL;
        pf = NULL;

        g_crf_malloc_hook = NULL;
        g_fail_malloc_at = 0;
      }
    }
  }

  {
    /* hit if (body && body_len > 0) branch 2, which is body but length 0 */
    c_rest_client_context *c = NULL;
    c_rest_client_init(&c);
    {
      struct c_rest_client_response *sr = NULL;
      c->client.send = mock_send_full;

      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, (void *)"b", 0,
                                 &sr);
      test_safe_response_free(&sr);
      sr = NULL;

      c_rest_client_request_async(c, "http://a", "GET", NULL, 0, (void *)"b", 0,
                                  NULL, NULL);

      c_rest_client_destroy(c);
    }
  }

  {
    /* Hitting missing branches in response generation */
    c_rest_client_context *c = NULL;
    c_rest_client_init(&c);
    {
      struct c_rest_client_response *sr = NULL;
      c->client.send = mock_send_full;

      /* Null out_res */
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, NULL);

      /* mock_send returns NULL */
      g_mock_send_return_null = 1;
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, &sr);
      sr = NULL;
      g_mock_send_return_null = 0;

      /* mock_send returns no body */
      g_mock_send_no_body = 1;
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, &sr);
      test_safe_response_free(&sr);
      sr = NULL;
      g_mock_send_no_body = 0;

      /* mock_send returns null keys/values in headers */
      g_mock_send_null_headers = 1;
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, &sr);
      test_safe_response_free(&sr);
      sr = NULL;
      g_mock_send_null_headers = 0;

      c_rest_client_destroy(c);
    }
  }

  {
    char *dec = NULL;
    c_rest_client_url_decode("%20", &dec);
    CRF_FREE(dec);
    dec = NULL;
    c_rest_client_url_decode("%09", &dec); /* 0-9 */
    CRF_FREE(dec);
    dec = NULL;
    c_rest_client_url_decode("%A1", &dec); /* A-F */
    CRF_FREE(dec);
    dec = NULL;
    c_rest_client_url_decode("%a1", &dec); /* a-f */
    CRF_FREE(dec);
    dec = NULL;
  }

  {
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;
    /* Hit eq < amp in parsing */
    c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    pf = NULL;

    /* no equals before amp */
    c_rest_client_parse_form_urlencoded("ab&c=d", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    pf = NULL;

    /* empty string */
    c_rest_client_parse_form_urlencoded("", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    pf = NULL;
  }

  /* Test edge cases for response parsing logic */
  {
    c_rest_client_context *c = NULL;
    c_rest_client_init(&c);
    {
      c->client.send = mock_send_full;

      /* Force out_res to NULL but res valid */
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, NULL);

      c_rest_client_destroy(c);
    }
  }

  /* mock_send failures */
  {
    int mm;
    for (mm = 1; mm <= 10; mm++) {
      g_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = mm;

      c_rest_client_parse_form_urlencoded("a=b", NULL, NULL);
      c_rest_client_url_decode("%20", NULL);

      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = 0;
    }
  }

  {
    /* Hitting more malloc failures and edge cases */
    struct c_rest_client_response *sr = NULL;
    c_rest_client_context *c = NULL;

    c_rest_client_init(&c);
    {
      c->client.send = mock_send_full;

      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, (void *)"test",
                                 4, &sr);
      test_safe_response_free(&sr);
      sr = NULL;

      c_rest_client_destroy(c);
    }
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

    c_rest_client_build_form_urlencoded(fields, 0, &body, &body_len);
    c_rest_client_build_form_urlencoded(NULL, 1, &body, &body_len);
    c_rest_client_build_form_urlencoded(fields, 1, NULL, &body_len);
    c_rest_client_build_form_urlencoded(fields, 1, &body, NULL);

    c_rest_client_parse_form_urlencoded("a=b", NULL, &pc);
    c_rest_client_parse_form_urlencoded("a=b", &pf, NULL);
    c_rest_client_parse_form_urlencoded(NULL, &pf, &pc);
    c_rest_client_parse_form_urlencoded("", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    pf = NULL;

    /* no equals */
    c_rest_client_parse_form_urlencoded("a&b", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    pf = NULL;

    {
      int mm;
      for (mm = 1; mm <= 10; mm++) {
        g_fail_malloc_at = -1;
        fail_malloc_n(0);
        g_crf_malloc_hook = fail_malloc_n;
        g_fail_malloc_at = mm;
        c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
        c_rest_client_form_fields_free(pf, pc);
        pf = NULL;
        pf = NULL;

        g_crf_malloc_hook = NULL;
        g_fail_malloc_at = 0;
      }
    }
  }

  /* Trigger method_from_str failure */
  {
    struct c_rest_client_context dummy_client;
    memset(&dummy_client, 0, sizeof(dummy_client));
    c_rest_client_request_sync(&dummy_client, "http://a", "UNKNOWN", NULL, 0,
                               NULL, 0, NULL);
  }

  {
    struct c_rest_client_response c_res;
    void *json_obj;
    memset(&c_res, 0, sizeof(c_res));

    c_rest_client_response_parse_json(NULL, NULL);
    c_rest_client_response_parse_json(&c_res, NULL);
    c_rest_client_response_parse_json(NULL, &json_obj);

    c_rest_client_response_parse_json(&c_res, &json_obj);

    c_rest_client_response_free(NULL);

    c_res.body = "{}";
    c_res.body_len = 2;
    c_rest_client_response_parse_json(&c_res, &json_obj);
    json_value_free(json_obj);
    json_obj = NULL;

    c_res.body = "invalid";
    c_res.body_len = 7;
    c_rest_client_response_parse_json(&c_res, &json_obj);

    c_res.body = NULL;
    c_res.body_len = 0;

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 1;
    c_res.body = "{}";
    c_res.body_len = 2;
    c_rest_client_response_parse_json(&c_res, &json_obj);
    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
  }

  {
    char *dec = NULL;
    char *body = NULL;
    size_t blen = 0;
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;
    struct c_rest_client_form_field fields[1];

    c_rest_client_url_encode(NULL, NULL);
    c_rest_client_url_decode(NULL, NULL);
    c_rest_client_url_decode("a+%ZZ", &dec);
    CRF_FREE(dec);
    dec = NULL;

    c_rest_client_url_decode("a+%6a", &dec);
    CRF_FREE(dec);
    dec = NULL;

    c_rest_client_build_form_urlencoded(NULL, 0, NULL, NULL);
    c_rest_client_build_form_urlencoded(fields, 0, &body, &blen);
    fields[0].key = NULL;
    fields[0].value = NULL;
    c_rest_client_build_form_urlencoded(fields, 1, &body, &blen);
    test_safe_free((void **)&body);

    c_rest_client_parse_form_urlencoded(NULL, NULL, NULL);
    c_rest_client_parse_form_urlencoded("a&b", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    { pf = NULL; }

    fields[0].key = "a";
    fields[0].value = "b";
    c_rest_client_build_form_urlencoded(fields, 1, &body, &blen);
    test_safe_free((void **)&body);

    c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    { pf = NULL; }
  }

  for (i = 1; i <= 150; i++) {
    c_rest_client_context *c;
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

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    c_rest_client_url_encode("a b", &enc);
    c_rest_client_url_decode("a+b", &dec);
    c_rest_client_url_decode("a%2X", &dec);
    c_rest_client_url_decode("a%X2", &dec);

    fields[0].key = "a";
    fields[0].value = "b";
    c_rest_client_build_form_urlencoded(fields, 1, &body, &blen);
    c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);

    c_rest_client_header_set(NULL, NULL, NULL, NULL);
    c_rest_client_header_set(&hdr, &hc, "a", "b");
    c_rest_client_build_auth_basic(NULL, NULL, NULL);
    c_rest_client_build_auth_basic("a", "b", &auth);
    test_safe_free((void **)&auth);
    auth = NULL;
    c_rest_client_build_auth_bearer(NULL, NULL);
    c_rest_client_build_auth_bearer("a", &auth);

    if (c_rest_client_init(&c) == C_REST_OK) {
      struct c_rest_client_header h[1];
      struct c_rest_client_response *sr = NULL;
      c->client.send = mock_send_full;
      h[0].key = "a";
      h[0].value = "b";
      c_rest_client_post_form_sync(NULL, NULL, NULL, 0, NULL, 0, NULL);
      c_rest_client_post_form_sync(c, "http://a", h, 1, fields, 1, NULL);
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, &sr);
      test_safe_response_free(&sr);
      c_rest_client_request_sync(c, "http://a", "POST", NULL, 0, NULL, 0, NULL);
      c_rest_client_request_sync(c, "http://a", "PUT", NULL, 0, NULL, 0, NULL);
      c_rest_client_request_sync(c, "http://a", "DELETE", NULL, 0, NULL, 0,
                                 NULL);
      c_rest_client_request_sync(c, "http://a", "PATCH", NULL, 0, NULL, 0,
                                 NULL);
      c_rest_client_request_sync(c, "http://a", "HEAD", NULL, 0, NULL, 0, NULL);
      c_rest_client_request_sync(c, "http://a", "OPTIONS", NULL, 0, NULL, 0,
                                 NULL);
      c_rest_client_request_sync(c, "http://a", "TRACE", NULL, 0, NULL, 0,
                                 NULL);
      c_rest_client_request_sync(c, "http://a", "CONNECT", NULL, 0, NULL, 0,
                                 NULL);
      c_rest_client_destroy(c);
    }

    c_rest_proxy_request(NULL, NULL, NULL);
    {
      extern int g_fail_malloc_at;
      g_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = 1;
      c_rest_proxy_request("http://a", NULL, NULL);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = 0;
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;

    CRF_FREE(enc);
    enc = NULL;
    CRF_FREE(dec);
    dec = NULL;
    if (body)
      CRF_FREE(body);
    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
    c_rest_client_form_fields_free(NULL, 0);
    c_rest_client_headers_free(hdr, hc);
    hdr = NULL;
    c_rest_client_headers_free(NULL, 0);
    test_safe_free((void **)&auth);
  }

  for (i = 1; i <= 20; i++) {
    struct c_rest_client_header *hdr = NULL;
    size_t hc = 0;

    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    c_rest_client_header_set(&hdr, &hc, "a", "b");
    c_rest_client_header_set(&hdr, &hc, "c", "d");

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;

    c_rest_client_headers_free(hdr, hc);
    hdr = NULL;
  }

  for (i = 1; i <= 5; i++) {
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;

    g_fail_calloc_at = -1;
    fail_calloc_n(0, 0);
    g_crf_calloc_hook = fail_calloc_n;
    g_fail_calloc_at = i;

    c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);

    g_crf_calloc_hook = NULL;
    g_fail_calloc_at = 0;

    c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
  }
}

int test_client(void) {
  int failed = 0;
  c_rest_client_context *client = NULL;
  struct c_rest_client_response *sync_res = NULL;
  struct c_rest_client_header headers[1];
  int res;
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
  const char *msgs[2];

  test_coverage();

  res = (int)c_rest_tls_init();
  failed += (res != C_REST_OK);

  res = (int)c_rest_client_init(&client);
  failed += (res != C_REST_OK);
  failed += (client == NULL);

  headers[0].key = "X-Test-Header";
  headers[0].value = "TestValue";

  res = (int)c_rest_client_request_sync(client, "http://localhost", "GET",
                                        headers, 1, NULL, 0, &sync_res);
  /* Should fail gracefully with connection refused or success if there's a
   * listener */
  test_safe_response_free(&sync_res);

  res =
      (int)c_rest_client_request_async(client, "http://localhost", "POST", NULL,
                                       0, "test", 4, async_callback, NULL);
  /* Async failure is fine as long as it handles the callback correctly or
   * safely fails */

  res = (int)c_rest_proxy_request("http://localhost/proxy", NULL, NULL);

  /* Test URL encoding/decoding */
  res = (int)c_rest_client_url_encode("test + && = ?", &encoded);
  failed += (res != C_REST_OK);
  failed += (strcmp(encoded, "test+%2B+%26%26+%3D+%3F") != 0);

  res = (int)c_rest_client_url_decode(encoded, &decoded);
  failed += (res != C_REST_OK);
  failed += (strcmp(decoded, "test + && = ?") != 0);

  CRF_FREE(encoded);
  CRF_FREE(decoded);

  /* Test Form URL encoded builder */
  fields[0].key = "grant_type";
  fields[0].value = "password";
  fields[1].key = "username";
  fields[1].value = "test user";

  res = (int)c_rest_client_build_form_urlencoded(fields, 2, &body, &body_len);
  failed += (res != C_REST_OK);
  failed += (strcmp(body, "grant_type=password&username=test+user") != 0);
  failed += (body_len != strlen("grant_type=password&username=test+user"));

  /* Test Form URL encoded parser */
  res = (int)c_rest_client_parse_form_urlencoded(body, &parsed_fields,
                                                 &parsed_count);
  failed += (res != C_REST_OK);
  failed += (parsed_fields == NULL);
  failed += (parsed_count != 2);
  failed += (strcmp(parsed_fields[0].key, "grant_type") != 0);
  failed += (strcmp(parsed_fields[0].value, "password") != 0);
  failed += (strcmp(parsed_fields[1].key, "username") != 0);
  failed += (strcmp(parsed_fields[1].value, "test user") != 0);
  test_safe_fields_free(&parsed_fields, &parsed_count);
  CRF_FREE(body);

  /* Test Header Builders */
  res = (int)c_rest_client_header_set(&custom_headers, &custom_headers_count,
                                      "Accept", "application/json");
  failed += (res != C_REST_OK);
  res = (int)c_rest_client_header_set(&custom_headers, &custom_headers_count,
                                      "Custom-Key", "Custom-Val");
  failed += (res != C_REST_OK);
  failed += (custom_headers_count != 2);
  { failed += (strcmp(custom_headers[0].key, "Accept") != 0); }
  c_rest_client_headers_free(custom_headers, custom_headers_count);

  /* Test Auth basic/bearer */
  res = (int)c_rest_client_build_auth_basic("Aladdin", "open sesame",
                                            &auth_basic);
  failed += (res != C_REST_OK);
  failed += (strcmp(auth_basic, "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==") != 0);
  CRF_FREE(auth_basic);

  res = (int)c_rest_client_build_auth_bearer("some_token", &auth_bearer);
  failed += (res != C_REST_OK);
  failed += (strcmp(auth_bearer, "Bearer some_token") != 0);
  CRF_FREE(auth_bearer);

  /* Test c_rest_client_post_form_sync */
  res = (int)c_rest_client_post_form_sync(client, "http://localhost", headers,
                                          1, fields, 2, &sync_res);
  test_safe_response_free(&sync_res);

  /* Test JSON parsing on dummy response */
  {
    struct c_rest_client_response dummy_res;
    dummy_res.body = (void *)"{\"key\":\"value\"}";
    dummy_res.body_len = strlen((char *)dummy_res.body);
    res = (int)c_rest_client_response_parse_json(&dummy_res, &json);
    failed += (res != C_REST_OK);
    failed += (json == NULL);
    test_safe_json_free(&json);
  }

  c_rest_client_destroy(client);

  msgs[0] = "test_client passed\n";
  msgs[1] = "test_client failed\n";
  printf("%s", msgs[failed != 0]);
  return failed;
}
