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
  printf("[MP] %s\n", message);
  return C_REST_OK;
}

static c_rest_error_t handle_work(struct c_rest_request *req,
                                  struct c_rest_response *res,
                                  void *user_data) {
  (void)req;
  (void)user_data;
  return c_rest_response_json(res, "{\"worker_id\": 1, \"status\": \"done\"}");
}

static void sig_handler(int sig) {
  (void)sig;
  exit(0);
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  c_rest_error_t rc;

  printf("Initializing Multi-Process Prefork Application...\n");

  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);
  rc = c_rest_init(C_REST_MODALITY_MULTI_PROCESS, &ctx);
  if (rc != 0) {
    fprintf(stderr, "Failed to initialize framework.\n");
    return 1;
  }

  ctx->logger.log_cb = my_log_cb;

  rc = c_rest_router_init(&router);
  if (rc != 0) {
    fprintf(stderr, "Failed to initialize router.\n");
    c_rest_destroy(ctx);
    return 1;
  }

  rc = c_rest_set_router(ctx, router);
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    c_rest_destroy(ctx);
    return 1;
  }
  rc = c_rest_router_add(router, "GET", "/api/v0/work", handle_work, NULL);
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    c_rest_destroy(ctx);
    return 1;
  }

  printf("Forking workers and starting master loop...\n");
  rc = c_rest_run(ctx);
  if (rc != 0) {
    fprintf(stderr, "Framework runtime error.\n");
  }

  printf("Shutting down...\n");
  rc = c_rest_router_destroy(router);
  if (rc != C_REST_OK) {
    c_rest_destroy(ctx);
    return 1;
  }
  rc = c_rest_destroy(ctx);
  if (rc != C_REST_OK)
    return 1;

  return 0;
}
