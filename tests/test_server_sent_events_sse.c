/* clang-format off */
#include "c_rest_sse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/* clang-format on */

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
  ASSERT_EQ(C_REST_SSE_OK, c_rest_sse_event_init(&ev));
  ASSERT_NULL(ev.id);
  ASSERT_NULL(ev.event);
  ASSERT_NULL(ev.data);
  ASSERT_EQ(-1, ev.retry);

  ASSERT_EQ(C_REST_SSE_OK, c_rest_sse_event_destroy(&ev));
  return 0;
}

static char *test_strdup(const char *s) {
  size_t len = strlen(s);
  char *copy = malloc(len + 1);
  if (copy) {
    memcpy(copy, s, len + 1);
  }
  return copy;
}

static int test_sse_event_clone(void) {
  struct c_rest_sse_event src;
  struct c_rest_sse_event dest;
  char *id = "123";
  char *event = "message";
  char *data = "hello world";

  c_rest_sse_event_init(&src);
  c_rest_sse_event_init(&dest);

  src.id = test_strdup(id);
  src.event = test_strdup(event);
  src.data = test_strdup(data);
  src.retry = 1000;

  ASSERT_EQ(C_REST_SSE_OK, c_rest_sse_event_clone(&src, &dest));

  ASSERT_STR_EQ(src.id, dest.id);
  ASSERT_STR_EQ(src.event, dest.event);
  ASSERT_STR_EQ(src.data, dest.data);
  ASSERT_EQ(src.retry, dest.retry);

  ASSERT(src.id != dest.id);

  free(src.id);
  free(src.event);
  free(src.data);

  ASSERT_EQ(C_REST_SSE_OK, c_rest_sse_event_destroy(&dest));
  return 0;
}

static int test_sse_serialize(void) {
  struct c_rest_sse_event ev;
  char *out = NULL;
  size_t len = 0;

  c_rest_sse_event_init(&ev);
  ev.id = test_strdup("42");
  ev.event = test_strdup("ping");
  ev.data = test_strdup("line1\nline2");
  ev.retry = 3000;

  ASSERT_EQ(C_REST_SSE_OK, c_rest_sse_serialize(&ev, &out, &len));
  ASSERT(out != NULL);

  ASSERT_STR_EQ(
      "id: 42\nevent: ping\nretry: 3000\ndata: line1\ndata: line2\n\n", out);

  free(ev.id);
  free(ev.event);
  free(ev.data);
  free(out);
  return 0;
}

static int test_sse_parse_complete(void) {
  struct c_rest_sse_context *ctx = NULL;
  struct c_rest_sse_event ev;
  const char *payload = "id: 1\nevent: custom\nretry: 500\ndata: test data\n\n";

  ASSERT_EQ(C_REST_SSE_OK, c_rest_sse_context_init(&ctx));
  c_rest_sse_event_init(&ev);

  ASSERT_EQ(C_REST_SSE_OK,
            c_rest_sse_parse(ctx, payload, strlen(payload), &ev));

  ASSERT_STR_EQ("1", ev.id);
  ASSERT_STR_EQ("custom", ev.event);
  ASSERT_EQ(500, ev.retry);
  ASSERT_STR_EQ("test data", ev.data);

  c_rest_sse_event_destroy(&ev);
  c_rest_sse_context_destroy(ctx);
  return 0;
}

static int test_sse_parse_fragmented(void) {
  struct c_rest_sse_context *ctx = NULL;
  struct c_rest_sse_event ev;

  ASSERT_EQ(C_REST_SSE_OK, c_rest_sse_context_init(&ctx));
  c_rest_sse_event_init(&ev);

  ASSERT_EQ(1, c_rest_sse_parse(ctx, "id: 2\nev", 8, &ev));
  ASSERT_EQ(1, c_rest_sse_parse(ctx, "ent: partial\ndat", 16, &ev));
  ASSERT_EQ(1, c_rest_sse_parse(ctx, "a: hello", 8, &ev));
  ASSERT_EQ(C_REST_SSE_OK, c_rest_sse_parse(ctx, "\n\n", 2, &ev));

  ASSERT_STR_EQ("2", ev.id);
  ASSERT_STR_EQ("partial", ev.event);
  ASSERT_STR_EQ("hello", ev.data);

  c_rest_sse_event_destroy(&ev);
  c_rest_sse_context_destroy(ctx);
  return 0;
}

#include "c_rest_response.h"

static int test_sse_wrappers(void) {
  struct c_rest_response res;
  struct c_rest_sse_event ev;
  int ret;

  memset(&res, 0, sizeof(res));

  /* Test init response without context (will succeed but not write) */
  ret = c_rest_sse_init_response(&res);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(1, res.headers_sent);
  ASSERT_EQ(200, res.status_code);

  /* Test send event without context (will fail at write_chunk context check) */
  c_rest_sse_event_init(&ev);
  ev.event = "test";
  ret = c_rest_sse_send_event(&res, &ev);
  ASSERT_EQ(1, ret); /* Should fail because ctx is NULL */

  ret = c_rest_sse_send_keepalive(&res);
  ASSERT_EQ(1, ret); /* Should fail because ctx is NULL */

  c_rest_response_cleanup(&res);
  return 0;
}

int test_server_sent_events_sse(void) {
  int res = 0;
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
