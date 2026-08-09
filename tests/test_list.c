/* clang-format off */
#include "c_rest_list.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

static int list_free_count = 0;
static void dummy_free_list(void *ptr) {
  if (ptr)
    list_free_count++;
}

static void *fail_malloc(size_t s) {
  (void)s;
  return NULL;
}
extern void *(*g_crf_malloc_hook)(size_t);

int test_list(void) {
  c_rest_list list;
  c_rest_error_t rc;
  void *val = NULL;
  int failed = 0;

  /* Null checks */
  rc = c_rest_list_init(NULL);
  failed += (rc == C_REST_OK);

  rc = c_rest_list_push_back(NULL, "item");
  failed += (rc == C_REST_OK);

  rc = c_rest_list_pop_front(NULL, &val);
  failed += (rc == C_REST_OK);

  rc = c_rest_list_pop_front(&list, NULL);
  failed += (rc == C_REST_OK);

  rc = c_rest_list_destroy(NULL, NULL);
  failed += (rc == C_REST_OK);

  g_crf_malloc_hook = fail_malloc;
  rc = c_rest_list_push_back(&list, "item");
  failed += (rc == C_REST_OK);
  g_crf_malloc_hook = NULL;

  /* Normal flow */
  rc = c_rest_list_init(&list);
  failed += (rc != C_REST_OK);

  rc = c_rest_list_push_back(&list, "item1");
  failed += (rc != C_REST_OK);

  rc = c_rest_list_push_back(&list, "item2");
  failed += (rc != C_REST_OK);

  rc = c_rest_list_pop_front(&list, &val);
  failed += (rc != C_REST_OK || strcmp((const char *)val, "item1") != 0);

  rc = c_rest_list_pop_front(&list, &val);
  failed += (rc != C_REST_OK || strcmp((const char *)val, "item2") != 0);

  rc = c_rest_list_pop_front(&list, &val);
  failed += (rc == C_REST_OK || val != NULL); /* Should fail, list is empty */

  rc = c_rest_list_destroy(&list, NULL);
  failed += (rc != C_REST_OK);

  /* Destroy non-empty list with NULL free func */
  rc = c_rest_list_init(&list);
  failed += (rc != C_REST_OK);
  rc = c_rest_list_push_back(&list, "item3");
  failed += (rc != C_REST_OK);
  rc = c_rest_list_destroy(&list, NULL);
  failed += (rc != C_REST_OK);

  /* Test destruction with free function */
  list_free_count = 0;
  rc = c_rest_list_init(&list);
  failed += (rc != C_REST_OK);
  rc = c_rest_list_push_back(&list, "item1");
  failed += (rc != C_REST_OK);
  rc = c_rest_list_destroy(&list, dummy_free_list);
  failed += (rc != C_REST_OK);
  failed += (list_free_count != 1);

  /* Destruction with free_data and null data */
  list_free_count = 0;
  rc = c_rest_list_init(&list);
  failed += (rc != C_REST_OK);
  rc = c_rest_list_push_back(&list, NULL);
  failed += (rc != C_REST_OK);
  rc = c_rest_list_destroy(&list, dummy_free_list);
  failed += (rc != C_REST_OK);
  failed += (list_free_count != 0);

  if (failed) {
    printf("test_list failed\n");
  } else {
    printf("test_list passed\n");
  }

  return failed;
}
