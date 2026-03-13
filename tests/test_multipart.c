/* clang-format off */
#include "c_rest_multipart.h"

#include <stdio.h>
#include <string.h>
/* clang-format on */

static int part_data_called = 0;
static int part_end_called = 0;

static void on_part_data(c_rest_multipart_parser *parser, const char *data,
                         size_t len) {
  (void)parser;
  (void)data;
  (void)len;
  part_data_called = 1;
}

static void on_part_end(c_rest_multipart_parser *parser) {
  (void)parser;
  part_end_called = 1;
}

int test_multipart(void) {
  c_rest_multipart_parser *parser = NULL;
  struct c_rest_multipart_callbacks callbacks;
  const char *data = "--boundary\r\nContent-Disposition: form-data; "
                     "name=\"field1\"\r\n\r\nvalue1\r\n--boundary--\r\n";
  size_t parsed;
  int res;

  printf("Running multipart parser tests...\n");

  callbacks.on_part_data = on_part_data;
  callbacks.on_part_end = on_part_end;

  res = c_rest_multipart_parser_init(&parser, "boundary", &callbacks, NULL);
  if (res != 0) {
    printf("Failed to init multipart parser\n");
    return 1;
  }

  parsed = c_rest_multipart_parser_execute(parser, data, strlen(data));
  if (parsed != strlen(data)) {
    printf("Failed to parse multipart data\n");
    return 1;
  }

  if (!part_data_called || !part_end_called) {
    printf("Multipart callbacks were not invoked appropriately\n");
    return 1;
  }

  c_rest_multipart_parser_destroy(parser);

  return 0;
}
