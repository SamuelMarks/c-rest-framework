/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/* clang-format on */

static c_rest_error_t my_log_cb(const char *message) {
  printf("[THREADED] %s\n", message);
  return C_REST_OK;
}

static c_rest_error_t handle_db_query(struct c_rest_request *req,
                                      struct c_rest_response *res,
                                      void *user_data) {
  (void)req;
  (void)user_data;
  /* Simulate a blocking DB query */
  (void)!c_rest_response_json(res, "{\"status\": \"success\", \"rows\": 42}");
  return 0;
}

static void sig_handler(int sig) {
  (void)sig;
  exit(0);
}

int main(void) {

  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  c_rest_error_t rc;

  printf("Initializing Threaded Synchronous Application...\n");

  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);
  rc = c_rest_init(C_REST_MODALITY_MULTI_THREAD, &ctx);
  if (rc != 0) {
    fprintf(stderr, "Failed to initialize framework.\n");
    return 1;
  }

  ctx->logger.log_cb = my_log_cb;

  rc = c_rest_router_init(&router);
  if (rc != 0) {
    fprintf(stderr, "Failed to initialize router.\n");
    (void)!c_rest_destroy(ctx);
    return 1;
  }

  (void)!c_rest_set_router(ctx, router);
  (void)!c_rest_router_add(router, "GET", "/api/v0/db/query", handle_db_query,
                           NULL);

  printf("Starting framework thread pool...\n");
  rc = c_rest_run(ctx);
  if (rc != 0) {
    fprintf(stderr, "Framework runtime error.\n");
  }

  printf("Shutting down...\n");
  (void)!c_rest_router_destroy(router);
  (void)!c_rest_destroy(ctx);

  return 0;
}
