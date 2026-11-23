/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static int g_fail_callbacks = 0;
static int g_fail_malloc_at = 0;
static int g_fail_realloc_at = 0;

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

struct basic_parser_state {
  int state;
  int has_error;
  int keep_alive;
  size_t content_length;
  int is_chunked;

  char *buf;
  size_t buf_len;
  size_t buf_cap;

  char *key_buf;
  size_t key_len;
  size_t key_cap;

  size_t body_read;
  size_t chunk_left;
};

static int method_called = 0;
static int url_called = 0;
static int complete_called = 0;

static c_rest_error_t on_method(c_rest_parser_context *ctx, const char *method,
                                size_t len) {
  (void)ctx;
  (void)method;
  (void)len;
  if (g_fail_callbacks > 0) {
    g_fail_callbacks--;
    if (g_fail_callbacks == 0)
      return C_REST_ERROR_GENERIC;
  }
  method_called = 1;
  return C_REST_OK;
}

static c_rest_error_t on_url(c_rest_parser_context *ctx, const char *url,
                             size_t len) {
  (void)ctx;
  (void)url;
  (void)len;
  if (g_fail_callbacks > 0) {
    g_fail_callbacks--;
    if (g_fail_callbacks == 0)
      return C_REST_ERROR_GENERIC;
  }
  url_called = 1;
  return C_REST_OK;
}

static c_rest_error_t on_complete(c_rest_parser_context *ctx) {
  (void)ctx;
  if (g_fail_callbacks > 0) {
    g_fail_callbacks--;
    if (g_fail_callbacks == 0)
      return C_REST_ERROR_GENERIC;
  }
  complete_called = 1;
  return C_REST_OK;
}

static c_rest_error_t on_header(c_rest_parser_context *ctx, const char *key,
                                size_t key_len, const char *val,
                                size_t val_len) {
  (void)ctx;
  (void)key;
  (void)key_len;
  (void)val;
  (void)val_len;
  if (g_fail_callbacks > 0) {
    g_fail_callbacks--;
    if (g_fail_callbacks == 0)
      return C_REST_ERROR_GENERIC;
  }
  return C_REST_OK;
}

static c_rest_error_t on_body(c_rest_parser_context *ctx, const char *data,
                              size_t len) {
  (void)ctx;
  (void)data;
  (void)len;
  if (g_fail_callbacks > 0) {
    g_fail_callbacks--;
    if (g_fail_callbacks == 0)
      return C_REST_ERROR_GENERIC;
  }
  return C_REST_OK;
}

static c_rest_error_t on_error(c_rest_parser_context *ctx, const char *msg) {
  (void)msg;
  if (ctx->user_data) {
    return C_REST_ERROR_GENERIC;
  }
  return C_REST_OK;
}

TEST test_parser_other_headers(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  int keep_alive;
  const char *req =
      "POST / HTTP/1.1\r\nConnection: keep-alive\r\nContent-Lengt: "
      "10\r\nTransfer-Encoding: gzip\r\n\r\n1234567890";
  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = on_header;
  callbacks.on_body = on_body;
  callbacks.on_complete = on_complete;
  callbacks.on_error = on_error;
  ASSERT_FALSE(c_rest_parser_get_basic_vtable(&vtable) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req, strlen(req), &parsed) !=
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_should_keep_alive(&ctx, &keep_alive) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);
  PASS();
}

TEST test_parser_no_callbacks(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req = "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\n12345";
  const char *req_chunked = "POST / HTTP/1.1\r\nTransfer-Encoding: "
                            "chunked\r\n\r\n5\r\n12345\r\n0\r\n\r\n";
  const char *req_err = "MALFORMED HTTP ";
  memset(&callbacks, 0, sizeof(callbacks));

  ASSERT_FALSE(c_rest_parser_get_basic_vtable(&vtable) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req, strlen(req), &parsed) !=
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req_chunked, strlen(req_chunked),
                                     &parsed) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_EQ(C_REST_ERROR_PARSE,
            c_rest_parser_execute(&ctx, req_err, strlen(req_err), &parsed));
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);
  PASS();
}

TEST test_parser_long_header(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req =
      "GET / "
      "HTTP/"
      "1.1\r\nX-Very-Long-Header-Key-That-Exceeds-Sixty-Four-Characters-By-A-"
      "Lot-To-Trigger-Reallocation-Of-Key-Buffer: true\r\n\r\n";
  memset(&callbacks, 0, sizeof(callbacks));
  ASSERT_FALSE(c_rest_parser_get_basic_vtable(&vtable) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req, strlen(req), &parsed) !=
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);
  PASS();
}

TEST test_parser_content_length_zero(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req = "POST / HTTP/1.1\r\nContent-Length: 0\r\n\r\n";
  memset(&callbacks, 0, sizeof(callbacks));
  ASSERT_FALSE(c_rest_parser_get_basic_vtable(&vtable) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req, strlen(req), &parsed) !=
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);
  PASS();
}

TEST test_parser_connection_close_whitespace(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  int keep_alive;
  const char *req = "GET / HTTP/1.1\r\nConnection:  close \r\n\r\n";
  const char *req_empty_key = "GET / HTTP/1.1\r\n: some_val\r\n\r\n";
  const char *req_empty_val = "GET / HTTP/1.1\r\nConnection:\r\n\r\n";
  memset(&callbacks, 0, sizeof(callbacks));
  ASSERT_FALSE(c_rest_parser_get_basic_vtable(&vtable) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req, strlen(req), &parsed) !=
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_should_keep_alive(&ctx, &keep_alive) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_EQ(C_REST_ERROR_PARSE,
            c_rest_parser_execute(&ctx, req_empty_key, strlen(req_empty_key),
                                  &parsed));
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_parser_execute(&ctx, req_empty_val,
                                             strlen(req_empty_val), &parsed));
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);
  PASS();
}

TEST test_parser_chunked_split(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req_part1 =
      "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\n12";
  const char *req_part2 = "345\r\n0\r\n\r\n";
  memset(&callbacks, 0, sizeof(callbacks));
  ASSERT_FALSE(c_rest_parser_get_basic_vtable(&vtable) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req_part1, strlen(req_part1),
                                     &parsed) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req_part2, strlen(req_part2),
                                     &parsed) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);
  PASS();
}

TEST test_parser_is_complete(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  int complete;
  const char *req1 = "GET / HTTP/1.1\r\n\r\n";
  const char *req2 = "POST / HTTP/1.1\r\nContent-Length: 1\r\n\r\na";
  const char *req3 =
      "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n";

  memset(&callbacks, 0, sizeof(callbacks));
  ASSERT_FALSE(c_rest_parser_get_basic_vtable(&vtable) != C_REST_OK);

  /* Incomplete body (Identity, state 7) */
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(
                   &ctx, "POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\n123", 40,
                   &parsed) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_is_complete(&ctx, &complete) != C_REST_OK);
  ASSERT_FALSE(complete != 0);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  /* Incomplete body (Chunked data, state 9) */
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(
      c_rest_parser_execute(
          &ctx, "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\n12",
          52, &parsed) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_is_complete(&ctx, &complete) != C_REST_OK);
  ASSERT_FALSE(complete != 0);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  /* Incomplete body (Chunked CRLF, state 11) */
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(
      c_rest_parser_execute(
          &ctx,
          "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\n12345\r",
          56, &parsed) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_is_complete(&ctx, &complete) != C_REST_OK);
  ASSERT_FALSE(complete != 0);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  /* Headers complete */
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req1, strlen(req1), &parsed) !=
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_is_complete(&ctx, &complete) != C_REST_OK);
  ASSERT_FALSE(complete != 1);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  /* Body complete */
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req2, strlen(req2), &parsed) !=
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_is_complete(&ctx, &complete) != C_REST_OK);
  ASSERT_FALSE(complete != 1);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  /* Chunked complete */
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req3, strlen(req3), &parsed) !=
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_is_complete(&ctx, &complete) != C_REST_OK);
  ASSERT_FALSE(complete != 1);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  /* Custom vtable rejected by c_rest_parser_is_complete */
  {
    struct c_rest_parser_vtable custom_vt;
    int dummy_st = 0;
    memset(&custom_vt, 0, sizeof(custom_vt));
    memset(&ctx, 0, sizeof(ctx));
    ctx.vtable = &custom_vt;
    ctx.internal_state = &dummy_st;
    ASSERT_FALSE(c_rest_parser_is_complete(&ctx, &complete) !=
                 C_REST_ERROR_GENERIC);
    ctx.vtable = vtable;
    ctx.internal_state = NULL;
    ASSERT_FALSE(c_rest_parser_is_complete(&ctx, &complete) !=
                 C_REST_ERROR_GENERIC);
  }

  /* Max buffer size DoS cap (URL/value and header key) */
  {
    char *oversized = (char *)malloc(70000);
    ASSERT_FALSE(oversized == NULL);
    if (oversized) {
      /* Oversized URL (> 65536 bytes) */
      memcpy(oversized, "GET /", 5);
      memset(oversized + 5, 'a', 66000);
      oversized[66005] = '\0';
      ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) !=
                   C_REST_OK);
      ASSERT_FALSE(c_rest_parser_execute(&ctx, oversized, 66005, &parsed) !=
                   C_REST_ERROR_PARSE);
      ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

      /* Oversized Header Key (> 65536 bytes) */
      memcpy(oversized, "GET / HTTP/1.1\r\n", 16);
      memset(oversized + 16, 'K', 66000);
      oversized[66016] = '\0';
      ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) !=
                   C_REST_OK);
      ASSERT_FALSE(c_rest_parser_execute(&ctx, oversized, 66016, &parsed) !=
                   C_REST_ERROR_PARSE);
      ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);
      free(oversized);
    }
  }

  PASS();
}

TEST test_parser_chunked(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  int keep_alive;
  int complete = 0;
  const char *req = "POST / HTTP/1.1\r\nTransfer-Encoding: "
                    "chunked\r\n\r\n4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n";
  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = on_header;
  callbacks.on_body = on_body;
  callbacks.on_complete = on_complete;
  callbacks.on_error = on_error;
  ASSERT_FALSE(c_rest_parser_get_basic_vtable(&vtable) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req, strlen(req), &parsed) !=
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_should_keep_alive(&ctx, &keep_alive) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_is_complete(&ctx, &complete) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);
  PASS();
}

TEST test_parser_content_length(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req = "POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\n1234567890";
  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = on_header;
  callbacks.on_body = on_body;
  callbacks.on_complete = on_complete;
  callbacks.on_error = on_error;
  ASSERT_FALSE(c_rest_parser_get_basic_vtable(&vtable) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(&ctx, req, strlen(req), &parsed) !=
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);
  PASS();
}
TEST test_parser_errors(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  int keep_alive = 1;
  const char *req = "GET / HTTP/1.1\r\nHeader-No-Colon\r\n\r\n";
  static const char *const bad_inputs[] = {
      " / HTTP/1.1\r\n\r\n",
      "GET\r\n",
      "G:T / HTTP/1.1\r\n\r\n",
      "G/T / HTTP/1.1\r\n\r\n",
      "G\x7FT / HTTP/1.1\r\n\r\n",
      "G\x01T / HTTP/1.1\r\n\r\n",
      "GET  HTTP/1.1\r\n\r\n",
      "GET /\x7F HTTP/1.1\r\n\r\n",
      "GET /\r\n",
      "GET x HTTP/1.1\r\n\r\n",
      "GET *foo HTTP/1.1\r\n\r\n",
      "GET badtarget HTTP/1.1\r\n\r\n",
      "GET / HTTP/2.0\r\n\r\n",
      "GET / HTTP/1.9\r\n\r\n",
      "GET / HTTP/1\r\n\r\n",
      "GET / HTTP/1.1\r\nBad Header: 1\r\n\r\n",
      "GET / HTTP/1.1\r\nBad\x7FHeader: 1\r\n\r\n",
      "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n;ext\r\n",
      "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n\r\n",
      "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\nZZ\r\n"};
  static const char *const valid_targets[] = {
      "OPTIONS * HTTP/1.0\r\n\r\n",
      "GET http://example.com/path HTTP/1.0\r\nConnection: keep-alive\r\n\r\n",
      ("GET https://example.com/path HTTP/1.1\r\n"
       "Connection: cl0se\r\n"
       "Connection: keep-alivX\r\n"
       "content-lengthX: 0\r\n"
       "transfer-encodingX: x\r\n"
       "connectionX: x\r\n\r\n"),
      ("POST / HTTP/1.1\r\nTransfer-Encoding: "
       "chunked\r\n\r\n4;ext=1\r\nWiki\r\n0;done\r\n\r\n")};
  size_t k;

  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = on_header;
  callbacks.on_body = on_body;
  callbacks.on_complete = on_complete;
  callbacks.on_error = on_error;
  ASSERT_FALSE(c_rest_parser_get_basic_vtable(&vtable) != C_REST_OK);
  ASSERT_FALSE(c_rest_parser_init(&ctx, vtable, &callbacks, NULL) != C_REST_OK);
  ASSERT_EQ(C_REST_ERROR_PARSE,
            c_rest_parser_execute(&ctx, req, strlen(req), &parsed));
  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);

  for (k = 0; k < sizeof(bad_inputs) / sizeof(bad_inputs[0]); k++) {
    ASSERT_EQ(C_REST_OK, c_rest_parser_init(&ctx, vtable, &callbacks, NULL));
    ASSERT_EQ(C_REST_ERROR_PARSE,
              c_rest_parser_execute(&ctx, bad_inputs[k], strlen(bad_inputs[k]),
                                    &parsed));
    ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx));
  }

  for (k = 0; k < sizeof(valid_targets) / sizeof(valid_targets[0]); k++) {
    ASSERT_EQ(C_REST_OK, c_rest_parser_init(&ctx, vtable, &callbacks, NULL));
    ASSERT_EQ(C_REST_OK,
              c_rest_parser_execute(&ctx, valid_targets[k],
                                    strlen(valid_targets[k]), &parsed));
    ASSERT_EQ(C_REST_OK, c_rest_parser_should_keep_alive(&ctx, &keep_alive));
    if (k == 0)
      ASSERT_EQ(0, keep_alive);
    if (k == 1)
      ASSERT_EQ(1, keep_alive);
    ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx));
  }

  /* Chunk extension with failing callbacks (chunk_left > 0 and chunk_left == 0)
   */
  {
    const char *chunk_ext_req = "POST / HTTP/1.1\r\nTransfer-Encoding: "
                                "chunked\r\n\r\n4;a=1\r\nWiki\r\n0;b=2\r\n\r\n";
    int fi;
    for (fi = 1; fi <= 6; fi++) {
      c_rest_error_t er;
      ASSERT_EQ(C_REST_OK, c_rest_parser_init(&ctx, vtable, &callbacks, NULL));
      g_fail_callbacks = fi;
      er = c_rest_parser_execute(&ctx, chunk_ext_req, strlen(chunk_ext_req),
                                 &parsed);
      ASSERT(er == C_REST_OK || er == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx));
    }
    g_fail_callbacks = 0;
  }

  /* Version append_buf realloc OOM and chunk-extension without on_complete */
  {
    char big_ver[300];
    struct c_rest_parser_callbacks no_cb;
    const char *ce_nocb = "POST / HTTP/1.1\r\nTransfer-Encoding: "
                          "chunked\r\n\r\n0;done\r\n\r\n";
    memset(&no_cb, 0, sizeof(no_cb));
    memcpy(big_ver, "GET / ", 6);
    memset(big_ver + 6, 'H', sizeof(big_ver) - 7);
    big_ver[sizeof(big_ver) - 1] = '\0';

    ASSERT_EQ(C_REST_OK, c_rest_parser_init(&ctx, vtable, &callbacks, NULL));
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = 2;
    ASSERT_NEQ(C_REST_OK,
               c_rest_parser_execute(&ctx, big_ver, strlen(big_ver), &parsed));
    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
    ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx));

    ASSERT_EQ(C_REST_OK, c_rest_parser_init(&ctx, vtable, &no_cb, NULL));
    ASSERT_EQ(C_REST_OK,
              c_rest_parser_execute(&ctx, ce_nocb, strlen(ce_nocb), &parsed));
    ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx));
  }

  /* NULL checks */
  ASSERT_FALSE(c_rest_parser_init(NULL, vtable, &callbacks, NULL) == C_REST_OK);
  ASSERT_FALSE(c_rest_parser_init(&ctx, NULL, &callbacks, NULL) == C_REST_OK);
  ASSERT_FALSE(c_rest_parser_execute(NULL, req, strlen(req), &parsed) ==
               C_REST_OK);
  ASSERT_FALSE(c_rest_parser_destroy(NULL) == C_REST_OK);
  PASS();
}

static c_rest_error_t
dummy_init(c_rest_parser_context *ctx,
           const struct c_rest_parser_callbacks *callbacks, void *user_data) {
  (void)ctx;
  (void)callbacks;
  (void)user_data;
  return C_REST_OK;
}

static c_rest_error_t dummy_execute(c_rest_parser_context *ctx,
                                    const char *data, size_t len,
                                    size_t *out_parsed) {
  (void)ctx;
  if (!data)
    return C_REST_ERROR_GENERIC;
  *out_parsed = len;
  return C_REST_OK;
}

static c_rest_error_t dummy_destroy(c_rest_parser_context *ctx) {
  (void)ctx;
  return C_REST_OK;
}

TEST test_parser_invalid_args_and_oom(void) {
  struct c_rest_parser_context ctx;
  struct c_rest_parser_vtable vtable;
  c_rest_error_t rc;
  size_t parsed = 0;
  int keep_alive;

  memset(&ctx, 0, sizeof(ctx));
  memset(&vtable, 0, sizeof(vtable));

  rc = c_rest_parser_init(NULL, &vtable, NULL, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_init(&ctx, NULL, NULL, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_init(&ctx, &vtable, NULL, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  vtable.init = dummy_init;
  vtable.execute = dummy_execute;
  vtable.destroy = dummy_destroy;

  rc = c_rest_parser_init(&ctx, &vtable, NULL, NULL);
  ASSERT_FALSE(rc != C_REST_OK);

  ctx.vtable = &vtable;
  rc = c_rest_parser_execute(&ctx, "data", 4, &parsed);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_parser_destroy(&ctx);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_parser_execute(NULL, "data", 4, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_execute(&ctx, NULL, 4, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_execute(&ctx, "data", 4, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  {
    void *tmp = ctx.internal_state;
    const struct c_rest_parser_vtable *basic_vtable_ptr = NULL;

    ctx.internal_state = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_parser_get_basic_vtable(&basic_vtable_ptr));
    ctx.vtable = basic_vtable_ptr;

    rc = c_rest_parser_execute(&ctx, "data", 4, &parsed);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

    rc = c_rest_parser_should_keep_alive(&ctx, &keep_alive);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

    rc = c_rest_parser_destroy(&ctx);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

    ctx.internal_state = tmp;
  }

  ctx.vtable = &vtable;
  vtable.execute = dummy_execute;
  rc = vtable.execute(&ctx, NULL, 4, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  ctx.vtable = NULL;
  rc = c_rest_parser_execute(&ctx, "data", 4, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  vtable.execute = NULL;
  ctx.vtable = &vtable;
  rc = c_rest_parser_execute(&ctx, "data", 4, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  vtable.execute = dummy_execute;

  rc = c_rest_parser_destroy(NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  vtable.destroy = NULL;
  rc = c_rest_parser_destroy(&ctx);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  vtable.destroy = dummy_destroy;

  ctx.vtable = NULL;
  rc = c_rest_parser_destroy(&ctx);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  ctx.vtable = &vtable;

  rc = c_rest_parser_should_keep_alive(NULL, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_should_keep_alive(&ctx, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_is_complete(NULL, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_is_complete(NULL, &keep_alive);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_is_complete(&ctx, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  {
    int complete_flag;
    rc = c_rest_parser_is_complete(&ctx, &complete_flag);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  }

  rc = c_rest_parser_get_basic_vtable(NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_should_keep_alive(&ctx, &keep_alive);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(keep_alive != 0);

  ctx.vtable = NULL;
  rc = c_rest_parser_should_keep_alive(&ctx, &keep_alive);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(keep_alive != 0);

  vtable.should_keep_alive = NULL;
  ctx.vtable = &vtable;
  rc = c_rest_parser_should_keep_alive(&ctx, &keep_alive);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(keep_alive != 0);

  {
    int i;
    char large_req[4096];
    memset(large_req, 'A', sizeof(large_req));
    large_req[sizeof(large_req) - 1] = '\0';
    memcpy(large_req, "GET /", 5);
    memcpy(large_req + 1000, " HTTP/1.1\r\nLong-Key: ", 21);
    memcpy(large_req + 4000, "\r\n\r\n", 4);

    for (i = 1; i <= 20; i++) {
      c_rest_parser_context ctx_fail;
      const struct c_rest_parser_vtable *real_vtable = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_parser_get_basic_vtable(&real_vtable));

      g_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;

      rc = c_rest_parser_init(&ctx_fail, real_vtable, NULL, NULL);
      if (rc == C_REST_OK) {
        size_t parsed2 = 0;
        rc = c_rest_parser_execute(&ctx_fail, large_req, strlen(large_req),
                                   &parsed2);
        ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx_fail));
      }
      g_crf_malloc_hook = NULL;
    }
    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;

    for (i = 1; i <= 40; i++) {
      c_rest_parser_context ctx_fail;
      const struct c_rest_parser_vtable *real_vtable = NULL;
      size_t parsed2 = 0;
      ASSERT_EQ(C_REST_OK, c_rest_parser_get_basic_vtable(&real_vtable));

      ASSERT_EQ(C_REST_OK,
                c_rest_parser_init(&ctx_fail, real_vtable, NULL, NULL));
      g_fail_realloc_at = -1;
      fail_realloc_n(NULL, 0);
      g_crf_realloc_hook = fail_realloc_n;
      g_fail_realloc_at = i;

      rc = c_rest_parser_execute(&ctx_fail, large_req, strlen(large_req),
                                 &parsed2);
      g_crf_realloc_hook = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx_fail));
    }

    for (i = 1; i <= 40; i++) {
      c_rest_parser_context ctx_fail;
      const struct c_rest_parser_vtable *real_vtable = NULL;
      size_t parsed2 = 0;
      ASSERT_EQ(C_REST_OK, c_rest_parser_get_basic_vtable(&real_vtable));

      ASSERT_EQ(C_REST_OK,
                c_rest_parser_init(&ctx_fail, real_vtable, NULL, NULL));
      g_fail_realloc_at = -1;
      fail_realloc_n(NULL, 0);
      g_crf_realloc_hook = fail_realloc_n;
      g_fail_realloc_at = i;

      rc = c_rest_parser_execute(
          &ctx_fail, "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n",
          47, &parsed2);
      if (rc == C_REST_OK) {
        char big_chunk_size[1024];
        memset(big_chunk_size, '1', sizeof(big_chunk_size));
        big_chunk_size[sizeof(big_chunk_size) - 1] = '\0';
        rc = c_rest_parser_execute(&ctx_fail, big_chunk_size, 1000, &parsed2);
      }
      g_crf_realloc_hook = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx_fail));
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }
  PASS();
}

TEST test_parser_main(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const char *valid_req =
      "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
  const char *malformed_req = "MALFORMED REQUEST DATA";
  size_t parsed;
  int res;
  const struct c_rest_parser_vtable *vtable = NULL;

  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = on_header;
  callbacks.on_body = on_body;
  callbacks.on_complete = on_complete;
  callbacks.on_error = on_error;

  res = (int)c_rest_parser_get_basic_vtable(&vtable);
  ASSERT_FALSE(res != 0);

  res = (int)c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  ASSERT_FALSE(res != 0);
  ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx));

  {
    int fail_idx;
    const char *reqs[] = {
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n",
        "POST / HTTP/1.1\r\nContent-Length: 4\r\n\r\nbody1234",
        ("POST / HTTP/1.1\r\nTransfer-Encoding: "
         "chunked\r\n\r\n4\r\nbody1234\r\n0\r\n\r\n")};
    int req_idx;
    for (req_idx = 0; req_idx < 3; req_idx++) {
      for (fail_idx = 1; fail_idx <= 10; fail_idx++) {
        c_rest_parser_context ctx_fail;
        size_t p = 0;
        c_rest_error_t exec_rc;
        ASSERT_EQ(C_REST_OK,
                  c_rest_parser_init(&ctx_fail, vtable, &callbacks, NULL));
        g_fail_callbacks = fail_idx;
        exec_rc = c_rest_parser_execute(&ctx_fail, reqs[req_idx],
                                        strlen(reqs[req_idx]), &p);
        ASSERT(exec_rc == C_REST_OK || exec_rc == C_REST_ERROR_GENERIC);
        ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx_fail));
      }
    }
    g_fail_callbacks = 0;
  }

  res = (int)c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  ASSERT_FALSE(res != 0);

  res = (int)c_rest_parser_execute(&ctx, valid_req, strlen(valid_req), &parsed);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(parsed != strlen(valid_req));

  ASSERT_FALSE(method_called == 0);
  ASSERT_FALSE(url_called == 0);
  ASSERT_FALSE(complete_called == 0);

  res = (int)c_rest_parser_execute(&ctx, malformed_req, strlen(malformed_req),
                                   &parsed);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(parsed != 0);

  {
    c_rest_parser_context ctx2;
    const struct c_rest_parser_vtable *vtable2 = NULL;
    struct c_rest_parser_callbacks callbacks2;
    callbacks2.on_method = on_method;
    callbacks2.on_url = on_url;
    callbacks2.on_header = on_header;
    callbacks2.on_body = on_body;
    callbacks2.on_complete = on_complete;
    callbacks2.on_error = on_error;
    ASSERT_EQ(C_REST_OK, c_rest_parser_get_basic_vtable(&vtable2));
    ASSERT_EQ(C_REST_OK,
              c_rest_parser_init(&ctx2, vtable2, &callbacks2, (void *)1));
    res = (int)c_rest_parser_execute(&ctx2, malformed_req,
                                     strlen(malformed_req), &parsed);
    ASSERT_FALSE(res != C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx2));

    ASSERT_EQ(C_REST_OK, c_rest_parser_init(&ctx2, vtable2, &callbacks2, NULL));
    res = (int)c_rest_parser_execute(&ctx2, malformed_req,
                                     strlen(malformed_req), &parsed);
    ASSERT_EQ((int)C_REST_ERROR_PARSE, res);

    res = (int)c_rest_parser_execute(&ctx2, "data", 4, &parsed);
    ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

    ((struct basic_parser_state *)ctx2.internal_state)->has_error = 0;
    ((struct basic_parser_state *)ctx2.internal_state)->state = 999;
    res = (int)c_rest_parser_execute(&ctx2, "data", 4, &parsed);
    ASSERT_FALSE(res != C_REST_ERROR_GENERIC);

    ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx2));
  }

  {
    char huge_method[512];
    c_rest_parser_context ctx3;
    memset(huge_method, 'M', sizeof(huge_method));
    huge_method[sizeof(huge_method) - 1] = '\0';
    ASSERT_EQ(C_REST_OK, c_rest_parser_init(&ctx3, vtable, &callbacks, NULL));
    g_fail_realloc_at = 1;
    g_crf_realloc_hook = fail_realloc_n;
    res = (int)c_rest_parser_execute(&ctx3, huge_method,
                                     sizeof(huge_method) - 1, &parsed);
    ASSERT_EQ(C_REST_OK, res);
    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
    ASSERT_EQ(C_REST_OK, c_rest_parser_destroy(&ctx3));
  }

  {
    c_rest_parser_context ctx3;
    const struct c_rest_parser_vtable *vtable3 = NULL;
    const char *req_nl = "\r\n\r\nGET / HTTP/1.1\r\n\r\n";
    ASSERT_EQ(C_REST_OK, c_rest_parser_get_basic_vtable(&vtable3));
    res = (int)c_rest_parser_init(&ctx3, vtable3, &callbacks, NULL);
    ASSERT_FALSE(res != C_REST_OK);
    ASSERT_EQ(C_REST_OK,
              vtable3->execute(&ctx3, req_nl, strlen(req_nl), &parsed));
    ASSERT_EQ(C_REST_OK, vtable3->destroy(&ctx3));
  }

  {
    c_rest_parser_context ctx3;
    const struct c_rest_parser_vtable *vtable3 = NULL;
    const char *req_empty_hdr =
        "GET / HTTP/1.1\r\nX-Prev: secret\r\nX-Empty:\r\n\r\n";
    ASSERT_EQ(C_REST_OK, c_rest_parser_get_basic_vtable(&vtable3));
    res = (int)c_rest_parser_init(&ctx3, vtable3, &callbacks, NULL);
    ASSERT_FALSE(res != C_REST_OK);
    res = (int)vtable3->execute(&ctx3, req_empty_hdr, strlen(req_empty_hdr),
                                &parsed);
    ASSERT_FALSE(res != C_REST_OK);
    ASSERT_FALSE(vtable3->destroy(&ctx3) != C_REST_OK);
  }

  ASSERT_FALSE(c_rest_parser_destroy(&ctx) != C_REST_OK);
  PASS();
}

SUITE(parser_suite) {
  RUN_TEST(test_parser_invalid_args_and_oom);
  RUN_TEST(test_parser_main);
  RUN_TEST(test_parser_other_headers);
  RUN_TEST(test_parser_no_callbacks);
  RUN_TEST(test_parser_long_header);
  RUN_TEST(test_parser_content_length_zero);
  RUN_TEST(test_parser_connection_close_whitespace);
  RUN_TEST(test_parser_chunked_split);
  RUN_TEST(test_parser_is_complete);
  RUN_TEST(test_parser_chunked);
  RUN_TEST(test_parser_content_length);
  RUN_TEST(test_parser_errors);
}
