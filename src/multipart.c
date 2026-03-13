/* clang-format off */
#include "c_rest_multipart.h"

#include <stdlib.h>
#include <string.h>
/* clang-format on */

struct c_rest_multipart_parser {
  char *boundary;
  struct c_rest_multipart_callbacks callbacks;
  void *user_data;
};

int c_rest_multipart_parser_init(
    c_rest_multipart_parser **out_parser, const char *boundary,
    const struct c_rest_multipart_callbacks *callbacks, void *user_data) {
  c_rest_multipart_parser *parser;

  if (!out_parser || !boundary)
    return 1;

  parser = (c_rest_multipart_parser *)malloc(sizeof(c_rest_multipart_parser));
  if (!parser)
    return 1;

  parser->boundary = (char *)malloc(strlen(boundary) + 1);
  if (!parser->boundary) {
    free(parser);
    return 1;
  }
#if defined(_MSC_VER)
  strcpy_s(parser->boundary, strlen(boundary) + 1, boundary);
#else
  strcpy(parser->boundary, boundary);
#endif

  if (callbacks) {
    parser->callbacks = *callbacks;
  } else {
    parser->callbacks.on_part_data = NULL;
    parser->callbacks.on_part_end = NULL;
  }

  parser->user_data = user_data;
  *out_parser = parser;
  return 0;
}

int c_rest_multipart_parser_execute(c_rest_multipart_parser *parser,
                                    const char *data, size_t len,
                                    size_t *out_parsed) {
  /* Basic mock: just passes all data as one part */
  if (!out_parsed)
    return 1;
  *out_parsed = 0;
  if (!parser || !data || len == 0)
    return 1;

  if (parser->callbacks.on_part_data) {
    parser->callbacks.on_part_data(parser, data, len);
  }

  if (parser->callbacks.on_part_end) {
    parser->callbacks.on_part_end(parser);
  }

  *out_parsed = len;
  return 0;
}

int c_rest_multipart_parser_destroy(c_rest_multipart_parser *parser) {
  if (!parser)
    return 1;
  if (parser->boundary)
    free(parser->boundary);
  free(parser);
  return 0;
}
