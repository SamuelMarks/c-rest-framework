/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_client.h"
#include "c_rest_tls.h"
#include "c_rest_platform.h"
#include <parson.h>

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

#include <c_abstract_http/http_types.h>

struct c_rest_client_context {
  struct HttpClient client;
};

static int g_mock_send_return_null = 0;
static int g_mock_send_no_body = 0;
static int g_mock_send_null_headers = 0;
static int g_mock_send_body_len_only = 0;

static int mock_send_full(struct HttpTransportContext *ctx,
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

static void test_coverage(void) {
  int i;
  extern void *(*g_crf_malloc_hook)(size_t);
  extern void *(*g_crf_realloc_hook)(void *, size_t);
  extern void *(*g_crf_calloc_hook)(size_t, size_t);

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
    if (out_enc)
      CRF_FREE(out_enc);

    /* hit decode boundaries */
    c_rest_client_url_decode("%2", &out_dec);
    if (out_dec)
      CRF_FREE(out_dec);
    c_rest_client_url_decode("%2G", &out_dec);
    if (out_dec)
      CRF_FREE(out_dec);
    c_rest_client_url_decode("%G2", &out_dec);
    if (out_dec)
      CRF_FREE(out_dec);

    /* Null arguments */
    c_rest_client_url_encode(NULL, &out_enc);
    c_rest_client_url_encode("test", NULL);
    c_rest_client_url_decode(NULL, &out_dec);
    c_rest_client_url_decode("test", NULL);
  }

  /* mock_send_full malloc testing */
  {
    c_rest_client_context *c = NULL;
    if (c_rest_client_init(&c) == C_REST_OK) {
      struct c_rest_client_response *sr = NULL;
      struct c_rest_client_header hdr[1];
      c->client.send = mock_send_full;

      hdr[0].key = "a";
      hdr[0].value = "b";

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
          if (sr)
            c_rest_client_response_free(sr);
          sr = NULL;

          g_crf_calloc_hook = NULL;
          g_fail_calloc_at = 0;

          g_fail_malloc_at = -1;
          fail_malloc_n(0);
          g_crf_malloc_hook = fail_malloc_n;
          g_fail_malloc_at = mm;

          c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0,
                                     &sr);
          if (sr)
            c_rest_client_response_free(sr);
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
    if (c_rest_client_init(&c) == C_REST_OK) {
      struct c_rest_client_response *sr = NULL;
      struct c_rest_client_header hdrs[1];
      struct c_rest_client_form_field fields[1];
      fields[0].key = "k";
      fields[0].value = "v";
      hdrs[0].key = "k";
      hdrs[0].value = "v";

      c->client.send = mock_send_full;

      /* Hit missing branches in c_rest_client_post_form_sync */
      c_rest_client_post_form_sync(NULL, "http://a", hdrs, 1, fields, 1,
                                   &sr); /* !client */
      if (sr)
        c_rest_client_response_free(sr);
      sr = NULL;
      c_rest_client_post_form_sync(c, NULL, hdrs, 1, fields, 1, &sr); /* !url */
      if (sr)
        c_rest_client_response_free(sr);
      sr = NULL;
      c_rest_client_post_form_sync(c, "http://a", NULL, 1, fields, 1,
                                   &sr); /* !headers with headers_count > 0 */
      if (sr)
        c_rest_client_response_free(sr);
      sr = NULL;
      c_rest_client_post_form_sync(c, "http://a", hdrs, 0, fields, 1,
                                   &sr); /* headers_count == 0 */
      if (sr)
        c_rest_client_response_free(sr);
      sr = NULL;
      c_rest_client_post_form_sync(c, "http://a", hdrs, 1, NULL, 0,
                                   &sr); /* !fields */
      if (sr)
        c_rest_client_response_free(sr);
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
    if (c_rest_client_header_set(&hdr, &hc, "a", "b") == C_REST_OK) {
      CRF_FREE((void *)hdr[0].key);
      hdr[0].key = NULL;
      CRF_FREE((void *)hdr[0].value);
      hdr[0].value = NULL;
      c_rest_client_headers_free(hdr, hc);
    }

    c_rest_client_build_auth_basic("user", NULL, &hdr_val);
    c_rest_client_build_auth_basic("user", "pass", NULL);
    if (hdr_val)
      CRF_FREE(hdr_val);
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
    if (c_rest_client_init(&c) == C_REST_OK) {
      struct c_rest_client_response *sr = NULL;
      c->client.send = mock_send_full;

      g_mock_send_body_len_only = 1;
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, &sr);
      if (sr)
        c_rest_client_response_free(sr);
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
    if (c_rest_client_init(&c) == C_REST_OK) {
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
    if (c_rest_client_init(&c) == C_REST_OK) {
      c_rest_client_request_async(c, "http://a", "GET", NULL, 0, NULL, 0, NULL,
                                  NULL);
      c_rest_client_destroy(c);
    }
  }

  {
    char *enc = NULL;
    c_rest_client_url_encode("", &enc); /* v < 10 branch */
    if (enc)
      CRF_FREE(enc);
  }
  {
    /* c_rest_client_post_form_sync without out_res */
    c_rest_client_context *c = NULL;
    if (c_rest_client_init(&c) == C_REST_OK) {
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
    if (c_rest_client_init(&c) == C_REST_OK) {
      c_rest_client_request_async(c, "http://a", NULL, NULL, 0, NULL, 0, NULL,
                                  NULL);
      c_rest_client_destroy(c);
    }
  }

  {
    char *enc = NULL;
    c_rest_client_url_encode("ÿ", &enc);
    if (enc)
      CRF_FREE(enc);
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
        if (pf)
          c_rest_client_form_fields_free(pf, pc);
        pf = NULL;

        g_crf_malloc_hook = NULL;
        g_fail_malloc_at = 0;
      }
    }

    /* hex decoding edge case testing */
    {
      char *dec = NULL;
      c_rest_client_url_decode("%0", &dec);
      if (dec)
        CRF_FREE(dec);
      c_rest_client_url_decode("%2+", &dec);
      if (dec)
        CRF_FREE(dec);
      c_rest_client_url_decode("%+2", &dec);
      if (dec)
        CRF_FREE(dec);
    }

    /* Hit early return for request async when missing required params */
    {
      c_rest_client_context *c = NULL;
      if (c_rest_client_init(&c) == C_REST_OK) {
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
    if (pf)
      c_rest_client_form_fields_free(pf, pc);
    pf = NULL;

    c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
    if (pf)
      c_rest_client_form_fields_free(pf, pc);
    pf = NULL;

    {
      int mm;
      for (mm = 1; mm <= 20; mm++) {
        g_fail_malloc_at = -1;
        fail_malloc_n(0);
        g_crf_malloc_hook = fail_malloc_n;
        g_fail_malloc_at = mm;

        c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
        if (pf)
          c_rest_client_form_fields_free(pf, pc);
        pf = NULL;

        g_crf_malloc_hook = NULL;
        g_fail_malloc_at = 0;
      }
    }

    /* Try to decode a hex with some crazy values */
    {
      char *dec = NULL;
      c_rest_client_url_decode("%0A", &dec);
      if (dec)
        CRF_FREE(dec);
      c_rest_client_url_decode("%1a", &dec);
      if (dec)
        CRF_FREE(dec);
      c_rest_client_url_decode("%1A", &dec);
      if (dec)
        CRF_FREE(dec);
      c_rest_client_url_decode("%g1", &dec);
      if (dec)
        CRF_FREE(dec);
      c_rest_client_url_decode("%1g", &dec);
      if (dec)
        CRF_FREE(dec);
    }
  }

  {
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;
    /* Hit early parsing empty string body correctly to branch 0 on loop */
    c_rest_client_parse_form_urlencoded("", &pf, &pc);

    /* Try a body with no & and only one side to test out loop */
    c_rest_client_parse_form_urlencoded("a", &pf, &pc);
    if (pf)
      c_rest_client_form_fields_free(pf, pc);
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
        if (pf)
          c_rest_client_form_fields_free(pf, pc);
        pf = NULL;

        g_crf_malloc_hook = NULL;
        g_fail_malloc_at = 0;
      }
    }
  }

  {
    /* hit if (body && body_len > 0) branch 2, which is body but length 0 */
    c_rest_client_context *c = NULL;
    if (c_rest_client_init(&c) == C_REST_OK) {
      struct c_rest_client_response *sr = NULL;
      c->client.send = mock_send_full;

      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, (void *)"b", 0,
                                 &sr);
      if (sr)
        c_rest_client_response_free(sr);
      sr = NULL;

      c_rest_client_request_async(c, "http://a", "GET", NULL, 0, (void *)"b", 0,
                                  NULL, NULL);

      c_rest_client_destroy(c);
    }
  }

  {
    /* Hitting missing branches in response generation */
    c_rest_client_context *c = NULL;
    if (c_rest_client_init(&c) == C_REST_OK) {
      struct c_rest_client_response *sr = NULL;
      c->client.send = mock_send_full;

      /* Null out_res */
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, NULL);

      /* mock_send returns NULL */
      g_mock_send_return_null = 1;
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, &sr);
      if (sr)
        c_rest_client_response_free(sr);
      sr = NULL;
      g_mock_send_return_null = 0;

      /* mock_send returns no body */
      g_mock_send_no_body = 1;
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, &sr);
      if (sr)
        c_rest_client_response_free(sr);
      sr = NULL;
      g_mock_send_no_body = 0;

      /* mock_send returns null keys/values in headers */
      g_mock_send_null_headers = 1;
      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, NULL, 0, &sr);
      if (sr)
        c_rest_client_response_free(sr);
      sr = NULL;
      g_mock_send_null_headers = 0;

      c_rest_client_destroy(c);
    }
  }

  {
    char *dec = NULL;
    c_rest_client_url_decode("%20", &dec);
    if (dec)
      CRF_FREE(dec);
    c_rest_client_url_decode("%09", &dec); /* 0-9 */
    if (dec)
      CRF_FREE(dec);
    c_rest_client_url_decode("%A1", &dec); /* A-F */
    if (dec)
      CRF_FREE(dec);
    c_rest_client_url_decode("%a1", &dec); /* a-f */
    if (dec)
      CRF_FREE(dec);
  }

  {
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;
    /* Hit eq < amp in parsing */
    c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
    if (pf)
      c_rest_client_form_fields_free(pf, pc);
    pf = NULL;

    /* no equals before amp */
    c_rest_client_parse_form_urlencoded("ab&c=d", &pf, &pc);
    if (pf)
      c_rest_client_form_fields_free(pf, pc);
    pf = NULL;

    /* empty string */
    c_rest_client_parse_form_urlencoded("", &pf, &pc);
    if (pf)
      c_rest_client_form_fields_free(pf, pc);
    pf = NULL;
  }

  /* Test edge cases for response parsing logic */
  {
    c_rest_client_context *c = NULL;
    if (c_rest_client_init(&c) == C_REST_OK) {
      struct c_rest_client_response *sr = NULL;
      struct c_rest_client_header hdrs[1];
      hdrs[0].key = "k";
      hdrs[0].value = "v";

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

    if (c_rest_client_init(&c) == C_REST_OK) {
      c->client.send = mock_send_full;

      c_rest_client_request_sync(c, "http://a", "GET", NULL, 0, (void *)"test",
                                 4, &sr);
      if (sr)
        c_rest_client_response_free(sr);
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

    /* no equals */
    c_rest_client_parse_form_urlencoded("a&b", &pf, &pc);
    if (pf)
      c_rest_client_form_fields_free(pf, pc);
    pf = NULL;

    {
      int mm;
      for (mm = 1; mm <= 10; mm++) {
        g_fail_malloc_at = -1;
        fail_malloc_n(0);
        g_crf_malloc_hook = fail_malloc_n;
        g_fail_malloc_at = mm;
        c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
        if (pf)
          c_rest_client_form_fields_free(pf, pc);
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
    if (json_obj)
      json_value_free(json_obj);

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
    char *enc = NULL, *dec = NULL;
    char *body = NULL;
    size_t blen = 0;
    struct c_rest_client_form_field *pf = NULL;
    size_t pc = 0;
    struct c_rest_client_form_field fields[1];

    c_rest_client_url_encode(NULL, NULL);
    c_rest_client_url_decode(NULL, NULL);
    c_rest_client_url_decode("a+%ZZ", &dec);
    if (dec)
      CRF_FREE(dec);

    c_rest_client_url_decode("a+%6a", &dec);
    if (dec)
      CRF_FREE(dec);

    c_rest_client_build_form_urlencoded(NULL, 0, NULL, NULL);
    c_rest_client_build_form_urlencoded(fields, 0, &body, &blen);
    fields[0].key = NULL;
    fields[0].value = NULL;
    c_rest_client_build_form_urlencoded(fields, 1, &body, &blen);
    if (body) {
      CRF_FREE(body);
      body = NULL;
    }

    c_rest_client_parse_form_urlencoded(NULL, NULL, NULL);
    c_rest_client_parse_form_urlencoded("a&b", &pf, &pc);
    if (pf) {
      c_rest_client_form_fields_free(pf, pc);
      pf = NULL;
    }

    fields[0].key = "a";
    fields[0].value = "b";
    c_rest_client_build_form_urlencoded(fields, 1, &body, &blen);
    if (body) {
      CRF_FREE(body);
      body = NULL;
    }

    c_rest_client_parse_form_urlencoded("a=b&c=d", &pf, &pc);
    if (pf) {
      c_rest_client_form_fields_free(pf, pc);
      pf = NULL;
    }
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
    if (auth)
      CRF_FREE(auth);
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
      if (sr)
        c_rest_client_response_free(sr);
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
      extern void *(*g_crf_malloc_hook)(size_t);
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

    if (enc)
      CRF_FREE(enc);
    if (dec)
      CRF_FREE(dec);
    if (body)
      CRF_FREE(body);
    if (pf)
      c_rest_client_form_fields_free(pf, pc);
    c_rest_client_form_fields_free(NULL, 0);
    if (hdr)
      c_rest_client_headers_free(hdr, hc);
    c_rest_client_headers_free(NULL, 0);
    if (auth)
      CRF_FREE(auth);
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

    if (hdr)
      c_rest_client_headers_free(hdr, hc);
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

    if (pf)
      c_rest_client_form_fields_free(pf, pc);
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

  res = c_rest_tls_init();
  failed += (res != C_REST_OK);

  res = c_rest_client_init(&client);
  failed += (res != C_REST_OK);
  failed += (client == NULL);

  headers[0].key = "X-Test-Header";
  headers[0].value = "TestValue";

  res = c_rest_client_request_sync(client, "http://localhost", "GET", headers,
                                   1, NULL, 0, &sync_res);
  /* Should fail gracefully with connection refused or success if there's a
   * listener */
  if (sync_res) {
    (void)!c_rest_client_response_free(sync_res);
    sync_res = NULL;
  }

  res = c_rest_client_request_async(client, "http://localhost", "POST", NULL, 0,
                                    "test", 4, async_callback, NULL);
  /* Async failure is fine as long as it handles the callback correctly or
   * safely fails */

  res = c_rest_proxy_request("http://localhost/proxy", NULL, NULL);

  /* Test URL encoding/decoding */
  res = c_rest_client_url_encode("test + & = ?", &encoded);
  failed += (res != C_REST_OK);
  failed += (strcmp(encoded, "test+%2B+%26+%3D+%3F") != 0);

  res = c_rest_client_url_decode(encoded, &decoded);
  failed += (res != C_REST_OK);
  failed += (strcmp(decoded, "test + & = ?") != 0);

  CRF_FREE(encoded);
  CRF_FREE(decoded);

  /* Test Form URL encoded builder */
  fields[0].key = "grant_type";
  fields[0].value = "password";
  fields[1].key = "username";
  fields[1].value = "test user";

  res = c_rest_client_build_form_urlencoded(fields, 2, &body, &body_len);
  failed += (res != C_REST_OK);
  failed += (strcmp(body, "grant_type=password&username=test+user") != 0);
  failed += (body_len != strlen("grant_type=password&username=test+user"));

  /* Test Form URL encoded parser */
  res =
      c_rest_client_parse_form_urlencoded(body, &parsed_fields, &parsed_count);
  failed += (res != C_REST_OK);
  failed += (parsed_count != 2);
  if (parsed_fields && parsed_count == 2) {
    failed += (strcmp(parsed_fields[0].key, "grant_type") != 0);
    failed += (strcmp(parsed_fields[0].value, "password") != 0);
    failed += (strcmp(parsed_fields[1].key, "username") != 0);
    failed += (strcmp(parsed_fields[1].value, "test user") != 0);
    c_rest_client_form_fields_free(parsed_fields, parsed_count);
  }
  CRF_FREE(body);

  /* Test Header Builders */
  res = c_rest_client_header_set(&custom_headers, &custom_headers_count,
                                 "Accept", "application/json");
  failed += (res != C_REST_OK);
  res = c_rest_client_header_set(&custom_headers, &custom_headers_count,
                                 "Custom-Key", "Custom-Val");
  failed += (res != C_REST_OK);
  failed += (custom_headers_count != 2);
  if (custom_headers_count > 0) {
    failed += (strcmp(custom_headers[0].key, "Accept") != 0);
  }
  c_rest_client_headers_free(custom_headers, custom_headers_count);

  /* Test Auth basic/bearer */
  res = c_rest_client_build_auth_basic("Aladdin", "open sesame", &auth_basic);
  failed += (res != C_REST_OK);
  failed += (strcmp(auth_basic, "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==") != 0);
  CRF_FREE(auth_basic);

  res = c_rest_client_build_auth_bearer("some_token", &auth_bearer);
  failed += (res != C_REST_OK);
  failed += (strcmp(auth_bearer, "Bearer some_token") != 0);
  CRF_FREE(auth_bearer);

  /* Test c_rest_client_post_form_sync */
  res = c_rest_client_post_form_sync(client, "http://localhost", headers, 1,
                                     fields, 2, &sync_res);
  if (sync_res) {
    (void)!c_rest_client_response_free(sync_res);
    sync_res = NULL;
  }

  /* Test JSON parsing on dummy response */
  {
    struct c_rest_client_response dummy_res;
    dummy_res.body = (void *)"{\"key\":\"value\"}";
    dummy_res.body_len = strlen((char *)dummy_res.body);
    res = c_rest_client_response_parse_json(&dummy_res, &json);
    failed += (res != C_REST_OK);
    failed += (json == NULL);
    if (json) {
      json_value_free((JSON_Value *)json);
    }
  }

  c_rest_client_destroy(client);

  msgs[0] = "test_client passed\n";
  msgs[1] = "test_client failed\n";
  printf("%s", msgs[failed != 0]);
  return failed;
}
