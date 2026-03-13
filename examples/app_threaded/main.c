/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"

#include <stdio.h>
#include <stdlib.h>
/* clang-format on */

static void my_log_cb(const char *message) {
  printf("[THREADED] %s\n", message);
}

static void handle_db_query(struct c_rest_request *req,
                            struct c_rest_response *res, void *user_data) {
  (void)req;
  (void)user_data;
  /* Simulate a blocking DB query */
  c_rest_response_json(res, "{\"status\": \"success\", \"rows\": 42}");
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  int rc;

  printf("Initializing Threaded Synchronous Application...\n");

  rc = c_rest_init(C_REST_MODALITY_MULTI_THREAD, &ctx);
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

  c_rest_router_add(router, "GET", "/db/query", handle_db_query, NULL);

  printf("Starting framework thread pool...\n");
  rc = c_rest_run(ctx);
  if (rc != 0) {
    fprintf(stderr, "Framework runtime error.\n");
  }

  printf("Shutting down...\n");
  c_rest_router_destroy(router);
  c_rest_destroy(ctx);

  return 0;
}
