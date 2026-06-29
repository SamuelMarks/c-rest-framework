/* clang-format off */
#include "c_rest_hashmap.h"
#include "test_protos.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* clang-format on */

int test_hashmap(void) {
  c_rest_hashmap map;
  c_rest_error_t rc;
  void *val = NULL;
  int failed = 0;

  rc = c_rest_hashmap_init(&map, 16);
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_hashmap_put(&map, "key1", "value1");
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_hashmap_put(&map, "key2", "value2");
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_hashmap_get(&map, "key1", &val);
  if (rc != C_REST_OK || strcmp((const char *)val, "value1") != 0)
    failed++;

  rc = c_rest_hashmap_get(&map, "nonexistent", &val);
  if (rc == C_REST_OK)
    failed++; /* Should fail */

  rc = c_rest_hashmap_remove(&map, "key1");
  if (rc != C_REST_OK)
    failed++;

  rc = c_rest_hashmap_get(&map, "key1", &val);
  if (rc == C_REST_OK)
    failed++; /* Should fail */

  rc = c_rest_hashmap_destroy(&map, NULL);
  if (rc != C_REST_OK)
    failed++;

  if (failed) {
    printf("test_hashmap failed\n");
  } else {
    printf("test_hashmap passed\n");
  }

  return failed;
}
