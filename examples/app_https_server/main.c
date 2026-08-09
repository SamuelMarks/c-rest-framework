#include <signal.h>
/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_tls.h"
#include <stdio.h>
/* clang-format on */

static c_rest_error_t hello_handler(struct c_rest_request *req,
                                    struct c_rest_response *res,
                                    void *user_data) {
  (void)req;
  (void)user_data;
  (void)!c_rest_response_html(res, "<h1>Hello Secure World!</h1>");
  return 0;
}

static void sig_handler(int sig) {
  (void)sig;
  exit(0);
}

int main(void) {

  struct c_rest_context *ctx = NULL;
  struct c_rest_tls_context *tls_ctx = NULL;
  c_rest_router *router = NULL;
  int res;

  if (c_rest_tls_init() != 0) {
    printf("No TLS backend available, skipping.\n");
    return 0;
  }

  res = c_rest_tls_context_init(&tls_ctx);
  if (res != 0) {
    printf("TLS context init failed.\n");
    return 1;
  }

  /* Assuming these certs exist alongside the binary */
  if (c_rest_tls_load_cert(tls_ctx, "server.crt") != 0 ||
      c_rest_tls_load_key(tls_ctx, "server.key") != 0) {
    printf("Warning: Could not load server.crt/server.key.\n");
  }

  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);
  res = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (res != 0) {
    printf("Failed to initialize framework.\n");
    (void)!c_rest_tls_context_destroy(tls_ctx);
    return 1;
  }
  ctx->tls_ctx = tls_ctx;

  (void)!c_rest_router_init(&router);
  (void)!c_rest_set_router(ctx, router);
  (void)!c_rest_router_add(router, "GET", "/api/v0/", hello_handler, NULL);

  /* In an actual implementation, a router should be bound to ctx.
   * e.g., ctx->router = router;
   */

  printf("Starting secure server on port 443... (Simulation)\n");
  /* c_rest_run(ctx); */

  (void)!c_rest_destroy(ctx);
  (void)!c_rest_tls_context_destroy(tls_ctx);
  (void)!c_rest_router_destroy(router);

  return 0;
}
