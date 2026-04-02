/* clang-format off */
#include "test_protos.h"
#include "c_rest_openapi.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parson.h"
/* clang-format on */

#if !defined(_MSC_VER)
static char *portable_strdup(const char *s) {
  size_t len = strlen(s) + 1;
  char *m = (char *)malloc(len);
  if (m) {
#if defined(_MSC_VER)
    strcpy_s(m, len, s);
#else
    strcpy(m, s);
#endif
  }
  return m;
}
#define _strdup portable_strdup
#endif

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

  struct c_rest_openapi_parameter param;
  struct c_rest_openapi_header header;
  const char *header_keys[] = {"X-Test"};
  const char *scopes[] = {"read"};
  struct c_rest_openapi_security_requirement sec_req;

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

  c_rest_router_get_openapi_spec(router, &spec);
  if (!spec) {
    printf("OpenAPI spec is NULL\n");
    return 1;
  }

  spec->info.title = _strdup("My API");
  spec->info.version = _strdup("2.0.0");

  spec->info.description = _strdup("Test Description");
  spec->info.terms_of_service = _strdup("http://example.com/tos");
  spec->info.contact.name = _strdup("API Support");
  spec->info.contact.url = _strdup("http://example.com/support");
  spec->info.contact.email = _strdup("support@example.com");
  spec->info.license.name = _strdup("Apache 2.0");
  spec->info.license.identifier = _strdup("Apache-2.0");
  spec->info.license.url =
      _strdup("http://www.apache.org/licenses/LICENSE-2.0.html");

  spec->external_docs.description = _strdup("Find out more");
  spec->external_docs.url = _strdup("http://swagger.io");

  spec->json_schema_dialect =
      _strdup("http://json-schema.org/draft-07/schema#");

  {
    struct c_rest_openapi_server srv;
    struct c_rest_openapi_server_variable var;
    const char *enums[] = {"80", "443"};
    memset(&srv, 0, sizeof(srv));
    memset(&var, 0, sizeof(var));
    srv.url = _strdup("http://example.com:{port}");
    srv.description = _strdup("Test Server");
    var.name = _strdup("port");
    var.default_value = _strdup("443");
    var.description = _strdup("Port number");
    var.enum_values = (const char **)malloc(2 * sizeof(char *));
    var.enum_values[0] = _strdup(enums[0]);
    var.enum_values[1] = _strdup(enums[1]);
    var.n_enum_values = 2;
    srv.variables =
        (struct c_rest_openapi_server_variable *)malloc(sizeof(var));
    memcpy(srv.variables, &var, sizeof(var));
    srv.n_variables = 1;
    spec->servers = (struct c_rest_openapi_server *)malloc(
        sizeof(struct c_rest_openapi_server));
    memcpy(spec->servers, &srv, sizeof(srv));
    spec->n_servers = 1;
  }

  {
    struct c_rest_openapi_tag tag;
    memset(&tag, 0, sizeof(tag));
    tag.name = _strdup("test_tag");
    tag.summary = _strdup("Test Tag Summary");
    tag.description = _strdup("Test Tag Description");
    tag.external_docs.description = _strdup("Docs");
    tag.external_docs.url = _strdup("http://example.com");
    tag.parent = _strdup("parent_tag");
    tag.kind = _strdup("navigation");
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

    scheme.name_key = _strdup("oauth2");
    scheme.type = _strdup("oauth2");
    scheme.description = _strdup("OAuth2");
    scheme.name = _strdup("Authorization");
    scheme.in = _strdup("header");
    scheme.scheme = _strdup("bearer");
    scheme.bearer_format = _strdup("JWT");
    scheme.open_id_connect_url = _strdup("http://example.com/.well-known");

    flow.authorization_url = _strdup("http://example.com/auth");
    flow.token_url = _strdup("http://example.com/token");
    flow.refresh_url = _strdup("http://example.com/refresh");
    flow.scopes_keys = (const char **)malloc(sizeof(char *));
    flow.scopes_keys[0] = _strdup(scope_keys[0]);
    flow.scopes_values = (const char **)malloc(sizeof(char *));
    flow.scopes_values[0] = _strdup(scope_vals[0]);
    flow.n_scopes = 1;

    /* We test one flow, the others are structurally identical but let's just
     * test implicit for coverage */
    scheme.flows.implicit = (struct c_rest_openapi_oauth_flow *)malloc(
        sizeof(struct c_rest_openapi_oauth_flow));
    memset(scheme.flows.implicit, 0, sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.implicit->authorization_url =
        _strdup("http://example.com/auth");
    scheme.flows.implicit->token_url = _strdup("http://example.com/token");
    scheme.flows.implicit->refresh_url = _strdup("http://example.com/refresh");
    scheme.flows.implicit->scopes_keys = (const char **)malloc(sizeof(char *));
    scheme.flows.implicit->scopes_keys[0] = _strdup("read");
    scheme.flows.implicit->scopes_values =
        (const char **)malloc(sizeof(char *));
    scheme.flows.implicit->scopes_values[0] = _strdup("Read access");
    scheme.flows.implicit->n_scopes = 1;

    scheme.flows.password = (struct c_rest_openapi_oauth_flow *)malloc(
        sizeof(struct c_rest_openapi_oauth_flow));
    memset(scheme.flows.password, 0, sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.password->authorization_url =
        _strdup("http://example.com/auth");
    scheme.flows.password->token_url = _strdup("http://example.com/token");
    scheme.flows.password->refresh_url = _strdup("http://example.com/refresh");
    scheme.flows.password->scopes_keys = (const char **)malloc(sizeof(char *));
    scheme.flows.password->scopes_keys[0] = _strdup("read");
    scheme.flows.password->scopes_values =
        (const char **)malloc(sizeof(char *));
    scheme.flows.password->scopes_values[0] = _strdup("Read access");
    scheme.flows.password->n_scopes = 1;

    scheme.flows.client_credentials =
        (struct c_rest_openapi_oauth_flow *)malloc(
            sizeof(struct c_rest_openapi_oauth_flow));
    memset(scheme.flows.client_credentials, 0,
           sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.client_credentials->authorization_url =
        _strdup("http://example.com/auth");
    scheme.flows.client_credentials->token_url =
        _strdup("http://example.com/token");
    scheme.flows.client_credentials->refresh_url =
        _strdup("http://example.com/refresh");
    scheme.flows.client_credentials->scopes_keys =
        (const char **)malloc(sizeof(char *));
    scheme.flows.client_credentials->scopes_keys[0] = _strdup("read");
    scheme.flows.client_credentials->scopes_values =
        (const char **)malloc(sizeof(char *));
    scheme.flows.client_credentials->scopes_values[0] = _strdup("Read access");
    scheme.flows.client_credentials->n_scopes = 1;

    scheme.flows.authorization_code =
        (struct c_rest_openapi_oauth_flow *)malloc(
            sizeof(struct c_rest_openapi_oauth_flow));
    memset(scheme.flows.authorization_code, 0,
           sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.authorization_code->authorization_url =
        _strdup("http://example.com/auth");
    scheme.flows.authorization_code->token_url =
        _strdup("http://example.com/token");
    scheme.flows.authorization_code->refresh_url =
        _strdup("http://example.com/refresh");
    scheme.flows.authorization_code->scopes_keys =
        (const char **)malloc(sizeof(char *));
    scheme.flows.authorization_code->scopes_keys[0] = _strdup("read");
    scheme.flows.authorization_code->scopes_values =
        (const char **)malloc(sizeof(char *));
    scheme.flows.authorization_code->scopes_values[0] = _strdup("Read access");
    scheme.flows.authorization_code->n_scopes = 1;

    spec->security_schemes = (struct c_rest_openapi_security_scheme *)malloc(
        sizeof(struct c_rest_openapi_security_scheme));
    memcpy(spec->security_schemes, &scheme, sizeof(scheme));
    spec->n_security_schemes = 1;

    spec->security = (struct c_rest_openapi_security_requirement *)malloc(
        sizeof(struct c_rest_openapi_security_requirement));
    spec->security[0].name = _strdup("oauth2");
    spec->security[0].scopes = (const char **)malloc(sizeof(char *));
    spec->security[0].scopes[0] = _strdup(scopes[0]);
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
