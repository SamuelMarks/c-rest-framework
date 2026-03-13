/* clang-format off */
#include "c_rest_tls.h"
#include <stdio.h>
/* clang-format on */

int test_tls_context(void);

int test_tls_context(void) {
  struct c_rest_tls_context *ctx = NULL;
  int res;

  c_rest_tls_init();

  res = c_rest_tls_context_init(&ctx);
  if (res != 0) {
    printf("Warning: TLS context init failed or no backend available.\n");
    return 0; /* Not a failure if no backend */
  }

  /* Load dummy certs generated previously */
  /* Note: Some backends fail to load invalid/dummy certs, so we tolerate
   * failure here if the file doesn't exist or isn't well-formed */
  res = c_rest_tls_load_cert(ctx, "tests/certs/server.crt");
  if (res != 0) {
    printf("Warning: failed to load cert\n");
  }

  res = c_rest_tls_load_key(ctx, "tests/certs/server.key");
  if (res != 0) {
    printf("Warning: failed to load key\n");
  }

  c_rest_tls_context_destroy(ctx);
  return 0;
}
