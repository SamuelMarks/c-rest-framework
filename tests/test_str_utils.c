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
  failed += (rc != C_REST_OK | cmp != 0);

  rc = c_rest_strcasecmp("Hello", "hella", &cmp);
  failed += (rc != C_REST_OK | cmp <= 0);

  rc = c_rest_strcasecmp("Hella", "hello", &cmp);
  failed += (rc != C_REST_OK | cmp >= 0);

  rc = c_rest_strcasecmp("A", "", &cmp);
  failed += (rc != C_REST_OK | cmp <= 0);

  rc = c_rest_strcasecmp("", "A", &cmp);
  failed += (rc != C_REST_OK | cmp >= 0);

  rc = c_rest_strcasecmp("HelloA", "Hello", &cmp);
  failed += (rc != C_REST_OK | cmp <= 0);

  rc = c_rest_strcasecmp("Hello", "HelloA", &cmp);
  failed += (rc != C_REST_OK | cmp >= 0);

  rc = c_rest_strncasecmp("HelloA", "Hello", 6, &cmp);
  failed += (rc != C_REST_OK | cmp <= 0);

  rc = c_rest_strncasecmp("Hello", "HelloA", 6, &cmp);
  failed += (rc != C_REST_OK | cmp >= 0);

  rc = c_rest_strcasecmp("Helloa", "Hello", &cmp);
  failed += (rc != C_REST_OK | cmp <= 0);

  rc = c_rest_strcasecmp("Hello", "Helloa", &cmp);
  failed += (rc != C_REST_OK | cmp >= 0);

  rc = c_rest_strncasecmp("Helloa", "Hello", 6, &cmp);
  failed += (rc != C_REST_OK | cmp <= 0);

  rc = c_rest_strncasecmp("Hello", "Helloa", 6, &cmp);
  failed += (rc != C_REST_OK | cmp >= 0);

  rc = c_rest_strcasecmp(NULL, "hello", &cmp);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_strcasecmp("Hello", NULL, &cmp);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_strcasecmp("Hello", "hello", NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_strncasecmp("HelloWorld", "helloWorld", 5, &cmp);
  failed += (rc != C_REST_OK | cmp != 0);

  rc = c_rest_strncasecmp("HelloW", "hellaW", 5, &cmp);
  failed += (rc != C_REST_OK | cmp <= 0);

  rc = c_rest_strncasecmp("HellaW", "helloW", 5, &cmp);
  failed += (rc != C_REST_OK | cmp >= 0);

  rc = c_rest_strncasecmp("A", "", 1, &cmp);
  failed += (rc != C_REST_OK | cmp <= 0);

  rc = c_rest_strncasecmp("", "A", 1, &cmp);
  failed += (rc != C_REST_OK | cmp >= 0);

  rc = c_rest_strncasecmp("Hello", "hello", 0, &cmp);
  failed += (rc != C_REST_OK | cmp != 0);

  rc = c_rest_strncasecmp("Hello", "helloa", (size_t)-1, &cmp);
  failed += (rc != C_REST_OK | cmp >= 0);

  rc = c_rest_strncasecmp(NULL, "helloWorld", 5, &cmp);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_strncasecmp("HelloWorld", NULL, 5, &cmp);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_strncasecmp("HelloWorld", "helloWorld", 5, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_strlcpy(buf, "abc", sizeof(buf), &len);
  failed += (rc != C_REST_OK | len != 3 | strcmp(buf, "abc") != 0);

  rc = c_rest_strlcpy(buf, "abcdef", 4, &len);
  failed += (rc != C_REST_OK | len != 6 | strcmp(buf, "abc") != 0);

  rc = c_rest_strlcpy(buf, "abc", 0, &len);
  failed += (rc != C_REST_OK | len != 3);

  rc = c_rest_strlcpy(NULL, "abc", sizeof(buf), &len);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_strlcpy(buf, NULL, sizeof(buf), &len);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_strlcpy(buf, "abc", sizeof(buf), NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  buf[0] = '\0';
  rc = c_rest_strlcat(buf, "def", sizeof(buf), &len);
  failed += (rc != C_REST_OK | len != 3 | strcmp(buf, "def") != 0);

  buf[0] = 'a';
  buf[1] = '\0';
  rc = c_rest_strlcat(buf, "def", 2, &len);
  failed += (rc != C_REST_OK | len != 4 | strcmp(buf, "a") != 0);

  buf[0] = 'a';
  buf[1] = '\0';
  rc = c_rest_strlcat(buf, "def", 3, &len);
  failed += (rc != C_REST_OK | len != 4 | strcmp(buf, "ad") != 0);

  /* Test where dsize <= dst_len */
  buf[0] = 'a';
  buf[1] = 'b';
  buf[2] = '\0';
  rc = c_rest_strlcat(buf, "def", 1, &len);
  failed += (rc != C_REST_OK | len != 4);

  rc = c_rest_strlcat(buf, "def", sizeof(buf), NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_strlcat(NULL, "def", sizeof(buf), &len);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_strlcat(buf, NULL, sizeof(buf), &len);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_url_decode(buf, "a%20b", 5);
  failed += (rc != C_REST_OK | strcmp(buf, "a b") != 0);

  rc = c_rest_url_decode(buf, "a+b", 3);
  failed += (rc != C_REST_OK | strcmp(buf, "a b") != 0);

  rc = c_rest_url_decode(buf, "a%", 2);
  failed += (rc != C_REST_OK | strcmp(buf, "a%") != 0);

  rc = c_rest_url_decode(buf, "a%2", 3);
  failed += (rc != C_REST_OK | strcmp(buf, "a%2") != 0);

  rc = c_rest_url_decode(NULL, "a%20b", 5);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_url_decode(buf, NULL, 5);
  failed += (rc != C_REST_ERROR_GENERIC);

  if (failed) {
    printf("test_str_utils failed: %d\n", failed);
  } else {
    printf("test_str_utils passed\n");
  }

  return failed;
}
