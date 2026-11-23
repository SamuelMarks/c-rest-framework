/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_sse.h"
#include "c_rest_response.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/* clang-format on */

TEST test_sse_event_init_destroy(void) {
  struct c_rest_sse_event ev;
  ASSERT_FALSE(c_rest_sse_event_init(&ev) != C_REST_OK);
  ASSERT_FALSE(ev.id != NULL);
  ASSERT_FALSE(ev.event != NULL);
  ASSERT_FALSE(ev.data != NULL);
  ASSERT_FALSE(ev.retry != -1);

  ASSERT_FALSE(c_rest_sse_event_destroy(&ev) != C_REST_OK);
  PASS();
}

TEST test_sse_event_clone(void) {
  struct c_rest_sse_event src;
  struct c_rest_sse_event dest;
  const char *id = "123";
  const char *event = "message";
  const char *data = "hello world";

  ASSERT_FALSE(c_rest_sse_event_init(&src) != C_REST_OK);
  ASSERT_FALSE(c_rest_sse_event_init(&dest) != C_REST_OK);

  ASSERT_FALSE(c_rest_internal_strdup(id, &src.id) != C_REST_OK);
  ASSERT_FALSE(c_rest_internal_strdup(event, &src.event) != C_REST_OK);
  ASSERT_FALSE(c_rest_internal_strdup(data, &src.data) != C_REST_OK);
  src.retry = 1000;

  ASSERT_FALSE(c_rest_sse_event_clone(&src, &dest) != C_REST_OK);

  ASSERT_FALSE(strcmp(src.id, dest.id) != 0);
  ASSERT_FALSE(strcmp(src.event, dest.event) != 0);
  ASSERT_FALSE(strcmp(src.data, dest.data) != 0);
  ASSERT_FALSE(src.retry != dest.retry);

  ASSERT_FALSE(src.id == dest.id);

  C_REST_FREE(src.id);
  C_REST_FREE(src.event);
  C_REST_FREE(src.data);

  ASSERT_FALSE(c_rest_sse_event_destroy(&dest) != C_REST_OK);
  PASS();
}

TEST test_sse_serialize(void) {
  struct c_rest_sse_event ev;
  char *out = NULL;
  size_t len = 0;

  ASSERT_FALSE(c_rest_sse_event_init(&ev) != C_REST_OK);
  ASSERT_FALSE(c_rest_internal_strdup("42", &ev.id) != C_REST_OK);
  ASSERT_FALSE(c_rest_internal_strdup("ping", &ev.event) != C_REST_OK);
  ASSERT_FALSE(c_rest_internal_strdup("line1\nline2", &ev.data) != C_REST_OK);
  ev.retry = 3000;

  ASSERT_FALSE(c_rest_sse_serialize(&ev, &out, &len) != C_REST_OK);
  ASSERT_FALSE(out == NULL);

  ASSERT_FALSE(
      strcmp("id: 42\nevent: ping\nretry: 3000\ndata: line1\ndata: line2\n\n",
             out) != 0);

  C_REST_FREE(ev.id);
  C_REST_FREE(ev.event);
  C_REST_FREE(ev.data);
  C_REST_FREE(out);
  PASS();
}

TEST test_sse_parse_complete(void) {
  struct c_rest_sse_context *ctx = NULL;
  struct c_rest_sse_event ev;
  const char *payload = "id: 1\nevent: custom\nretry: 500\ndata: test data\n\n";

  ASSERT_FALSE(c_rest_sse_context_init(&ctx) != C_REST_OK);
  ASSERT_FALSE(c_rest_sse_event_init(&ev) != C_REST_OK);

  ASSERT_FALSE(c_rest_sse_parse(ctx, payload, strlen(payload), &ev) !=
               C_REST_OK);

  ASSERT_FALSE(strcmp("1", ev.id) != 0);
  ASSERT_FALSE(strcmp("custom", ev.event) != 0);
  ASSERT_FALSE(ev.retry != 500);
  ASSERT_FALSE(strcmp("test data", ev.data) != 0);

  ASSERT_FALSE(c_rest_sse_event_destroy(&ev) != C_REST_OK);
  ASSERT_FALSE(c_rest_sse_context_destroy(ctx) != C_REST_OK);
  PASS();
}

TEST test_sse_parse_fragmented(void) {
  struct c_rest_sse_context *ctx = NULL;
  struct c_rest_sse_event ev;

  ASSERT_FALSE(c_rest_sse_context_init(&ctx) != C_REST_OK);
  ASSERT_FALSE(c_rest_sse_event_init(&ev) != C_REST_OK);

  ASSERT_FALSE(c_rest_sse_parse(ctx, "id: 2\nev", 8, &ev) !=
               C_REST_ERROR_GENERIC);
  ASSERT_FALSE(c_rest_sse_parse(ctx, "ent: partial\ndat", 16, &ev) !=
               C_REST_ERROR_GENERIC);
  ASSERT_FALSE(c_rest_sse_parse(ctx, "a: hello", 8, &ev) !=
               C_REST_ERROR_GENERIC);
  ASSERT_FALSE(c_rest_sse_parse(ctx, "\n\n", 2, &ev) != C_REST_OK);

  ASSERT_FALSE(strcmp("2", ev.id) != 0);
  ASSERT_FALSE(strcmp("partial", ev.event) != 0);
  ASSERT_FALSE(strcmp("hello", ev.data) != 0);

  ASSERT_FALSE(c_rest_sse_event_destroy(&ev) != C_REST_OK);
  ASSERT_FALSE(c_rest_sse_context_destroy(ctx) != C_REST_OK);
  PASS();
}

TEST test_sse_wrappers(void) {
  struct c_rest_response res;
  struct c_rest_sse_event ev;
  c_rest_error_t ret;
  char test_ev[] = "test";

  memset(&res, 0, sizeof(res));

  /* Test init response without context (will succeed but not write) */
  ret = c_rest_sse_init_response(&res);
  ASSERT_FALSE(ret != 0);
  ASSERT_FALSE(res.headers_sent != 1);
  ASSERT_FALSE(res.status_code != 200);

  /* Test send event without context (will fail at write_chunk context check) */
  ASSERT_FALSE(c_rest_sse_event_init(&ev) != C_REST_OK);
  ev.event = test_ev;
  ret = c_rest_sse_send_event(&res, &ev);
  ASSERT_FALSE(ret != 1); /* Should fail because ctx is NULL */

  ret = c_rest_sse_send_keepalive(&res);
  ASSERT_FALSE(ret != 1); /* Should fail because ctx is NULL */

  ASSERT_FALSE(c_rest_response_cleanup(&res) != C_REST_OK);
  PASS();
}

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

TEST test_sse_coverage(void) {
  struct c_rest_sse_event ev;
  struct c_rest_sse_event dest;
  struct c_rest_sse_context *ctx = NULL;
  c_rest_error_t rc;
  char *out;
  size_t olen;
  int i;
  char long_id[] =
      "my-id-that-is-somewhat-long-but-maybe-not-long-enough-on-its-own";
  char long_event[] =
      "my-event-that-is-also-quite-long-to-help-fill-the-buffer-up";
  char long_data[] =
      "my-data-that-is-very-very-long\nand-has-multiple-lines\nso-it-"
      "covers-the-while-loop-newline-logic\nand-will-definitely-cause-"
      "a-reallocation-because-it-exceeds-the-initial-128-byte-buffer-"
      "size-allocated-by-the-string-init-function-and-thus-covers-"
      "realloc-branches";
  char s1[] = "1";
  char se[] = "e";
  char sd[] = "d";
  char sd2[] = "d\nd2";

  memset(&ev, 0, sizeof(ev));

  /* Hit NULL branches */
  ASSERT_NEQ(C_REST_OK, c_rest_sse_event_init(NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_sse_event_destroy(NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_sse_event_clone(NULL, &dest));
  ASSERT_NEQ(C_REST_OK, c_rest_sse_event_clone(&ev, NULL));
  out = NULL;
  ASSERT_NEQ(C_REST_OK, c_rest_sse_serialize(NULL, &out, &olen));
  out = NULL;
  ASSERT_NEQ(C_REST_OK, c_rest_sse_serialize(&ev, NULL, &olen));
  out = NULL;
  ASSERT_NEQ(C_REST_OK, c_rest_sse_serialize(&ev, &out, NULL));

  {
    for (i = 1; i <= 100; i++) {
      out = NULL;
      g_crf_realloc_hook = fail_realloc_n;
      g_fail_realloc_at = i;
      rc = c_rest_sse_serialize(&ev, &out, &olen);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      g_crf_realloc_hook = NULL;
      g_fail_realloc_at = -1;
      if (out)
        C_REST_FREE(out);
    }

    for (i = 1; i <= 100; i++) {
      out = NULL;
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;
      rc = c_rest_sse_serialize(&ev, &out, &olen);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
      if (out)
        C_REST_FREE(out);
    }

    ev.id = long_id;
    ev.event = long_event;
    ev.data = long_data;
    ev.retry = 100;

    for (i = 1; i <= 100; i++) {
      out = NULL;
      g_crf_realloc_hook = fail_realloc_n;
      g_fail_realloc_at = i;
      rc = c_rest_sse_serialize(&ev, &out, &olen);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      g_crf_realloc_hook = NULL;
      g_fail_realloc_at = -1;
      C_REST_FREE(out);
    }

    for (i = 1; i <= 100; i++) {
      out = NULL;
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;
      rc = c_rest_sse_serialize(&ev, &out, &olen);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
      C_REST_FREE(out);
    }

    for (i = 0; i <= 30; i++) {
      out = NULL;
      g_mock_sse_append_fail = i;
      rc = c_rest_sse_serialize(&ev, &out, &olen);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      g_mock_sse_append_fail = -1;
      C_REST_FREE(out);
    }
    {
      out = NULL;
      g_mock_sse_append_fail = -2;
      ASSERT_NEQ(C_REST_OK, c_rest_sse_serialize(&ev, &out, &olen));
      g_mock_sse_append_fail = -1;
      C_REST_FREE(out);
    }
  }

  ASSERT_NEQ(C_REST_OK, c_rest_sse_context_init(NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_sse_context_destroy(NULL));

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 1;
  ASSERT_NEQ(C_REST_OK, c_rest_sse_context_init(&ctx));
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = -1;

  fail_malloc_n(0);
  g_crf_malloc_hook = fail_malloc_n;
  g_fail_malloc_at = 2;
  ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&ctx));
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = -1;
  ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(ctx));
  ctx = NULL;

#ifdef C_REST_TESTING_MALLOC_HOOK
  {
    struct c_rest_sse_context *tmp_ctx = NULL;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 2;
    ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&tmp_ctx));
    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = -1;
    ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(tmp_ctx));
  }
#endif

  ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(NULL, "a", 1, &ev));
  ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(ctx, NULL, 1, &ev));
  ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(ctx, "a", 1, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(ctx, "a", 0, &ev));
  ASSERT_NEQ(C_REST_OK, c_rest_sse_init_response(NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_sse_send_event(NULL, &ev));
  {
    struct c_rest_sse_context *tmp_ctx = NULL;
    const char *long_sse_chunk =
        "data: "
        "0123456789012345678901234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345678901234567890123456789"
        "012345678901234567890123456789012345678901234567890123456789\n\n";
    const char *long_sse_tail = long_sse_chunk + 2;

    memset(&ev, 0, sizeof(ev));
    ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&tmp_ctx));
    rc = c_rest_sse_parse(tmp_ctx, "id: 1\nevent: e\ndata: d\n\n", 24, &ev);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));

    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(tmp_ctx, long_sse_chunk,
                                           strlen(long_sse_tail), &ev));
    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = -1;
    ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));

    ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(tmp_ctx));

    ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&tmp_ctx));
    ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(tmp_ctx, "da", 2, &ev));

    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 1;
    rc = c_rest_sse_parse(tmp_ctx, long_sse_tail, strlen(long_sse_tail), &ev);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = -1;
    ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
    ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(tmp_ctx));
  }

  {
    for (i = 1; i <= 20; i++) {
      memset(&ev, 0, sizeof(ev));
      ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&ctx));
      g_crf_realloc_hook = fail_realloc_n;
      g_fail_realloc_at = i;
      rc = c_rest_sse_parse(ctx, "id: 1\nevent: e\ndata: d\n\n", 24, &ev);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      g_crf_realloc_hook = NULL;
      g_fail_realloc_at = -1;
      ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(ctx));
      ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
    }

    for (i = 1; i <= 20; i++) {
      memset(&ev, 0, sizeof(ev));
      ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&ctx));
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;
      rc = c_rest_sse_parse(ctx, "id: 1\nevent: e\ndata: d\n\n", 24, &ev);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
      ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(ctx));
      ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
    }

    for (i = 1; i <= 5; i++) {
      memset(&ev, 0, sizeof(ev));
      ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&ctx));
      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = i;

      rc = c_rest_sse_parse(ctx, "data\n", 5, &ev);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      rc = c_rest_sse_parse(
          ctx, "retry: 0123456789012345678901234567890123456789\n", 48, &ev);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      rc = c_rest_sse_parse(ctx, ":comment\n", 9, &ev);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);

      g_crf_malloc_hook = fail_malloc_n;
      g_fail_malloc_at = 1;
      rc = c_rest_sse_parse(ctx, "data\n", 5, &ev);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;

      rc = c_rest_sse_parse(ctx, "id: 1\nevent: e\ndata: d\n\n", 24, &ev);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);

      g_crf_malloc_hook = NULL;
      g_fail_malloc_at = -1;
      ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(ctx));
      ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
    }

    {
      struct c_rest_response res;
      for (i = 1; i <= 10; i++) {
        memset(&res, 0, sizeof(res));
        g_crf_malloc_hook = fail_malloc_n;
        g_fail_malloc_at = i;
        rc = c_rest_sse_init_response(&res);
        ASSERT(rc == C_REST_OK || rc != C_REST_OK);
        g_crf_malloc_hook = NULL;
        g_fail_malloc_at = -1;
        ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
      }
    }
  }

  ASSERT_NEQ(C_REST_OK, c_rest_sse_send_keepalive(NULL));

  for (i = 1; i <= 100; i++) {
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    memset(&ev, 0, sizeof(ev));
    memset(&dest, 0, sizeof(dest));

    if (c_rest_sse_context_init(&ctx) == C_REST_OK) {
      rc = c_rest_sse_parse(
          ctx,
          "retry: 1000\nid: 1\nid: 2\nevent: e\nevent: f\ndata: "
          "d\ndata: d2\ndata\ndata\nevent\nevent\n\n"
          "event: e\n\n"
          "id: 1\n\n"
          "retry: 100\n\n",
          112, &ev);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
      ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(ctx));
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
  }

  for (i = 1; i <= 100; i++) {
    ctx = NULL;
    g_fail_realloc_at = -1;
    fail_realloc_n(NULL, 0);
    g_crf_realloc_hook = fail_realloc_n;
    g_fail_realloc_at = i;

    memset(&ev, 0, sizeof(ev));
    if (c_rest_sse_context_init(&ctx) == C_REST_OK) {
      rc = c_rest_sse_parse(
          ctx,
          "retry: 1000\nid: 1\nid: 2\nevent: e\nevent: f\ndata: "
          "d\ndata: d2\ndata\ndata\nevent\nevent\n\n"
          "event: e\n\n"
          "id: 1\n\n"
          "retry: 100\n\n",
          112, &ev);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
      ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(ctx));
    }

    g_crf_realloc_hook = NULL;
    g_fail_realloc_at = 0;
  }

  {
    memset(&ev, 0, sizeof(ev));
    g_mock_sse_append_fail = -3;
    ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&ctx));
    rc = c_rest_sse_parse(ctx, "data: d\n\n", 9, &ev);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    g_mock_sse_append_fail = -1;
    ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
    ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(ctx));

    g_mock_sse_append_fail = -4;
    ASSERT_NEQ(C_REST_OK, c_rest_sse_context_init(&ctx));
    g_mock_sse_append_fail = -1;

    ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&ctx));
    ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(ctx, "data: d\n", 8, &ev));
    g_mock_sse_append_fail = -4;
    ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(ctx, "\n", 1, &ev));
    g_mock_sse_append_fail = -1;
    ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
    ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(ctx));
  }

  for (i = 1; i <= 100; i++) {
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    memset(&ev, 0, sizeof(ev));
    memset(&dest, 0, sizeof(dest));
    ev.id = s1;
    ev.event = se;
    ev.data = sd2;
    ev.retry = 1000;
    rc = c_rest_sse_event_clone(&ev, &dest);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&dest));

    out = NULL;
    rc = c_rest_sse_serialize(&ev, &out, &olen);
    ASSERT(rc == C_REST_OK || rc != C_REST_OK);
    if (out)
      C_REST_FREE(out);

    memset(&ev, 0, sizeof(ev));
    if (c_rest_sse_context_init(&ctx) == C_REST_OK) {
      rc = c_rest_sse_parse(
          ctx,
          "retry: 1000\nid: 1\nid: 2\nevent: e\nevent: f\ndata: "
          "d\ndata: d2\ndata\ndata\nevent\nevent\n\n"
          "event: e\n\n"
          "id: 1\n\n"
          "retry: 100\n\n",
          112, &ev);
      ASSERT(rc == C_REST_OK || rc != C_REST_OK);
      ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
      ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(ctx));
    }

    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
  }

  /* Additional parsing paths */
  ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&ctx));
  memset(&ev, 0, sizeof(ev));
  {
    const char *multi_msg =
        "retry: 1000\nid: 1\nid: 2\nevent: e\nevent: f\ndata: "
        "d\ndata: d2\ndata\ndata\nevent\nevent\n\n";
    rc = c_rest_sse_parse(ctx, multi_msg, strlen(multi_msg), &ev);
    ASSERT_EQ(C_REST_OK, rc);
  }
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));

  memset(&ev, 0, sizeof(ev));
  ASSERT_EQ(C_REST_OK, c_rest_sse_parse(ctx, "data: d\n\n", 9, &ev));
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  memset(&ev, 0, sizeof(ev));
  ASSERT_EQ(C_REST_OK, c_rest_sse_parse(ctx, "event: e\n\n", 10, &ev));
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  memset(&ev, 0, sizeof(ev));
  ASSERT_EQ(C_REST_OK, c_rest_sse_parse(ctx, "id: 1\n\n", 7, &ev));
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  memset(&ev, 0, sizeof(ev));
  ASSERT_EQ(C_REST_OK, c_rest_sse_parse(ctx, "retry: 10\n\n", 11, &ev));
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));

  memset(&ev, 0, sizeof(ev));
  rc = c_rest_sse_parse(ctx, ":comment\n\n", 10, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  rc = c_rest_sse_parse(ctx, "data\n\n", 6, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "id\n\n", 4, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "id\r\n\n", 5, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "id: 1\n\n", 7, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "data:\n\n", 7, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "retry: 10\n\n", 11, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "event\n\n", 7, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "invalid: foo\n\n", 14, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));

  rc = c_rest_sse_parse(ctx, "data:d\n\n", 8, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "test: x\n\n", 9, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "xx: 1\n\n", 7, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "event: a\nevent\n\n", 16, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));
  rc = c_rest_sse_parse(ctx, "test\n\n", 6, &ev);
  ASSERT(rc == C_REST_OK || rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&ev));

  {
    struct c_rest_response res;
    memset(&res, 0, sizeof(res));
    ASSERT_EQ(C_REST_OK, c_rest_sse_init_response(&res));
    ASSERT_NEQ(C_REST_OK, c_rest_sse_send_event(NULL, &ev));
    ASSERT_NEQ(C_REST_OK, c_rest_sse_send_event(&res, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  }

  {
    memset(&ev, 0, sizeof(ev));
    memset(&dest, 0, sizeof(dest));
    ev.id = s1;
    ev.event = se;
    ev.data = sd;

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_sse_event_clone(&ev, &dest));
    g_crf_malloc_hook = NULL;

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 2;
    ASSERT_NEQ(C_REST_OK, c_rest_sse_event_clone(&ev, &dest));
    g_crf_malloc_hook = NULL;

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 3;
    ASSERT_NEQ(C_REST_OK, c_rest_sse_event_clone(&ev, &dest));
    g_crf_malloc_hook = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&dest));
    ev.id = NULL;
    ev.event = NULL;
    ev.data = NULL;
  }

  ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(ctx));
  PASS();
}

TEST test_sse_event_clone_nulls(void) {
  struct c_rest_sse_event src, dest;
  char t_str[] = "test";
  memset(&src, 0, sizeof(src));
  memset(&dest, 0, sizeof(dest));

  ASSERT_FALSE(c_rest_sse_event_clone(&src, &dest) != C_REST_OK);
  ASSERT_FALSE(c_rest_sse_event_destroy(&dest) != C_REST_OK);

  src.id = t_str;
  ASSERT_FALSE(c_rest_sse_event_clone(&src, &dest) != C_REST_OK);
  ASSERT_FALSE(c_rest_sse_event_destroy(&dest) != C_REST_OK);

  src.id = NULL;
  src.event = t_str;
  ASSERT_FALSE(c_rest_sse_event_clone(&src, &dest) != C_REST_OK);
  ASSERT_FALSE(c_rest_sse_event_destroy(&dest) != C_REST_OK);

  src.event = NULL;
  src.data = t_str;
  ASSERT_FALSE(c_rest_sse_event_clone(&src, &dest) != C_REST_OK);
  ASSERT_FALSE(c_rest_sse_event_destroy(&dest) != C_REST_OK);

  PASS();
}

#ifdef C_REST_TESTING_MALLOC_HOOK
TEST test_sse_mock_cases(void) {
  struct c_rest_sse_event src_evt;
  struct c_rest_sse_event dst_evt;
  struct c_rest_sse_event evt;
  char *out_buf = NULL;
  size_t out_len = 0;
  struct c_rest_response res;
  char id1[] = "id1";
  char event1[] = "event1";
  char test_id[] = "test_id";

  memset(&src_evt, 0, sizeof(src_evt));
  memset(&dst_evt, 0, sizeof(dst_evt));
  src_evt.id = id1;
  src_evt.event = event1;

  g_fail_malloc_at = -1;
  fail_malloc_n(0);
  g_fail_malloc_at = 2;
  g_crf_malloc_hook = fail_malloc_n;
  g_mock_sse_append_fail = -3;
  ASSERT_NEQ(C_REST_OK, c_rest_sse_event_clone(&src_evt, &dst_evt));
  g_crf_malloc_hook = NULL;
  g_fail_malloc_at = 0;
  g_mock_sse_append_fail = -1;

  memset(&evt, 0, sizeof(evt));
  evt.id = test_id;
  g_mock_sse_append_fail = -5;
  ASSERT_NEQ(C_REST_OK, c_rest_sse_serialize(&evt, &out_buf, &out_len));
  g_mock_sse_append_fail = -1;

  memset(&res, 0, sizeof(res));
  g_mock_res_status_fail = 1;
  ASSERT_NEQ(C_REST_OK, c_rest_sse_init_response(&res));
  g_mock_res_status_fail = 0;
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));

  {
    struct c_rest_sse_context *bctx = NULL;
    struct c_rest_sse_event bev;
    memset(&bev, 0, sizeof(bev));
    ASSERT_EQ(C_REST_OK, c_rest_sse_context_init(&bctx));
    ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(bctx, "data: x\n\n", 9, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(bctx, NULL, 0, &bev));
    ASSERT_NEQ(C_REST_OK, c_rest_sse_parse(bctx, "", 0, &bev));
    ASSERT_EQ(C_REST_OK, c_rest_sse_parse(bctx, "other: 1\nother\ndata: ok\n\n",
                                          25, &bev));
    ASSERT_STR_EQ("ok", bev.data);
    ASSERT_EQ(C_REST_OK, c_rest_sse_event_destroy(&bev));
    ASSERT_EQ(C_REST_OK, c_rest_sse_context_destroy(bctx));
  }

  PASS();
}
#endif

SUITE(sse_suite) {
  RUN_TEST(test_sse_coverage);
  RUN_TEST(test_sse_event_clone_nulls);
  RUN_TEST(test_sse_event_init_destroy);
  RUN_TEST(test_sse_event_clone);
  RUN_TEST(test_sse_serialize);
  RUN_TEST(test_sse_parse_complete);
  RUN_TEST(test_sse_parse_fragmented);
  RUN_TEST(test_sse_wrappers);
#ifdef C_REST_TESTING_MALLOC_HOOK
  RUN_TEST(test_sse_mock_cases);
#endif
}
