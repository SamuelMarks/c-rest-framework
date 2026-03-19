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
  struct c_rest_router *router = NULL;
  struct c_rest_openapi_spec *spec;
  struct c_rest_openapi_operation op;
  struct c_rest_context *ctx = NULL;
  int rc;
  const char *tags[] = {"greeting"};

  struct c_rest_openapi_response res_hello;
  struct c_rest_openapi_media_type res_mt_hello;
  const char *res_keys_hello[] = {"application/json"};

  c_rest_router_init(&router);
  c_rest_enable_openapi(router, "/openapi.json");
  c_rest_enable_swagger_ui(router, "/docs", "/openapi.json");

  spec = c_rest_router_get_openapi_spec(router);

  if (spec) {
    c_rest_openapi_spec_add_component_schema(
        spec, "HelloResponse",
        "{\"type\": \"object\", \"properties\": {\"message\": {\"type\": "
        "\"string\"}}}");
  }

  c_rest_enable_openapi(router, "/api/v0/openapi.json");
  c_rest_enable_swagger_ui(router, "/api/v0/docs", "/api/v0/openapi.json");

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

  c_rest_router_add_openapi(router, "GET", "/api/hello", handle_hello_world,
                            NULL, &op);

  /* Note: c_rest_init logic omitted to keep it small, but let's assume ctx is
   * used */
  rc = c_rest_init(&ctx);
  if (rc == 0 && ctx) {
    c_rest_set_router(ctx, router);
    c_rest_set_log_callback(ctx, my_log_cb);

    /* Normally we would c_rest_run(ctx) here but it blocks */
    c_rest_destroy(ctx);
  }

  c_rest_router_destroy(router);
  return 0;
}
