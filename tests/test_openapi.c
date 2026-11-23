/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_openapi.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_testing_mocks.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parson.h"
/* clang-format on */

static char *test_strdup(const char *s) {
  char *out = NULL;
  if (c_rest_internal_strdup(s, &out) != C_REST_OK)
    return NULL;
  return out;
}

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

static void *my_json_malloc(size_t sz) {
  void *p = NULL;
  if (C_REST_MALLOC(sz, &p) != C_REST_OK)
    return NULL;
  return p;
}
static void my_json_free(void *ptr) { C_REST_FREE(ptr); }

TEST test_openapi_oom_and_edge_cases(void) {
  struct c_rest_openapi_spec *spec = NULL;
  char *json_str = NULL;
  int i;
  struct c_rest_openapi_operation op;
  const char *tags[] = {"test_tag"};
  const char *scopes[] = {"read"};
  struct c_rest_openapi_security_requirement sec_req;
  c_rest_error_t rc;

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
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_init(&tmpspec));

      g_crf_malloc_hook = fail_malloc_n;
      g_crf_realloc_hook = fail_realloc_n;
      g_malloc_fail_after = k;
      g_fail_realloc_at = k;

      rc = c_rest_openapi_spec_add_component_schema(tmpspec, "D1", "{}");
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);

      g_crf_malloc_hook = NULL;
      g_crf_realloc_hook = NULL;

      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(tmpspec));
    }
  }

  {
    int k;
    for (k = 0; k < 30; k++) {
      struct c_rest_openapi_spec *tmpspec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_init(&tmpspec));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_component_schema(tmpspec, "D1", "{}"));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_component_schema(tmpspec, "D2", "{}"));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_component_schema(tmpspec, "D3", "{}"));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_component_schema(tmpspec, "D4", "{}"));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_component_schema(tmpspec, "D5", "{}"));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_component_schema(tmpspec, "D6", "{}"));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_component_schema(tmpspec, "D7", "{}"));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_component_schema(tmpspec, "D8", "{}"));

      g_crf_malloc_hook = fail_malloc_n;
      g_crf_realloc_hook = fail_realloc_n;
      g_malloc_fail_after = k;
      g_fail_realloc_at = k;

      rc = c_rest_openapi_spec_add_component_schema(tmpspec, "D9", "{}");
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);

      g_crf_malloc_hook = NULL;
      g_crf_realloc_hook = NULL;

      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(tmpspec));
    }
  }

  {
    int m;
    for (m = 0; m < 30; m++) {
      struct c_rest_openapi_spec *tmpspec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_init(&tmpspec));

      g_crf_malloc_hook = fail_malloc_n;
      g_crf_realloc_hook = fail_realloc_n;
      g_malloc_fail_after = m;
      g_fail_realloc_at = m;

      rc = c_rest_openapi_spec_add_path(tmpspec, "p1", "GET", &op);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);

      g_crf_malloc_hook = NULL;
      g_crf_realloc_hook = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(tmpspec));
    }
  }

  {
    int m;
    for (m = 0; m < 50; m++) {
      struct c_rest_openapi_spec *tmpspec = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_init(&tmpspec));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_path(tmpspec, "p1", "GET", &op));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_path(tmpspec, "p2", "GET", &op));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_path(tmpspec, "p3", "GET", &op));
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_path(tmpspec, "p4", "GET", &op));

      g_crf_malloc_hook = fail_malloc_n;
      g_crf_realloc_hook = fail_realloc_n;
      g_malloc_fail_after = m;
      g_fail_realloc_at = m;

      rc = c_rest_openapi_spec_add_path(tmpspec, "p5", "GET", &op);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);

      g_crf_malloc_hook = NULL;
      g_crf_realloc_hook = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(tmpspec));
    }
  }

  {
    struct c_rest_openapi_spec *tmpspec = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_init(&tmpspec));
    for (i = 0; i < 10; i++) {
      char buf[32];
#if defined(_MSC_VER)
      sprintf_s(buf, sizeof(buf), "Comp%d", i);
#else
      sprintf(buf, "Comp%d", i);
#endif
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_component_schema(tmpspec, buf, "{}"));
    }
    ASSERT_NEQ(C_REST_OK, c_rest_openapi_spec_to_json(tmpspec, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(tmpspec));
  }

  ASSERT_NEQ(C_REST_OK, c_rest_openapi_spec_to_json(NULL, &json_str));

  {
    struct c_rest_router *router = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&router));

    ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(router, "GET", "/api/test",
                                                   dummy_handler, NULL, &op));
    ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(router, "POST", "/api/test",
                                                   dummy_handler, NULL, &op));
    ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(router, "PUT", "/api/test",
                                                   dummy_handler, NULL, &op));
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add_openapi(router, "DELETE", "/api/test",
                                        dummy_handler, NULL, &op));
    ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(router, "PATCH", "/api/test",
                                                   dummy_handler, NULL, &op));
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add_openapi(router, "OPTIONS", "/api/test",
                                        dummy_handler, NULL, &op));
    ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(router, "HEAD", "/api/test",
                                                   dummy_handler, NULL, &op));
    ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(router, "TRACE", "/api/test",
                                                   dummy_handler, NULL, &op));
    ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(router, "QUERY", "/api/test",
                                                   dummy_handler, NULL, &op));
    ASSERT_EQ(C_REST_OK,
              c_rest_router_add_openapi(router, "UNKNOWN", "/api/test",
                                        dummy_handler, NULL, &op));

    ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(router, "GET", "/api/test2",
                                                   dummy_handler, NULL, &op));

    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router));
  }

  {
    struct c_rest_router *router = NULL;
    char m_get[] = "GET";
    char p_docs1[] = "/docs1";
    char p_json[] = "/openapi.json";
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&router));
    ASSERT_EQ(C_REST_OK, c_rest_enable_openapi(router, "/openapi.json"));
    ASSERT_EQ(C_REST_OK, c_rest_router_get_openapi_spec(router, &spec));

    for (i = 0; i < 250; i++) {
      g_crf_malloc_hook = fail_malloc_n;
      g_malloc_fail_after = i;
      json_str = NULL;
      rc = c_rest_openapi_spec_to_json(spec, &json_str);
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      json_free_serialized_string(json_str);
    }
    g_crf_malloc_hook = NULL;
    g_malloc_fail_after = -1;

    ASSERT_EQ(C_REST_OK,
              c_rest_enable_swagger_ui(router, "/docs1", "/openapi.json"));
    {
      struct c_rest_request req2;
      struct c_rest_response res2;
      int jj;
      for (jj = 0; jj < 50; jj++) {
        memset(&req2, 0, sizeof(req2));
        memset(&res2, 0, sizeof(res2));
        req2.method = m_get;
        req2.path = p_docs1;
        g_crf_malloc_hook = fail_malloc_n;
        g_malloc_fail_after = jj;
        rc = c_rest_router_dispatch(router, &req2, &res2);
        ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
               rc == C_REST_ERROR_GENERIC);
        ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2));
        memset(&res2, 0, sizeof(res2));
        req2.path = p_json;
        rc = c_rest_router_dispatch(router, &req2, &res2);
        ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
               rc == C_REST_ERROR_GENERIC);
        g_crf_malloc_hook = NULL;
        g_malloc_fail_after = -1;
        ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2));
      }
    }

    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router));
  }

  {
    struct c_rest_request req2;
    struct c_rest_response res2;
    struct c_rest_router *router2 = NULL;
    char m_get[] = "GET";
    char p_json[] = "/openapi.json";
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&router2));
    ASSERT_EQ(C_REST_OK, c_rest_enable_openapi(router2, "/openapi.json"));
    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));
    req2.method = m_get;
    req2.path = p_json;

    ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router2, &req2, &res2));
    ASSERT(res2.body != NULL);
    ASSERT(strstr(res2.body, "openapi") != NULL);
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2));

    {
      int q;
      for (q = 0; q < 200; q++) {
        memset(&res2, 0, sizeof(res2));
        req2.method = m_get;
        req2.path = p_json;
        g_crf_malloc_hook = fail_malloc_n;
        g_malloc_fail_after = q;
        rc = c_rest_router_dispatch(router2, &req2, &res2);
        ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
               rc == C_REST_ERROR_GENERIC);
        g_crf_malloc_hook = NULL;
        g_malloc_fail_after = -1;
        ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2));
      }
    }

    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router2));
  }

  {
    struct c_rest_request req2;
    struct c_rest_response res2;
    struct c_rest_router *router2 = NULL;
    char m_get[] = "GET";
    char p_test[] = "/api/test";
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&router2));
    ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(router2, "GET", "/api/test",
                                                   dummy_handler, NULL, NULL));
    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));
    req2.method = m_get;
    req2.path = p_test;
    ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router2, &req2, &res2));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res2));
    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router2));
  }

  {
    int j;
    for (j = 0; j < 5; j++) {
      struct c_rest_router *router2 = NULL;
      ASSERT_EQ(C_REST_OK, c_rest_router_init(&router2));
      g_crf_malloc_hook = fail_malloc_n;
      g_malloc_fail_after = j;
      rc = c_rest_enable_swagger_ui(router2, "/docs2", "/openapi.json");
      ASSERT(rc == C_REST_OK || rc == C_REST_ERROR_OOM ||
             rc == C_REST_ERROR_GENERIC);
      g_crf_malloc_hook = NULL;
      g_malloc_fail_after = -1;
      ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router2));
    }
  }

  {
    struct c_rest_openapi_spec *sp = NULL;
    struct c_rest_openapi_operation op_dummy;
    memset(&op_dummy, 0, sizeof(op_dummy));
    ASSERT_NEQ(C_REST_OK, c_rest_openapi_spec_init(NULL));
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_openapi_spec_to_json(NULL, NULL));
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_init(&sp));
    ASSERT_NEQ(C_REST_OK,
               c_rest_openapi_spec_add_component_schema(NULL, "a", "b"));
    ASSERT_NEQ(C_REST_OK,
               c_rest_openapi_spec_add_component_schema(sp, NULL, "b"));
    ASSERT_NEQ(C_REST_OK,
               c_rest_openapi_spec_add_component_schema(sp, "a", NULL));
    ASSERT_NEQ(C_REST_OK,
               c_rest_openapi_spec_add_path(NULL, "route", "GET", &op_dummy));
    ASSERT_NEQ(C_REST_OK,
               c_rest_openapi_spec_add_path(sp, NULL, "GET", &op_dummy));
    ASSERT_NEQ(C_REST_OK,
               c_rest_openapi_spec_add_path(sp, "route", NULL, &op_dummy));
    ASSERT_NEQ(C_REST_OK,
               c_rest_openapi_spec_add_path(sp, "route", "GET", NULL));
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(sp));
  }

  PASS();
}

TEST test_openapi_partial_schemas_and_router_docs(void) {
  {
    struct c_rest_openapi_spec *sp = NULL;
    char *json_out = NULL;
    struct c_rest_openapi_operation op_empty;
    struct c_rest_openapi_operation op_partial;
    struct c_rest_openapi_security_requirement my_sec_req;
    const char *my_tags[2];

    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_init(&sp));
    sp->openapi_version = test_strdup("3.1.0");

    memset(&op_empty, 0, sizeof(op_empty));
    ASSERT_EQ(C_REST_OK,
              c_rest_openapi_spec_add_path(sp, "/empty", "GET", &op_empty));
    ASSERT_EQ(C_REST_OK,
              c_rest_openapi_spec_add_path(sp, "/empty", "POST", &op_empty));

    {
      struct c_rest_openapi_operation op_desc_only;
      memset(&op_desc_only, 0, sizeof(op_desc_only));
      op_desc_only.description = "Only Desc";
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_add_path(sp, "/desc", "GET",
                                                        &op_desc_only));
    }
    {
      struct c_rest_openapi_operation op_tags_only;
      memset(&op_tags_only, 0, sizeof(op_tags_only));
      my_tags[0] = "tag1";
      op_tags_only.tags = my_tags;
      op_tags_only.n_tags = 1;
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_add_path(sp, "/tags", "GET",
                                                        &op_tags_only));
    }
    {
      struct c_rest_openapi_operation op_req_only;
      struct c_rest_openapi_request_body req;
      memset(&op_req_only, 0, sizeof(op_req_only));
      memset(&req, 0, sizeof(req));
      op_req_only.request_body = &req;
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_path(sp, "/req", "GET", &op_req_only));
    }
    {
      struct c_rest_openapi_operation op_res_only;
      memset(&op_res_only, 0, sizeof(op_res_only));
      op_res_only.n_responses = 1;
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_path(sp, "/res", "GET", &op_res_only));
    }
    {
      struct c_rest_openapi_operation op_id_only;
      memset(&op_id_only, 0, sizeof(op_id_only));
      op_id_only.operation_id = "justId";
      ASSERT_EQ(C_REST_OK,
                c_rest_openapi_spec_add_path(sp, "/id", "GET", &op_id_only));
    }
    {
      struct c_rest_openapi_operation op_sec_no_ptr;
      memset(&op_sec_no_ptr, 0, sizeof(op_sec_no_ptr));
      op_sec_no_ptr.summary = "sum";
      op_sec_no_ptr.n_security = 1;
      ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_add_path(sp, "/sec", "GET",
                                                        &op_sec_no_ptr));
    }

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
    op_partial.external_docs.url = "http://example.com";
    op_partial.external_docs.description = NULL;

    ASSERT_EQ(C_REST_OK,
              c_rest_openapi_spec_add_path(sp, "/partial", "GET", &op_partial));
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_add_component_schema(
                             sp, "Invalid", "{invalid json"));

    sp->n_servers = 1;
    ASSERT_EQ(C_REST_OK, C_REST_CALLOC(1, sizeof(struct c_rest_openapi_server),
                                       &sp->servers));
    sp->servers[0].url = test_strdup("http://test");
    sp->n_tags = 2;
    ASSERT_EQ(C_REST_OK,
              C_REST_CALLOC(2, sizeof(struct c_rest_openapi_tag), &sp->tags));
    sp->tags[0].name = test_strdup("tag1");
    sp->tags[0].external_docs.url = test_strdup("http://tagurl");
    sp->tags[0].external_docs.description = NULL;
    sp->tags[1].name = test_strdup("tag2");

    sp->n_security = 1;
    ASSERT_EQ(C_REST_OK,
              C_REST_CALLOC(1,
                            sizeof(struct c_rest_openapi_security_requirement),
                            &sp->security));
    sp->security[0].name = test_strdup("sec");

    sp->info.contact.name = test_strdup("Name");
    sp->info.contact.url = NULL;
    sp->info.contact.email = NULL;
    sp->info.license.name = test_strdup("License");
    sp->external_docs.url = test_strdup("http://docs");
    sp->external_docs.description = NULL;
    sp->swagger_openapi_url = test_strdup("/swagger.json");

    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_to_json(sp, &json_out));
    ASSERT(json_out != NULL);
    ASSERT(strstr(json_out, "3.1.0") != NULL);
    json_free_serialized_string(json_out);

    C_REST_FREE((void *)sp->info.contact.name);
    sp->info.contact.name = NULL;
    sp->info.contact.url = test_strdup("http://contact");
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_to_json(sp, &json_out));
    ASSERT(strstr(json_out, "contact") != NULL);
    json_free_serialized_string(json_out);

    C_REST_FREE((void *)sp->info.contact.url);
    sp->info.contact.url = NULL;
    sp->info.contact.email = test_strdup("email@example.com");
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_to_json(sp, &json_out));
    ASSERT(strstr(json_out, "email@example.com") != NULL);
    json_free_serialized_string(json_out);

    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(sp));
  }

  {
    struct c_rest_router *r2 = NULL;
    char *jout = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_router_init(&r2));
    ASSERT_NEQ(C_REST_OK, c_rest_router_get_openapi_spec(r2, NULL));
    ASSERT_NEQ(C_REST_OK, c_rest_router_get_openapi_spec(NULL, NULL));

    ASSERT_EQ(C_REST_OK, c_rest_enable_openapi(r2, "/docs"));
    ASSERT_NEQ(C_REST_OK, c_rest_enable_openapi(NULL, "/docs"));
    ASSERT_NEQ(C_REST_OK, c_rest_enable_openapi(r2, NULL));

    ASSERT_EQ(C_REST_OK,
              c_rest_enable_swagger_ui(r2, "/docs", "/openapi.json"));
    ASSERT_NEQ(C_REST_OK,
               c_rest_enable_swagger_ui(NULL, "/docs", "/openapi.json"));
    ASSERT_NEQ(C_REST_OK, c_rest_enable_swagger_ui(r2, NULL, "/openapi.json"));
    ASSERT_NEQ(C_REST_OK, c_rest_enable_swagger_ui(r2, "/docs", NULL));

    {
      struct c_rest_router *r3 = NULL;
      struct c_rest_request req_ui;
      struct c_rest_response res_ui;
      struct c_rest_openapi_spec *sp3 = NULL;
      char m_get[] = "GET";
      char p_docs[] = "/docs";
      ASSERT_EQ(C_REST_OK, c_rest_router_init(&r3));
      ASSERT_EQ(C_REST_OK,
                c_rest_enable_swagger_ui(r3, "/docs", "/openapi.json"));

      memset(&req_ui, 0, sizeof(req_ui));
      memset(&res_ui, 0, sizeof(res_ui));
      req_ui.method = m_get;
      req_ui.path = p_docs;

      ASSERT_EQ(C_REST_OK, c_rest_router_get_openapi_spec(r3, &sp3));
      C_REST_FREE((void *)sp3->swagger_openapi_url);
      sp3->swagger_openapi_url = NULL;

      ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(r3, &req_ui, &res_ui));
      ASSERT(res_ui.body != NULL);
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_ui));
      ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r3));
    }

    ASSERT_NEQ(C_REST_OK, c_rest_openapi_spec_to_json(NULL, &jout));
    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r2));
  }

  PASS();
}

TEST test_openapi_full_spec_and_security(void) {
  struct c_rest_router *router = NULL;
  struct c_rest_openapi_spec *spec = NULL;
  struct c_rest_openapi_operation op;
  const char *tags[] = {"test_tag"};
  int idx;
  char *json_str = NULL;

  struct c_rest_openapi_response op_res;
  struct c_rest_openapi_media_type op_res_mt;
  const char *op_res_keys[1];

  struct c_rest_openapi_request_body op_req;
  struct c_rest_openapi_media_type op_req_mt;
  const char *op_req_keys[1];

  struct c_rest_request req;
  struct c_rest_response res;
  char m_get[] = "GET";
  char p_docs[] = "/docs";

  struct c_rest_openapi_parameter param;
  struct c_rest_openapi_header header;
  const char *header_keys[] = {"X-Test"};
  const char *scopes[] = {"read"};
  struct c_rest_openapi_security_requirement sec_req;

  json_set_allocation_functions(my_json_malloc, my_json_free);

  op_res_keys[0] = "application/json";
  op_req_keys[0] = "application/json";

  ASSERT_EQ(C_REST_OK, c_rest_router_init(&router));

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

  ASSERT_EQ(C_REST_OK, c_rest_enable_openapi(router, "/openapi.json"));
  ASSERT_EQ(C_REST_OK,
            c_rest_enable_swagger_ui(router, "/docs", "/openapi.json"));

  op.summary = "Test Summary";
  op.description = "Test Desc";

  ASSERT_EQ(C_REST_OK, c_rest_router_add_openapi(router, "GET", "/api/test",
                                                 dummy_handler, NULL, &op));
  ASSERT_EQ(C_REST_OK, c_rest_router_get_openapi_spec(router, &spec));
  ASSERT(spec != NULL);

  spec->paths[0].summary = test_strdup("test sum");
  spec->paths[0].description = test_strdup("test desc");
  spec->info.title = test_strdup("My API");
  spec->info.version = test_strdup("2.0.0");
  spec->info.description = test_strdup("Test Description");
  spec->info.terms_of_service = test_strdup("http://example.com/tos");
  spec->info.contact.name = test_strdup("API Support");
  spec->info.contact.url = test_strdup("http://example.com/support");
  spec->info.contact.email = test_strdup("support@example.com");
  spec->info.license.name = test_strdup("Apache 2.0");
  spec->info.license.identifier = test_strdup("Apache-2.0");
  spec->info.license.url =
      test_strdup("http://www.apache.org/licenses/LICENSE-2.0.html");

  ASSERT_EQ(C_REST_OK,
            c_rest_enable_swagger_ui(router, "/docs", "/openapi.json"));
  ASSERT_NEQ(C_REST_OK, c_rest_enable_openapi(router, NULL));
  ASSERT_NEQ(C_REST_OK,
             c_rest_enable_swagger_ui(NULL, "/docs", "/openapi.json"));

  spec->external_docs.description = test_strdup("Find out more");
  spec->external_docs.url = test_strdup("http://swagger.io");
  spec->json_schema_dialect =
      test_strdup("http://json-schema.org/draft-07/schema#");

  {
    struct c_rest_openapi_server srv;
    struct c_rest_openapi_server_variable var;
    const char *enums[] = {"80", "443"};
    memset(&srv, 0, sizeof(srv));
    memset(&var, 0, sizeof(var));
    srv.url = test_strdup("http://example.com:{port}");
    srv.description = test_strdup("Test Server");
    var.name = test_strdup("port");
    var.default_value = test_strdup("443");
    var.description = test_strdup("Port number");
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(2 * sizeof(char *), &var.enum_values));
    var.enum_values[0] = test_strdup(enums[0]);
    var.enum_values[1] = test_strdup(enums[1]);
    var.n_enum_values = 2;
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(var), &srv.variables));
#if defined(_MSC_VER)
    memcpy_s(srv.variables, sizeof(var), &var, sizeof(var));
#else
    memcpy(srv.variables, &var, sizeof(var));
#endif
    srv.n_variables = 1;
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(struct c_rest_openapi_server),
                                       &spec->servers));
#if defined(_MSC_VER)
    memcpy_s(spec->servers, sizeof(srv), &srv, sizeof(srv));
#else
    memcpy(spec->servers, &srv, sizeof(srv));
#endif
    spec->n_servers = 1;
  }

  {
    struct c_rest_openapi_tag tag;
    memset(&tag, 0, sizeof(tag));
    tag.name = test_strdup("test_tag");
    tag.summary = test_strdup("Test Tag Summary");
    tag.description = test_strdup("Test Tag Description");
    tag.external_docs.description = test_strdup("Docs");
    tag.external_docs.url = test_strdup("http://example.com");
    tag.parent = test_strdup("parent_tag");
    tag.kind = test_strdup("navigation");
    ASSERT_EQ(C_REST_OK,
              C_REST_MALLOC(sizeof(struct c_rest_openapi_tag), &spec->tags));
#if defined(_MSC_VER)
    memcpy_s(spec->tags, sizeof(tag), &tag, sizeof(tag));
#else
    memcpy(spec->tags, &tag, sizeof(tag));
#endif
    spec->n_tags = 1;
  }

  {
    struct c_rest_openapi_security_scheme scheme;
    memset(&scheme, 0, sizeof(scheme));

    scheme.name_key = test_strdup("oauth2");
    scheme.type = test_strdup("oauth2");
    scheme.description = test_strdup("OAuth2");
    scheme.name = test_strdup("Authorization");
    scheme.in = test_strdup("header");
    scheme.scheme = test_strdup("bearer");
    scheme.bearer_format = test_strdup("JWT");
    scheme.open_id_connect_url = test_strdup("http://example.com/.well-known");

    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(struct c_rest_openapi_oauth_flow),
                                       &scheme.flows.implicit));
    memset(scheme.flows.implicit, 0, sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.implicit->authorization_url =
        test_strdup("http://example.com/auth");
    scheme.flows.implicit->token_url = test_strdup("http://example.com/token");
    scheme.flows.implicit->refresh_url =
        test_strdup("http://example.com/refresh");
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(char *),
                                       &scheme.flows.implicit->scopes_keys));
    scheme.flows.implicit->scopes_keys[0] = test_strdup("read");
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(char *),
                                       &scheme.flows.implicit->scopes_values));
    scheme.flows.implicit->scopes_values[0] = test_strdup("Read access");
    scheme.flows.implicit->n_scopes = 1;

    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(struct c_rest_openapi_oauth_flow),
                                       &scheme.flows.password));
    memset(scheme.flows.password, 0, sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.password->authorization_url =
        test_strdup("http://example.com/auth");
    scheme.flows.password->token_url = test_strdup("http://example.com/token");
    scheme.flows.password->refresh_url =
        test_strdup("http://example.com/refresh");
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(char *),
                                       &scheme.flows.password->scopes_keys));
    scheme.flows.password->scopes_keys[0] = test_strdup("read");
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(char *),
                                       &scheme.flows.password->scopes_values));
    scheme.flows.password->scopes_values[0] = test_strdup("Read access");
    scheme.flows.password->n_scopes = 1;

    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(struct c_rest_openapi_oauth_flow),
                                       &scheme.flows.client_credentials));
    memset(scheme.flows.client_credentials, 0,
           sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.client_credentials->authorization_url =
        test_strdup("http://example.com/auth");
    scheme.flows.client_credentials->token_url =
        test_strdup("http://example.com/token");
    scheme.flows.client_credentials->refresh_url =
        test_strdup("http://example.com/refresh");
    ASSERT_EQ(C_REST_OK,
              C_REST_MALLOC(sizeof(char *),
                            &scheme.flows.client_credentials->scopes_keys));
    scheme.flows.client_credentials->scopes_keys[0] = test_strdup("read");
    ASSERT_EQ(C_REST_OK,
              C_REST_MALLOC(sizeof(char *),
                            &scheme.flows.client_credentials->scopes_values));
    scheme.flows.client_credentials->scopes_values[0] =
        test_strdup("Read access");
    scheme.flows.client_credentials->n_scopes = 1;

    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(struct c_rest_openapi_oauth_flow),
                                       &scheme.flows.authorization_code));
    memset(scheme.flows.authorization_code, 0,
           sizeof(struct c_rest_openapi_oauth_flow));
    scheme.flows.authorization_code->authorization_url =
        test_strdup("http://example.com/auth");
    scheme.flows.authorization_code->token_url =
        test_strdup("http://example.com/token");
    scheme.flows.authorization_code->refresh_url =
        test_strdup("http://example.com/refresh");
    ASSERT_EQ(C_REST_OK,
              C_REST_MALLOC(sizeof(char *),
                            &scheme.flows.authorization_code->scopes_keys));
    scheme.flows.authorization_code->scopes_keys[0] = test_strdup("read");
    ASSERT_EQ(C_REST_OK,
              C_REST_MALLOC(sizeof(char *),
                            &scheme.flows.authorization_code->scopes_values));
    scheme.flows.authorization_code->scopes_values[0] =
        test_strdup("Read access");
    scheme.flows.authorization_code->n_scopes = 1;

    ASSERT_EQ(C_REST_OK,
              C_REST_MALLOC(sizeof(struct c_rest_openapi_security_scheme),
                            &spec->security_schemes));
#if defined(_MSC_VER)
    memcpy_s(spec->security_schemes, sizeof(scheme), &scheme, sizeof(scheme));
#else
    memcpy(spec->security_schemes, &scheme, sizeof(scheme));
#endif
    spec->n_security_schemes = 1;

    ASSERT_EQ(C_REST_OK,
              C_REST_MALLOC(sizeof(struct c_rest_openapi_security_requirement),
                            &spec->security));
    spec->security[0].name = test_strdup("oauth2");
    ASSERT_EQ(C_REST_OK,
              C_REST_MALLOC(sizeof(char *), &spec->security[0].scopes));
    spec->security[0].scopes[0] = test_strdup(scopes[0]);
    spec->security[0].n_scopes = 1;
    spec->n_security = 1;
  }

  ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_add_component_schema(
                           spec, "DummyReq",
                           "{\"type\":\"object\",\"properties\":{\"id\":{"
                           "\"type\":\"integer\"}}}"));

  for (idx = 0; idx < 20; idx++) {
    char buf[32];
#if defined(_MSC_VER)
    sprintf_s(buf, sizeof(buf), "CompTest%d", idx);
#else
    sprintf(buf, "CompTest%d", idx);
#endif
    ASSERT_EQ(C_REST_OK,
              c_rest_openapi_spec_add_component_schema(spec, buf, "{}"));
#if defined(_MSC_VER)
    sprintf_s(buf, sizeof(buf), "/api/more%d", idx);
#else
    sprintf(buf, "/api/more%d", idx);
#endif
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_add_path(spec, buf, "GET", &op));
  }

  ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_to_json(spec, &json_str));
  ASSERT(json_str != NULL);
  ASSERT(strstr(json_str, "openapi") != NULL);
  ASSERT(strstr(json_str, "3.1.0") != NULL);
  ASSERT(strstr(json_str, "My API") != NULL);
  ASSERT(strstr(json_str, "DummyReq") != NULL);
  ASSERT(strstr(json_str, "oauth2") != NULL);
  ASSERT(strstr(json_str, "Apache-2.0") != NULL);
  json_free_serialized_string(json_str);

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));
  req.method = m_get;
  req.path = p_docs;
  ASSERT_EQ(C_REST_OK, c_rest_router_dispatch(router, &req, &res));
  ASSERT_EQ(200, res.status_code);
  ASSERT(res.body != NULL);
  ASSERT(strstr(res.body, "SwaggerUIBundle") != NULL);
  ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res));
  ASSERT_EQ(C_REST_OK, c_rest_router_destroy(router));

  PASS();
}

TEST test_openapi_null_flows_and_mock_branches(void) {
  {
    struct c_rest_openapi_spec *spec_null_flows = NULL;
    struct c_rest_openapi_security_scheme sch;
    char *json_out = NULL;
    struct c_rest_openapi_server srv;
    struct c_rest_openapi_server_variable var;

    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_init(&spec_null_flows));

    memset(&srv, 0, sizeof(srv));
    srv.url = test_strdup("http://test.com");
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(2 * sizeof(struct c_rest_openapi_server),
                                       &spec_null_flows->servers));
#if defined(_MSC_VER)
    memcpy_s(&spec_null_flows->servers[0], sizeof(srv), &srv, sizeof(srv));
#else
    memcpy(&spec_null_flows->servers[0], &srv, sizeof(srv));
#endif

    memset(&var, 0, sizeof(var));
    var.name = test_strdup("var");
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(var), &srv.variables));
#if defined(_MSC_VER)
    memcpy_s(srv.variables, sizeof(var), &var, sizeof(var));
#else
    memcpy(srv.variables, &var, sizeof(var));
#endif
    srv.n_variables = 1;
    srv.url = NULL;
#if defined(_MSC_VER)
    memcpy_s(&spec_null_flows->servers[1], sizeof(srv), &srv, sizeof(srv));
#else
    memcpy(&spec_null_flows->servers[1], &srv, sizeof(srv));
#endif
    spec_null_flows->n_servers = 2;

    memset(&sch, 0, sizeof(sch));
    sch.name_key = test_strdup("test_key");

    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(struct c_rest_openapi_oauth_flow),
                                       &sch.flows.implicit));
    memset(sch.flows.implicit, 0, sizeof(struct c_rest_openapi_oauth_flow));
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(struct c_rest_openapi_oauth_flow),
                                       &sch.flows.password));
    memset(sch.flows.password, 0, sizeof(struct c_rest_openapi_oauth_flow));
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(struct c_rest_openapi_oauth_flow),
                                       &sch.flows.client_credentials));
    memset(sch.flows.client_credentials, 0,
           sizeof(struct c_rest_openapi_oauth_flow));
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(sizeof(struct c_rest_openapi_oauth_flow),
                                       &sch.flows.authorization_code));
    memset(sch.flows.authorization_code, 0,
           sizeof(struct c_rest_openapi_oauth_flow));

    ASSERT_EQ(C_REST_OK,
              C_REST_MALLOC(sizeof(struct c_rest_openapi_security_scheme),
                            &spec_null_flows->security_schemes));
#if defined(_MSC_VER)
    memcpy_s(spec_null_flows->security_schemes, sizeof(sch), &sch, sizeof(sch));
#else
    memcpy(spec_null_flows->security_schemes, &sch, sizeof(sch));
#endif
    spec_null_flows->n_security_schemes = 1;

    ASSERT_EQ(C_REST_OK,
              c_rest_openapi_spec_to_json(spec_null_flows, &json_out));
    ASSERT(json_out != NULL);
    json_free_serialized_string(json_out);
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(spec_null_flows));
  }

  {
    struct c_rest_openapi_spec *spec_noflows = NULL;
    struct c_rest_openapi_security_scheme sch;
    char *json_out = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_init(&spec_noflows));

    memset(&sch, 0, sizeof(sch));
    sch.name_key = test_strdup("test_key2");
    ASSERT_EQ(C_REST_OK,
              C_REST_MALLOC(sizeof(struct c_rest_openapi_security_scheme),
                            &spec_noflows->security_schemes));
#if defined(_MSC_VER)
    memcpy_s(spec_noflows->security_schemes, sizeof(sch), &sch, sizeof(sch));
#else
    memcpy(spec_noflows->security_schemes, &sch, sizeof(sch));
#endif
    spec_noflows->n_security_schemes = 1;

    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_to_json(spec_noflows, &json_out));
    ASSERT(json_out != NULL);
    json_free_serialized_string(json_out);
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(spec_noflows));
  }

  {
    struct c_rest_openapi_spec *spec_empty = NULL;
    struct c_rest_openapi_operation op_empty;
    char *json_out_empty = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_init(&spec_empty));
    memset(&op_empty, 0, sizeof(op_empty));
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_add_path(spec_empty, "/api/empty",
                                                      "GET", &op_empty));
    ASSERT_EQ(C_REST_OK,
              c_rest_openapi_spec_to_json(spec_empty, &json_out_empty));
    ASSERT(json_out_empty != NULL);
    json_free_serialized_string(json_out_empty);
    ASSERT_EQ(C_REST_OK, c_rest_openapi_spec_destroy(spec_empty));
  }

  {
    struct c_rest_router *r_mock = NULL;
    struct c_rest_request req_m;
    struct c_rest_response res_m;
    char m_get[] = "GET";
    char p_json[] = "/openapi.json";
    char p_docs[] = "/docs";

    ASSERT_EQ(C_REST_OK, c_rest_router_init(&r_mock));
    ASSERT_EQ(C_REST_OK, c_rest_enable_openapi(r_mock, "/openapi.json"));
    ASSERT_EQ(C_REST_OK,
              c_rest_enable_swagger_ui(r_mock, "/docs", "/openapi.json"));

    memset(&req_m, 0, sizeof(req_m));
    memset(&res_m, 0, sizeof(res_m));
    req_m.method = m_get;
    req_m.path = p_json;

    g_mock_openapi_fail = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r_mock, &req_m, &res_m));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_m));

    memset(&res_m, 0, sizeof(res_m));
    g_mock_openapi_fail = 2;
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r_mock, &req_m, &res_m));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_m));

    memset(&res_m, 0, sizeof(res_m));
    g_mock_openapi_fail = 3;
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r_mock, &req_m, &res_m));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_m));

    memset(&res_m, 0, sizeof(res_m));
    g_mock_res_status_fail = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r_mock, &req_m, &res_m));
    g_mock_res_status_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_m));

    memset(&req_m, 0, sizeof(req_m));
    memset(&res_m, 0, sizeof(res_m));
    req_m.method = m_get;
    req_m.path = p_docs;

    g_mock_openapi_fail = 4;
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r_mock, &req_m, &res_m));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_m));

    memset(&res_m, 0, sizeof(res_m));
    g_mock_openapi_fail = 5;
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r_mock, &req_m, &res_m));
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_m));

    memset(&res_m, 0, sizeof(res_m));
    g_mock_res_status_fail = 1;
    ASSERT_NEQ(C_REST_OK, c_rest_router_dispatch(r_mock, &req_m, &res_m));
    g_mock_res_status_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&res_m));

    g_mock_openapi_fail = 6;
    ASSERT_NEQ(C_REST_OK,
               c_rest_enable_swagger_ui(r_mock, "/docs_fail", "/openapi.json"));
    g_mock_openapi_fail = 0;

    ASSERT_EQ(C_REST_OK, c_rest_router_destroy(r_mock));
  }

  PASS();
}

SUITE(openapi_suite) {
  RUN_TEST(test_openapi_full_spec_and_security);
  RUN_TEST(test_openapi_oom_and_edge_cases);
  RUN_TEST(test_openapi_partial_schemas_and_router_docs);
  RUN_TEST(test_openapi_null_flows_and_mock_branches);
}
