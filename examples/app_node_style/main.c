/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static void my_log_cb(const char *message) { printf("[APP] %s\n", message); }

static int handle_hello_world(struct c_rest_request *req,
                              struct c_rest_response *res, void *user_data) {
  (void)req;
  (void)user_data;
  c_rest_response_json(res, "{\"message\": \"Hello from Async Node Style!\"}");
  return 0;
}

static int handle_echo(struct c_rest_request *req, struct c_rest_response *res,
                       void *user_data) {
  char buf[512];
  const char *name = "Guest";
  (void)user_data;

  if (req->query) {
    /* simplistic query handling for demo purposes */
    if (strncmp(req->query, "name=", 5) == 0) {
      name = req->query + 5;
    }
  }

#if defined(_MSC_VER)
  sprintf_s(buf, sizeof(buf), "{\"echo\": \"Hello %s\"}", name);
#else
  sprintf(buf, "{\"echo\": \"Hello %s\"}", name);
#endif

  c_rest_response_json(res, buf);
  return 0;
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  int rc;

  printf("Initializing Node-style Async Application...\n");

  rc = c_rest_init(C_REST_MODALITY_ASYNC, &ctx);
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

  c_rest_set_router(ctx, router);
  c_rest_router_add(router, "GET", "/api/v0/hello", handle_hello_world, NULL);
  c_rest_router_add(router, "GET", "/api/v0/echo", handle_echo, NULL);

  /* In a real app, router is hooked into ctx here */

  printf("Starting framework event loop...\n");
  rc = c_rest_run(ctx);
  if (rc != 0) {
    fprintf(stderr, "Framework runtime error.\n");
  }

  printf("Shutting down...\n");
  c_rest_router_destroy(router);
  c_rest_destroy(ctx);

  return 0;
}
