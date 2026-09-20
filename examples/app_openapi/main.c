/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_modality.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_router.h"
#include "c_rest_openapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <string.h>
/* clang-format on */

static c_rest_error_t handle_hello_world(struct c_rest_request *req,
                                         struct c_rest_response *res,
                                         void *user_data) {
  (void)req;
  (void)user_data;
  return c_rest_response_json(res,
                              "{\"message\": \"Hello from OpenAPI Example!\"}");
}

static void sig_handler(int sig) {
  (void)sig;
  exit(0);
}

int main(void) {

  struct c_rest_router *router = NULL;
  struct c_rest_openapi_spec *spec = NULL;
  struct c_rest_openapi_operation op;
  struct c_rest_context *ctx = NULL;
  c_rest_error_t rc;
  const char *tags[] = {"greeting"};

  struct c_rest_openapi_response res_hello;
  struct c_rest_openapi_media_type res_mt_hello;
  const char *res_keys_hello[] = {"application/json"};

  rc = c_rest_router_init(&router);
  if (rc != C_REST_OK)
    return 1;
  rc = c_rest_enable_openapi(router, "/openapi.json");
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    return 1;
  }
  rc = c_rest_enable_swagger_ui(router, "/docs", "/openapi.json");
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    return 1;
  }

  rc = c_rest_router_get_openapi_spec(router, &spec);
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    return 1;
  }

  if (spec) {
    rc = c_rest_openapi_spec_add_component_schema(
        spec, "HelloResponse",
        "{\"type\": \"object\", \"properties\": {\"message\": {\"type\": "
        "\"string\"}}}");
    if (rc != C_REST_OK) {
      c_rest_router_destroy(router);
      return 1;
    }
  }

  rc = c_rest_enable_openapi(router, "/api/v0/openapi.json");
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    return 1;
  }
  rc = c_rest_enable_swagger_ui(router, "/api/v0/docs", "/api/v0/openapi.json");
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    return 1;
  }

  memset(&op, 0, sizeof(op));
  op.operation_id = "helloWorld";
  op.summary = "Greeting endpoint";
  op.description = "Returns a friendly greeting message.";
  op.tags = tags;
  op.n_tags = 1;

  memset(&res_hello, 0, sizeof(res_hello));
  memset(&res_mt_hello, 0, sizeof(res_mt_hello));
  res_hello.status_code = "200";
  res_hello.description = "Successful response";
  res_mt_hello.schema.ref_name = "HelloResponse";
  res_hello.content_keys = res_keys_hello;
  res_hello.content_values = &res_mt_hello;
  res_hello.n_content = 1;
  op.responses = &res_hello;
  op.n_responses = 1;

  rc = c_rest_router_add_openapi(router, "GET", "/api/hello",
                                 handle_hello_world, NULL, &op);
  if (rc != C_REST_OK) {
    c_rest_router_destroy(router);
    return 1;
  }

  /* Note: c_rest_init logic omitted to keep it small, but let's assume ctx is
   * used */
  signal(SIGTERM, sig_handler);
  signal(SIGINT, sig_handler);
  rc = c_rest_init(C_REST_MODALITY_SYNC, &ctx);
  if (rc == 0) {
    rc = c_rest_set_router(ctx, router);
    if (rc != C_REST_OK) {
      c_rest_destroy(ctx);
      c_rest_router_destroy(router);
      return 1;
    }

    /* Normally we would c_rest_run(ctx) here but it blocks */
    rc = c_rest_destroy(ctx);
    if (rc != C_REST_OK) {
      c_rest_router_destroy(router);
      return 1;
    }
  }

  rc = c_rest_router_destroy(router);
  if (rc != C_REST_OK)
    return 1;
  return 0;
}
