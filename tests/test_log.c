/* clang-format off */
#include "c_rest_log.h"
#include "test_protos.h"
#include <stdio.h>
/* clang-format on */

int test_log(void) {
  c_rest_error_t rc;
  int failed = 0;

  LOG_DEBUG("This is a test log: %d", 42);
  rc = c_rest_framework_log_debug(
      "Direct call to c_rest_framework_log_debug: %d", 43);
  failed += (rc != C_REST_OK);

  if (failed) {
    printf("test_log failed\n");
  } else {
    printf("test_log passed\n");
  }
  return failed;
}
