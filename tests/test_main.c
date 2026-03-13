/* clang-format off */
#include <stdio.h>
#include <stdlib.h>

#include "c_rest_modality.h"
/* clang-format on */

static void test_logger(const char *msg) { printf("LOG: %s\n", msg); }

static int test_init_destroy(void) {
  struct c_rest_context *ctx = NULL;
  int res;

  res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (res != 0 || ctx == NULL) {
    printf("Failed to init C_REST_MODALITY_SYNC\n");
    return 1;
  }

  ctx->logger.log_cb = test_logger;

  res = c_rest_destroy(ctx);
  if (res != 0) {
    printf("Failed to destroy ctx\n");
    return 1;
  }

  return 0;
}

static int test_all_enums(void) {
  int i;
  int res;
  struct c_rest_context *ctx = NULL;

  for (i = 0; i <= C_REST_MODALITY_MESSAGE_PASSING; ++i) {
    res = c_rest_init((enum c_rest_modality_type)i, &ctx);
    if (res != 0 || ctx == NULL) {
      printf("Failed to init enum %d\n", i);
      return 1;
    }
    res = c_rest_destroy(ctx);
    if (res != 0) {
      printf("Failed to destroy enum %d\n", i);
      return 1;
    }
  }
  return 0;
}

int test_multiplatform_integration(void);
int test_parser(void);
int test_client(void);
int test_multipart(void);
int test_router(void);
int test_request_response(void);
int test_orm_integration(void);
int test_crypto(void);
int test_tls_context(void);
int test_tls_integration(void);

int main(void) {
  int res = 0;

  printf("Running test_init_destroy...\n");
  res = test_init_destroy();
  if (res != 0)
    return res;

  printf("Running test_all_enums...\n");
  res = test_all_enums();
  if (res != 0)
    return res;

  printf("Running test_multiplatform_integration...\n");
  res = test_multiplatform_integration();
  if (res != 0)
    return res;

  printf("Running test_parser...\n");
  res = test_parser();
  if (res != 0)
    return res;

  printf("Running test_client...\n");
  res = test_client();
  if (res != 0)
    return res;

  printf("Running test_multipart...\n");
  res = test_multipart();
  if (res != 0)
    return res;

  printf("Running test_router...\n");
  res = test_router();
  if (res != 0)
    return res;

  printf("Running test_request_response...\n");
  res = test_request_response();
  if (res != 0)
    return res;

  printf("Running test_orm_integration...\n");
  res = test_orm_integration();
  if (res != 0)
    return res;

  printf("Running test_crypto...\n");
  res = test_crypto();
  if (res != 0)
    return res;

  printf("Running test_tls_context...\n");
  res = test_tls_context();
  if (res != 0)
    return res;

  printf("Running test_tls_integration...\n");
  res = test_tls_integration();
  if (res != 0)
    return res;

  printf("All tests passed.\n");
  return 0;
}
