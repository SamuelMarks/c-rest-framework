/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_base64.h"
#include <string.h>
/* clang-format on */

TEST test_base64_encode_decode(void) {
  const char *str = "Hello World!";
  const char *str2 = "Hello World";
  const char *str3 = "Hello Worl";
  const char *b64_str = "SGVsbG8gV29ybGQh";
  unsigned char buf[64];
  char b64[64];
  size_t len;

  ASSERT_NEQ(C_REST_OK, c_rest_base64_encode((const unsigned char *)str,
                                             strlen(str), b64, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_base64_decode(NULL, 10, buf, &len));
  ASSERT_NEQ(C_REST_OK,
             c_rest_base64_decode(b64_str, strlen(b64_str), buf, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_base64_decode("abc", 3, buf, &len));

  len = 1;
  ASSERT_NEQ(C_REST_OK,
             c_rest_base64_decode(b64_str, strlen(b64_str), buf, &len));

  ASSERT_EQ(C_REST_OK, c_rest_base64_encode((const unsigned char *)str,
                                            strlen(str), NULL, &len));
  ASSERT_EQ(C_REST_OK, c_rest_base64url_encode((const unsigned char *)str,
                                               strlen(str), NULL, &len));
  ASSERT_EQ(C_REST_OK,
            c_rest_base64_decode(b64_str, strlen(b64_str), NULL, &len));

  len = sizeof(b64);
  ASSERT_EQ(C_REST_OK, c_rest_base64_encode((const unsigned char *)str,
                                            strlen(str), b64, &len));
  ASSERT_STR_EQ("SGVsbG8gV29ybGQh", b64);

  len = sizeof(b64);
  ASSERT_EQ(C_REST_OK, c_rest_base64_encode((const unsigned char *)str2,
                                            strlen(str2), b64, &len));
  ASSERT_STR_EQ("SGVsbG8gV29ybGQ=", b64);

  len = sizeof(b64);
  ASSERT_EQ(C_REST_OK, c_rest_base64_encode((const unsigned char *)str3,
                                            strlen(str3), b64, &len));
  ASSERT_STR_EQ("SGVsbG8gV29ybA==", b64);

  len = sizeof(b64);
  ASSERT_EQ(C_REST_OK, c_rest_base64_encode((const unsigned char *)str,
                                            strlen(str), b64, &len));
  ASSERT_STR_EQ("SGVsbG8gV29ybGQh", b64);

  len = sizeof(buf);
  ASSERT_EQ(C_REST_OK, c_rest_base64_decode(b64, strlen(b64), buf, &len));
  ASSERT_EQ(strlen(str), len);
  ASSERT_MEM_EQ(str, buf, len);

  len = sizeof(buf);
  ASSERT_EQ(C_REST_OK, c_rest_base64_decode("SGVsbG8gV29ybGQ=", 16, buf, &len));
  ASSERT_EQ(strlen(str2), len);
  ASSERT_MEM_EQ(str2, buf, len);

  len = sizeof(buf);
  ASSERT_EQ(C_REST_OK, c_rest_base64_decode("SGVsbG8gV29ybA==", 16, buf, &len));
  ASSERT_EQ(strlen(str3), len);
  ASSERT_MEM_EQ(str3, buf, len);

  len = sizeof(b64);
  ASSERT_EQ(C_REST_OK, c_rest_base64url_encode((const unsigned char *)str,
                                               strlen(str), b64, &len));
  ASSERT_STR_EQ("SGVsbG8gV29ybGQh", b64);

  len = sizeof(b64);
  ASSERT_EQ(C_REST_OK,
            c_rest_base64url_encode((const unsigned char *)"\xfa\xef\xfe", 3,
                                    b64, &len));
  ASSERT_STR_EQ("-u_-", b64);

  len = sizeof(b64);
  ASSERT_EQ(C_REST_OK,
            c_rest_base64_encode((const unsigned char *)"\xfa\xef\xfe", 3, b64,
                                 &len));
  ASSERT_STR_EQ("+u/+", b64);

  len = sizeof(buf);
  ASSERT_NEQ(C_REST_OK, c_rest_base64url_decode(b64, strlen(b64), buf, &len));

  len = sizeof(buf);
  ASSERT_EQ(C_REST_OK, c_rest_base64url_decode("-u_-", 4, buf, &len));
  ASSERT_MEM_EQ("\xfa\xef\xfe", buf, 3);

  len = sizeof(buf);
  ASSERT_EQ(C_REST_OK, c_rest_base64_decode("+u/+", 4, buf, &len));
  ASSERT_MEM_EQ("\xfa\xef\xfe", buf, 3);

  len = sizeof(buf);
  ASSERT_EQ(C_REST_OK, c_rest_base64_decode("0123", 4, buf, &len));

  len = sizeof(buf);
  ASSERT_NEQ(C_REST_OK, c_rest_base64_decode("=abc", 4, buf, &len));
  len = sizeof(buf);
  ASSERT_NEQ(C_REST_OK, c_rest_base64_decode("a=bc", 4, buf, &len));
  len = sizeof(buf);
  ASSERT_NEQ(C_REST_OK, c_rest_base64_decode("SGVsb*8g", 8, buf, &len));
  len = sizeof(buf);
  ASSERT_NEQ(C_REST_OK, c_rest_base64_decode("SGVsbG8*", 8, buf, &len));
  len = sizeof(buf);
  ASSERT_NEQ(C_REST_OK, c_rest_base64_decode("SGVsbG8~", 8, buf, &len));

  len = sizeof(buf);
  ASSERT_EQ(C_REST_OK, c_rest_base64_decode("", 0, buf, &len));
  ASSERT_EQ((size_t)0, len);

  len = sizeof(buf);
  ASSERT_NEQ(C_REST_OK, c_rest_base64url_decode("a", 1, buf, &len));
  len = sizeof(buf);
  ASSERT_NEQ(C_REST_OK, c_rest_base64url_decode("=", 1, buf, &len));
  len = sizeof(buf);
  ASSERT_NEQ(C_REST_OK, c_rest_base64url_decode("-u/+", 4, buf, &len));

  PASS();
}

SUITE(base64_suite) { RUN_TEST(test_base64_encode_decode); }
