/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"

#include <stdio.h>
#include <stdlib.h>
/* clang-format on */

static void my_log_cb(const char *message) { printf("[MP] %s\n", message); }

static void handle_work(struct c_rest_request *req, struct c_rest_response *res,
                        void *user_data) {
  (void)req;
  (void)user_data;
  c_rest_response_json(res, "{\"worker_id\": 1, \"status\": \"done\"}");
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  int rc;

  printf("Initializing Multi-Process Prefork Application...\n");

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

  c_rest_router_add(router, "GET", "/work", handle_work, NULL);

  printf("Forking workers and starting master loop...\n");
  rc = c_rest_run(ctx);
  if (rc != 0) {
    fprintf(stderr, "Framework runtime error.\n");
  }

  printf("Shutting down...\n");
  c_rest_router_destroy(router);
  c_rest_destroy(ctx);

  return 0;
}
