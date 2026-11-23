/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_multipart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

#ifdef C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING

static int g_fail_malloc_at = 0;

static int g_cb_fail_at = 0;
static int g_cb_call_count = 0;

static c_rest_error_t on_part_begin(c_rest_multipart_parser *parser) {
  (void)parser;
  if (g_cb_fail_at > 0 && ++g_cb_call_count == g_cb_fail_at)
    return C_REST_ERROR_GENERIC;
  printf("Part begin\n");
  return C_REST_OK;
}

static c_rest_error_t on_header_field(c_rest_multipart_parser *parser,
                                      const char *at, size_t length) {
  (void)parser;
  if (g_cb_fail_at > 0 && ++g_cb_call_count == g_cb_fail_at)
    return C_REST_ERROR_GENERIC;
  printf("Header field: %.*s\n", (int)length, at);
  return C_REST_OK;
}

static c_rest_error_t on_header_value(c_rest_multipart_parser *parser,
                                      const char *at, size_t length) {
  (void)parser;
  if (g_cb_fail_at > 0 && ++g_cb_call_count == g_cb_fail_at)
    return C_REST_ERROR_GENERIC;
  printf("Header value: %.*s\n", (int)length, at);
  return C_REST_OK;
}

static c_rest_error_t on_headers_complete(c_rest_multipart_parser *parser) {
  (void)parser;
  if (g_cb_fail_at > 0 && ++g_cb_call_count == g_cb_fail_at)
    return C_REST_ERROR_GENERIC;
  printf("Headers complete\n");
  return C_REST_OK;
}

static c_rest_error_t on_part_data(c_rest_multipart_parser *parser,
                                   const char *at, size_t length) {
  (void)parser;
  if (g_cb_fail_at > 0 && ++g_cb_call_count == g_cb_fail_at)
    return C_REST_ERROR_GENERIC;
  printf("Part data: %.*s\n", (int)length, at);
  return C_REST_OK;
}

static c_rest_error_t on_part_end(c_rest_multipart_parser *parser) {
  (void)parser;
  if (g_cb_fail_at > 0 && ++g_cb_call_count == g_cb_fail_at)
    return C_REST_ERROR_GENERIC;
  printf("Part end\n");
  return C_REST_OK;
}

static c_rest_error_t on_body_end(c_rest_multipart_parser *parser) {
  (void)parser;
  if (g_cb_fail_at > 0 && ++g_cb_call_count == g_cb_fail_at)
    return C_REST_ERROR_GENERIC;
  printf("Body end\n");
  return C_REST_OK;
}

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  if (g_fail_malloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_malloc_at) {
    printf("fail_malloc_n triggered at %d\n", alloc_count);
    alloc_count = 0;
    g_fail_malloc_at = 0;
    return NULL;
  }
  printf("fail_malloc_n success at %d\n", alloc_count);
  return malloc(size);
}

struct dummy_parser_state {
  char *boundary;
  size_t boundary_length;
  size_t index;
  size_t boundary_match_index;
  int state;
};

TEST test_multipart_invalid_args(void) {
  size_t parsed = 0;
  c_rest_multipart_parser *parser = NULL;
  struct c_rest_multipart_callbacks callbacks;
  c_rest_error_t rc;
  int i;

  memset(&callbacks, 0, sizeof(callbacks));

  /* Test NULL args */
  rc = c_rest_multipart_parser_init(NULL, "boundary", &callbacks, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_multipart_parser_init(&parser, NULL, &callbacks, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_multipart_parser_destroy(NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  rc = c_rest_multipart_parser_execute(NULL, "data", 4, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  parser = (c_rest_multipart_parser *)1;
  rc = c_rest_multipart_parser_execute(parser, NULL, 4, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  /* Test NULL out_parsed */
  parser = (c_rest_multipart_parser *)1;
  rc = c_rest_multipart_parser_execute(parser, "data", 4, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));

  /* Test invalid start */
  rc = c_rest_multipart_parser_execute(parser, "x", 1, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));
  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  printf("invalid start\n");
  fflush(stdout);
  rc = c_rest_multipart_parser_execute(parser, "-x", 2, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  rc = c_rest_multipart_parser_execute(parser, "--boundaryx", 11, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  /* test invalid header field start */
  printf("invalid header start\n");
  fflush(stdout);
  rc = c_rest_multipart_parser_execute(parser, "--boundary\rx", 12, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));
  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));

  rc = c_rest_multipart_parser_execute(parser, "--boundary\r\n:\r\n\r\n", 18,
                                       &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));
  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));

  /* test valid part begin but no callbacks */
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\nHead:\r\n\r\nData\r\n--boundary--\r\n", 41,
      &parsed);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  rc = c_rest_multipart_parser_execute(parser, "-\r", 2, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Test invalid boundary */
  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  printf("invalid boundary\n");
  fflush(stdout);
  rc = c_rest_multipart_parser_execute(parser, "--x", 3, &parsed);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  callbacks.on_part_begin = on_part_begin;
  callbacks.on_header_field = on_header_field;
  callbacks.on_header_value = on_header_value;
  callbacks.on_headers_complete = on_headers_complete;
  callbacks.on_part_data = on_part_data;
  callbacks.on_part_end = on_part_end;
  callbacks.on_body_end = on_body_end;

  /* Edge case: \r not followed by \n */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\rx--boundary--\r\n",
      67, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: \r\n not followed by - */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\nx--boundary--\r\n",
      68, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: \r\n- not followed by - */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n-x--boundary--\r\n",
      69, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: \r\n-- partial boundary match */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n--boundarX--boundary--\r\n",
      76, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: \r\n--boundary partial match */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n--boundaryX--boundary--\r\n",
      77, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* The same edge cases but without callbacks */
  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\rx--boundary--\r\n",
      67, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\nx--boundary--\r\n",
      68, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n-x--boundary--\r\n",
      69, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n--boundarX--boundary--\r\n",
      76, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: trailing part data without boundary */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\na", 56,
      &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: trailing part data without boundary without callbacks */
  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\na", 56,
      &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: empty header field */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n: empty field\r\n\r\n\r\n--boundary--\r\n", 41,
      &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: empty header value */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\nHeader:\r\n\r\n\r\n--boundary--\r\n", 36, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: zero length execute in part data */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\na", 57,
      &parsed);
  rc = c_rest_multipart_parser_execute(parser, "", 0, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: intermediate boundary without callbacks */
  ASSERT_EQ(C_REST_OK,
            c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL));
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n\r\n\r\n--boundary\r\n", 28, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: boundary match index == 2 mismatch */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(parser,
                                       "--boundary\r\nContent-Disposition: "
                                       "form-data; name=\"f\"\r\n\r\na\r\n--X",
                                       62, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: s_headers_almost_done without LF */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc =
      c_rest_multipart_parser_execute(parser, "--boundary\r\n\rX", 14, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: s_header_value_almost_done without LF */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(parser, "--boundary\r\nHeader: value\rX",
                                       27, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: s_part_data_almost_end without - */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n\r\n\r\n--boundary-X", 29, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: s_end without CR */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n\r\n\r\n--boundary--X", 30, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Edge case: invalid state */
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  ((struct dummy_parser_state *)parser)->state = 999;
  rc = c_rest_multipart_parser_execute(parser, "data", 4, &parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));

  /* Callback failure percolation tests */
  {
    const char *complex_body =
        "--boundary\r\nH: v\r\n\r\na\rx\r\nx\r\n-x\r\n--x\r\n--boX\r\n"
        "--boundary\r\nH2: v2\r\n\r\nb\r\n--boundary--\r\n";
    for (i = 1; i <= 25; i++) {
      g_cb_fail_at = i;
      g_cb_call_count = 0;
      ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                        &callbacks, NULL));
      rc = c_rest_multipart_parser_execute(parser, complex_body,
                                           strlen(complex_body), &parsed);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_GENERIC);
      ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));
    }
    /* Trailing part data callback failure */
    g_cb_fail_at = 5;
    g_cb_call_count = 0;
    ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                      &callbacks, NULL));
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_multipart_parser_execute(
                  parser, "--boundary\r\nH: v\r\n\r\ntrail", 25, &parsed));
    ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));
    g_cb_fail_at = 0;
    g_cb_call_count = 0;
  }

  for (i = 1; i <= 2; i++) {
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    rc = c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  }
  g_fail_malloc_at = 0;

  {
    c_rest_multipart_parser *clone = NULL;
    void *ud = NULL;

    rc = c_rest_multipart_parser_init(&parser, "boundary", &callbacks,
                                      (void *)0x123);
    ASSERT_FALSE(rc != C_REST_OK);

    rc = c_rest_multipart_parser_get_user_data(NULL, &ud);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

    rc = c_rest_multipart_parser_get_user_data(parser, NULL);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

    rc = c_rest_multipart_parser_get_user_data(parser, &ud);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_FALSE(ud != (void *)0x123);

    rc = c_rest_multipart_parser_clone(NULL, &clone);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

    rc = c_rest_multipart_parser_clone(parser, NULL);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

    rc = c_rest_multipart_parser_clone(parser, &clone);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(clone));

    /* OOM in clone */
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 1;
    rc = c_rest_multipart_parser_clone(parser, &clone);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 2;
    rc = c_rest_multipart_parser_clone(parser, &clone);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    g_crf_malloc_hook = NULL;

    ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));
  }
  PASS();
}

TEST test_multipart_valid_stream(void) {
  c_rest_multipart_parser *parser = NULL;
  struct c_rest_multipart_callbacks callbacks;
  size_t parsed = 0;
  const char *data =
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"field1\"\r\n\r\nvalue1\r\n--boundary\r\nContent-Disposition: "
      "form-data; name=\"field2\"\r\n\r\nvalue2\r\n--boundary--\r\n";

  callbacks.on_part_begin = on_part_begin;
  callbacks.on_header_field = on_header_field;
  callbacks.on_header_value = on_header_value;
  callbacks.on_headers_complete = on_headers_complete;
  callbacks.on_part_data = on_part_data;
  callbacks.on_part_end = on_part_end;
  callbacks.on_body_end = on_body_end;

  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_execute(parser, data,
                                                       strlen(data), &parsed));
  ASSERT_EQ(strlen(data), parsed);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));
  PASS();
}

SUITE(multipart_suite) {
  RUN_TEST(test_multipart_invalid_args);
  RUN_TEST(test_multipart_valid_stream);
}

#else

TEST test_multipart_disabled(void) { PASS(); }
SUITE(multipart_suite) { RUN_TEST(test_multipart_disabled); }

#endif /* C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING */
