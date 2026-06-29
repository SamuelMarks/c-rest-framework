/* clang-format off */
#include "c_rest_str_utils.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

int test_str_utils(void) {
  int failed = 0;
  int cmp;
  c_rest_error_t rc;
  char buf[32];
  size_t len;

  rc = c_rest_strcasecmp("Hello", "hello", &cmp);
  if (rc != C_REST_OK || cmp != 0)
    failed++;

  rc = c_rest_strncasecmp("HelloWorld", "helloWorld", 5, &cmp);
  if (rc != C_REST_OK || cmp != 0)
    failed++;

  rc = c_rest_strlcpy(buf, "abc", sizeof(buf), &len);
  if (rc != C_REST_OK || len != 3 || strcmp(buf, "abc") != 0)
    failed++;

  rc = c_rest_strlcat(buf, "def", sizeof(buf), &len);
  if (rc != C_REST_OK || len != 6 || strcmp(buf, "abcdef") != 0)
    failed++;

  rc = c_rest_url_decode(buf, "a%20b", 5);
  if (rc != C_REST_OK || strcmp(buf, "a b") != 0)
    failed++;

  if (failed) {
    printf("test_str_utils failed\n");
  } else {
    printf("test_str_utils passed\n");
  }

  return failed;
}
