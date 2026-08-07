/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"

#include <stdio.h>
#include <string.h>

#include "c_rest_base64.h"
/* clang-format on */

int test_base64(void) {
  const char *str = "Hello World!";
  const char *str2 = "Hello World";
  const char *str3 = "Hello Worl";
  const char *b64_str = "SGVsbG8gV29ybGQh";
  unsigned char buf[64];
  char b64[64];
  size_t len;
  int rc;
  int failed = 0;
  const char *msgs[2];

  printf("Running test_base64...\n");

  /* Null checks */
  rc = c_rest_base64_encode((const unsigned char *)str, strlen(str), b64, NULL);
  failed += (rc == C_REST_OK);

  rc = c_rest_base64_decode(NULL, 10, buf, &len);
  failed += (rc == C_REST_OK);

  rc = c_rest_base64_decode(b64_str, strlen(b64_str), buf, NULL);
  failed += (rc == C_REST_OK);

  /* Decode bad length (not % 4) */
  rc = c_rest_base64_decode("abc", 3, buf, &len);
  failed += (rc == C_REST_OK);

  /* Decode buffer too small */
  len = 1;
  rc = c_rest_base64_decode(b64_str, strlen(b64_str), buf, &len);
  failed += (rc == C_REST_OK);

  /* Test sizes */
  rc =
      c_rest_base64_encode((const unsigned char *)str, strlen(str), NULL, &len);
  failed += (rc != C_REST_OK);

  rc = c_rest_base64url_encode((const unsigned char *)str, strlen(str), NULL,
                               &len);
  failed += (rc != C_REST_OK);

  rc = c_rest_base64_decode(b64_str, strlen(b64_str), NULL, &len);
  failed += (rc != C_REST_OK);

  /* Test padding lengths for encode */
  /* len % 3 == 0 (Hello World!) */
  len = sizeof(b64);
  rc = c_rest_base64_encode((const unsigned char *)str, strlen(str), b64, &len);
  failed += (rc != C_REST_OK);
  failed += (strcmp(b64, "SGVsbG8gV29ybGQh") != 0);

  /* len % 3 == 2 (Hello World) */
  len = sizeof(b64);
  rc = c_rest_base64_encode((const unsigned char *)str2, strlen(str2), b64,
                            &len);
  failed += (rc != C_REST_OK);
  failed += (strcmp(b64, "SGVsbG8gV29ybGQ=") != 0);

  /* len % 3 == 1 (Hello Worl) */
  len = sizeof(b64);
  rc = c_rest_base64_encode((const unsigned char *)str3, strlen(str3), b64,
                            &len);
  failed += (rc != C_REST_OK);
  failed += (strcmp(b64, "SGVsbG8gV29ybA==") != 0);

  /* Encode */
  len = sizeof(b64);
  rc = c_rest_base64_encode((const unsigned char *)str, strlen(str), b64, &len);
  failed += (rc != C_REST_OK);
  failed += (strcmp(b64, "SGVsbG8gV29ybGQh") != 0);

  /* Decode */
  len = sizeof(buf);
  rc = c_rest_base64_decode(b64, strlen(b64), buf, &len);
  failed += (rc != C_REST_OK);
  failed += (len != strlen(str));
  failed += (memcmp(buf, str, len) != 0);

  /* Decode with 1 padding */
  len = sizeof(buf);
  rc = c_rest_base64_decode("SGVsbG8gV29ybGQ=", 16, buf, &len);
  failed += (rc != C_REST_OK);
  failed += (len != strlen(str2));
  failed += (memcmp(buf, str2, len) != 0);

  /* Decode with 2 padding */
  len = sizeof(buf);
  rc = c_rest_base64_decode("SGVsbG8gV29ybA==", 16, buf, &len);
  failed += (rc != C_REST_OK);
  failed += (len != strlen(str3));
  failed += (memcmp(buf, str3, len) != 0);

  /* URL Encode */
  len = sizeof(b64);
  rc = c_rest_base64url_encode((const unsigned char *)str, strlen(str), b64,
                               &len);
  failed += (rc != C_REST_OK);
  failed += (strcmp(b64, "SGVsbG8gV29ybGQh") != 0);

  /* URL Encode special chars */
  len = sizeof(b64);
  rc = c_rest_base64url_encode((const unsigned char *)"\xfa\xef\xfe", 3, b64,
                               &len);
  failed += (rc != C_REST_OK);
  failed += (strcmp(b64, "-u_-") != 0);

  len = sizeof(b64);
  rc =
      c_rest_base64_encode((const unsigned char *)"\xfa\xef\xfe", 3, b64, &len);
  failed += (rc != C_REST_OK);
  failed += (strcmp(b64, "+u/+ ") == 0); /* Just checking they differ */

  /* URL Decode */
  len = sizeof(buf);
  rc = c_rest_base64url_decode(b64, strlen(b64), buf, &len);

  len = sizeof(buf);
  rc = c_rest_base64url_decode("-u_-", 4, buf, &len);
  failed += (rc != C_REST_OK);
  failed += (memcmp(buf, "\xfa\xef\xfe", 3) != 0);

  len = sizeof(buf);
  rc = c_rest_base64_decode("+u/+", 4, buf, &len);
  failed += (rc != C_REST_OK);
  failed += (memcmp(buf, "\xfa\xef\xfe", 3) != 0);

  /* Decode special chars error cases handling (invalid chars) */
  len = sizeof(buf);
  rc = c_rest_base64_decode("0123", 4, buf, &len);
  failed += (rc != C_REST_OK);

  /* Test malformed base64 with '=' at position 0 */
  len = sizeof(buf);
  rc = c_rest_base64_decode("=abc", 4, buf, &len);
  failed += (rc == C_REST_OK);

  /* Test malformed base64 with '=' at position 1 */
  len = sizeof(buf);
  rc = c_rest_base64_decode("a=bc", 4, buf, &len);
  failed += (rc == C_REST_OK);

  /* Test invalid character in standard decode */
  len = sizeof(buf);
  rc = c_rest_base64_decode("SGVsb*8g", 8, buf, &len);
  failed += (rc == C_REST_OK);

  /* Test invalid character at val_d (8th character) */
  len = sizeof(buf);
  rc = c_rest_base64_decode("SGVsbG8*", 8, buf, &len);
  failed += (rc == C_REST_OK);

  /* Test invalid character > 'z' but > 'a' e.g. '~' */
  len = sizeof(buf);
  rc = c_rest_base64_decode("SGVsbG8~", 8, buf, &len);
  failed += (rc == C_REST_OK);

  /* Test empty string */
  len = sizeof(buf);
  rc = c_rest_base64_decode("", 0, buf, &len);
  failed += (rc != C_REST_OK);
  failed += (len != 0);

  /* Test 1-character URL decode */
  len = sizeof(buf);
  rc = c_rest_base64url_decode("a", 1, buf, &len);
  failed += (rc == C_REST_OK);

  /* Test 1-character URL decode with '=' */
  len = sizeof(buf);
  rc = c_rest_base64url_decode("=", 1, buf, &len);
  failed += (rc == C_REST_OK);

  /* Test invalid character in url decode */
  len = sizeof(buf);
  rc = c_rest_base64url_decode("-u/+", 4, buf, &len);
  failed += (rc == C_REST_OK);

  msgs[0] = "test_base64 passed.\n";
  msgs[1] = "test_base64 failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}
