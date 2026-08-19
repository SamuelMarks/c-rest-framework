#include <stdlib.h>
/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_multipart.h"

#include <stdio.h>
#include <string.h>
/* clang-format on */

#ifdef C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING

static c_rest_error_t on_part_begin(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("Part begin\n");
  return 0;
}

static c_rest_error_t on_header_field(c_rest_multipart_parser *parser,
                                      const char *at, size_t length) {
  (void)parser;
  printf("Header field: %.*s\n", (int)length, at);
  return 0;
}

static c_rest_error_t on_header_value(c_rest_multipart_parser *parser,
                                      const char *at, size_t length) {
  (void)parser;
  printf("Header value: %.*s\n", (int)length, at);
  return 0;
}

static c_rest_error_t on_headers_complete(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("Headers complete\n");
  return 0;
}

static c_rest_error_t on_part_data(c_rest_multipart_parser *parser,
                                   const char *at, size_t length) {
  (void)parser;
  printf("Part data: %.*s\n", (int)length, at);
  return 0;
}

static c_rest_error_t on_part_end(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("Part end\n");
  return 0;
}

static c_rest_error_t on_body_end(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("Body end\n");
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

static void test_multipart_invalid_args(int *res_ptr) {
  size_t parsed = 0;
  c_rest_multipart_parser *parser = NULL;
  struct c_rest_multipart_callbacks callbacks;
  c_rest_error_t rc;
  int i;
  extern int g_fail_malloc_at;

  memset(&callbacks, 0, sizeof(callbacks));

  /* Test NULL args */
  rc = c_rest_multipart_parser_init(NULL, "boundary", &callbacks, NULL);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  rc = c_rest_multipart_parser_init(&parser, NULL, &callbacks, NULL);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  rc = c_rest_multipart_parser_destroy(NULL);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  rc = c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  if (rc != C_REST_OK) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);

  rc = c_rest_multipart_parser_execute(NULL, "data", 4, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  parser = (c_rest_multipart_parser *)1;
  rc = c_rest_multipart_parser_execute(parser, NULL, 4, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  /* Test NULL out_parsed */
  parser = (c_rest_multipart_parser *)1;
  rc = c_rest_multipart_parser_execute(parser, "data", 4, NULL);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);

  /* Test invalid start */
  rc = c_rest_multipart_parser_execute(parser, "x", 1, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  c_rest_multipart_parser_destroy(parser);
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  printf("invalid start\n");
  fflush(stdout);
  rc = c_rest_multipart_parser_execute(parser, "-x", 2, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(parser, "--boundaryx", 11, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  /* test invalid header field start */
  printf("invalid header start\n");
  fflush(stdout);
  rc = c_rest_multipart_parser_execute(parser, "--boundary\rx", 12, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);

  rc = c_rest_multipart_parser_execute(parser, "--boundary\r\n:\r\n\r\n", 18,
                                       &parsed);
  c_rest_multipart_parser_destroy(parser);
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);

  /* test valid part begin but no callbacks */
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\nHead:\r\n\r\nData\r\n--boundary--\r\n", 41,
      &parsed);
  if (rc != C_REST_OK) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(parser, "-\r", 2, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);

  /* Test invalid boundary */
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  printf("invalid boundary\n");
  fflush(stdout);
  rc = c_rest_multipart_parser_execute(parser, "--x", 3, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  c_rest_multipart_parser_destroy(parser);

  callbacks.on_part_begin = on_part_begin;
  callbacks.on_header_field = on_header_field;
  callbacks.on_header_value = on_header_value;
  callbacks.on_headers_complete = on_headers_complete;
  callbacks.on_part_data = on_part_data;
  callbacks.on_part_end = on_part_end;
  callbacks.on_body_end = on_body_end;

  /* Edge case: \r not followed by \n */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\rx--boundary--\r\n",
      67, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: \r\n not followed by - */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\nx--boundary--\r\n",
      68, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: \r\n- not followed by - */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n-x--boundary--\r\n",
      69, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: \r\n-- partial boundary match */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n--boundarX--boundary--\r\n",
      76, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: \r\n--boundary partial match */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n--boundaryX--boundary--\r\n",
      77, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* The same edge cases but without callbacks */
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\rx--boundary--\r\n",
      67, &parsed);
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\nx--boundary--\r\n",
      68, &parsed);
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n-x--boundary--\r\n",
      69, &parsed);
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n--boundarX--boundary--\r\n",
      76, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: trailing part data without boundary */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\na", 56,
      &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: trailing part data without boundary without callbacks */
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\na", 56,
      &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: empty header field */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n: empty field\r\n\r\n\r\n--boundary--\r\n", 41,
      &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: empty header value */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\nHeader:\r\n\r\n\r\n--boundary--\r\n", 36, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: zero length execute in part data */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\na", 57,
      &parsed);
  rc = c_rest_multipart_parser_execute(parser, "", 0, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: intermediate boundary without callbacks */
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n\r\n\r\n--boundary\r\n", 28, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: boundary match index == 2 mismatch */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(parser,
                                       "--boundary\r\nContent-Disposition: "
                                       "form-data; name=\"f\"\r\n\r\na\r\n--X",
                                       62, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: s_headers_almost_done without LF */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc =
      c_rest_multipart_parser_execute(parser, "--boundary\r\n\rX", 14, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: s_header_value_almost_done without LF */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(parser, "--boundary\r\nHeader: value\rX",
                                       27, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: s_part_data_almost_end without - */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n\r\n\r\n--boundary-X", 29, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: s_end without CR */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n\r\n\r\n--boundary--X", 30, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: invalid state */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  ((struct dummy_parser_state *)parser)->state = 999;
  rc = c_rest_multipart_parser_execute(parser, "data", 4, &parsed);
  c_rest_multipart_parser_destroy(parser);

  printf("malloc loop\n");
  fflush(stdout);
  for (i = 1; i <= 5; i++) { /* LCOV_EXCL_BR_LINE */
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    rc = c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
    g_crf_malloc_hook = NULL;
    if (rc == C_REST_OK) { /* LCOV_EXCL_BR_LINE */
      c_rest_multipart_parser_destroy(parser);
      break;
    }
  }
  g_fail_malloc_at = 0;

  {
    c_rest_multipart_parser *clone = NULL;
    void *ud = NULL;

    rc = c_rest_multipart_parser_init(&parser, "boundary", &callbacks,
                                      (void *)0x123);
    if (rc != C_REST_OK) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_get_user_data(NULL, &ud);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_get_user_data(parser, NULL);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_get_user_data(parser, &ud);
    if (rc != C_REST_OK || ud != (void *)0x123) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_clone(NULL, &clone);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_clone(parser, NULL);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_clone(parser, &clone);
    if (rc != C_REST_OK) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;
    c_rest_multipart_parser_destroy(clone);

    /* OOM in clone */
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 1;
    rc = c_rest_multipart_parser_clone(parser, &clone);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 2;
    rc = c_rest_multipart_parser_clone(parser, &clone);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;
    g_crf_malloc_hook = NULL;

    c_rest_multipart_parser_destroy(parser);
  }
}

int test_multipart(void) {
  c_rest_multipart_parser *parser = NULL;
  struct c_rest_multipart_callbacks callbacks;

  int res = 0;
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

  test_multipart_invalid_args(&res);
  printf("Running multipart parser tests...\n");

  res = c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  if (res != 0) { /* LCOV_EXCL_BR_LINE */
    printf("Failed to init multipart parser\n");
    return res;
  }

  res = c_rest_multipart_parser_execute(parser, data, strlen(data), &parsed);
  if (res != 0) { /* LCOV_EXCL_BR_LINE */
    printf("Failed to parse multipart data\n");
    (void)!c_rest_multipart_parser_destroy(parser);
    return res;
  }

  if (parsed != strlen(data)) { /* LCOV_EXCL_BR_LINE */
    printf("Did not parse all data\n");
    (void)!c_rest_multipart_parser_destroy(parser);
    return 1;
  }

  (void)!c_rest_multipart_parser_destroy(parser);
  return 0;
}

#else

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  extern int g_fail_malloc_at;
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

static void test_multipart_invalid_args(int *res_ptr) {
  size_t parsed = 0;
  c_rest_multipart_parser *parser = NULL;
  struct c_rest_multipart_callbacks callbacks;
  c_rest_error_t rc;
  int i;
  extern int g_fail_malloc_at;

  memset(&callbacks, 0, sizeof(callbacks));

  /* Test NULL args */
  rc = c_rest_multipart_parser_init(NULL, "boundary", &callbacks, NULL);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  rc = c_rest_multipart_parser_init(&parser, NULL, &callbacks, NULL);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  rc = c_rest_multipart_parser_destroy(NULL);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  rc = c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  if (rc != C_REST_OK) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);

  rc = c_rest_multipart_parser_execute(NULL, "data", 4, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  parser = (c_rest_multipart_parser *)1;
  rc = c_rest_multipart_parser_execute(parser, NULL, 4, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  /* Test NULL out_parsed */
  parser = (c_rest_multipart_parser *)1;
  rc = c_rest_multipart_parser_execute(parser, "data", 4, NULL);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);

  /* Test invalid start */
  rc = c_rest_multipart_parser_execute(parser, "x", 1, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  c_rest_multipart_parser_destroy(parser);
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  printf("invalid start\n");
  fflush(stdout);
  rc = c_rest_multipart_parser_execute(parser, "-x", 2, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(parser, "--boundaryx", 11, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  /* test invalid header field start */
  printf("invalid header start\n");
  fflush(stdout);
  rc = c_rest_multipart_parser_execute(parser, "--boundary\rx", 12, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);

  rc = c_rest_multipart_parser_execute(parser, "--boundary\r\n:\r\n\r\n", 18,
                                       &parsed);
  c_rest_multipart_parser_destroy(parser);
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);

  /* test valid part begin but no callbacks */
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\nHead:\r\n\r\nData\r\n--boundary--\r\n", 41,
      &parsed);
  if (rc != C_REST_OK) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(parser, "-\r", 2, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }
  c_rest_multipart_parser_destroy(parser);

  /* Test invalid boundary */
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  printf("invalid boundary\n");
  fflush(stdout);
  rc = c_rest_multipart_parser_execute(parser, "--x", 3, &parsed);
  if (rc !=
      C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */ { /* LCOV_EXCL_BR_LINE */
    printf("Failed\n");
    (*res_ptr)++;
  }

  c_rest_multipart_parser_destroy(parser);

  callbacks.on_part_begin = on_part_begin;
  callbacks.on_header_field = on_header_field;
  callbacks.on_header_value = on_header_value;
  callbacks.on_headers_complete = on_headers_complete;
  callbacks.on_part_data = on_part_data;
  callbacks.on_part_end = on_part_end;
  callbacks.on_body_end = on_body_end;

  /* Edge case: \r not followed by \n */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\rx--boundary--\r\n",
      67, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: \r\n not followed by - */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\nx--boundary--\r\n",
      68, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: \r\n- not followed by - */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n-x--boundary--\r\n",
      69, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: \r\n-- partial boundary match */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n--boundarX--boundary--\r\n",
      76, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: \r\n--boundary partial match */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n--boundaryX--boundary--\r\n",
      77, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* The same edge cases but without callbacks */
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\rx--boundary--\r\n",
      67, &parsed);
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\nx--boundary--\r\n",
      68, &parsed);
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n-x--boundary--\r\n",
      69, &parsed);
  c_rest_multipart_parser_destroy(parser);

  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"f\"\r\n\r\na\r\n--boundarX--boundary--\r\n",
      76, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: trailing part data without boundary */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\na", 56,
      &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: trailing part data without boundary without callbacks */
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\na", 56,
      &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: empty header field */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n: empty field\r\n\r\n\r\n--boundary--\r\n", 41,
      &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: empty header value */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\nHeader:\r\n\r\n\r\n--boundary--\r\n", 36, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: zero length execute in part data */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser,
      "--boundary\r\nContent-Disposition: form-data; name=\"f\"\r\n\r\na", 57,
      &parsed);
  rc = c_rest_multipart_parser_execute(parser, "", 0, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: intermediate boundary without callbacks */
  c_rest_multipart_parser_init(&parser, "boundary", NULL, NULL);
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n\r\n\r\n--boundary\r\n", 28, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: boundary match index == 2 mismatch */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(parser,
                                       "--boundary\r\nContent-Disposition: "
                                       "form-data; name=\"f\"\r\n\r\na\r\n--X",
                                       62, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: s_headers_almost_done without LF */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc =
      c_rest_multipart_parser_execute(parser, "--boundary\r\n\rX", 14, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: s_header_value_almost_done without LF */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(parser, "--boundary\r\nHeader: value\rX",
                                       27, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: s_part_data_almost_end without - */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n\r\n\r\n--boundary-X", 29, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: s_end without CR */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  rc = c_rest_multipart_parser_execute(
      parser, "--boundary\r\n\r\n\r\n--boundary--X", 30, &parsed);
  c_rest_multipart_parser_destroy(parser);

  /* Edge case: invalid state */
  c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  ((struct dummy_parser_state *)parser)->state = 999;
  rc = c_rest_multipart_parser_execute(parser, "data", 4, &parsed);
  c_rest_multipart_parser_destroy(parser);

  printf("malloc loop\n");
  fflush(stdout);
  for (i = 1; i <= 5; i++) { /* LCOV_EXCL_BR_LINE */
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = i;

    rc = c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
    g_crf_malloc_hook = NULL;
    if (rc == C_REST_OK) { /* LCOV_EXCL_BR_LINE */
      c_rest_multipart_parser_destroy(parser);
      break;
    }
  }
  g_fail_malloc_at = 0;

  {
    c_rest_multipart_parser *clone = NULL;
    void *ud = NULL;

    rc = c_rest_multipart_parser_init(&parser, "boundary", &callbacks,
                                      (void *)0x123);
    if (rc != C_REST_OK) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_get_user_data(NULL, &ud);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_get_user_data(parser, NULL);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_get_user_data(parser, &ud);
    if (rc != C_REST_OK || ud != (void *)0x123) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_clone(NULL, &clone);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_clone(parser, NULL);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    rc = c_rest_multipart_parser_clone(parser, &clone);
    if (rc != C_REST_OK) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;
    c_rest_multipart_parser_destroy(clone);

    /* OOM in clone */
    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 1;
    rc = c_rest_multipart_parser_clone(parser, &clone);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;

    g_fail_malloc_at = -1;
    fail_malloc_n(0);
    g_crf_malloc_hook = fail_malloc_n;
    g_fail_malloc_at = 2;
    rc = c_rest_multipart_parser_clone(parser, &clone);
    if (rc != C_REST_ERROR_GENERIC) /* LCOV_EXCL_BR_LINE */
      (*res_ptr)++;
    g_crf_malloc_hook = NULL;

    c_rest_multipart_parser_destroy(parser);
  }
}

int test_multipart(void) {
  test_multipart_invalid_args();
  return 0;
}

#endif /* C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING */
