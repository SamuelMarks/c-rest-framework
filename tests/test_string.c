/* clang-format off */
#include "c_rest_string.h"
#include "test_protos.h"
#include <string.h>
/* clang-format on */

static void *failing_malloc_hook(size_t size) {
  (void)size;
  return NULL;
}

static void *failing_realloc_hook(void *ptr, size_t size) {
  (void)ptr;
  (void)size;
  return NULL;
}

TEST test_string_operations_and_oom(void) {
  c_rest_string str;
  c_rest_string empty_str;
  char large_buf[100];
  int i;

  ASSERT_NEQ(C_REST_OK, c_rest_string_init(NULL, 16));

  g_crf_malloc_hook = failing_malloc_hook;
  ASSERT_NEQ(C_REST_OK, c_rest_string_init(&str, 16));
  g_crf_malloc_hook = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_string_init(&str, 0));
  ASSERT_EQ((size_t)16, str.capacity);
  ASSERT_EQ(C_REST_OK, c_rest_string_destroy(&str));

  ASSERT_EQ(C_REST_OK, c_rest_string_init(&str, 16));

  g_crf_realloc_hook = failing_realloc_hook;
  ASSERT_NEQ(C_REST_OK, c_rest_string_append(&str, "01234567890123456789", 20));
  g_crf_realloc_hook = NULL;

  g_crf_realloc_hook = failing_realloc_hook;
  ASSERT_NEQ(C_REST_OK,
             c_rest_string_append(&str, "a", ((size_t)-1) - str.length - 1));
  g_crf_realloc_hook = NULL;

  ASSERT_NEQ(C_REST_OK, c_rest_string_append(&str, "a", ((size_t)-1)));
  ASSERT_NEQ(C_REST_OK, c_rest_string_append(NULL, "data", 4));
  ASSERT_NEQ(C_REST_OK, c_rest_string_append(&str, NULL, 4));
  ASSERT_NEQ(C_REST_OK, c_rest_string_append(&str, "data", 0));

  ASSERT_NEQ(C_REST_OK, c_rest_string_append_cstr(NULL, "hello"));
  ASSERT_NEQ(C_REST_OK, c_rest_string_append_cstr(&str, NULL));

  ASSERT_EQ(C_REST_OK, c_rest_string_append_cstr(&str, "hello"));
  ASSERT_EQ(C_REST_OK, c_rest_string_append(&str, " world", 6));
  ASSERT_STR_EQ("hello world", str.data);

  for (i = 0; i < 99; i++) {
    large_buf[i] = 'a';
  }
  large_buf[99] = '\0';

  ASSERT_EQ(C_REST_OK, c_rest_string_append_cstr(&str, large_buf));
  ASSERT(str.capacity >= str.length + 1);

  ASSERT_NEQ(C_REST_OK, c_rest_string_destroy(NULL));

  empty_str.data = NULL;
  empty_str.length = 0;
  empty_str.capacity = 0;
  ASSERT_EQ(C_REST_OK, c_rest_string_destroy(&empty_str));

  ASSERT_EQ(C_REST_OK, c_rest_string_destroy(&str));

  str.data = NULL;
  str.capacity = 0;
  str.length = 0;
  ASSERT_EQ(C_REST_OK, c_rest_string_append_cstr(&str, "test"));
  ASSERT_EQ((size_t)16, str.capacity);
  ASSERT_EQ(C_REST_OK, c_rest_string_destroy(&str));

  str.data = NULL;
  str.capacity = ((size_t)-1) / 2 + 1;
  str.length = str.capacity - 1;
  g_crf_realloc_hook = failing_realloc_hook;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_string_append_cstr(&str, "test"));
  g_crf_realloc_hook = NULL;

  PASS();
}

SUITE(string_suite) { RUN_TEST(test_string_operations_and_oom); }
