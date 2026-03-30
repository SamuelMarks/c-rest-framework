/* clang-format off */
#include "c_rest_multipart.h"

#include <stdio.h>
#include <string.h>
/* clang-format on */

#ifdef C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING

static int on_part_begin(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("Part begin\n");
  return 0;
}

static int on_header_field(c_rest_multipart_parser *parser, const char *at,
                           size_t length) {
  (void)parser;
  printf("Header field: %.*s\n", (int)length, at);
  return 0;
}

static int on_header_value(c_rest_multipart_parser *parser, const char *at,
                           size_t length) {
  (void)parser;
  printf("Header value: %.*s\n", (int)length, at);
  return 0;
}

static int on_headers_complete(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("Headers complete\n");
  return 0;
}

static int on_part_data(c_rest_multipart_parser *parser, const char *at,
                        size_t length) {
  (void)parser;
  printf("Part data: %.*s\n", (int)length, at);
  return 0;
}

static int on_part_end(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("Part end\n");
  return 0;
}

static int on_body_end(c_rest_multipart_parser *parser) {
  (void)parser;
  printf("Body end\n");
  return 0;
}

int test_full_multipart_form_streaming(void) {
  c_rest_multipart_parser *parser = NULL;
  struct c_rest_multipart_callbacks callbacks;
  int res;
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

  printf("Running multipart parser tests...\n");

  res = c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  if (res != 0) {
    printf("Failed to init multipart parser\n");
    return res;
  }

  res = c_rest_multipart_parser_execute(parser, data, strlen(data), &parsed);
  if (res != 0) {
    printf("Failed to parse multipart data\n");
    c_rest_multipart_parser_destroy(parser);
    return res;
  }

  if (parsed != strlen(data)) {
    printf("Did not parse all data\n");
    c_rest_multipart_parser_destroy(parser);
    return 1;
  }

  c_rest_multipart_parser_destroy(parser);
  return 0;
}

#else

int test_full_multipart_form_streaming(void) { return 0; }

#endif /* C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING */
