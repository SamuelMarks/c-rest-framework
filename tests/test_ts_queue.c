/* clang-format off */
#include "c_rest_ts_queue.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

int test_ts_queue(void) {
  c_rest_ts_queue q;
  int failed = 0;
  void *val = NULL;
  c_rest_error_t rc;

  rc = c_rest_ts_queue_init(&q);
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_ts_queue_push(&q, "test1");
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_ts_queue_pop(&q, &val);
  if (rc != C_REST_OK || strcmp((const char *)val, "test1") != 0)
    failed++;

  rc = c_rest_ts_queue_close(&q);
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_ts_queue_destroy(&q, NULL);
  if (rc != C_REST_OK)
    failed++;

  if (failed) {
    printf("test_ts_queue failed\n");
  } else {
    printf("test_ts_queue passed\n");
  }

  return failed;
}
