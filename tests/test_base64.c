/* clang-format off */
#include "test_protos.h"

#include <stdio.h>
#include <string.h>

#include "c_rest_base64.h"
/* clang-format on */

int test_base64(void) {
  const char *str = "Hello World!";
  unsigned char buf[64];
  char b64[64];
  size_t len;
  int rc;

  printf("Running test_base64...\n");

  /* Encode */
  len = sizeof(b64);
  rc = c_rest_base64_encode((const unsigned char *)str, strlen(str), b64, &len);
  if (rc != 0) {
    printf("c_rest_base64_encode failed with %d\n", rc);
    return 1;
  }
  if (strcmp(b64, "SGVsbG8gV29ybGQh") != 0) {
    printf("c_rest_base64_encode returned %s, expected SGVsbG8gV29ybGQh\n",
           b64);
    return 1;
  }

  /* Decode */
  len = sizeof(buf);
  rc = c_rest_base64_decode(b64, strlen(b64), buf, &len);
  if (rc != 0) {
    printf("c_rest_base64_decode failed with %d\n", rc);
    return 1;
  }
  if (len != strlen(str) || memcmp(buf, str, len) != 0) {
    printf("c_rest_base64_decode mismatch\n");
    return 1;
  }

  /* URL Encode */
  len = sizeof(b64);
  rc = c_rest_base64url_encode((const unsigned char *)str, strlen(str), b64,
                               &len);
  if (rc != 0) {
    printf("c_rest_base64url_encode failed with %d\n", rc);
    return 1;
  }
  if (strcmp(b64, "SGVsbG8gV29ybGQh") != 0) {
    printf("c_rest_base64url_encode mismatch: %s\n", b64);
    return 1;
  }

  /* URL Decode */
  len = sizeof(buf);
  rc = c_rest_base64url_decode(b64, strlen(b64), buf, &len);
  if (rc != 0) {
    printf("c_rest_base64url_decode failed with %d\n", rc);
    return 1;
  }
  if (len != strlen(str) || memcmp(buf, str, len) != 0) {
    printf("c_rest_base64url_decode mismatch\n");
    return 1;
  }

  printf("test_base64 passed.\n");
  return 0;
}
