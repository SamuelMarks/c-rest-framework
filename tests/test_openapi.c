/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
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

static c_rest_error_t dummy_handler(struct c_rest_request *req,
                                    struct c_rest_response *res,
                                    void *user_data) {
  (void)req;
  (void)res;
  (void)user_data;
  return C_REST_OK;
}

static int g_malloc_fail_after = -1;

static int g_fail_realloc_at = -1;
static void *fail_realloc_n(void *ptr, size_t size) {
  if (g_fail_realloc_at == 0) {
    g_fail_realloc_at--;
    return NULL;
  }
  if (g_fail_realloc_at > 0) {
    g_fail_realloc_at--;
  }
  return realloc(ptr, size);
}

static void *fail_malloc_n(size_t size) {
  if (g_malloc_fail_after == 0) {
    g_malloc_fail_after--;
    return NULL;
  }
  if (g_malloc_fail_after > 0) {
    g_malloc_fail_after--;
  }
  return malloc(size);
}

static int test_openapi_errors(void) {
  struct c_rest_openapi_spec *spec = NULL;
  char *json_str = NULL;
  int ret;
  extern void *(*g_crf_malloc_hook)(size_t);
  extern void *(*g_crf_realloc_hook)(void *, size_t);
  int i;
  struct c_rest_openapi_operation op;
  const char *tags[] = {"test_tag"};
  const char *scopes[] = {"read"};
  struct c_rest_openapi_security_requirement sec_req;
  memset(&sec_req, 0, sizeof(sec_req));
  sec_req.name = "oauth2";
  sec_req.scopes = scopes;
  sec_req.n_scopes = 1;

  memset(&op, 0, sizeof(op));
  op.operation_id = "testId";
  op.summary = "Test Summary";
  op.description = "Test Desc";
  op.tags = tags;
  op.n_tags = 1;
  op.security = &sec_req;
  op.n_security = 1;
  op.external_docs.url = "http://example.com/docs";
  op.external_docs.description = "Endpoint Docs";

  {
    int k;
    for (k = 0; k < 20; k++) {
      struct c_rest_openapi_spec *tmpspec = NULL;
      c_rest_openapi_spec_init(&tmpspec);

      g_crf_malloc_hook = fail_malloc_n;
      g_crf_realloc_hook = fail_realloc_n;
      g_malloc_fail_after = k;
      g_fail_realloc_at = k;

      c_rest_openapi_spec_add_component_schema(tmpspec, "D1", "{}");

      g_crf_malloc_hook = NULL;
      g_crf_realloc_hook = NULL;

      c_rest_openapi_spec_destroy(tmpspec);
    }
  }

  {
    int k;
    for (k = 0; k < 30; k++) {
      struct c_rest_openapi_spec *tmpspec = NULL;
      c_rest_openapi_spec_init(&tmpspec);
      c_rest_openapi_spec_add_component_schema(tmpspec, "D1", "{}");
      c_rest_openapi_spec_add_component_schema(tmpspec, "D2", "{}");
      c_rest_openapi_spec_add_component_schema(tmpspec, "D3", "{}");
      c_rest_openapi_spec_add_component_schema(tmpspec, "D4", "{}");
      c_rest_openapi_spec_add_component_schema(tmpspec, "D5", "{}");
      c_rest_openapi_spec_add_component_schema(tmpspec, "D6", "{}");
      c_rest_openapi_spec_add_component_schema(tmpspec, "D7", "{}");
      c_rest_openapi_spec_add_component_schema(tmpspec, "D8", "{}");

      g_crf_malloc_hook = fail_malloc_n;
      g_crf_realloc_hook = fail_realloc_n;
      g_malloc_fail_after = k;
      g_fail_realloc_at = k;

      c_rest_openapi_spec_add_component_schema(tmpspec, "D9", "{}");

      g_crf_malloc_hook = NULL;
      g_crf_realloc_hook = NULL;

      c_rest_openapi_spec_destroy(tmpspec);
    }
  }

  {
    int m;
    for (m = 0; m < 30; m++) {
      struct c_rest_openapi_spec *tmpspec = NULL;
      c_rest_openapi_spec_init(&tmpspec);

      g_crf_malloc_hook = fail_malloc_n;
      g_crf_realloc_hook = fail_realloc_n;
      g_malloc_fail_after = m;
      g_fail_realloc_at = m;

      c_rest_openapi_spec_add_path(tmpspec, "p1", "GET", &op);

      g_crf_malloc_hook = NULL;
      g_crf_realloc_hook = NULL;
      c_rest_openapi_spec_destroy(tmpspec);
    }
  }

  {
    int m;
    for (m = 0; m < 50; m++) {
      struct c_rest_openapi_spec *tmpspec = NULL;
      c_rest_openapi_spec_init(&tmpspec);
      c_rest_openapi_spec_add_path(tmpspec, "p1", "GET", &op);
      c_rest_openapi_spec_add_path(tmpspec, "p2", "GET", &op);
      c_rest_openapi_spec_add_path(tmpspec, "p3", "GET", &op);
      c_rest_openapi_spec_add_path(tmpspec, "p4", "GET", &op);

      g_crf_malloc_hook = fail_malloc_n;
      g_crf_realloc_hook = fail_realloc_n;
      g_malloc_fail_after = m;
      g_fail_realloc_at = m;

      c_rest_openapi_spec_add_path(tmpspec, "p5", "GET", &op);

      g_crf_malloc_hook = NULL;
      g_crf_realloc_hook = NULL;
      c_rest_openapi_spec_destroy(tmpspec);
    }
  }
  {
    struct c_rest_openapi_spec *tmpspec = NULL;
    c_rest_openapi_spec_init(&tmpspec);
    for (i = 0; i < 10; i++) {
      char buf[32];
      sprintf(buf, "Comp%d", i);
      c_rest_openapi_spec_add_component_schema(tmpspec, buf, "{}");
    }
    c_rest_openapi_spec_to_json(tmpspec, NULL);
    c_rest_openapi_spec_destroy(tmpspec);
  }

  c_rest_openapi_spec_to_json(NULL, &json_str);

  {
    struct c_rest_router *router = NULL;
    c_rest_router_init(&router);

    /* methods */
    c_rest_router_add_openapi(router, "GET", "/api/test", dummy_handler, NULL,
                              &op);
    c_rest_router_add_openapi(router, "POST", "/api/test", dummy_handler, NULL,
                              &op);
    c_rest_router_add_openapi(router, "PUT", "/api/test", dummy_handler, NULL,
                              &op);
    c_rest_router_add_openapi(router, "DELETE", "/api/test", dummy_handler,
                              NULL, &op);
    c_rest_router_add_openapi(router, "PATCH", "/api/test", dummy_handler, NULL,
                              &op);
    c_rest_router_add_openapi(router, "OPTIONS", "/api/test", dummy_handler,
                              NULL, &op);
    c_rest_router_add_openapi(router, "HEAD", "/api/test", dummy_handler, NULL,
                              &op);
    c_rest_router_add_openapi(router, "TRACE", "/api/test", dummy_handler, NULL,
                              &op);
    c_rest_router_add_openapi(router, "QUERY", "/api/test", dummy_handler, NULL,
                              &op);
    c_rest_router_add_openapi(router, "UNKNOWN", "/api/test", dummy_handler,
                              NULL, &op);

    /* second path */
    c_rest_router_add_openapi(router, "GET", "/api/test2", dummy_handler, NULL,
                              &op);

    c_rest_router_destroy(router);
  }

  {
    struct c_rest_router *router = NULL;
    c_rest_router_init(&router);
    c_rest_enable_openapi(router, "/openapi.json");
    c_rest_router_get_openapi_spec(router, &spec);

    for (i = 0; i < 250; i++) {
      g_crf_malloc_hook = fail_malloc_n;
      g_malloc_fail_after = i;
      ret = c_rest_openapi_spec_to_json(spec, &json_str);
      if (ret == C_REST_OK) {
        json_free_serialized_string(json_str);
      }
    }
    g_crf_malloc_hook = NULL;
    g_malloc_fail_after = -1;

    c_rest_enable_swagger_ui(router, "/docs1", "/openapi.json");
    {
      struct c_rest_request req2;
      struct c_rest_response res2;
      int jj;
      for (jj = 0; jj < 50; jj++) {
        memset(&req2, 0, sizeof(req2));
        memset(&res2, 0, sizeof(res2));
        req2.method = "GET";
        req2.path = "/docs1";
        g_crf_malloc_hook = fail_malloc_n;
        g_malloc_fail_after = jj;
        c_rest_router_dispatch(router, &req2, &res2);
        c_rest_response_cleanup(&res2);
        memset(&res2, 0, sizeof(res2));
        req2.path = "/openapi.json";
        c_rest_router_dispatch(router, &req2, &res2);
        g_crf_malloc_hook = NULL;
        g_malloc_fail_after = -1;
        c_rest_response_cleanup(&res2);
      }
    }

    c_rest_router_destroy(router);
  }

  {
    struct c_rest_request req2;
    struct c_rest_response res2;
    struct c_rest_router *router2 = NULL;
    c_rest_router_init(&router2);
    c_rest_enable_openapi(router2, "/openapi.json");
    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));
    req2.method = "GET";
    req2.path = "/openapi.json";

    c_rest_router_dispatch(router2, &req2, &res2);
    c_rest_response_cleanup(&res2);

    {
      int q;
      for (q = 0; q < 200; q++) {
        memset(&res2, 0, sizeof(res2));
        req2.method = "GET";
        req2.path = "/openapi.json";
        g_crf_malloc_hook = fail_malloc_n;
        g_malloc_fail_after = q;
        c_rest_router_dispatch(router2, &req2, &res2);
        g_crf_malloc_hook = NULL;
        g_malloc_fail_after = -1;
        c_rest_response_cleanup(&res2);
      }
    }

    c_rest_router_destroy(router2);
  }

  {
    struct c_rest_request req2;
    struct c_rest_response res2;
    struct c_rest_router *router2 = NULL;
    c_rest_router_init(&router2);
    c_rest_router_add_openapi(router2, "GET", "/api/test", dummy_handler, NULL,
                              NULL);
    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));
    req2.method = "GET";
    req2.path = "/api/test";
    c_rest_router_dispatch(router2, &req2, &res2);
    c_rest_response_cleanup(&res2);
    c_rest_router_destroy(router2);
  }

  {
    int j;
    for (j = 0; j < 5; j++) {
      struct c_rest_router *router2 = NULL;
      c_rest_router_init(&router2);
      g_crf_malloc_hook = fail_malloc_n;
      g_malloc_fail_after = j;
      c_rest_enable_swagger_ui(router2, "/docs2", "/openapi.json");
      g_crf_malloc_hook = NULL;
      g_malloc_fail_after = -1;
      c_rest_router_destroy(router2);
    }
  }

  {
    struct c_rest_openapi_spec *sp = NULL;
    struct c_rest_openapi_operation op_dummy;
    memset(&op_dummy, 0, sizeof(op_dummy));
    c_rest_openapi_spec_init(NULL);
    c_rest_openapi_spec_destroy(NULL);
    c_rest_openapi_spec_to_json(NULL, NULL);
    c_rest_openapi_spec_init(&sp);
    c_rest_openapi_spec_add_component_schema(NULL, "a", "b");
    c_rest_openapi_spec_add_component_schema(sp, NULL, "b");
    c_rest_openapi_spec_add_component_schema(sp, "a", NULL);
    c_rest_openapi_spec_add_path(NULL, "route", "GET", &op_dummy);
    c_rest_openapi_spec_add_path(sp, NULL, "GET", &op_dummy);
    c_rest_openapi_spec_add_path(sp, "route", NULL, &op_dummy);
    c_rest_openapi_spec_add_path(sp, "route", "GET", NULL);
    c_rest_openapi_spec_destroy(sp);
  }

  {
    /* Hitting missing branches */
    struct c_rest_openapi_spec *sp = NULL;
    char *json_out = NULL;
    struct c_rest_openapi_operation op_empty;
    struct c_rest_openapi_operation op_partial;
    struct c_rest_openapi_security_requirement my_sec_req;
    const char *my_tags[2];

    c_rest_openapi_spec_init(&sp);

    /* Hit version non-NULL */
    if (sp->openapi_version) {
      CRF_FREE((void *)sp->openapi_version);
    }
    sp->openapi_version = CRF_STRDUP("3.1.0");

    memset(&op_empty, 0, sizeof(op_empty));
    c_rest_openapi_spec_add_path(sp, "/empty", "GET", &op_empty);
    c_rest_openapi_spec_add_path(sp, "/empty", "POST", &op_empty);

    /* Hitting early return combinations */
    {
      struct c_rest_openapi_operation op_desc_only;
      memset(&op_desc_only, 0, sizeof(op_desc_only));
      op_desc_only.description = "Only Desc";
      c_rest_openapi_spec_add_path(sp, "/desc", "GET", &op_desc_only);
    }
    {
      struct c_rest_openapi_operation op_tags_only;
      memset(&op_tags_only, 0, sizeof(op_tags_only));
      my_tags[0] = "tag1";
      op_tags_only.tags = my_tags;
      op_tags_only.n_tags = 1;
      c_rest_openapi_spec_add_path(sp, "/tags", "GET", &op_tags_only);
    }
    {
      struct c_rest_openapi_operation op_req_only;
      struct c_rest_openapi_request_body req;
      memset(&op_req_only, 0, sizeof(op_req_only));
      memset(&req, 0, sizeof(req));
      op_req_only.request_body = &req;
      c_rest_openapi_spec_add_path(sp, "/req", "GET", &op_req_only);
    }
    {
      struct c_rest_openapi_operation op_res_only;
      memset(&op_res_only, 0, sizeof(op_res_only));
      op_res_only.n_responses = 1;
      /* Notice responses is NULL, hits false branch of n_responses > 0 &&
       * responses */
      c_rest_openapi_spec_add_path(sp, "/res", "GET", &op_res_only);
    }
    {
      struct c_rest_openapi_operation op_id_only;
      memset(&op_id_only, 0, sizeof(op_id_only));
      op_id_only.operation_id = "justId";
      c_rest_openapi_spec_add_path(sp, "/id", "GET", &op_id_only);
    }
    {
      struct c_rest_openapi_operation op_sec_no_ptr;
      memset(&op_sec_no_ptr, 0, sizeof(op_sec_no_ptr));
      op_sec_no_ptr.summary = "sum";
      op_sec_no_ptr.n_security = 1;
      /* security pointer is NULL, hits false branch */
      c_rest_openapi_spec_add_path(sp, "/sec", "GET", &op_sec_no_ptr);
    }

    /* Partially filled operation */
    memset(&op_partial, 0, sizeof(op_partial));
    my_tags[0] = "tag1";
    my_tags[1] = NULL;
    op_partial.tags = my_tags;
    op_partial.n_tags = 2;

    memset(&my_sec_req, 0, sizeof(my_sec_req));
    my_sec_req.name = "oauth2";
    my_sec_req.n_scopes = 1;
    my_sec_req.scopes = NULL;
    op_partial.security = &my_sec_req;
    op_partial.n_security = 1;

    /* Cover external_docs with URL but no description */
    op_partial.external_docs.url = "http://example.com";
    op_partial.external_docs.description = NULL;

    c_rest_openapi_spec_add_path(sp, "/partial", "GET", &op_partial);
    c_rest_openapi_spec_add_component_schema(sp, "Invalid", "{invalid json");

    sp->n_servers = 1;
    sp->servers = (struct c_rest_openapi_server *)CRF_CALLOC(
        1, sizeof(struct c_rest_openapi_server));
    sp->servers[0].url = CRF_STRDUP("http://test");
    /* Tag testing */
    sp->n_tags = 2;
    sp->tags = (struct c_rest_openapi_tag *)CRF_CALLOC(
        2, sizeof(struct c_rest_openapi_tag));
    sp->tags[0].name = CRF_STRDUP("tag1");
    sp->tags[0].external_docs.url = CRF_STRDUP("http://tagurl");
    sp->tags[0].external_docs.description = NULL;
    sp->tags[1].name = CRF_STRDUP("tag2");

    sp->n_security = 1;
    sp->security = (struct c_rest_openapi_security_requirement *)CRF_CALLOC(
        1, sizeof(struct c_rest_openapi_security_requirement));
    sp->security[0].name = CRF_STRDUP("sec");

    /* Info contact testing combinations */
    sp->info.contact.name = CRF_STRDUP("Name");
    sp->info.contact.url = NULL;
    sp->info.contact.email = NULL;
    sp->info.license.name = CRF_STRDUP("License");

    sp->external_docs.url = CRF_STRDUP("http://docs");
    sp->external_docs.description = NULL;

    sp->swagger_openapi_url = CRF_STRDUP("/swagger.json");

    c_rest_openapi_spec_to_json(sp, &json_out);
    if (json_out)
      json_free_serialized_string(json_out);

    /* Second pass to cover the other contact combinations */
    CRF_FREE((void *)sp->info.contact.name);
    sp->info.contact.name = NULL;
    sp->info.contact.url = CRF_STRDUP("http://contact");
    c_rest_openapi_spec_to_json(sp, &json_out);
    if (json_out)
      json_free_serialized_string(json_out);

    CRF_FREE((void *)sp->info.contact.url);
    sp->info.contact.url = NULL;
    sp->info.contact.email = CRF_STRDUP("email@example.com");
    c_rest_openapi_spec_to_json(sp, &json_out);
    if (json_out)
      json_free_serialized_string(json_out);

    c_rest_openapi_spec_destroy(sp);
  }

  {
    /* Router docs testing edge cases */
    struct c_rest_router *r2 = NULL;
    char *jout = NULL;
    c_rest_router_init(&r2);
    /* router openapi spec getter edge cases */
    c_rest_router_get_openapi_spec(r2, NULL);
    c_rest_router_get_openapi_spec(NULL, NULL);

    /* enable openapi edge cases */
    c_rest_enable_openapi(r2, "/docs");
    c_rest_enable_openapi(NULL, "/docs");
    c_rest_enable_openapi(r2, NULL);

    c_rest_enable_swagger_ui(r2, "/docs", "/openapi.json");
    c_rest_enable_swagger_ui(NULL, "/docs", "/openapi.json");
    c_rest_enable_swagger_ui(r2, NULL, "/openapi.json");
    c_rest_enable_swagger_ui(r2, "/docs", NULL);

    /* force spec NULL inside swagger UI */
    {
      struct c_rest_router *r3 = NULL;
      struct c_rest_request req_ui;
      struct c_rest_response res_ui;
      struct c_rest_openapi_spec *sp3 = NULL;
      c_rest_router_init(&r3);
      c_rest_enable_swagger_ui(r3, "/docs", "/openapi.json");

      memset(&req_ui, 0, sizeof(req_ui));
      memset(&res_ui, 0, sizeof(res_ui));
      req_ui.method = "GET";
      req_ui.path = "/docs";

      /* Force swagger_openapi_url to NULL to hit the branch */
      c_rest_router_get_openapi_spec(r3, &sp3);
      if (sp3 && sp3->swagger_openapi_url) {
        CRF_FREE((void *)sp3->swagger_openapi_url);
        sp3->swagger_openapi_url = NULL;
      }

      c_rest_router_dispatch(r3, &req_ui, &res_ui);
      c_rest_response_cleanup(&res_ui);

      c_rest_router_destroy(r3);
    }

    /* get json edge case */
    c_rest_openapi_spec_to_json(NULL, &jout);
    c_rest_router_destroy(r2);
  }

  return 0;
}
static void *my_json_malloc(size_t sz) { return CRF_MALLOC(sz); }
static void my_json_free(void *ptr) { CRF_FREE(ptr); }

int test_openapi(void) {

  struct c_rest_router *router = NULL;
  struct c_rest_openapi_spec *spec = NULL;
  struct c_rest_openapi_operation op;
  const char *tags[] = {"test_tag"};
  int ret;
  char *json_str = NULL;

  extern void json_set_allocation_functions(void *(*)(size_t),
                                            void (*)(void *));

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

  json_set_allocation_functions(my_json_malloc, my_json_free);

  op_res_keys[0] = "application/json";
  op_req_keys[0] = "application/json";

  printf("Running OpenAPI tests...\n");

  ret = c_rest_router_init(&router);

  memset(&op, 0, sizeof(op));
  op.summary = "Test endpoint";
  op.description = "A dummy test endpoint";
  op.operation_id = "testEndpoint";
  op.deprecated = 1;
  op.tags = tags;
  op.n_tags = 1;
  op.external_docs.url = "http://example.com/docs";
  op.external_docs.description = "Endpoint Docs";

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

  (void)!c_rest_enable_openapi(router, "/openapi.json");
  (void)!c_rest_enable_swagger_ui(router, "/docs", "/openapi.json");

  op.summary = "Test Summary";
  op.description = "Test Desc";

  ret = c_rest_router_add_openapi(router, "GET", "/api/test", dummy_handler,
                                  NULL, &op);

  (void)!c_rest_router_get_openapi_spec(router, &spec);

  if (spec->n_paths > 0) {
    spec->paths[0].summary = CRF_STRDUP("test sum");
    spec->paths[0].description = CRF_STRDUP("test desc");
  }

  spec->info.title = CRF_STRDUP("My API");
  spec->info.version = CRF_STRDUP("2.0.0");

  spec->info.description = CRF_STRDUP("Test Description");
  spec->info.terms_of_service = CRF_STRDUP("http://example.com/tos");
  spec->info.contact.name = CRF_STRDUP("API Support");
  spec->info.contact.url = CRF_STRDUP("http://example.com/support");
  spec->info.contact.email = CRF_STRDUP("support@example.com");
  spec->info.license.name = CRF_STRDUP("Apache 2.0");
  spec->info.license.identifier = CRF_STRDUP("Apache-2.0");
  spec->info.license.url =
      CRF_STRDUP("http://www.apache.org/licenses/LICENSE-2.0.html");

  c_rest_enable_swagger_ui(router, "/docs", "/openapi.json");
  c_rest_enable_openapi(router, NULL);
  c_rest_enable_swagger_ui(NULL, "/docs", "/openapi.json");

  spec->external_docs.description = CRF_STRDUP("Find out more");
  spec->external_docs.url = CRF_STRDUP("http://swagger.io");

  spec->json_schema_dialect =
      CRF_STRDUP("http://json-schema.org/draft-07/schema#");

  {
    struct c_rest_openapi_server srv;
    struct c_rest_openapi_server_variable var;
    const char *enums[] = {"80", "443"};
    memset(&srv, 0, sizeof(srv));
    memset(&var, 0, sizeof(var));
    srv.url = CRF_STRDUP("http://example.com:{port}");
    srv.description = CRF_STRDUP("Test Server");
    var.name = CRF_STRDUP("port");
    var.default_value = CRF_STRDUP("443");
    var.description = CRF_STRDUP("Port number");
    var.enum_values = (const char **)CRF_MALLOC(2 * sizeof(char *));
    var.enum_values[0] = CRF_STRDUP(enums[0]);
    var.enum_values[1] = CRF_STRDUP(enums[1]);
    var.n_enum_values = 2;
    srv.variables =
        (struct c_rest_openapi_server_variable *)CRF_MALLOC(sizeof(var));
    memcpy(srv.variables, &var, sizeof(var));
    srv.n_variables = 1;
    spec->servers = (struct c_rest_openapi_server *)CRF_MALLOC(
        sizeof(struct c_rest_openapi_server));
    memcpy(spec->servers, &srv, sizeof(srv));
    spec->n_servers = 1;
  }

  {
    struct c_rest_openapi_tag tag;
    memset(&tag, 0, sizeof(tag));
    tag.name = CRF_STRDUP("test_tag");
    tag.summary = CRF_STRDUP("Test Tag Summary");
    tag.description = CRF_STRDUP("Test Tag Description");
    tag.external_docs.description = CRF_STRDUP("Docs");
    tag.external_docs.url = CRF_STRDUP("http://example.com");
    tag.parent = CRF_STRDUP("parent_tag");
    tag.kind = CRF_STRDUP("navigation");
    spec->tags = (struct c_rest_openapi_tag *)CRF_MALLOC(
        sizeof(struct c_rest_openapi_tag));
    memcpy(spec->tags, &tag, sizeof(tag));
    spec->n_tags = 1;
  }

  {
    struct c_rest_openapi_security_scheme scheme;

    memset(&scheme, 0, sizeof(scheme));

    scheme.name_key = CRF_STRDUP("oauth2");
    scheme.type = CRF_STRDUP("oauth2");
    scheme.description = CRF_STRDUP("OAuth2");
    scheme.name = CRF_STRDUP("Authorization");
    scheme.in = CRF_STRDUP("header");
    scheme.scheme = CRF_STRDUP("bearer");
    scheme.bearer_format = CRF_STRDUP("JWT");
    scheme.open_id_connect_url = CRF_STRDUP("http://example.com/.well-known");

    /* We test one flow, the others are structurally identical but let's just
     * test implicit for coverage */
    scheme.flows.implicit = (struct c_rest_openapi_oauth_flow *)CRF_MALLOC(
        sizeof(struct c_rest_openapi_oauth_flow));
    memset(scheme.flows.implicit, 0, sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.implicit->authorization_url =
        CRF_STRDUP("http://example.com/auth");
    scheme.flows.implicit->token_url = CRF_STRDUP("http://example.com/token");
    scheme.flows.implicit->refresh_url =
        CRF_STRDUP("http://example.com/refresh");
    scheme.flows.implicit->scopes_keys =
        (const char **)CRF_MALLOC(sizeof(char *));
    scheme.flows.implicit->scopes_keys[0] = CRF_STRDUP("read");
    scheme.flows.implicit->scopes_values =
        (const char **)CRF_MALLOC(sizeof(char *));
    scheme.flows.implicit->scopes_values[0] = CRF_STRDUP("Read access");
    scheme.flows.implicit->n_scopes = 1;

    scheme.flows.password = (struct c_rest_openapi_oauth_flow *)CRF_MALLOC(
        sizeof(struct c_rest_openapi_oauth_flow));
    memset(scheme.flows.password, 0, sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.password->authorization_url =
        CRF_STRDUP("http://example.com/auth");
    scheme.flows.password->token_url = CRF_STRDUP("http://example.com/token");
    scheme.flows.password->refresh_url =
        CRF_STRDUP("http://example.com/refresh");
    scheme.flows.password->scopes_keys =
        (const char **)CRF_MALLOC(sizeof(char *));
    scheme.flows.password->scopes_keys[0] = CRF_STRDUP("read");
    scheme.flows.password->scopes_values =
        (const char **)CRF_MALLOC(sizeof(char *));
    scheme.flows.password->scopes_values[0] = CRF_STRDUP("Read access");
    scheme.flows.password->n_scopes = 1;

    scheme.flows.client_credentials =
        (struct c_rest_openapi_oauth_flow *)CRF_MALLOC(
            sizeof(struct c_rest_openapi_oauth_flow));
    memset(scheme.flows.client_credentials, 0,
           sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.client_credentials->authorization_url =
        CRF_STRDUP("http://example.com/auth");
    scheme.flows.client_credentials->token_url =
        CRF_STRDUP("http://example.com/token");
    scheme.flows.client_credentials->refresh_url =
        CRF_STRDUP("http://example.com/refresh");
    scheme.flows.client_credentials->scopes_keys =
        (const char **)CRF_MALLOC(sizeof(char *));
    scheme.flows.client_credentials->scopes_keys[0] = CRF_STRDUP("read");
    scheme.flows.client_credentials->scopes_values =
        (const char **)CRF_MALLOC(sizeof(char *));
    scheme.flows.client_credentials->scopes_values[0] =
        CRF_STRDUP("Read access");
    scheme.flows.client_credentials->n_scopes = 1;

    scheme.flows.authorization_code =
        (struct c_rest_openapi_oauth_flow *)CRF_MALLOC(
            sizeof(struct c_rest_openapi_oauth_flow));
    memset(scheme.flows.authorization_code, 0,
           sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.authorization_code->authorization_url =
        CRF_STRDUP("http://example.com/auth");
    scheme.flows.authorization_code->token_url =
        CRF_STRDUP("http://example.com/token");
    scheme.flows.authorization_code->refresh_url =
        CRF_STRDUP("http://example.com/refresh");
    scheme.flows.authorization_code->scopes_keys =
        (const char **)CRF_MALLOC(sizeof(char *));
    scheme.flows.authorization_code->scopes_keys[0] = CRF_STRDUP("read");
    scheme.flows.authorization_code->scopes_values =
        (const char **)CRF_MALLOC(sizeof(char *));
    scheme.flows.authorization_code->scopes_values[0] =
        CRF_STRDUP("Read access");
    scheme.flows.authorization_code->n_scopes = 1;

    spec->security_schemes =
        (struct c_rest_openapi_security_scheme *)CRF_MALLOC(
            sizeof(struct c_rest_openapi_security_scheme));
    memcpy(spec->security_schemes, &scheme, sizeof(scheme));
    spec->n_security_schemes = 1;

    spec->security = (struct c_rest_openapi_security_requirement *)CRF_MALLOC(
        sizeof(struct c_rest_openapi_security_requirement));
    spec->security[0].name = CRF_STRDUP("oauth2");
    spec->security[0].scopes = (const char **)CRF_MALLOC(sizeof(char *));
    spec->security[0].scopes[0] = CRF_STRDUP(scopes[0]);
    spec->security[0].n_scopes = 1;
    spec->n_security = 1;
  }

  ret = c_rest_openapi_spec_add_component_schema(
      spec, "DummyReq",
      "{\"type\":\"object\",\"properties\":{\"id\":{\"type\":\"integer\"}}}");

  for (ret = 0; ret < 20; ret++) {
    char buf[32];
    sprintf(buf, "CompTest%d", ret);
    c_rest_openapi_spec_add_component_schema(spec, buf, "{}");
    sprintf(buf, "/api/more%d", ret);
    c_rest_openapi_spec_add_path(spec, buf, "GET", &op);
  }

  ret = c_rest_openapi_spec_to_json(spec, &json_str);

  if (json_str)
    json_free_serialized_string(json_str);

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  req.method = "GET";
  req.path = "/docs";
  (void)!c_rest_router_dispatch(router, &req, &res);

  (void)!c_rest_response_cleanup(&res);

  (void)!c_rest_router_destroy(router);

  test_openapi_errors();

  {
    struct c_rest_openapi_spec *spec_null_flows = NULL;
    struct c_rest_openapi_security_scheme sch;
    char *json_out = NULL;
    struct c_rest_openapi_server srv;
    struct c_rest_openapi_server_variable var;

    c_rest_openapi_spec_init(&spec_null_flows);

    memset(&srv, 0, sizeof(srv));
    srv.url = CRF_STRDUP("http://test.com");
    spec_null_flows->servers = (struct c_rest_openapi_server *)CRF_MALLOC(
        2 * sizeof(struct c_rest_openapi_server));
    memcpy(&spec_null_flows->servers[0], &srv, sizeof(srv));

    memset(&var, 0, sizeof(var));
    var.name = CRF_STRDUP("var");
    srv.variables =
        (struct c_rest_openapi_server_variable *)CRF_MALLOC(sizeof(var));
    memcpy(srv.variables, &var, sizeof(var));
    srv.n_variables = 1;
    srv.url = NULL;
    memcpy(&spec_null_flows->servers[1], &srv, sizeof(srv));
    spec_null_flows->n_servers = 2;

    memset(&sch, 0, sizeof(sch));
    sch.name_key = CRF_STRDUP("test_key");

    sch.flows.implicit = (struct c_rest_openapi_oauth_flow *)CRF_MALLOC(
        sizeof(struct c_rest_openapi_oauth_flow));
    memset(sch.flows.implicit, 0, sizeof(struct c_rest_openapi_oauth_flow));
    sch.flows.password = (struct c_rest_openapi_oauth_flow *)CRF_MALLOC(
        sizeof(struct c_rest_openapi_oauth_flow));
    memset(sch.flows.password, 0, sizeof(struct c_rest_openapi_oauth_flow));
    sch.flows.client_credentials =
        (struct c_rest_openapi_oauth_flow *)CRF_MALLOC(
            sizeof(struct c_rest_openapi_oauth_flow));
    memset(sch.flows.client_credentials, 0,
           sizeof(struct c_rest_openapi_oauth_flow));
    sch.flows.authorization_code =
        (struct c_rest_openapi_oauth_flow *)CRF_MALLOC(
            sizeof(struct c_rest_openapi_oauth_flow));
    memset(sch.flows.authorization_code, 0,
           sizeof(struct c_rest_openapi_oauth_flow));

    spec_null_flows->security_schemes =
        (struct c_rest_openapi_security_scheme *)CRF_MALLOC(
            sizeof(struct c_rest_openapi_security_scheme));
    memcpy(spec_null_flows->security_schemes, &sch, sizeof(sch));
    spec_null_flows->n_security_schemes = 1;

    c_rest_openapi_spec_to_json(spec_null_flows, &json_out);
    if (json_out)
      json_free_serialized_string(json_out);

    c_rest_openapi_spec_destroy(spec_null_flows);
  }

  {
    struct c_rest_openapi_spec *spec_noflows = NULL;
    struct c_rest_openapi_security_scheme sch;
    char *json_out = NULL;
    c_rest_openapi_spec_init(&spec_noflows);

    memset(&sch, 0, sizeof(sch));
    sch.name_key = CRF_STRDUP("test_key2");
    spec_noflows->security_schemes =
        (struct c_rest_openapi_security_scheme *)CRF_MALLOC(
            sizeof(struct c_rest_openapi_security_scheme));
    memcpy(spec_noflows->security_schemes, &sch, sizeof(sch));
    spec_noflows->n_security_schemes = 1;

    c_rest_openapi_spec_to_json(spec_noflows, &json_out);
    if (json_out)
      json_free_serialized_string(json_out);

    c_rest_openapi_spec_destroy(spec_noflows);
  }

  {
    struct c_rest_openapi_spec *spec_empty = NULL;
    struct c_rest_openapi_operation op_empty;
    char *json_out_empty = NULL;
    c_rest_openapi_spec_init(&spec_empty);
    memset(&op_empty, 0, sizeof(op_empty));
    c_rest_openapi_spec_add_path(spec_empty, "/api/empty", "GET", &op_empty);
    c_rest_openapi_spec_to_json(spec_empty, &json_out_empty);
    if (json_out_empty)
      json_free_serialized_string(json_out_empty);
    c_rest_openapi_spec_destroy(spec_empty);
  }

  {
    struct c_rest_openapi_spec *sp = NULL;
    struct c_rest_openapi_operation op_dummy;
    memset(&op_dummy, 0, sizeof(op_dummy));
    c_rest_openapi_spec_init(NULL);
    c_rest_openapi_spec_destroy(NULL);
    c_rest_openapi_spec_to_json(NULL, NULL);
    c_rest_openapi_spec_init(&sp);
    c_rest_openapi_spec_add_component_schema(NULL, "a", "b");
    c_rest_openapi_spec_add_component_schema(sp, NULL, "b");
    c_rest_openapi_spec_add_component_schema(sp, "a", NULL);
    c_rest_openapi_spec_add_path(NULL, "route", "GET", &op_dummy);
    c_rest_openapi_spec_add_path(sp, NULL, "GET", &op_dummy);
    c_rest_openapi_spec_add_path(sp, "route", NULL, &op_dummy);
    c_rest_openapi_spec_add_path(sp, "route", "GET", NULL);
    c_rest_openapi_spec_destroy(sp);
  }

  return 0;
}
