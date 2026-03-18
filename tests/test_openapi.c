/* clang-format off */
#include "c_rest_openapi.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* clang-format on */

static int dummy_handler(struct c_rest_request *req,
                         struct c_rest_response *res, void *user_data) {
  (void)req;
  (void)res;
  (void)user_data;
  return 0;
}

int test_openapi(void) {
  c_rest_router *router = NULL;
  struct c_rest_openapi_spec *spec = NULL;
  struct c_rest_openapi_operation op;
  struct c_rest_request req;
  struct c_rest_response res;
  const char *tags[] = {"test_tag"};
  int ret;

  printf("Running OpenAPI tests...\n");

  ret = c_rest_router_init(&router);
  if (ret != 0 || !router) {
    printf("Failed to init router\n");
    return 1;
  }

  memset(&op, 0, sizeof(op));
  op.summary = "Test endpoint";
  op.description = "A dummy test endpoint";
  op.tags = tags;
  op.n_tags = 1;
  op.req_body_schema.ref_name = "DummyReq";
  op.res_body_schema.ref_name = "DummyRes";

  ret = c_rest_router_add_openapi(router, "POST", "/api/test", dummy_handler,
                                  NULL, &op);
  if (ret != 0) {
    printf("Failed to add openapi route\n");
    return 1;
  }

  spec = c_rest_router_get_openapi_spec(router);
  if (!spec) {
    printf("Failed to get openapi spec\n");
    return 1;
  }

  ret = c_rest_openapi_spec_add_component_schema(
      spec, "DummyReq",
      "{\"type\": \"object\", \"properties\": {\"id\": {\"type\": "
      "\"integer\"}}}");
  if (ret != 0) {
    printf("Failed to add component schema\n");
    return 1;
  }

  ret = c_rest_enable_openapi(router, "/openapi.json");
  if (ret != 0) {
    printf("Failed to enable openapi route\n");
    return 1;
  }

  spec = c_rest_router_get_openapi_spec(router);
  if (!spec) {
    printf("Failed to get openapi spec\n");
    return 1;
  }

  if (spec->n_paths != 1 || strcmp(spec->paths[0].route, "/api/test") != 0) {
    printf("OpenAPI spec paths mismatch\n");
    return 1;
  }

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  req.method = "GET";
  req.path = "/openapi.json";

  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0 || res.status_code != 200) {
    printf("Failed to dispatch openapi route (status %d)\n", res.status_code);
    return 1;
  }

  if (!res.body || strstr(res.body, "openapi") == NULL ||
      strstr(res.body, "Test endpoint") == NULL) {
    printf("OpenAPI spec output mismatch: %s\n", res.body ? res.body : "NULL");
    return 1;
  }

  printf("Generated OpenAPI spec:\n%s\n", res.body);

  /* Note: The string sent via c_rest_response_json is usually duplicated or the
   * response handles it, let's clean up response. */
  if (res.body) {
    free(res.body);
  }

  /* Test Swagger UI */
  ret = c_rest_enable_swagger_ui(router, "/docs", "/openapi.json");
  if (ret != 0) {
    printf("Failed to enable swagger ui route\n");
    return 1;
  }

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  req.method = "GET";
  req.path = "/docs";

  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0 || res.status_code != 200) {
    printf("Failed to dispatch swagger ui route (status %d)\n",
           res.status_code);
    return 1;
  }

  if (!res.body || strstr(res.body, "SwaggerUIBundle") == NULL ||
      strstr(res.body, "\"/openapi.json\"") == NULL) {
    printf("Swagger UI output mismatch: %s\n", res.body ? res.body : "NULL");
    return 1;
  }

  if (res.body) {
    free(res.body);
  }

  c_rest_router_destroy(router);
  return 0;
}
