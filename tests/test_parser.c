int g_fail_callbacks = 0;

#include <stdlib.h>
/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_parser.h"

#include <stdio.h>
#include <string.h>
/* clang-format on */

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
extern int g_fail_callbacks;

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
  if (msg && strcmp(msg, "Malformed") == 0 && ctx->user_data) {
    return C_REST_ERROR_GENERIC; /* to hit the branch if user_data is set */
  }
  return C_REST_OK;
}

static int test_parser_other_headers(void) {
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
  (void)!c_rest_parser_get_basic_vtable(&vtable);
  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  (void)!c_rest_parser_should_keep_alive(&ctx, &keep_alive);
  (void)!c_rest_parser_destroy(&ctx);
  return 0;
}

static int test_parser_no_callbacks(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req = "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\n12345";
  const char *req_chunked = "POST / HTTP/1.1\r\nTransfer-Encoding: "
                            "chunked\r\n\r\n5\r\n12345\r\n0\r\n\r\n";
  const char *req_err = "MALFORMED HTTP";
  memset(&callbacks, 0, sizeof(callbacks));

  (void)!c_rest_parser_get_basic_vtable(&vtable);
  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  (void)!c_rest_parser_destroy(&ctx);

  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req_chunked, strlen(req_chunked), &parsed);
  (void)!c_rest_parser_destroy(&ctx);

  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req_err, strlen(req_err), &parsed);
  (void)!c_rest_parser_destroy(&ctx);
  return 0;
}

static int test_parser_long_header(void) {
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
  (void)!c_rest_parser_get_basic_vtable(&vtable);
  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  (void)!c_rest_parser_destroy(&ctx);
  return 0;
}

static int test_parser_content_length_zero(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req = "POST / HTTP/1.1\r\nContent-Length: 0\r\n\r\n";
  memset(&callbacks, 0, sizeof(callbacks));
  (void)!c_rest_parser_get_basic_vtable(&vtable);
  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  (void)!c_rest_parser_destroy(&ctx);
  return 0;
}

static int test_parser_connection_close_whitespace(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  int keep_alive;
  const char *req = "GET / HTTP/1.1\r\nConnection:  close \r\n\r\n";
  memset(&callbacks, 0, sizeof(callbacks));
  (void)!c_rest_parser_get_basic_vtable(&vtable);
  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  (void)!c_rest_parser_should_keep_alive(&ctx, &keep_alive);
  (void)!c_rest_parser_destroy(&ctx);
  return 0;
}

static int test_parser_chunked_split(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req_part1 =
      "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\n12";
  const char *req_part2 = "345\r\n0\r\n\r\n";
  memset(&callbacks, 0, sizeof(callbacks));
  (void)!c_rest_parser_get_basic_vtable(&vtable);
  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req_part1, strlen(req_part1), &parsed);
  (void)!c_rest_parser_execute(&ctx, req_part2, strlen(req_part2), &parsed);
  (void)!c_rest_parser_destroy(&ctx);
  return 0;
}

static int test_parser_is_complete(void) {
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
  (void)!c_rest_parser_get_basic_vtable(&vtable);

  /* Incomplete body (Identity) */
  c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  c_rest_parser_execute(
      &ctx, "POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\n123", 40, &parsed);
  c_rest_parser_is_complete(&ctx, &complete);
  c_rest_parser_destroy(&ctx);

  /* Incomplete body (Chunked) */
  c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  c_rest_parser_execute(
      &ctx, "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\n12", 50,
      &parsed);
  c_rest_parser_is_complete(&ctx, &complete);
  c_rest_parser_destroy(&ctx);

  /* Headers complete */
  c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  c_rest_parser_execute(&ctx, req1, strlen(req1), &parsed);
  c_rest_parser_is_complete(&ctx, &complete);
  c_rest_parser_destroy(&ctx);

  /* Body complete */
  c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  c_rest_parser_execute(&ctx, req2, strlen(req2), &parsed);
  c_rest_parser_is_complete(&ctx, &complete);
  c_rest_parser_destroy(&ctx);

  /* Chunked complete */
  c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  c_rest_parser_execute(&ctx, req3, strlen(req3), &parsed);
  c_rest_parser_is_complete(&ctx, &complete);
  c_rest_parser_destroy(&ctx);

  return 0;
}

static int test_parser_chunked(void) {
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
  (void)!c_rest_parser_get_basic_vtable(&vtable);
  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  (void)!c_rest_parser_should_keep_alive(&ctx, &keep_alive);
  (void)!c_rest_parser_is_complete(&ctx, &complete);
  (void)!c_rest_parser_destroy(&ctx);
  return 0;
}

static int test_parser_content_length(void) {
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
  (void)!c_rest_parser_get_basic_vtable(&vtable);
  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  (void)!c_rest_parser_destroy(&ctx);
  return 0;
}
static int test_parser_errors(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const struct c_rest_parser_vtable *vtable = NULL;
  size_t parsed;
  const char *req = "GET / HTTP/1.1\r\nHeader-No-Colon\r\n\r\n";
  int keep_alive;
  int complete = 0;
  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = on_header;
  callbacks.on_body = on_body;
  callbacks.on_complete = on_complete;
  callbacks.on_error = on_error;
  (void)!c_rest_parser_get_basic_vtable(&vtable);
  (void)!c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  (void)!c_rest_parser_execute(&ctx, req, strlen(req), &parsed);
  (void)!c_rest_parser_destroy(&ctx);

  /* NULL checks */
  (void)!c_rest_parser_init(NULL, vtable, &callbacks, NULL);
  (void)!c_rest_parser_init(&ctx, NULL, &callbacks, NULL);
  (void)!c_rest_parser_execute(NULL, req, strlen(req), &parsed);
  (void)!c_rest_parser_destroy(NULL);
  return 0;
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
  if (!data && len > 0)
    return C_REST_ERROR_GENERIC;
  *out_parsed = len;
  return C_REST_OK;
}

static c_rest_error_t dummy_destroy(c_rest_parser_context *ctx) {
  (void)ctx;
  return C_REST_OK;
}

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

static void *fail_realloc_n(void *ptr, size_t size) {
  static int alloc_count = 0;
  extern int g_fail_realloc_at;
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

static void test_invalid_args(int *res_ptr) {
  struct c_rest_parser_context ctx;
  struct c_rest_parser_vtable vtable;
  c_rest_error_t rc;
  size_t parsed = 0;
  int keep_alive;

  memset(&ctx, 0, sizeof(ctx));
  memset(&vtable, 0, sizeof(vtable));

  /* Test NULL ctx */
  rc = c_rest_parser_init(NULL, &vtable, NULL, NULL);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_init(&ctx, NULL, NULL, NULL);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);
  /* Test NULL vtable->init */
  rc = c_rest_parser_init(&ctx, &vtable, NULL, NULL);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  vtable.init = dummy_init;
  vtable.execute = dummy_execute;
  vtable.destroy = dummy_destroy;

  rc = c_rest_parser_init(&ctx, &vtable, NULL, NULL);
  *res_ptr += (rc != C_REST_OK);

  ctx.vtable = &vtable;
  rc = c_rest_parser_execute(&ctx, "data", 4, &parsed);
  *res_ptr += (rc != C_REST_OK);

  rc = c_rest_parser_destroy(&ctx);
  *res_ptr += (rc != C_REST_OK);

  rc = c_rest_parser_execute(NULL, "data", 4, &parsed);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_execute(&ctx, NULL, 4, &parsed);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_execute(&ctx, "data", 4, NULL);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  {
    void *tmp = ctx.internal_state;
    const struct c_rest_parser_vtable *basic_vtable_ptr = NULL;

    ctx.internal_state = NULL;
    c_rest_parser_get_basic_vtable(&basic_vtable_ptr);
    ctx.vtable = basic_vtable_ptr;

    rc = c_rest_parser_execute(&ctx, "data", 4, &parsed);
    *res_ptr += (rc != C_REST_ERROR_GENERIC);

    rc = c_rest_parser_should_keep_alive(&ctx, &keep_alive);
    *res_ptr += (rc != C_REST_ERROR_GENERIC);

    rc = c_rest_parser_destroy(&ctx);
    *res_ptr += (rc != C_REST_ERROR_GENERIC);

    ctx.internal_state = tmp;
  }

  ctx.vtable = &vtable;
  vtable.execute = NULL;
  rc = c_rest_parser_execute(&ctx, "data", 4, &parsed);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);
  vtable.execute = dummy_execute;

  ctx.vtable = NULL;
  rc = c_rest_parser_execute(&ctx, "data", 4, &parsed);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);
  ctx.vtable = &vtable;

  rc = c_rest_parser_destroy(NULL);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  vtable.destroy = NULL;
  rc = c_rest_parser_destroy(&ctx);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);
  vtable.destroy = dummy_destroy;

  ctx.vtable = NULL;
  rc = c_rest_parser_destroy(&ctx);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);
  ctx.vtable = &vtable;

  rc = c_rest_parser_should_keep_alive(NULL, NULL);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_should_keep_alive(&ctx, NULL);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_is_complete(NULL, NULL);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_is_complete(NULL, &keep_alive);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_is_complete(&ctx, NULL);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  {
    int complete_flag;
    rc = c_rest_parser_is_complete(&ctx, &complete_flag);
    *res_ptr += (rc != C_REST_ERROR_GENERIC);
  }

  rc = c_rest_parser_get_basic_vtable(NULL);
  *res_ptr += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_parser_should_keep_alive(&ctx, &keep_alive);
  *res_ptr += (rc != C_REST_OK || keep_alive != 0);

  ctx.vtable = NULL;
  rc = c_rest_parser_should_keep_alive(&ctx, &keep_alive);
  *res_ptr += (rc != C_REST_OK || keep_alive != 0);

  vtable.should_keep_alive = NULL;
  ctx.vtable = &vtable;
  rc = c_rest_parser_should_keep_alive(&ctx, &keep_alive);
  *res_ptr += (rc != C_REST_OK || keep_alive != 0);

  /* Test malloc/realloc failures */
  {
    int i;
    extern void *(*g_crf_malloc_hook)(size_t);
    extern void *(*g_crf_realloc_hook)(void *, size_t);
    extern int g_fail_malloc_at;
    extern int g_fail_realloc_at;

    char large_req[4096];
    memset(large_req, 'A', sizeof(large_req));
    large_req[sizeof(large_req) - 1] = '\0';
    memcpy(large_req, "GET /", 5);
    memcpy(large_req + 1000, " HTTP/1.1\r\nLong-Key: ", 21);
    memcpy(large_req + 4000, "\r\n\r\n", 4);

    for (i = 1; i <= 20; i++) {
      c_rest_parser_context ctx_fail;
      const struct c_rest_parser_vtable *real_vtable = NULL;
      c_rest_parser_get_basic_vtable(&real_vtable);

      g_fail_malloc_at = -1;
      fail_malloc_n(0);
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;

      rc = c_rest_parser_init(&ctx_fail, real_vtable, NULL, NULL);
      if (rc == C_REST_OK) {
        size_t parsed2 = 0;
        rc = c_rest_parser_execute(&ctx_fail, large_req, strlen(large_req),
                                   &parsed2);
        c_rest_parser_destroy(&ctx_fail);
      }
      g_crf_malloc_hook = NULL;
    }
    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;

    for (i = 1; i <= 40; i++) {
      c_rest_parser_context ctx_fail;
      const struct c_rest_parser_vtable *real_vtable = NULL;
      c_rest_parser_get_basic_vtable(&real_vtable);

      rc = c_rest_parser_init(&ctx_fail, real_vtable, NULL, NULL);
      if (rc == C_REST_OK) {
        size_t parsed2 = 0;
        g_fail_realloc_at = -1;
        fail_realloc_n(NULL, 0);
        g_crf_realloc_hook = fail_realloc_n;
        g_fail_realloc_at = i;

        rc = c_rest_parser_execute(&ctx_fail, large_req, strlen(large_req),
                                   &parsed2);
        (void)rc;
        g_crf_realloc_hook = NULL;
        c_rest_parser_destroy(&ctx_fail);
      }
    }

    for (i = 1; i <= 40; i++) {
      c_rest_parser_context ctx_fail;
      const struct c_rest_parser_vtable *real_vtable = NULL;
      c_rest_parser_get_basic_vtable(&real_vtable);

      rc = c_rest_parser_init(&ctx_fail, real_vtable, NULL, NULL);
      if (rc == C_REST_OK) {
        size_t parsed2 = 0;
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
        (void)rc;
        g_crf_realloc_hook = NULL;
        c_rest_parser_destroy(&ctx_fail);
      }
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }
}

int test_parser(void) {
  c_rest_parser_context ctx;
  struct c_rest_parser_callbacks callbacks;
  const char *valid_req =
      "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
  const char *malformed_req = "MALFORMED REQUEST DATA";
  size_t parsed;
  int res;
  int failed = 0;
  const char *msgs[2];

  const struct c_rest_parser_vtable *vtable = NULL;

  printf("Running parser tests...\n");
  test_invalid_args(&failed);

  callbacks.on_method = on_method;
  callbacks.on_url = on_url;
  callbacks.on_header = on_header;
  callbacks.on_body = on_body;
  callbacks.on_complete = on_complete;
  callbacks.on_error = on_error;

  res = c_rest_parser_get_basic_vtable(&vtable);
  failed += (res != 0 || !vtable);
  if (vtable) {
    res = c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
    failed += (res != 0);
  }

  /* Force callbacks to fail individually to hit error propagation paths */
  {
    int fail_idx;
    const char *reqs[] = {
        "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n",            /* No body */
        "POST / HTTP/1.1\r\nContent-Length: 4\r\n\r\nbody1234", /* Identity body
                                                                   with extra
                                                                   data to hit
                                                                   to_read >
                                                                   remaining */
        ("POST / HTTP/1.1\r\nTransfer-Encoding: "
         "chunked\r\n\r\n4\r\nbody1234\r\n0\r\n\r\n") /* Chunked body with extra
                                                        data */
    };
    int req_idx;
    for (req_idx = 0; req_idx < 3; req_idx++) {
      for (fail_idx = 1; fail_idx <= 10; fail_idx++) {
        c_rest_parser_context ctx_fail;
        size_t p = 0;
        c_rest_parser_init(&ctx_fail, vtable, &callbacks, NULL);
        g_fail_callbacks = fail_idx;
        c_rest_parser_execute(&ctx_fail, reqs[req_idx], strlen(reqs[req_idx]),
                              &p);
        c_rest_parser_destroy(&ctx_fail);
      }
    }
    g_fail_callbacks = 0;
  }

  res = c_rest_parser_init(&ctx, vtable, &callbacks, NULL);
  failed += (res != 0);
  if (res == C_REST_OK) {
    res = c_rest_parser_execute(&ctx, valid_req, strlen(valid_req), &parsed);
    failed += (res != 0 || parsed != strlen(valid_req));

    failed += (!method_called || !url_called || !complete_called);

    /* Test malformed request */
    res = c_rest_parser_execute(&ctx, malformed_req, strlen(malformed_req),
                                &parsed);
    failed += (res != 0 || parsed != 0);
  }

  /* Test malformed request hitting the error branch from on_error */
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
    c_rest_parser_get_basic_vtable(&vtable2);
    c_rest_parser_init(&ctx2, vtable2, &callbacks2, (void *)1);
    res = c_rest_parser_execute(&ctx2, malformed_req, strlen(malformed_req),
                                &parsed);
    failed += (res != C_REST_ERROR_GENERIC);
    c_rest_parser_destroy(&ctx2);

    c_rest_parser_init(&ctx2, vtable2, &callbacks2, NULL);
    res = c_rest_parser_execute(&ctx2, malformed_req, strlen(malformed_req),
                                &parsed);
    failed += (res != 0);

    /* Test executing when state already has error */
    res = c_rest_parser_execute(&ctx2, "data", 4, &parsed);
    failed += (res != C_REST_ERROR_GENERIC);

    /* Reset error, set invalid state */
    ((struct basic_parser_state *)ctx2.internal_state)->has_error = 0;
    ((struct basic_parser_state *)ctx2.internal_state)->state = 999;
    res = c_rest_parser_execute(&ctx2, "data", 4, &parsed);
    failed += (res != C_REST_ERROR_GENERIC);

    c_rest_parser_destroy(&ctx2);
  }

  /* Test malloc fail during method parsing (case 0) */
  {
    char huge_method[512];
    c_rest_parser_context ctx3;
    extern int g_fail_realloc_at;
    extern void *(*g_crf_realloc_hook)(void *, size_t);
    memset(huge_method, 'M', sizeof(huge_method));
    huge_method[sizeof(huge_method) - 1] = '\0';
    c_rest_parser_init(&ctx3, vtable, &callbacks, NULL);
    g_fail_realloc_at = 1; /* 1st realloc in append_buf */
    g_crf_realloc_hook = fail_realloc_n;
    res = c_rest_parser_execute(&ctx3, huge_method, sizeof(huge_method) - 1,
                                &parsed);
    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
    c_rest_parser_destroy(&ctx3);
  }

  {
    c_rest_parser_context ctx3;
    const struct c_rest_parser_vtable *vtable3 = NULL;
    c_rest_parser_get_basic_vtable(&vtable3);
    res = c_rest_parser_init(&ctx3, vtable3, &callbacks, NULL);
    failed += (res != C_REST_OK);
    if (res == C_REST_OK) {
      const char *req_nl = "\r\n\r\nGET / HTTP/1.1\r\n\r\n";
      vtable3->execute(&ctx3, req_nl, strlen(req_nl), &parsed);
      vtable3->destroy(&ctx3);
    }
  }

  (void)!c_rest_parser_destroy(&ctx);
  test_parser_other_headers();
  test_parser_no_callbacks();
  test_parser_long_header();
  test_parser_content_length_zero();
  test_parser_connection_close_whitespace();
  test_parser_chunked_split();
  test_parser_is_complete();
  test_parser_chunked();
  test_parser_content_length();
  test_parser_errors();

  msgs[0] = "test_parser passed\n";
  msgs[1] = "test_parser failed\n";
  printf("%s", msgs[failed != 0]);
  return failed;
}
