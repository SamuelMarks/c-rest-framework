/* clang-format off */
#include "c_rest_string.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

extern void *(*g_crf_malloc_hook)(size_t);
extern void *(*g_crf_realloc_hook)(void *, size_t);

static void *failing_malloc_hook(size_t size) {
  (void)size;
  return NULL;
}

static void *failing_realloc_hook(void *ptr, size_t size) {
  (void)ptr;
  (void)size;
  return NULL;
}

int test_string(void) {
  c_rest_string str;
  c_rest_string empty_str;
  c_rest_error_t rc;
  int failed = 0;
  char large_buf[100];
  int i;

  /* Test NULL pointer in init */
  rc = c_rest_string_init(NULL, 16);
  failed += (rc == C_REST_OK);

  /* Test malloc failure during init by using hook */
  g_crf_malloc_hook = failing_malloc_hook;
  rc = c_rest_string_init(&str, 16);
  g_crf_malloc_hook = NULL;
  failed += (rc == C_REST_OK);

  /* Test initial capacity 0 */
  rc = c_rest_string_init(&str, 0);
  failed += (rc != C_REST_OK | str.capacity != 16);
  (void)!c_rest_string_destroy(&str);

  /* Normal init */
  rc = c_rest_string_init(&str, 16);
  failed += (rc != C_REST_OK);

  /* Test realloc failure during append by using hook */
  g_crf_realloc_hook = failing_realloc_hook;
  /* append 20 chars to trigger realloc from 16 */
  rc = c_rest_string_append(&str, "01234567890123456789", 20);
  g_crf_realloc_hook = NULL;
  failed += (rc == C_REST_OK);

  /* Test realloc failure during append by appending an impossible length */
  /* This relies on (size_t)-1 - str.length - 1 passing the overflow check,
   * but the resulting allocation of (size_t)-1 bytes will fail. */
  g_crf_realloc_hook = failing_realloc_hook;
  rc = c_rest_string_append(&str, "a", ((size_t)-1) - str.length - 1);
  g_crf_realloc_hook = NULL;
  failed += (rc == C_REST_OK);

  /* Test integer overflow check in append */
  rc = c_rest_string_append(&str, "a", ((size_t)-1));
  failed += (rc == C_REST_OK);

  /* Test NULL pointer in append */
  rc = c_rest_string_append(NULL, "data", 4);
  failed += (rc == C_REST_OK);
  rc = c_rest_string_append(&str, NULL, 4);
  failed += (rc == C_REST_OK);
  rc = c_rest_string_append(&str, "data", 0);
  failed += (rc == C_REST_OK);

  /* Test NULL pointer in append_cstr */
  rc = c_rest_string_append_cstr(NULL, "hello");
  failed += (rc == C_REST_OK);
  rc = c_rest_string_append_cstr(&str, NULL);
  failed += (rc == C_REST_OK);

  rc = c_rest_string_append_cstr(&str, "hello");
  failed += (rc != C_REST_OK);

  rc = c_rest_string_append(&str, " world", 6);
  failed += (rc != C_REST_OK);
  failed += (strcmp(str.data, "hello world") != 0);

  /* Force reallocation: string capacity is 16, current length is 11 */
  for (i = 0; i < 99; i++) {
    large_buf[i] = 'a';
  }
  large_buf[99] = '\0';

  /* This will trigger the first realloc and the while loop for new_cap */
  rc = c_rest_string_append_cstr(&str, large_buf);
  failed += (rc != C_REST_OK);
  failed += (str.capacity < str.length + 1);

  /* Test destroy */
  rc = c_rest_string_destroy(NULL);
  failed += (rc == C_REST_OK);

  empty_str.data = NULL;
  empty_str.length = 0;
  empty_str.capacity = 0;
  rc = c_rest_string_destroy(&empty_str);
  failed += (rc != C_REST_OK);

  rc = c_rest_string_destroy(&str);
  failed += (rc != C_REST_OK);

  /* Test zero capacity doubling fallback when capacity = 0 */
  str.data = NULL;
  str.capacity = 0;
  str.length = 0;
  rc = c_rest_string_append_cstr(&str, "test");
  failed += (rc != C_REST_OK);
  failed += (str.capacity != 16);
  (void)!c_rest_string_destroy(&str);

  /* Test capacity overflow */
  str.data = NULL;
  str.capacity = ((size_t)-1) / 2 + 1;
  str.length = str.capacity - 1;
  g_crf_realloc_hook = failing_realloc_hook;
  printf("Before c_rest_string_append_cstr\n");
  rc = c_rest_string_append_cstr(&str, "test");
  printf("After c_rest_string_append_cstr\n");
  g_crf_realloc_hook = NULL;
  failed += (rc != C_REST_ERROR_GENERIC);

  if (failed) {

    printf("test_string failed\n");
  } else {
    printf("test_string passed\n");
  }
  return failed;
}
