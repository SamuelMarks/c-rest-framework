/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_http23.h"
#include "c_rest_response.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* clang-format on */

static int g_malloc_fail_after = -1;
static void *fail_malloc_n(size_t size) {
  int do_fail = (g_malloc_fail_after == 0);
  g_malloc_fail_after -= (g_malloc_fail_after > 0);
  return do_fail ? NULL : malloc(size);
}
static void *fail_realloc_n(void *ptr, size_t size) {
  int do_fail = (g_malloc_fail_after == 0);
  g_malloc_fail_after -= (g_malloc_fail_after > 0);
  return do_fail ? NULL : realloc(ptr, size);
}

TEST test_http2_rfc9113_framing(void) {
  c_rest_http23_ctx_t *ctx = NULL;
  size_t consumed = 0;
  int is_ready = 0;
  struct c_rest_request *req = NULL;
  struct c_rest_response res_obj;
  char *out_buf = NULL;
  size_t out_len = 0;
  char hello_body[] = "hello";
  const char *hdr_val = NULL;

  unsigned char wire[128];
  size_t pos = 0;
  static const char hdr_block[] =
      "\r\n:method: POST\r\n:path: /api/items\r\nx-test: ok\r\n";
  size_t hb_len = sizeof(hdr_block) - 1;

  memcpy(wire + pos, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24);
  pos += 24;
  memset(wire + pos, 0, 9);
  wire[pos + 3] = 0x04U;
  pos += 9;
  wire[pos + 0] = 0;
  wire[pos + 1] = 0;
  wire[pos + 2] = (unsigned char)hb_len;
  wire[pos + 3] = 0x01U;
  wire[pos + 4] = 0x04U;
  wire[pos + 5] = 0;
  wire[pos + 6] = 0;
  wire[pos + 7] = 0;
  wire[pos + 8] = 1;
  pos += 9;
  memcpy(wire + pos, hdr_block, hb_len);
  pos += hb_len;
  memset(wire + pos, 0, 9);
  wire[pos + 3] = 0x06U;
  pos += 9;
  memset(wire + pos, 0, 9);
  wire[pos + 8] = 1;
  pos += 9;
  wire[pos + 0] = 0;
  wire[pos + 1] = 0;
  wire[pos + 2] = 3;
  wire[pos + 3] = 0x00U;
  wire[pos + 4] = 0x01U;
  wire[pos + 5] = 0;
  wire[pos + 6] = 0;
  wire[pos + 7] = 0;
  wire[pos + 8] = 1;
  pos += 9;
  memcpy(wire + pos, "abc", 3);
  pos += 3;
  wire[pos + 0] = 0;
  wire[pos + 1] = 0;
  wire[pos + 2] = 10;
  wire[pos + 3] = 0x01U;

  ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &ctx));
  ASSERT_EQ(C_REST_OK,
            c_rest_http23_process(ctx, (const char *)wire, pos + 4, &consumed));
  ASSERT_EQ(pos, consumed);

  {
    unsigned char trunc_frame[12] = {0, 0, 10, 0x01U, 0x05U, 0,
                                     0, 0, 1,  'a',   'b',   'c'};
    size_t c2 = 99;
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_process(ctx, (const char *)trunc_frame, 12, &c2));
    ASSERT_EQ((size_t)0, c2);
  }

  ASSERT_EQ(C_REST_OK, c_rest_http23_is_request_ready(ctx, &is_ready));
  ASSERT_EQ(1, is_ready);

  ASSERT_EQ(C_REST_OK, c_rest_http23_get_request(ctx, &req));
  ASSERT_STR_EQ("POST", req->method);
  ASSERT_STR_EQ("/api/items", req->path);
  ASSERT_EQ((size_t)3, req->body_len);
  ASSERT_STR_EQ("abc", req->body);
  ASSERT_EQ(C_REST_OK, c_rest_request_get_header(req, "x-test", &hdr_val));
  ASSERT_STR_EQ("ok", hdr_val);

  memset(&res_obj, 0, sizeof(res_obj));
  res_obj.status_code = 201;
  ASSERT_EQ(C_REST_OK,
            c_rest_response_set_header(&res_obj, "content-type", "text/plain"));
  {
    struct c_rest_header null_h;
    memset(&null_h, 0, sizeof(null_h));
    null_h.next = res_obj.headers;
    res_obj.headers = &null_h;

    res_obj.body = hello_body;
    res_obj.body_len = 5;
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_format_response(ctx, &res_obj, &out_buf, &out_len));
    res_obj.headers = null_h.next;
  }
  ASSERT(out_buf != NULL);
  ASSERT_EQ(0x01, (unsigned char)out_buf[3]);
  ASSERT_EQ(0x04, (unsigned char)out_buf[4]);
  ASSERT_EQ(0x01, (unsigned char)out_buf[8]);
  ASSERT(strstr(out_buf + 9, ":status: 201\r\n") != NULL);
  ASSERT(strstr(out_buf + 9, "content-type: text/plain\r\n") != NULL);
  C_REST_FREE(out_buf);

  out_buf = NULL;
  res_obj.status_code = 0;
  res_obj.body = NULL;
  res_obj.body_len = 0;
  ASSERT_EQ(C_REST_OK,
            c_rest_http23_format_response(ctx, &res_obj, &out_buf, &out_len));
  ASSERT_EQ(0x01, (unsigned char)out_buf[3]);
  ASSERT_EQ(0x05, (unsigned char)out_buf[4]);
  C_REST_FREE(out_buf);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_obj));

  ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_destroy(ctx));
  PASS();
}

TEST test_http23_indexed_headers_and_http3(void) {
  c_rest_http23_ctx_t *ctx = NULL;
  size_t consumed = 0;
  int ready = 0;
  struct c_rest_request *req = NULL;
  struct c_rest_response res_obj;
  char *out_buf = NULL;
  size_t out_len = 0;
  char body_txt[] = "h3body";

  {
    unsigned char h2_idx[15] = {0, 0,     6,     0x01U, 0x05U, 0,     0,    0,
                                1, 0x82U, 0x83U, 0x84U, 0x85U, 0x86U, 0x87U};
    ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &ctx));
    ASSERT_EQ(C_REST_OK, c_rest_http23_process(ctx, (const char *)h2_idx,
                                               sizeof(h2_idx), &consumed));
    ASSERT_EQ(sizeof(h2_idx), consumed);
    ASSERT_EQ(C_REST_OK, c_rest_http23_get_request(ctx, &req));
    ASSERT_STR_EQ("POST", req->method);
    ASSERT_STR_EQ("/index.html", req->path);
    ASSERT_STR_EQ("https", req->scheme);
    ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_destroy(ctx));
  }

  {
    unsigned char h3_wire[11] = {0x01U, 0x02U, 0x82U, 0x84U, 0x00U, 0x40U,
                                 0x04U, 'd',   'a',   't',   'a'};
    ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP3, &ctx));
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_process(ctx, (const char *)h3_wire, 1, &consumed));
    ASSERT_EQ((size_t)0, consumed);
    ASSERT_EQ(C_REST_OK, c_rest_http23_process(ctx, (const char *)(h3_wire + 4),
                                               2, &consumed));
    ASSERT_EQ((size_t)0, consumed);
    ASSERT_EQ(C_REST_OK, c_rest_http23_process(ctx, (const char *)(h3_wire + 4),
                                               5, &consumed));
    ASSERT_EQ((size_t)0, consumed);
    {
      unsigned char h3_settings[2] = {0x04U, 0x00U};
      ASSERT_EQ(C_REST_OK, c_rest_http23_process(ctx, (const char *)h3_settings,
                                                 2, &consumed));
      ASSERT_EQ((size_t)2, consumed);
    }

    ASSERT_EQ(C_REST_OK, c_rest_http23_process(ctx, (const char *)h3_wire,
                                               sizeof(h3_wire), &consumed));
    ASSERT_EQ(sizeof(h3_wire), consumed);
    ASSERT_EQ(C_REST_OK, c_rest_http23_is_request_ready(ctx, &ready));
    ASSERT_EQ(1, ready);
    ASSERT_EQ(C_REST_OK, c_rest_http23_get_request(ctx, &req));
    ASSERT_STR_EQ("GET", req->method);
    ASSERT_STR_EQ("/", req->path);
    ASSERT_STR_EQ("data", req->body);

    memset(&res_obj, 0, sizeof(res_obj));
    res_obj.status_code = 200;
    res_obj.body = body_txt;
    res_obj.body_len = 6;
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_format_response(ctx, &res_obj, &out_buf, &out_len));
    ASSERT_EQ(0x01, (unsigned char)out_buf[0]);
    C_REST_FREE(out_buf);

    res_obj.body = NULL;
    res_obj.body_len = 0;
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_format_response(ctx, &res_obj, &out_buf, &out_len));
    ASSERT_EQ(0x01, (unsigned char)out_buf[0]);
    C_REST_FREE(out_buf);

    res_obj.body = body_txt;
    res_obj.body_len = 20000;
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
              c_rest_http23_format_response(ctx, &res_obj, &out_buf, &out_len));

    ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_destroy(ctx));
  }

  PASS();
}

TEST test_http23_errors_and_oom(void) {
  c_rest_http23_ctx_t *ctx = NULL;
  size_t cons = 0;
  int ready = 0;
  struct c_rest_request *req = NULL;
  struct c_rest_response res_obj;
  char *out_buf = NULL;
  size_t out_len = 0;
  int k;

  memset(&res_obj, 0, sizeof(res_obj));

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, NULL));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_http23_ctx_init((c_rest_protocol_t)99, &ctx));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_http23_ctx_destroy(NULL));

  ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &ctx));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_http23_process(NULL, "a", 1, &cons));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_http23_process(ctx, NULL, 1, &cons));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_http23_process(ctx, "a", 1, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_http23_process(ctx, "a", 0, &cons));
  ASSERT_EQ((size_t)0, cons);

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_http23_is_request_ready(NULL, &ready));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_http23_is_request_ready(ctx, NULL));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_http23_get_request(NULL, &req));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_http23_get_request(ctx, NULL));
  ASSERT_EQ(C_REST_ERROR_NOT_FOUND, c_rest_http23_get_request(ctx, &req));

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_http23_format_response(NULL, &res_obj, &out_buf, &out_len));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_http23_format_response(ctx, NULL, &out_buf, &out_len));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_http23_format_response(ctx, &res_obj, NULL, &out_len));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_http23_format_response(ctx, &res_obj, &out_buf, NULL));

  {
    unsigned char bad_settings[9] = {0, 0, 0, 0x04U, 0, 0, 0, 0, 1};
    unsigned char bad_settings_len[10] = {0, 0, 1, 0x04U, 0, 0, 0, 0, 0, 0};
    unsigned char bad_hdr_sid[9] = {0, 0, 0, 0x01U, 0x04U, 0, 0, 0, 0};
    unsigned char bad_data_sid[9] = {0, 0, 0, 0x00U, 0x01U, 0, 0, 0, 0};
    unsigned char bad_hpack[10] = {0, 0, 1, 0x01U, 0x04U, 0, 0, 0, 1, 0xFFU};
    unsigned char bad_line[13] = {0, 0, 4,   0x01U, 0x04U, 0,   0,
                                  0, 1, 'b', 'a',   'd',   '\n'};
    unsigned char bad_empty_colon[11] = {0, 0, 2, 0x01U, 0x04U, 0,
                                         0, 0, 1, ':',   '\n'};
    unsigned char no_nl_hdr[12] = {0, 0, 3, 0x01U, 0x04U, 0,
                                   0, 0, 1, 'a',   ':',   '1'};
    ASSERT_EQ(C_REST_ERROR_PARSE,
              c_rest_http23_process(ctx, (const char *)bad_settings, 9, &cons));
    ASSERT_EQ(
        C_REST_ERROR_PARSE,
        c_rest_http23_process(ctx, (const char *)bad_settings_len, 10, &cons));
    ASSERT_EQ(C_REST_ERROR_PARSE,
              c_rest_http23_process(ctx, (const char *)bad_hdr_sid, 9, &cons));
    ASSERT_EQ(C_REST_ERROR_PARSE,
              c_rest_http23_process(ctx, (const char *)bad_data_sid, 9, &cons));
    ASSERT_EQ(C_REST_ERROR_PARSE,
              c_rest_http23_process(ctx, (const char *)bad_hpack, 10, &cons));
    ASSERT_EQ(C_REST_ERROR_PARSE,
              c_rest_http23_process(ctx, (const char *)bad_line, 13, &cons));
    ASSERT_EQ(
        C_REST_ERROR_PARSE,
        c_rest_http23_process(ctx, (const char *)bad_empty_colon, 11, &cons));
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_process(ctx, (const char *)no_nl_hdr, 12, &cons));
  }

  /* Additional RFC 9113/9114 branch coverage: empty pseudo-header val, leading
   * LF, pseudo-header without 2nd colon, 7-char/5-char non-method/path names,
   * END_STREAM without method or without path, status > 599, key with NULL val,
   * H3 hdr_len > 16383, and request_cleanup failure in ctx_destroy */
  {
    c_rest_http23_ctx_t *bctx = NULL;
    /* Leading '\n', ":abcdef: 1\n" (colon=7 != :method), ":abcd: 1\n"
     * (colon=5 != :path), ":method:   \n" (empty val len=0) */
    static const char edge_hdrs[] = "\n:abcdef: 1\n:abcd: 1\n:method:   \n";
    unsigned char h2_edge[64];
    size_t eh_len = sizeof(edge_hdrs) - 1;
    unsigned char h2_no_method_es[15] = {
        0, 0, 6, 0x01U, 0x05U, 0, 0, 0, 1, 'a', ':', ' ', '1', '\r', '\n'};
    unsigned char h2_data_es[10] = {0, 0, 1, 0x00U, 0x01U, 0, 0, 0, 1, 'x'};
    unsigned char h2_bad_pseudo[14] = {0, 0, 5,   0x01U, 0x04U, 0,   0,
                                       0, 1, ':', 'f',   'o',   'o', '\n'};

    ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &bctx));
    /* END_STREAM HEADERS & DATA when method_buf == NULL */
    ASSERT_EQ(C_REST_OK, c_rest_http23_process(
                             bctx, (const char *)h2_no_method_es, 15, &cons));
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_process(bctx, (const char *)h2_data_es, 10, &cons));

    /* Set method_buf (empty len=0) without path_buf, then END_STREAM on HEADERS
     * & DATA */
    memset(h2_edge, 0, 9);
    h2_edge[2] = (unsigned char)eh_len;
    h2_edge[3] = 0x01U;
    h2_edge[4] = 0x05U;
    h2_edge[8] = 1;
    memcpy(h2_edge + 9, edge_hdrs, eh_len);
    ASSERT_EQ(C_REST_OK, c_rest_http23_process(bctx, (const char *)h2_edge,
                                               9 + eh_len, &cons));
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_process(bctx, (const char *)h2_data_es, 10, &cons));

    /* Pseudo-header missing second colon */
    ASSERT_EQ(
        C_REST_ERROR_PARSE,
        c_rest_http23_process(bctx, (const char *)h2_bad_pseudo, 14, &cons));

    /* Status > 599 and header with non-NULL key + NULL value */
    {
      struct c_rest_header half_h;
      char hk[] = "x-half";
      memset(&half_h, 0, sizeof(half_h));
      half_h.key = hk;
      half_h.value = NULL;
      res_obj.status_code = 999;
      res_obj.headers = &half_h;
      res_obj.body = NULL;
      res_obj.body_len = 0;
      ASSERT_EQ(C_REST_OK, c_rest_http23_format_response(bctx, &res_obj,
                                                         &out_buf, &out_len));
      C_REST_FREE(out_buf);
      res_obj.headers = NULL;
    }

    /* Request cleanup failure in ctx_destroy */
    g_mock_req_cleanup_fail = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_http23_ctx_destroy(bctx));
    g_mock_req_cleanup_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_destroy(bctx));
  }

  /* HTTP/3 DATA with method_buf set & path_buf NULL, and hdr_len > 16383 */
  {
    c_rest_http23_ctx_t *h3b = NULL;
    unsigned char h3_m_only[3] = {0x01U, 0x01U, 0x82U};
    unsigned char h3_d_only[3] = {0x00U, 0x01U, 'x'};
    unsigned char h3_no_m[6] = {0x01U, 0x04U, 'a', ':', '1', '\n'};
    struct c_rest_header big_h;
    char big_k[] = "x-big";
    char *big_v = (char *)malloc(16400);

    memset(big_v, 'a', 16399);
    big_v[16399] = '\0';
    memset(&big_h, 0, sizeof(big_h));
    big_h.key = big_k;
    big_h.value = big_v;

    ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP3, &h3b));
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_process(h3b, (const char *)h3_no_m, 6, &cons));
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_process(h3b, (const char *)h3_m_only, 3, &cons));
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_process(h3b, (const char *)h3_d_only, 3, &cons));

    res_obj.status_code = 200;
    res_obj.headers = &big_h;
    res_obj.body = NULL;
    res_obj.body_len = 0;
    ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
              c_rest_http23_format_response(h3b, &res_obj, &out_buf, &out_len));
    res_obj.headers = NULL;
    free(big_v);
    ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_destroy(h3b));
  }

  ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_destroy(ctx));

  ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP3, &ctx));
  {
    unsigned char bad_h3_type[2] = {0x80U, 0x00U};
    unsigned char bad_h3_len[2] = {0x01U, 0x80U};
    unsigned char bad_h3_hdr[3] = {0x01U, 0x01U, 0xFFU};
    unsigned char h3_incomplete_req[3] = {0x01U, 0x01U, 0x82U};
    unsigned char h3_data_only[3] = {0x00U, 0x01U, 'x'};
    ASSERT_EQ(C_REST_ERROR_PARSE,
              c_rest_http23_process(ctx, (const char *)bad_h3_type, 2, &cons));
    ASSERT_EQ(C_REST_ERROR_PARSE,
              c_rest_http23_process(ctx, (const char *)bad_h3_len, 2, &cons));
    ASSERT_EQ(C_REST_ERROR_PARSE,
              c_rest_http23_process(ctx, (const char *)bad_h3_hdr, 3, &cons));
    ASSERT_EQ(C_REST_OK,
              c_rest_http23_process(ctx, (const char *)h3_data_only, 3, &cons));
    ASSERT_EQ(C_REST_OK, c_rest_http23_process(
                             ctx, (const char *)h3_incomplete_req, 3, &cons));
  }
  ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_destroy(ctx));

  for (k = 0; k < 12; k++) {
    c_rest_http23_ctx_t *octx = NULL;
    c_rest_error_t rc;
    unsigned char h2_full[] = {
        0,   0,   26,    0x01U, 0x04U, 0,   0,    0,   1,   ':',  'm',  'e',
        't', 'h', 'o',   'd',   ':',   ' ', 'G',  'E', 'T', '\n', ':',  'p',
        'a', 't', 'h',   ':',   ' ',   '/', '\n', 'k', ':', 'v',  '\n', 0,
        0,   2,   0x00U, 0x01U, 0,     0,   0,    1,   'h', 'i'};

    g_crf_malloc_hook = fail_malloc_n;
    g_crf_realloc_hook = fail_realloc_n;
    g_malloc_fail_after = k;
    rc = c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &octx);
    if (rc == C_REST_OK) {
      rc = c_rest_http23_process(octx, (const char *)h2_full, sizeof(h2_full),
                                 &cons);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM);
      rc = c_rest_http23_format_response(octx, &res_obj, &out_buf, &out_len);
      if (rc == C_REST_OK)
        C_REST_FREE(out_buf);
      g_crf_malloc_hook = NULL;
      g_crf_realloc_hook = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_destroy(octx));
    } else {
      ASSERT_EQ(C_REST_ERROR_OOM, rc);
    }
    g_crf_malloc_hook = NULL;
    g_crf_realloc_hook = NULL;
    g_malloc_fail_after = -1;
  }

  {
    c_rest_http23_ctx_t *h3c = NULL;
    unsigned char h3_data[3] = {0x00U, 0x01U, 'z'};
    unsigned char h2_idx_each[4][10] = {
        {0, 0, 1, 0x01U, 0x04U, 0, 0, 0, 1, 0x82U},
        {0, 0, 1, 0x01U, 0x04U, 0, 0, 0, 1, 0x83U},
        {0, 0, 1, 0x01U, 0x04U, 0, 0, 0, 1, 0x84U},
        {0, 0, 1, 0x01U, 0x04U, 0, 0, 0, 1, 0x85U}};
    int idx;

    ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP3, &h3c));
    g_crf_malloc_hook = fail_malloc_n;
    g_crf_realloc_hook = fail_realloc_n;
    g_malloc_fail_after = 0;
    ASSERT_EQ(C_REST_ERROR_OOM,
              c_rest_http23_format_response(h3c, &res_obj, &out_buf, &out_len));
    g_malloc_fail_after = 0;
    ASSERT_EQ(C_REST_ERROR_OOM,
              c_rest_http23_process(h3c, (const char *)h3_data, 3, &cons));
    g_crf_malloc_hook = NULL;
    g_crf_realloc_hook = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_destroy(h3c));

    for (idx = 0; idx < 4; idx++) {
      c_rest_http23_ctx_t *ic = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_init(C_REST_PROTOCOL_HTTP2, &ic));
      g_crf_malloc_hook = fail_malloc_n;
      g_malloc_fail_after = 0;
      ASSERT_EQ(
          C_REST_ERROR_OOM,
          c_rest_http23_process(ic, (const char *)h2_idx_each[idx], 10, &cons));
      g_crf_malloc_hook = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_http23_ctx_destroy(ic));
    }
  }

  PASS();
}

SUITE(http23_suite) {
  RUN_TEST(test_http2_rfc9113_framing);
  RUN_TEST(test_http23_indexed_headers_and_http3);
  RUN_TEST(test_http23_errors_and_oom);
}
