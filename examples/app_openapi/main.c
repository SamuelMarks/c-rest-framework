/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_openapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static void my_log_cb(const char *message) { printf("[APP] %s\n", message); }

static int handle_hello_world(struct c_rest_request *req,
                              struct c_rest_response *res, void *user_data) {
  (void)req;
  (void)user_data;
  c_rest_response_json(res, "{\"message\": \"Hello from OpenAPI Example!\"}");
  return 0;
}

int main(void) {
  struct c_rest_context *ctx = NULL;
  c_rest_router *router = NULL;
  struct c_rest_openapi_spec *spec = NULL;
  struct c_rest_openapi_operation op;
  int rc;
  const char *tags[] = {"Demo"};

  printf("Initializing OpenAPI Example Application...\n");

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

  memset(&op, 0, sizeof(op));
  op.summary = "Hello World Endpoint";
  op.description = "Returns a simple hello world JSON message.";
  op.tags = tags;
  op.n_tags = 1;
  op.res_body_schema.ref_name = "HelloResponse";

  c_rest_router_add_openapi(router, "GET", "/api/v0/hello", handle_hello_world,
                            NULL, &op);

  spec = c_rest_router_get_openapi_spec(router);
  if (spec) {
    c_rest_openapi_spec_add_component_schema(
        spec, "HelloResponse",
        "{\"type\": \"object\", \"properties\": {\"message\": {\"type\": "
        "\"string\"}}}");
  }

  c_rest_enable_openapi(router, "/api/v0/openapi.json");
  c_rest_enable_swagger_ui(router, "/api/v0/docs", "/api/v0/openapi.json");

  /* Skip actual blocking run for CI test safety */
  printf("Configuration complete. To run the server, uncomment "
         "c_rest_run(ctx).\n");

  /*
  printf("Starting framework event loop...\n");
  rc = c_rest_run(ctx);
  if (rc != 0) {
    fprintf(stderr, "Framework runtime error.\n");
  }
  */

  printf("Shutting down...\n");
  c_rest_router_destroy(router);
  c_rest_destroy(ctx);

  return 0;
}
