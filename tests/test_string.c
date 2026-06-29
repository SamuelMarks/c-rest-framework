/* clang-format off */
#include "c_rest_string.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

int test_string(void) {
  c_rest_string str;
  c_rest_error_t rc;
  int failed = 0;

  rc = c_rest_string_init(&str, 16);
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_string_append_cstr(&str, "hello");
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_string_append(&str, " world", 6);
  if (rc != C_REST_OK)
    failed++;

  if (strcmp(str.data, "hello world") != 0)
    failed++;

  rc = c_rest_string_destroy(&str);
  if (rc != C_REST_OK)
    failed++;

  if (failed) {
    printf("test_string failed\n");
  } else {
    printf("test_string passed\n");
  }
  return failed;
}
