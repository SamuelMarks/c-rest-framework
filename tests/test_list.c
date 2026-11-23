/* clang-format off */
#include "c_rest_list.h"
#include "test_protos.h"
#include <string.h>
/* clang-format on */

static int list_free_count = 0;
static c_rest_error_t dummy_free_list(void *ptr) {
  (void)ptr;
  list_free_count++;
  return C_REST_OK;
}

static c_rest_error_t fail_free_list(void *ptr) {
  (void)ptr;
  list_free_count++;
  return C_REST_ERROR_IO;
}

static void *fail_malloc(size_t s) {
  (void)s;
  return NULL;
}

TEST test_list_operations(void) {
  c_rest_list list;
  void *val = NULL;
  char item1[] = "item1";
  char item2[] = "item2";
  char item3[] = "item3";

  ASSERT_NEQ(C_REST_OK, c_rest_list_init(NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_list_push_back(NULL, item1));
  ASSERT_NEQ(C_REST_OK, c_rest_list_pop_front(NULL, &val));
  ASSERT_NEQ(C_REST_OK, c_rest_list_pop_front(&list, NULL));
  ASSERT_NEQ(C_REST_OK, c_rest_list_destroy(NULL, NULL));

  ASSERT_EQ(C_REST_OK, c_rest_list_init(&list));
  g_crf_malloc_hook = fail_malloc;
  ASSERT_NEQ(C_REST_OK, c_rest_list_push_back(&list, item1));
  g_crf_malloc_hook = NULL;

  ASSERT_EQ(C_REST_OK, c_rest_list_push_back(&list, item1));
  ASSERT_EQ(C_REST_OK, c_rest_list_push_back(&list, item2));

  ASSERT_EQ(C_REST_OK, c_rest_list_pop_front(&list, &val));
  ASSERT_STR_EQ("item1", (const char *)val);

  ASSERT_EQ(C_REST_OK, c_rest_list_pop_front(&list, &val));
  ASSERT_STR_EQ("item2", (const char *)val);

  ASSERT_NEQ(C_REST_OK, c_rest_list_pop_front(&list, &val));
  ASSERT_EQ(NULL, val);

  ASSERT_EQ(C_REST_OK, c_rest_list_destroy(&list, NULL));

  ASSERT_EQ(C_REST_OK, c_rest_list_init(&list));
  ASSERT_EQ(C_REST_OK, c_rest_list_push_back(&list, item3));
  ASSERT_EQ(C_REST_OK, c_rest_list_destroy(&list, NULL));

  list_free_count = 0;
  ASSERT_EQ(C_REST_OK, c_rest_list_init(&list));
  ASSERT_EQ(C_REST_OK, c_rest_list_push_back(&list, item1));
  ASSERT_EQ(C_REST_OK, c_rest_list_destroy(&list, dummy_free_list));
  ASSERT_EQ(1, list_free_count);

  list_free_count = 0;
  ASSERT_EQ(C_REST_OK, c_rest_list_init(&list));
  ASSERT_EQ(C_REST_OK, c_rest_list_push_back(&list, NULL));
  ASSERT_EQ(C_REST_OK, c_rest_list_destroy(&list, dummy_free_list));
  ASSERT_EQ(0, list_free_count);

  list_free_count = 0;
  ASSERT_EQ(C_REST_OK, c_rest_list_init(&list));
  ASSERT_EQ(C_REST_OK, c_rest_list_push_back(&list, item1));
  ASSERT_EQ(C_REST_OK, c_rest_list_push_back(&list, item2));
  ASSERT_EQ(C_REST_ERROR_IO, c_rest_list_destroy(&list, fail_free_list));
  ASSERT_EQ(2, list_free_count);

  PASS();
}

SUITE(list_suite) { RUN_TEST(test_list_operations); }
