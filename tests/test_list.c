/* clang-format off */
#include "c_rest_list.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

int test_list(void) {
  c_rest_list list;
  c_rest_error_t rc;
  void *val = NULL;
  int failed = 0;

  rc = c_rest_list_init(&list);
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_list_push_back(&list, "item1");
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_list_push_back(&list, "item2");
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_list_pop_front(&list, &val);
  if (rc != C_REST_OK || strcmp((const char *)val, "item1") != 0)
    failed++;

  rc = c_rest_list_pop_front(&list, &val);
  if (rc != C_REST_OK || strcmp((const char *)val, "item2") != 0)
    failed++;

  rc = c_rest_list_pop_front(&list, &val);
  if (rc == C_REST_OK)
    failed++; /* Should fail, list is empty */

  rc = c_rest_list_destroy(&list, NULL);
  if (rc != C_REST_OK)
    failed++;

  if (failed) {
    printf("test_list failed\n");
  } else {
    printf("test_list passed\n");
  }

  return failed;
}
