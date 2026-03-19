/* clang-format off */
#include "c_rest_openapi.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parson.h"
/* clang-format on */

static int dummy_handler(struct c_rest_request *req,
                         struct c_rest_response *res, void *user_data) {
  (void)req;
  (void)res;
  (void)user_data;
  return 0;
}

int test_openapi(void) {
  struct c_rest_router *router = NULL;
  struct c_rest_openapi_spec *spec = NULL;
  struct c_rest_openapi_operation op;
  const char *tags[] = {"test_tag"};
  int ret;
  char *json_str = NULL;

  struct c_rest_openapi_response op_res;
  struct c_rest_openapi_media_type op_res_mt;
  const char *op_res_keys[1];

  struct c_rest_openapi_request_body op_req;
  struct c_rest_openapi_media_type op_req_mt;
  const char *op_req_keys[1];

  struct c_rest_request req;
  struct c_rest_response res;

  op_res_keys[0] = "application/json";
  op_req_keys[0] = "application/json";

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

  memset(&op_res, 0, sizeof(op_res));
  memset(&op_res_mt, 0, sizeof(op_res_mt));
  op_res.status_code = "200";
  op_res.description = "Successful response";
  op_res_mt.schema.ref_name = "DummyRes";
  op_res.content_keys = op_res_keys;
  op_res.content_values = &op_res_mt;
  op_res.n_content = 1;
  op.responses = &op_res;
  op.n_responses = 1;

  memset(&op_req, 0, sizeof(op_req));
  memset(&op_req_mt, 0, sizeof(op_req_mt));
  op_req_mt.schema.ref_name = "DummyReq";
  op_req.content_keys = op_req_keys;
  op_req.content_values = &op_req_mt;
  op_req.n_content = 1;
  op.request_body = &op_req;

  struct c_rest_openapi_parameter param;
  struct c_rest_openapi_header header;
  const char *header_keys[] = {"X-Test"};
  const char *scopes[] = {"read"};

  memset(&param, 0, sizeof(param));
  param.name = "id";
  param.in = "query";
  param.description = "Test ID";
  param.required = 1;
  param.deprecated = 1;
  param.allow_empty_value = 1;
  param.style = "form";
  param.explode = 1;
  param.allow_reserved = 1;
  param.example = "123";
  param.schema.ref_name = "DummyReq";
  op.parameters = &param;
  op.n_parameters = 1;

  memset(&header, 0, sizeof(header));
  header.description = "Test Header";
  header.required = 1;
  header.deprecated = 1;
  header.style = "simple";
  header.explode = 1;
  header.schema.ref_name = "DummyReq";

  op_res.header_keys = header_keys;
  op_res.header_values = &header;
  op_res.n_headers = 1;

  struct c_rest_openapi_security_requirement sec_req;
  memset(&sec_req, 0, sizeof(sec_req));
  sec_req.name = "oauth2";
  sec_req.scopes = scopes;
  sec_req.n_scopes = 1;
  op.security = &sec_req;
  op.n_security = 1;

  c_rest_enable_openapi(router, "/openapi.json");
  c_rest_enable_swagger_ui(router, "/docs", "/openapi.json");

  ret = c_rest_router_add_openapi(router, "GET", "/api/test", dummy_handler,
                                  NULL, &op);
  if (ret != 0) {
    printf("Failed to add openapi route\n");
    return 1;
  }

  spec = c_rest_router_get_openapi_spec(router);
  if (!spec) {
    printf("OpenAPI spec is NULL\n");
    return 1;
  }

  spec->info.title = (char *)"My API";
  spec->info.version = (char *)"2.0.0";

  spec->info.description = "Test Description";
  spec->info.terms_of_service = "http://example.com/tos";
  spec->info.contact.name = "API Support";
  spec->info.contact.url = "http://example.com/support";
  spec->info.contact.email = "support@example.com";
  spec->info.license.name = "Apache 2.0";
  spec->info.license.identifier = "Apache-2.0";
  spec->info.license.url = "http://www.apache.org/licenses/LICENSE-2.0.html";

  spec->external_docs.description = "Find out more";
  spec->external_docs.url = "http://swagger.io";

  spec->json_schema_dialect = "http://json-schema.org/draft-07/schema#";

  {
    struct c_rest_openapi_server srv;
    struct c_rest_openapi_server_variable var;
    const char *enums[] = {"80", "443"};
    memset(&srv, 0, sizeof(srv));
    memset(&var, 0, sizeof(var));
    srv.url = "http://example.com:{port}";
    srv.description = "Test Server";
    var.name = "port";
    var.default_value = "443";
    var.description = "Port number";
    var.enum_values = enums;
    var.n_enum_values = 2;
    srv.variables = &var;
    srv.n_variables = 1;
    spec->servers = (struct c_rest_openapi_server *)malloc(
        sizeof(struct c_rest_openapi_server));
    memcpy(spec->servers, &srv, sizeof(srv));
    spec->n_servers = 1;
  }

  {
    struct c_rest_openapi_tag tag;
    memset(&tag, 0, sizeof(tag));
    tag.name = "test_tag";
    tag.summary = "Test Tag Summary";
    tag.description = "Test Tag Description";
    tag.external_docs.description = "Docs";
    tag.external_docs.url = "http://example.com";
    tag.parent = "parent_tag";
    tag.kind = "navigation";
    spec->tags =
        (struct c_rest_openapi_tag *)malloc(sizeof(struct c_rest_openapi_tag));
    memcpy(spec->tags, &tag, sizeof(tag));
    spec->n_tags = 1;
  }

  {
    struct c_rest_openapi_security_scheme scheme;
    struct c_rest_openapi_oauth_flow flow;
    const char *scope_keys[] = {"read"};
    const char *scope_vals[] = {"Read access"};

    memset(&scheme, 0, sizeof(scheme));
    memset(&flow, 0, sizeof(flow));

    scheme.name_key = "oauth2";
    scheme.type = "oauth2";
    scheme.description = "OAuth2";
    scheme.name = "Authorization";
    scheme.in = "header";
    scheme.scheme = "bearer";
    scheme.bearer_format = "JWT";
    scheme.open_id_connect_url = "http://example.com/.well-known";

    flow.authorization_url = "http://example.com/auth";
    flow.token_url = "http://example.com/token";
    flow.refresh_url = "http://example.com/refresh";
    flow.scopes_keys = scope_keys;
    flow.scopes_values = scope_vals;
    flow.n_scopes = 1;

    /* We test one flow, the others are structurally identical but let's just
     * test implicit for coverage */
    scheme.flows.implicit = (struct c_rest_openapi_oauth_flow *)malloc(
        sizeof(struct c_rest_openapi_oauth_flow));
    memcpy(scheme.flows.implicit, &flow, sizeof(flow));

    scheme.flows.password = (struct c_rest_openapi_oauth_flow *)malloc(
        sizeof(struct c_rest_openapi_oauth_flow));
    memcpy(scheme.flows.password, &flow, sizeof(flow));

    scheme.flows.client_credentials =
        (struct c_rest_openapi_oauth_flow *)malloc(
            sizeof(struct c_rest_openapi_oauth_flow));
    memcpy(scheme.flows.client_credentials, &flow, sizeof(flow));

    scheme.flows.authorization_code =
        (struct c_rest_openapi_oauth_flow *)malloc(
            sizeof(struct c_rest_openapi_oauth_flow));
    memcpy(scheme.flows.authorization_code, &flow, sizeof(flow));

    spec->security_schemes = (struct c_rest_openapi_security_scheme *)malloc(
        sizeof(struct c_rest_openapi_security_scheme));
    memcpy(spec->security_schemes, &scheme, sizeof(scheme));
    spec->n_security_schemes = 1;

    spec->security = (struct c_rest_openapi_security_requirement *)malloc(
        sizeof(struct c_rest_openapi_security_requirement));
    spec->security[0].name = "oauth2";
    spec->security[0].scopes = scopes;
    spec->security[0].n_scopes = 1;
    spec->n_security = 1;
  }

  ret = c_rest_openapi_spec_add_component_schema(
      spec, "DummyReq",
      "{\"type\":\"object\",\"properties\":{\"id\":{\"type\":\"integer\"}}}");
  if (ret != 0) {
    printf("Failed to add component schema\n");
    return 1;
  }

  ret = c_rest_openapi_spec_to_json(spec, &json_str);
  if (ret != 0 || !json_str) {
    printf("Failed to generate OpenAPI JSON\n");
    return 1;
  }

  if (strstr(json_str, "\"DummyReq\"") == NULL ||
      strstr(json_str, "\"test_tag\"") == NULL) {
    printf("OpenAPI JSON missing expected components or tags.\n%s\n", json_str);
    return 1;
  }
  json_free_serialized_string(json_str);

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  req.method = "GET";
  req.path = "/docs";
  c_rest_router_dispatch(router, &req, &res);

  if (res.status_code != 200) {
    printf("Swagger UI handler failed with status %d\n", res.status_code);
    return 1;
  }

  if (res.body == NULL || strstr(res.body, "\"/openapi.json\"") == NULL) {
    printf("Swagger UI output mismatch: %s\n", res.body ? res.body : "NULL");
    return 1;
  }

  if (res.body) {
    free(res.body);
  }

  c_rest_router_destroy(router);
  return 0;
}
