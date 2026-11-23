/* clang-format off */
#include "c_rest_str_utils.h"
#include "test_protos.h"
#include <string.h>
/* clang-format on */

TEST test_str_utils_case_and_copy(void) {
  int cmp = 0;
  char buf[32];
  size_t len = 0;

  ASSERT_EQ(C_REST_OK, c_rest_strcasecmp("Hello", "hello", &cmp));
  ASSERT_EQ(0, cmp);

  ASSERT_EQ(C_REST_OK, c_rest_strcasecmp("Hello", "hella", &cmp));
  ASSERT(cmp > 0);

  ASSERT_EQ(C_REST_OK, c_rest_strcasecmp("Hella", "hello", &cmp));
  ASSERT(cmp < 0);

  ASSERT_EQ(C_REST_OK, c_rest_strcasecmp("A", "", &cmp));
  ASSERT(cmp > 0);

  ASSERT_EQ(C_REST_OK, c_rest_strcasecmp("", "A", &cmp));
  ASSERT(cmp < 0);

  ASSERT_EQ(C_REST_OK, c_rest_strcasecmp("HelloA", "Hello", &cmp));
  ASSERT(cmp > 0);

  ASSERT_EQ(C_REST_OK, c_rest_strcasecmp("Hello", "HelloA", &cmp));
  ASSERT(cmp < 0);

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("HelloA", "Hello", 6, &cmp));
  ASSERT(cmp > 0);

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("Hello", "HelloA", 6, &cmp));
  ASSERT(cmp < 0);

  ASSERT_EQ(C_REST_OK, c_rest_strcasecmp("Helloa", "Hello", &cmp));
  ASSERT(cmp > 0);

  ASSERT_EQ(C_REST_OK, c_rest_strcasecmp("Hello", "Helloa", &cmp));
  ASSERT(cmp < 0);

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("Helloa", "Hello", 6, &cmp));
  ASSERT(cmp > 0);

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("Hello", "Helloa", 6, &cmp));
  ASSERT(cmp < 0);

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_strcasecmp(NULL, "hello", &cmp));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_strcasecmp("Hello", NULL, &cmp));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_strcasecmp("Hello", "hello", NULL));

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("HelloWorld", "helloWorld", 5, &cmp));
  ASSERT_EQ(0, cmp);

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("HelloW", "hellaW", 5, &cmp));
  ASSERT(cmp > 0);

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("HellaW", "helloW", 5, &cmp));
  ASSERT(cmp < 0);

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("A", "", 1, &cmp));
  ASSERT(cmp > 0);

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("", "A", 1, &cmp));
  ASSERT(cmp < 0);

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("Hello", "hello", 0, &cmp));
  ASSERT_EQ(0, cmp);

  ASSERT_EQ(C_REST_OK, c_rest_strncasecmp("Hello", "helloa", (size_t)-1, &cmp));
  ASSERT(cmp < 0);

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_strncasecmp(NULL, "helloWorld", 5, &cmp));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_strncasecmp("HelloWorld", NULL, 5, &cmp));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_strncasecmp("HelloWorld", "helloWorld", 5, NULL));

  ASSERT_EQ(C_REST_OK, c_rest_strlcpy(buf, "abc", sizeof(buf), &len));
  ASSERT_EQ((size_t)3, len);
  ASSERT_STR_EQ("abc", buf);

  ASSERT_EQ(C_REST_OK, c_rest_strlcpy(buf, "abcdef", 4, &len));
  ASSERT_EQ((size_t)6, len);
  ASSERT_STR_EQ("abc", buf);

  ASSERT_EQ(C_REST_OK, c_rest_strlcpy(buf, "abc", 0, &len));
  ASSERT_EQ((size_t)3, len);

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_strlcpy(NULL, "abc", sizeof(buf), &len));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_strlcpy(buf, NULL, sizeof(buf), &len));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_strlcpy(buf, "abc", sizeof(buf), NULL));

  buf[0] = '\0';
  ASSERT_EQ(C_REST_OK, c_rest_strlcat(buf, "def", sizeof(buf), &len));
  ASSERT_EQ((size_t)3, len);
  ASSERT_STR_EQ("def", buf);

  buf[0] = 'a';
  buf[1] = '\0';
  ASSERT_EQ(C_REST_OK, c_rest_strlcat(buf, "def", 2, &len));
  ASSERT_EQ((size_t)4, len);
  ASSERT_STR_EQ("a", buf);

  buf[0] = 'a';
  buf[1] = '\0';
  ASSERT_EQ(C_REST_OK, c_rest_strlcat(buf, "def", 3, &len));
  ASSERT_EQ((size_t)4, len);
  ASSERT_STR_EQ("ad", buf);

  buf[0] = 'a';
  buf[1] = 'b';
  buf[2] = '\0';
  ASSERT_EQ(C_REST_OK, c_rest_strlcat(buf, "def", 1, &len));
  ASSERT_EQ((size_t)4, len);

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_strlcat(buf, "def", sizeof(buf), NULL));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_strlcat(NULL, "def", sizeof(buf), &len));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_strlcat(buf, NULL, sizeof(buf), &len));

  ASSERT_EQ(C_REST_OK, c_rest_url_decode(buf, "a%20b", 5));
  ASSERT_STR_EQ("a b", buf);

  ASSERT_EQ(C_REST_OK, c_rest_url_decode(buf, "a+b", 3));
  ASSERT_STR_EQ("a b", buf);

  ASSERT_EQ(C_REST_OK, c_rest_url_decode(buf, "a%", 2));
  ASSERT_STR_EQ("a%", buf);

  ASSERT_EQ(C_REST_OK, c_rest_url_decode(buf, "a%2", 3));
  ASSERT_STR_EQ("a%2", buf);

  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_url_decode(NULL, "a%20b", 5));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_url_decode(buf, NULL, 5));

  ASSERT_EQ(C_REST_OK, c_rest_sprintf_s(buf, sizeof(buf), "hello %d", 42));
  ASSERT_STR_EQ("hello 42", buf);
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_sprintf_s(NULL, sizeof(buf), "hello"));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_sprintf_s(buf, 0, "hello"));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, c_rest_sprintf_s(buf, sizeof(buf), NULL));

  ASSERT_EQ(C_REST_OK, c_rest_strcpy_s(buf, sizeof(buf), "world"));
  ASSERT_STR_EQ("world", buf);

  ASSERT_EQ(C_REST_OK, c_rest_strncpy_s(buf, sizeof(buf), "testing", 4));
  buf[4] = '\0';
  ASSERT_MEM_EQ("test", buf, 4);

  PASS();
}

SUITE(str_utils_suite) { RUN_TEST(test_str_utils_case_and_copy); }
