/* clang-format off */
#include "c_rest_log.h"
#include "test_protos.h"
#include <stdio.h>
/* clang-format on */

int test_log(void) {
  LOG_DEBUG("This is a test log: %d", 42);
  printf("test_log passed\n");
  return 0;
}
