/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_multipart.h"
#include <string.h>
/* clang-format on */

#ifdef C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING

struct streaming_capture {
  int part_begins;
  int headers_completed;
  int part_ends;
  int body_ends;
  char last_field[64];
  char last_value[128];
  char part1_data[64];
  char part2_data[64];
};

static void copy_clamped(char *dst, size_t dst_sz, const char *src,
                         size_t len) {
  size_t n = len < (dst_sz - 1) ? len : (dst_sz - 1);
  memcpy(dst, src, n);
  dst[n] = '\0';
}

static c_rest_error_t on_part_begin(c_rest_multipart_parser *parser) {
  struct streaming_capture *cap = NULL;
  if (c_rest_multipart_parser_get_user_data(parser, (void **)&cap) ==
          C_REST_OK &&
      cap) {
    cap->part_begins++;
  }
  return C_REST_OK;
}

static c_rest_error_t on_header_field(c_rest_multipart_parser *parser,
                                      const char *at, size_t length) {
  struct streaming_capture *cap = NULL;
  if (c_rest_multipart_parser_get_user_data(parser, (void **)&cap) ==
          C_REST_OK &&
      cap) {
    copy_clamped(cap->last_field, sizeof(cap->last_field), at, length);
  }
  return C_REST_OK;
}

static c_rest_error_t on_header_value(c_rest_multipart_parser *parser,
                                      const char *at, size_t length) {
  struct streaming_capture *cap = NULL;
  if (c_rest_multipart_parser_get_user_data(parser, (void **)&cap) ==
          C_REST_OK &&
      cap) {
    copy_clamped(cap->last_value, sizeof(cap->last_value), at, length);
  }
  return C_REST_OK;
}

static c_rest_error_t on_headers_complete(c_rest_multipart_parser *parser) {
  struct streaming_capture *cap = NULL;
  if (c_rest_multipart_parser_get_user_data(parser, (void **)&cap) ==
          C_REST_OK &&
      cap) {
    cap->headers_completed++;
  }
  return C_REST_OK;
}

static c_rest_error_t on_part_data(c_rest_multipart_parser *parser,
                                   const char *at, size_t length) {
  struct streaming_capture *cap = NULL;
  if (c_rest_multipart_parser_get_user_data(parser, (void **)&cap) ==
          C_REST_OK &&
      cap) {
    if (cap->part_begins == 1) {
      copy_clamped(cap->part1_data, sizeof(cap->part1_data), at, length);
    } else if (cap->part_begins == 2) {
      copy_clamped(cap->part2_data, sizeof(cap->part2_data), at, length);
    }
  }
  return C_REST_OK;
}

static c_rest_error_t on_part_end(c_rest_multipart_parser *parser) {
  struct streaming_capture *cap = NULL;
  if (c_rest_multipart_parser_get_user_data(parser, (void **)&cap) ==
          C_REST_OK &&
      cap) {
    cap->part_ends++;
  }
  return C_REST_OK;
}

static c_rest_error_t on_body_end(c_rest_multipart_parser *parser) {
  struct streaming_capture *cap = NULL;
  if (c_rest_multipart_parser_get_user_data(parser, (void **)&cap) ==
          C_REST_OK &&
      cap) {
    cap->body_ends++;
  }
  return C_REST_OK;
}

TEST test_multipart_streaming_parts(void) {
  c_rest_multipart_parser *parser = NULL;
  struct c_rest_multipart_callbacks callbacks;
  struct streaming_capture cap;
  size_t parsed = 0;
  const char *data =
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"field1\"\r\n\r\nvalue1\r\n--boundary\r\nContent-Disposition: "
      "form-data; name=\"field2\"\r\n\r\nvalue2\r\n--boundary--\r\n";

  memset(&cap, 0, sizeof(cap));
  callbacks.on_part_begin = on_part_begin;
  callbacks.on_header_field = on_header_field;
  callbacks.on_header_value = on_header_value;
  callbacks.on_headers_complete = on_headers_complete;
  callbacks.on_part_data = on_part_data;
  callbacks.on_part_end = on_part_end;
  callbacks.on_body_end = on_body_end;

  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_init(&parser, "boundary",
                                                    &callbacks, &cap));
  ASSERT(parser != NULL);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_execute(parser, data,
                                                       strlen(data), &parsed));
  ASSERT_EQ(strlen(data), parsed);
  ASSERT_EQ(2, cap.part_begins);
  ASSERT_EQ(2, cap.headers_completed);
  ASSERT_EQ(2, cap.part_ends);
  ASSERT_EQ(1, cap.body_ends);
  ASSERT_STR_EQ("Content-Disposition", cap.last_field);
  ASSERT_STR_EQ("form-data; name=\"field2\"", cap.last_value);
  ASSERT_STR_EQ("value1", cap.part1_data);
  ASSERT_STR_EQ("value2", cap.part2_data);
  ASSERT_EQ(C_REST_OK, c_rest_multipart_parser_destroy(parser));
  PASS();
}

#else

TEST test_multipart_streaming_parts(void) { PASS(); }

#endif /* C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING */

SUITE(full_multipart_streaming_suite) {
  RUN_TEST(test_multipart_streaming_parts);
}
