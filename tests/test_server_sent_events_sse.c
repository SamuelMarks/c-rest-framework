/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_sse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ASSERT_EQ(expected, actual)                                            \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      printf("%s:%d: Assertion failed: expected %d, got %d\n", __FILE__,       \
             __LINE__, (int)(expected), (int)(actual));                        \
      return 1;                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_STR_EQ(expected, actual)                                        \
  do {                                                                         \
    if ((const void *)(expected) == NULL && (const void *)(actual) == NULL) break; \
    if ((const void *)(actual) == NULL) {                                      \
      printf("%s:%d: Assertion failed: expected '%s', got NULL\n", __FILE__,   \
             __LINE__, ((const void *)(expected) != NULL) ? (expected) : "NULL");                      \
      return 1;                                                                \
    }                                                                          \
    if ((const void *)(expected) == NULL) {                                    \
      printf("%s:%d: Assertion failed: expected NULL, got '%s'\n", __FILE__,   \
             __LINE__, (actual));                                              \
      return 1;                                                                \
    }                                                                          \
    if (strcmp((expected), (actual)) != 0) {                                   \
      printf("%s:%d: Assertion failed: expected '%s', got '%s'\n", __FILE__,   \
             __LINE__, (expected), (actual));                                  \
      return 1;                                                                \
    }                                                                          \
  } while (0)

#define ASSERT(condition)                                                      \
  do {                                                                         \
    if (!(condition)) {                                                        \
      printf("%s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #condition); \
      return 1;                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_NULL(actual)                                                    \
  do {                                                                         \
    if ((actual) != NULL) {                                                    \
      printf("%s:%d: Assertion failed: expected NULL\n", __FILE__, __LINE__);  \
      return 1;                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_NOT_NULL(actual)                                                \
  do {                                                                         \
    if ((actual) == NULL) {                                                    \
      printf("%s:%d: Assertion failed: expected NOT NULL\n", __FILE__,         \
             __LINE__);                                                        \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static int test_sse_event_init_destroy(void) {
  struct c_rest_sse_event ev;
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_init(&ev));
  ASSERT_NULL(ev.id);
  ASSERT_NULL(ev.event);
  ASSERT_NULL(ev.data);
  ASSERT_EQ(-1, ev.retry);

  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  return 0;
}

static int test_sse_event_clone(void) {
  struct c_rest_sse_event src;
  struct c_rest_sse_event dest;
  char *id = "123";
  char *event = "message";
  char *data = "hello world";

  (void)!c_rest_sse_event_init(&src);
  (void)!c_rest_sse_event_init(&dest);

  src.id = CRF_STRDUP(id);
  src.event = CRF_STRDUP(event);
  src.data = CRF_STRDUP(data);
  src.retry = 1000;

  ASSERT_EQ(C_REST_OK, c_rest_sse_event_clone(&src, &dest));

  ASSERT_STR_EQ(src.id, dest.id);
  ASSERT_STR_EQ(src.event, dest.event);
  ASSERT_STR_EQ(src.data, dest.data);
  ASSERT_EQ(src.retry, dest.retry);

  ASSERT(src.id != dest.id);

  CRF_FREE(src.id);
  CRF_FREE(src.event);
  CRF_FREE(src.data);

  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&dest));
  return 0;
}

static int test_sse_serialize(void) {
  struct c_rest_sse_event ev;
  char *out = NULL;
  size_t len = 0;

  (void)!c_rest_sse_event_init(&ev);
  ev.id = CRF_STRDUP("42");
  ev.event = CRF_STRDUP("ping");
  ev.data = CRF_STRDUP("line1\nline2");
  ev.retry = 3000;

  ASSERT_EQ(C_REST_OK, c_rest_sse_serialize(&ev, &out, &len));
  ASSERT(out != NULL);

  ASSERT_STR_EQ(
      "id: 42\nevent: ping\nretry: 3000\ndata: line1\ndata: line2\n\n", out);

  CRF_FREE(ev.id);
  CRF_FREE(ev.event);
  CRF_FREE(ev.data);
  CRF_FREE(out);
  return 0;
}

static int test_sse_parse_complete(void) {
  struct c_rest_sse_context *ctx = NULL;
  struct c_rest_sse_event ev;
  const char *payload = "id: 1\nevent: custom\nretry: 500\ndata: test data\n\n";

  ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&ctx));
  (void)!c_rest_sse_event_init(&ev);

  ASSERT_EQ(C_REST_OK,
            c_rest_sse_parse(ctx, payload, strlen(payload), &ev));

  ASSERT_STR_EQ("1", ev.id);
  ASSERT_STR_EQ("custom", ev.event);
  ASSERT_EQ(500, ev.retry);
  ASSERT_STR_EQ("test data", ev.data);

  (void)!c_rest_sse_event_destroy(&ev);
  (void)!c_rest_sse_context_destroy(ctx);
  return 0;
}

static int test_sse_parse_fragmented(void) {
  struct c_rest_sse_context *ctx = NULL;
  struct c_rest_sse_event ev;

  ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&ctx));
  (void)!c_rest_sse_event_init(&ev);

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_sse_parse(ctx, "id: 2\nev", 8, &ev));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_sse_parse(ctx, "ent: partial\ndat", 16, &ev));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_sse_parse(ctx, "a: hello", 8, &ev));
  ASSERT_EQ(C_REST_OK, c_rest_sse_parse(ctx, "\n\n", 2, &ev));

  ASSERT_STR_EQ("2", ev.id);
  ASSERT_STR_EQ("partial", ev.event);
  ASSERT_STR_EQ("hello", ev.data);

  (void)!c_rest_sse_event_destroy(&ev);
  (void)!c_rest_sse_context_destroy(ctx);
  return 0;
}

#include "c_rest_response.h"
/* clang-format on */

static int test_sse_wrappers(void) {
  struct c_rest_response res;
  struct c_rest_sse_event ev;
  c_rest_error_t ret;

  memset(&res, 0, sizeof(res));

  /* Test init response without context (will succeed but not write) */
  ret = c_rest_sse_init_response(&res);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(1, res.headers_sent);
  ASSERT_EQ(200, res.status_code);

  /* Test send event without context (will fail at write_chunk context check) */
  (void)!c_rest_sse_event_init(&ev);
  ev.event = "test";
  ret = c_rest_sse_send_event(&res, &ev);
  ASSERT_EQ(1, ret); /* Should fail because ctx is NULL */

  ret = c_rest_sse_send_keepalive(&res);
  ASSERT_EQ(1, ret); /* Should fail because ctx is NULL */

  (void)!c_rest_response_cleanup(&res);
  return 0;
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

static void test_coverage(void) {
  struct c_rest_sse_event ev;
  struct c_rest_sse_event dest;
  struct c_rest_sse_context *ctx = NULL;
  char *out;
  size_t olen;
  int i;
  extern int g_fail_malloc_at;
  extern int g_fail_realloc_at;
  (void)out;
  (void)olen;
  memset(&ev, 0, sizeof(ev));

  /* Hit NULL branches */
  c_rest_sse_event_init(NULL);
  c_rest_sse_event_destroy(NULL);
  c_rest_sse_event_clone(NULL, &dest);
  c_rest_sse_event_clone(&ev, NULL);
  out = NULL;
  c_rest_sse_serialize(NULL, &out, &olen);
  if (out)
    CRF_FREE(out);
  out = NULL;
  c_rest_sse_serialize(&ev, NULL, &olen);
  if (out)
    CRF_FREE(out);
  out = NULL;
  c_rest_sse_serialize(&ev, &out, NULL);
  if (out)
    CRF_FREE(out);

  {

    for (i = 1; i <= 100; i++) {
      out = NULL;
      g_crf_realloc_hook = fail_realloc_n;
      g_fail_realloc_at = i;
      c_rest_sse_serialize(&ev, &out, &olen);
      g_crf_realloc_hook = NULL;
      g_fail_realloc_at = -1;
      if (out)
        CRF_FREE(out);
    }

    for (i = 1; i <= 100; i++) {
      out = NULL;
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;
      c_rest_sse_serialize(&ev, &out, &olen);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
      if (out)
        CRF_FREE(out);
    }

    ev.id = "my-id-that-is-somewhat-long-but-maybe-not-long-enough-on-its-own";
    ev.event = "my-event-that-is-also-quite-long-to-help-fill-the-buffer-up";
    ev.data = "my-data-that-is-very-very-long\nand-has-multiple-lines\nso-it-"
              "covers-the-while-loop-newline-logic\nand-will-definitely-cause-"
              "a-reallocation-because-it-exceeds-the-initial-128-byte-buffer-"
              "size-allocated-by-the-string-init-function-and-thus-covers-"
              "realloc-branches";
    ev.retry = 100;

    for (i = 1; i <= 100; i++) {
      out = NULL;
      g_crf_realloc_hook = fail_realloc_n;
      g_fail_realloc_at = i;
      c_rest_sse_serialize(&ev, &out, &olen);
      g_crf_realloc_hook = NULL;
      g_fail_realloc_at = -1;
      if (out)
        CRF_FREE(out);
    }

    for (i = 1; i <= 100; i++) {
      out = NULL;
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;
      c_rest_sse_serialize(&ev, &out, &olen);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
      if (out)
        CRF_FREE(out);
    }

    for (i = 0; i <= 30; i++) {
      out = NULL;
      g_mock_sse_append_fail = i;
      c_rest_sse_serialize(&ev, &out, &olen);
      g_mock_sse_append_fail = -1;
      if (out)
        CRF_FREE(out);
    }
    {
      out = NULL;
      g_mock_sse_append_fail = -2;
      c_rest_sse_serialize(&ev, &out, &olen);
      g_mock_sse_append_fail = -1;
      if (out)
        CRF_FREE(out);
    }
  }

  c_rest_sse_context_init(NULL);
  c_rest_sse_context_destroy(NULL);

  /* Trigger error inside c_rest_sse_context_init */
  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 1;
  c_rest_sse_context_init(&ctx);
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = -1;

  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 2;
  c_rest_sse_context_init(&ctx);
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = -1;

  /* Try to make c_rest_sse_event_init fail when it mallocs */
  /* Since it doesn't malloc initially, only if string is added, maybe it fails
   * on string alloc? */
  {
    struct c_rest_sse_context *tmp_ctx = NULL;
#ifdef C_REST_TESTING_MALLOC_HOOK
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 2; /* c_rest_sse_event_init needs a malloc? Actually
    c_rest_sse_event_init doesnt malloc if nothing is set. Wait, what does
    c_rest_sse_event_init do? Ah, we need to fail
    `append_to_string(&ctx->current_event.data, "", 0)` inside parse.
    */
    c_rest_sse_context_init(&tmp_ctx);
    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = -1;
#endif
  }

  c_rest_sse_parse(NULL, "a", 1, &ev);
  c_rest_sse_parse(ctx, NULL, 1, &ev);
  c_rest_sse_parse(ctx, "a", 1, NULL);
  c_rest_sse_parse(ctx, "a", 0, &ev);
  c_rest_sse_init_response(NULL);
  c_rest_sse_send_event(NULL, &ev);
  {
    struct c_rest_sse_context *tmp_ctx = NULL;

    c_rest_sse_context_init(&tmp_ctx);
    c_rest_sse_parse(tmp_ctx, "id: 1\nevent: e\ndata: d\n\n", 25, &ev);

    /* Failure on reallocation of parse buffer */
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = 1;
    c_rest_sse_parse(
        tmp_ctx,
        "data: "
        "0123456789012345678901234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345678901234567890123456789"
        "012345678901234567890123456789012345678901234567890123456789\n\n",
        strlen("ta: "
               "012345678901234567890123456789012345678901234567890123456789012"
               "345678901234567890123456789012345678901234567890123456789012345"
               "678901234567890123456789012345678901234567890123456789012345678"
               "901234567890123456789012345678901234567890123456789012345678901"
               "234567890123456789\n\n"),
        &ev);
    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = -1;

    c_rest_sse_context_destroy(tmp_ctx);

    c_rest_sse_context_init(&tmp_ctx);
    c_rest_sse_parse(tmp_ctx, "da", 2, &ev);

    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 1;
    c_rest_sse_parse(
        tmp_ctx,
        "ta: "
        "0123456789012345678901234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345678901234567890123456789"
        "012345678901234567890123456789012345678901234567890123456789\n\n",
        strlen("ta: "
               "012345678901234567890123456789012345678901234567890123456789012"
               "345678901234567890123456789012345678901234567890123456789012345"
               "678901234567890123456789012345678901234567890123456789012345678"
               "901234567890123456789012345678901234567890123456789012345678901"
               "234567890123456789\n\n"),
        &ev);
    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = -1;
    c_rest_sse_context_destroy(tmp_ctx);
  }

  {
    for (i = 1; i <= 20; i++) {
      c_rest_sse_context_init(&ctx);
      g_crf_realloc_hook = fail_realloc_n;
      g_fail_realloc_at = i;
      c_rest_sse_parse(ctx, "id: 1\nevent: e\ndata: d\n\n", 25, &ev);
      g_crf_realloc_hook = NULL;
      g_fail_realloc_at = -1;
      c_rest_sse_context_destroy(ctx);
      c_rest_sse_event_destroy(&ev);
    }

    for (i = 1; i <= 20; i++) {
      c_rest_sse_context_init(&ctx);
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;
      c_rest_sse_parse(ctx, "id: 1\nevent: e\ndata: d\n\n", 25, &ev);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
      c_rest_sse_context_destroy(ctx);
      c_rest_sse_event_destroy(&ev);
    }

    for (i = 1; i <= 5; i++) {
      c_rest_sse_context_init(&ctx);
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;

      c_rest_sse_parse(ctx, "data\n", 5, &ev);
      c_rest_sse_parse(ctx, "retry: 0123456789012345678901234567890123456789\n",
                       48, &ev);

      c_rest_sse_parse(ctx, ":comment\n", 9, &ev);

      /* Failure on empty data string append */
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = 1;
      c_rest_sse_parse(ctx, "data\n", 5, &ev);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;

      c_rest_sse_parse(ctx, "id: 1\nevent: e\ndata: d\n\n", 25, &ev);

      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
      c_rest_sse_context_destroy(ctx);
      c_rest_sse_event_destroy(&ev);
    }

    /* Test response init failure */
    {
      struct c_rest_response res;
      memset(&res, 0, sizeof(res));

      /* c_rest_response_set_status */

      /* c_rest_response_set_header failures */
      for (i = 1; i <= 10; i++) {
        memset(&res, 0, sizeof(res));
        g_crf_malloc_hook = fail_malloc_n;
        g_fail_malloc_at = i;
        c_rest_sse_init_response(&res);
        g_crf_malloc_hook = NULL;
        g_fail_malloc_at = -1;
        c_rest_response_cleanup(&res);
      }
    }
  }

  /* c_rest_sse_send_event((void*)1, NULL); */
  c_rest_sse_send_keepalive(NULL);

  for (i = 1; i <= 100; i++) {
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    memset(&ev, 0, sizeof(ev));
    memset(&dest, 0, sizeof(dest));
    ev.id = "1";
    ev.event = "e";
    ev.data = "d";
    /* c_rest_sse_event_clone(&ev, &dest);
    c_rest_sse_event_destroy(&dest); */

    out = NULL;
    /* out = NULL;
    c_rest_sse_serialize(&ev, &out, &olen);
    if (out) CRF_FREE(out); */

    ev.id = NULL;
    ev.event = NULL;
    ev.data = NULL;

    if (c_rest_sse_context_init(&ctx) == C_REST_OK) {
      c_rest_sse_parse(ctx,
                       "retry: 1000\nid: 1\nid: 2\nevent: e\nevent: f\ndata: "
                       "d\ndata: d2\ndata\ndata\nevent\nevent\n\n"
                       "event: e\n\n"
                       "id: 1\n\n"
                       "retry: 100\n\n",
                       112, &ev);
      c_rest_sse_event_destroy(&ev);
      c_rest_sse_context_destroy(ctx);
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
  }

  for (i = 1; i <= 100; i++) {
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    ev.id = NULL;
    ev.event = NULL;
    ev.data = NULL;

    if (c_rest_sse_context_init(&ctx) == C_REST_OK) {
      c_rest_sse_parse(ctx,
                       "retry: 1000\nid: 1\nid: 2\nevent: e\nevent: f\ndata: "
                       "d\ndata: d2\ndata\ndata\nevent\nevent\n\n"
                       "event: e\n\n"
                       "id: 1\n\n"
                       "retry: 100\n\n",
                       112, &ev);
      c_rest_sse_event_destroy(&ev);
      c_rest_sse_context_destroy(ctx);
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }

  {
    g_mock_sse_append_fail = -3;
    if (c_rest_sse_context_init(&ctx) == C_REST_OK) {
      c_rest_sse_parse(ctx, "data: d\n\n", 9, &ev);
      c_rest_sse_event_destroy(&ev);
      c_rest_sse_context_destroy(ctx);
    }
    g_mock_sse_append_fail = -4;
    if (c_rest_sse_context_init(&ctx) == C_REST_OK) {
      c_rest_sse_parse(ctx, "data: d\n\n", 9, &ev);
      c_rest_sse_event_destroy(&ev);
      c_rest_sse_context_destroy(ctx);
    }
    g_mock_sse_append_fail = -1;
  }

  for (i = 1; i <= 100; i++) {
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    memset(&ev, 0, sizeof(ev));
    memset(&dest, 0, sizeof(dest));
    ev.id = "1";
    ev.event = "e";
    ev.data = "d\nd2";
    ev.retry = 1000;
    c_rest_sse_event_clone(&ev, &dest);
    c_rest_sse_event_destroy(&dest);

    out = NULL;
    c_rest_sse_serialize(&ev, &out, &olen);
    if (out)
      CRF_FREE(out);

    ev.id = NULL;
    ev.event = NULL;
    ev.data = NULL;

    ev.id = NULL;
    ev.event = NULL;
    ev.data = NULL;

    if (c_rest_sse_context_init(&ctx) == C_REST_OK) {
      c_rest_sse_parse(ctx,
                       "retry: 1000\nid: 1\nid: 2\nevent: e\nevent: f\ndata: "
                       "d\ndata: d2\ndata\ndata\nevent\nevent\n\n"
                       "event: e\n\n"
                       "id: 1\n\n"
                       "retry: 100\n\n",
                       112, &ev);
      c_rest_sse_event_destroy(&ev);
      c_rest_sse_context_destroy(ctx);
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
  }

  /* Additional parsing paths */
  c_rest_sse_context_init(&ctx);
  memset(&ev, 0, sizeof(ev));
  c_rest_sse_parse(ctx,
                   "retry: 1000\nid: 1\nid: 2\nevent: e\nevent: f\ndata: "
                   "d\ndata: d2\ndata\ndata\nevent\nevent\n\n",
                   83, &ev);
  c_rest_sse_event_destroy(&ev);

  memset(&ev, 0, sizeof(ev));
  c_rest_sse_parse(ctx, "data: d\n\n", 9, &ev);
  c_rest_sse_event_destroy(&ev);
  memset(&ev, 0, sizeof(ev));
  c_rest_sse_parse(ctx, "event: e\n\n", 10, &ev);
  c_rest_sse_event_destroy(&ev);
  memset(&ev, 0, sizeof(ev));
  c_rest_sse_parse(ctx, "id: 1\n\n", 7, &ev);
  c_rest_sse_event_destroy(&ev);
  memset(&ev, 0, sizeof(ev));
  c_rest_sse_parse(ctx, "retry: 10\n\n", 11, &ev);
  c_rest_sse_event_destroy(&ev);

  memset(&ev, 0, sizeof(ev));
  c_rest_sse_parse(ctx, ":comment\n\n", 11, &ev);
  c_rest_sse_parse(ctx, "data\n\n", 7, &ev);
  c_rest_sse_parse(ctx, "id\n\n", 5, &ev);
  c_rest_sse_parse(ctx, "id\r\n\n", 6, &ev);
  c_rest_sse_parse(ctx, "id:\n\n", 5, &ev);
  c_rest_sse_parse(ctx, "retry\n\n", 7, &ev);
  c_rest_sse_parse(ctx, "event\n\n", 7, &ev);
  c_rest_sse_parse(ctx, "invalid: foo\n\n", 14, &ev);

  /* Additional branch coverage parses */
  c_rest_sse_parse(ctx, "data:d\n\n", 8, &ev);  /* No space after colon */
  c_rest_sse_parse(ctx, "test: x\n\n", 9, &ev); /* len 4 but not data */
  c_rest_sse_parse(ctx, "xx: 1\n\n", 7, &ev);   /* len 2 but not id */
  c_rest_sse_parse(ctx, "event: a\nevent\n\n", 16,
                   &ev); /* event without colon but event is already set */
  c_rest_sse_parse(ctx, "test\n\n", 6,
                   &ev); /* len 4 without colon but not data */

  {
    struct c_rest_response res;
    memset(&res, 0, sizeof(res));
    c_rest_sse_init_response(&res);
    c_rest_sse_send_event(NULL, &ev);  /* res is NULL */
    c_rest_sse_send_event(&res, NULL); /* ev is NULL */
    c_rest_response_cleanup(&res);
  }

  {
    memset(&ev, 0, sizeof(ev));
    ev.id = "1";
    ev.event = "e";
    ev.data = "d";

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 1;
    c_rest_sse_event_clone(&ev, &dest);
    g_crf_malloc_hook = NULL;

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 2;
    c_rest_sse_event_clone(&ev, &dest);
    g_crf_malloc_hook = NULL;

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 3;
    c_rest_sse_event_clone(&ev, &dest);
    g_crf_malloc_hook = NULL;

    c_rest_sse_event_destroy(&dest);
    ev.id = NULL;
    ev.event = NULL;
    ev.data = NULL;
  }

  /* c_rest_sse_event_destroy(&ev); */
  c_rest_sse_context_destroy(ctx);
}

static int test_sse_event_clone_nulls(void) {
  struct c_rest_sse_event src, dest;
  memset(&src, 0, sizeof(src));
  memset(&dest, 0, sizeof(dest));

  /* All NULLs */
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_clone(&src, &dest));
  c_rest_sse_event_destroy(&dest);

  /* Some NULLs */
  src.id = "test";
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_clone(&src, &dest));
  c_rest_sse_event_destroy(&dest);

  src.id = NULL;
  src.event = "test";
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_clone(&src, &dest));
  c_rest_sse_event_destroy(&dest);

  src.event = NULL;
  src.data = "test";
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_clone(&src, &dest));
  c_rest_sse_event_destroy(&dest);

  return 0;
}

int test_server_sent_events_sse(void) {
  int res = 0;
  test_coverage();
  res |= test_sse_event_clone_nulls();
  res |= test_sse_event_init_destroy();
  res |= test_sse_event_clone();
  res |= test_sse_serialize();
  res |= test_sse_parse_complete();
  res |= test_sse_parse_fragmented();
  res |= test_sse_wrappers();

  if (res == 0) {
    printf("test_server_sent_events_sse passed.\n");
  } else {
    printf("test_server_sent_events_sse failed.\n");
  }
  return res;
}
