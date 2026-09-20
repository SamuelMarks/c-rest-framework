/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_modality.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_sse.h"
#include "c_rest_websocket.h"
#include "c_rest_multipart.h"
#include "c_rest_jwt_middleware.h"
#include "c_rest_crypto.h"
#include "c_rest_graphql.h"
#include "c_rest_router.h"
#include "c_rest_http23.h"
#include "c_rest_template.h"
#include "c_rest_tls.h"
#include "c_rest_platform.h"
#include "c_rest_openapi.h"
#if defined(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART) && !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
#include "c_rest_hot_reload.h"
#endif
#include "c_rest_testing_mocks.h"

static int g_examples_failed = 0;
#define TEST static int
#define PASS() return g_examples_failed
#define ASSERT_EQ(a, b) do { g_examples_failed += ((a) != (b)); } while (0)
#define ASSERT(a) do { g_examples_failed += (!(a)); } while (0)

int app_node_style_main(void);
int app_threaded_main(void);
#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
int app_multi_process_main(void);
#endif
int app_http23_main(void);
int app_openapi_main(void);
int app_template_engine_main(void);
int app_https_server_main(void);
#if defined(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART) && !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
int app_hot_reload_main(void);
#endif
int app_server_sent_events_sse_main(void);
int app_websockets_main(void);
int app_full_multi_main(void);
int app_jwt_json_w_main(void);
int app_graphql_api_main(void);

static void dummy_exit(int s) { (void)s; }
static int g_intercept_run = 0;
static c_rest_error_t mock_c_rest_run(struct c_rest_context *ctx) {
  if (g_intercept_run == 1) {
    return C_REST_ERROR_GENERIC;
  }
  if (g_intercept_run == 2) {
    return C_REST_OK;
  }
  return c_rest_run(ctx);
}

static int g_ex_set_router_fail = 0;
static c_rest_error_t mock_ex_c_rest_set_router(struct c_rest_context *c,
                                                c_rest_router *r) {
  if (g_ex_set_router_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_set_router(c, r);
}

static int g_ex_router_add_fail_at = 0;
static int g_ex_router_add_count = 0;
static c_rest_error_t mock_ex_c_rest_router_add(c_rest_router *r, const char *m,
                                                const char *p,
                                                c_rest_handler_fn h, void *u) {
  g_ex_router_add_count++;
  if (g_ex_router_add_fail_at > 0 &&
      g_ex_router_add_count == g_ex_router_add_fail_at) {
    return C_REST_ERROR_GENERIC;
  }
  return c_rest_router_add(r, m, p, h, u);
}

static int g_ex_router_destroy_fail = 0;
static c_rest_error_t mock_ex_c_rest_router_destroy(c_rest_router *r) {
  if (g_ex_router_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_destroy(r);
}

static int g_ex_destroy_fail = 0;
static c_rest_error_t mock_ex_c_rest_destroy(struct c_rest_context *c) {
  if (g_ex_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_destroy(c);
}

static int g_ex_router_use_fail = 0;
static c_rest_error_t mock_ex_c_rest_router_use(c_rest_router *r, const char *p,
                                                c_rest_middleware_fn m,
                                                void *u) {
  if (g_ex_router_use_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_use(r, p, m, u);
}

#define exit(s) dummy_exit(s)
#define c_rest_run mock_c_rest_run
#define c_rest_set_router mock_ex_c_rest_set_router
#define c_rest_router_add mock_ex_c_rest_router_add
#define c_rest_router_destroy mock_ex_c_rest_router_destroy
#define c_rest_destroy mock_ex_c_rest_destroy
#define c_rest_router_use mock_ex_c_rest_router_use

#define main app_node_style_main
#include "../examples/app_node_style/main.c"
#undef main
#undef c_rest_router_init

#define main app_threaded_main
#define sig_handler sig_handler_threaded
#define my_log_cb my_log_cb_threaded
#include "../examples/app_threaded/main.c"
#undef my_log_cb
#undef sig_handler
#undef main
#undef c_rest_router_init

#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
#define main app_multi_process_main
#define sig_handler sig_handler_mp
#define my_log_cb my_log_cb_mp
#include "../examples/app_multi_process/main.c"
#undef my_log_cb
#undef sig_handler
#undef main
#undef c_rest_router_init
#endif

#define main app_http23_main
static int g_h23_process_fail = 0;
static c_rest_error_t mock_h23_process(c_rest_http23_ctx_t *c, const char *d,
                                       size_t l, size_t *cons) {
  if (g_h23_process_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_http23_process(c, d, l, cons);
}
static int g_h23_get_req_fail = 0;
static c_rest_error_t mock_h23_get_request(c_rest_http23_ctx_t *c,
                                           struct c_rest_request **r) {
  if (g_h23_get_req_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_http23_get_request(c, r);
}
static int g_h23_format_res_fail = 0;
static c_rest_error_t mock_h23_format_response(c_rest_http23_ctx_t *c,
                                               struct c_rest_response *r,
                                               char **b, size_t *l) {
  if (g_h23_format_res_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_http23_format_response(c, r, b, l);
}
static int g_h23_ctx_destroy_fail = 0;
static c_rest_error_t mock_h23_ctx_destroy(c_rest_http23_ctx_t *c) {
  if (g_h23_ctx_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_http23_ctx_destroy(c);
}
#define c_rest_http23_process mock_h23_process
#define c_rest_http23_get_request mock_h23_get_request
#define c_rest_http23_format_response mock_h23_format_response
#define c_rest_http23_ctx_destroy mock_h23_ctx_destroy
#include "../examples/app_http23/main.c"
#undef main
#undef c_rest_router_init
#undef c_rest_http23_ctx_destroy
#undef c_rest_http23_format_response
#undef c_rest_http23_get_request
#undef c_rest_http23_process

static int g_openapi_swagger_fail_at = 0;
static int g_openapi_swagger_count = 0;
static c_rest_error_t mock_openapi_swagger(c_rest_router *r, const char *p,
                                           const char *u) {
  g_openapi_swagger_count++;
  if (g_openapi_swagger_fail_at > 0 &&
      g_openapi_swagger_count == g_openapi_swagger_fail_at)
    return C_REST_ERROR_GENERIC;
  return c_rest_enable_swagger_ui(r, p, u);
}
static int g_openapi_get_spec_fail = 0;
static int g_openapi_get_spec_null = 0;
static c_rest_error_t
mock_openapi_get_spec(c_rest_router *r, struct c_rest_openapi_spec **s) {
  if (g_openapi_get_spec_fail)
    return C_REST_ERROR_GENERIC;
  if (g_openapi_get_spec_null) {
    *s = NULL;
    return C_REST_OK;
  }
  return c_rest_router_get_openapi_spec(r, s);
}
static int g_openapi_ctx_init_fail = 0;
static c_rest_error_t
mock_openapi_c_rest_init(enum c_rest_modality_type m,
                         struct c_rest_context **c) {
  if (g_openapi_ctx_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_init(m, c);
}
static int g_openapi_add_schema_fail = 0;
static c_rest_error_t mock_openapi_add_schema(struct c_rest_openapi_spec *s,
                                              const char *n, const char *sch) {
  if (g_openapi_add_schema_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_openapi_spec_add_component_schema(s, n, sch);
}
static int g_openapi_enable_fail_at = 0;
static int g_openapi_enable_count = 0;
static c_rest_error_t mock_openapi_enable(c_rest_router *r, const char *p) {
  g_openapi_enable_count++;
  if (g_openapi_enable_fail_at > 0 &&
      g_openapi_enable_count == g_openapi_enable_fail_at)
    return C_REST_ERROR_GENERIC;
  return c_rest_enable_openapi(r, p);
}
static int g_openapi_add_route_fail = 0;
static c_rest_error_t
mock_openapi_add_route(c_rest_router *r, const char *m, const char *p,
                       c_rest_handler_fn h, void *u,
                       struct c_rest_openapi_operation *op) {
  if (g_openapi_add_route_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_add_openapi(r, m, p, h, u, op);
}
#define c_rest_enable_swagger_ui mock_openapi_swagger
#define c_rest_router_get_openapi_spec mock_openapi_get_spec
#define c_rest_init mock_openapi_c_rest_init
#define c_rest_openapi_spec_add_component_schema mock_openapi_add_schema
#define c_rest_enable_openapi mock_openapi_enable
#define c_rest_router_add_openapi mock_openapi_add_route
#define main app_openapi_main
#define sig_handler sig_handler_openapi
#define handle_hello_world handle_hello_world_openapi
#include "../examples/app_openapi/main.c"
#undef handle_hello_world
#undef sig_handler
#undef main
#undef c_rest_router_init
#undef c_rest_router_add_openapi
#undef c_rest_enable_openapi
#undef c_rest_openapi_spec_add_component_schema
#undef c_rest_init
#undef c_rest_router_get_openapi_spec
#undef c_rest_enable_swagger_ui

static int g_tpl_init_fail = 0;
static c_rest_error_t mock_tpl_init(struct c_rest_template_context *c,
                                    const char *t) {
  if (g_tpl_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_template_init(c, t);
}
static int g_tpl_add_route_fail = 0;
static c_rest_error_t mock_tpl_add_route(c_rest_router *r, const char *m,
                                         const char *p,
                                         struct c_rest_template_context *c,
                                         c_rest_template_data_fn pr, void *u) {
  if (g_tpl_add_route_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_add_template(r, m, p, c, pr, u);
}
static int g_tpl_destroy_fail = 0;
static c_rest_error_t
mock_tpl_destroy(struct c_rest_template_context *c) {
  if (g_tpl_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_template_destroy(c);
}
#define c_rest_template_init mock_tpl_init
#define c_rest_router_add_template mock_tpl_add_route
#define c_rest_template_destroy mock_tpl_destroy
#define main app_template_engine_main
#define sig_handler sig_handler_tpl
#include "../examples/app_template_engine/main.c"
#undef sig_handler
#undef main
#undef c_rest_router_init
#undef c_rest_template_destroy
#undef c_rest_router_add_template
#undef c_rest_template_init

static int g_https_tls_init_fail = 0;
static c_rest_error_t mock_https_tls_init(void) {
  if (g_https_tls_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_tls_init();
}
static int g_https_tls_ctx_init_fail = 0;
static c_rest_error_t
mock_https_tls_ctx_init(struct c_rest_tls_context **c) {
  if (g_https_tls_ctx_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_tls_context_init(c);
}
static int g_https_tls_ctx_destroy_fail = 0;
static c_rest_error_t
mock_https_tls_ctx_destroy(struct c_rest_tls_context *c) {
  if (g_https_tls_ctx_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_tls_context_destroy(c);
}
#define c_rest_tls_init mock_https_tls_init
#define c_rest_tls_context_init mock_https_tls_ctx_init
#define c_rest_tls_context_destroy mock_https_tls_ctx_destroy
#define main app_https_server_main
#define sig_handler sig_handler_https
#include "../examples/app_https_server/main.c"
#undef sig_handler
#undef main
#undef c_rest_router_init
#undef c_rest_tls_context_destroy
#undef c_rest_tls_context_init
#undef c_rest_tls_init

#if defined(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART) &&                       \
    !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
static int g_hr_platform_init_fail = 0;
static c_rest_error_t mock_hr_platform_init(void) {
  if (g_hr_platform_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_platform_init();
}
static int g_hr_init_fail = 0;
static c_rest_error_t mock_hr_init(c_rest_hot_reload_ctx_t **c,
                                    struct c_rest_logger *logger) {
  if (g_hr_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_hot_reload_init(c, logger);
}
static int g_hr_watch_fail = 0;
static c_rest_error_t mock_hr_add_watch(c_rest_hot_reload_ctx_t *c,
                                        const char *p) {
  if (g_hr_watch_fail > 0) {
    g_hr_watch_fail--;
    return C_REST_ERROR_GENERIC;
  }
  if (!c || !p)
    return C_REST_ERROR_INVALID_ARG;
  return C_REST_OK;
}
static int g_hr_register_routes_fail = 0;
static c_rest_error_t mock_hr_register_routes(c_rest_router *r,
                                              const char *p) {
  if (g_hr_register_routes_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_hot_reload_register_routes(r, p);
}
static int g_hr_destroy_fail = 0;
static c_rest_error_t mock_hr_destroy(c_rest_hot_reload_ctx_t *c) {
  if (g_hr_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_hot_reload_destroy(c);
}
static int g_hr_start_fail = 0;
static c_rest_error_t mock_hr_start(c_rest_hot_reload_ctx_t *c,
                                    c_rest_hot_reload_callback_t f, void *u) {
  if (g_hr_start_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_hot_reload_start(c, f, u);
}
static int g_hr_platform_cleanup_fail = 0;
static c_rest_error_t mock_hr_platform_cleanup(void) {
  if (g_hr_platform_cleanup_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_platform_cleanup();
}
#define c_rest_platform_init mock_hr_platform_init
#define c_rest_hot_reload_init mock_hr_init
#define c_rest_hot_reload_add_watch mock_hr_add_watch
#define c_rest_hot_reload_register_routes mock_hr_register_routes
#define c_rest_hot_reload_start mock_hr_start
#define c_rest_hot_reload_destroy mock_hr_destroy
#define c_rest_platform_cleanup mock_hr_platform_cleanup
#define main app_hot_reload_main
#define sig_handler sig_handler_hr
#include "../examples/app_hot_reload/main.c"
#undef sig_handler
#undef main
#undef c_rest_platform_cleanup
#undef c_rest_router_init
#undef c_rest_hot_reload_destroy
#undef c_rest_hot_reload_start
#undef c_rest_hot_reload_register_routes
#undef c_rest_hot_reload_add_watch
#undef c_rest_hot_reload_init
#undef c_rest_platform_init
#endif

static int g_sse_event_init_fail = 0;
static c_rest_error_t
mock_c_rest_sse_event_init(struct c_rest_sse_event *e) {
  if (g_sse_event_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_sse_event_init(e);
}
static int g_sse_send_event_ret = -1;
static int g_sse_keepalive_ret = -1;
static c_rest_error_t mock_c_rest_sse_send_event(struct c_rest_response *res, const struct c_rest_sse_event *ev) {
  if (g_sse_send_event_ret != -1) return (c_rest_error_t)g_sse_send_event_ret;
  return c_rest_sse_send_event(res, ev);
}
static c_rest_error_t mock_c_rest_sse_send_keepalive(struct c_rest_response *res) {
  if (g_sse_keepalive_ret != -1) return (c_rest_error_t)g_sse_keepalive_ret;
  return c_rest_sse_send_keepalive(res);
}
static int g_sse_add_route_fail = 0;
static c_rest_error_t mock_c_rest_router_add_sse(c_rest_router *router, const char *path, c_rest_handler_fn handler, void *user_data) {
  if (g_sse_add_route_fail) return C_REST_ERROR_GENERIC;
  return c_rest_router_add_sse(router, path, handler, user_data);
}
static int g_sse_router_init_fail = 0;
static c_rest_error_t mock_c_rest_router_init(c_rest_router **router) {
  if (g_sse_router_init_fail) return C_REST_ERROR_GENERIC;
  return c_rest_router_init(router);
}
#define c_rest_sse_event_init mock_c_rest_sse_event_init
#define c_rest_router_init mock_c_rest_router_init
#define c_rest_router_add_sse mock_c_rest_router_add_sse
#define c_rest_sse_send_event mock_c_rest_sse_send_event
#define c_rest_sse_send_keepalive mock_c_rest_sse_send_keepalive
#define main app_server_sent_events_sse_main
#define sig_handler sig_handler_sse
#include "../examples/app_server_sent_events_sse/main.c"
#undef sig_handler
#undef main
#undef c_rest_router_init
#undef c_rest_sse_send_event
#undef c_rest_router_add_sse
#undef c_rest_router_init
#undef c_rest_sse_send_keepalive
#undef c_rest_sse_event_init

static int g_ws_router_init_fail = 0;
static int g_ws_add_route_fail = 0;
static c_rest_error_t mock_ws_router_init(c_rest_router **router) {
  if (g_ws_router_init_fail) return C_REST_ERROR_GENERIC;
  return c_rest_router_init(router);
}
static c_rest_error_t mock_ws_router_add_websocket(c_rest_router *router, const char *path,
                                                  c_rest_websocket_on_message_fn on_message,
                                                  c_rest_websocket_on_close_fn on_close,
                                                  void *user_data) {
  if (g_ws_add_route_fail) return C_REST_ERROR_GENERIC;
  return c_rest_router_add_websocket(router, path, on_message, on_close, user_data);
}

#define main app_websockets_main
#define sig_handler sig_handler_ws
#define c_rest_router_init mock_ws_router_init
#define c_rest_router_add_websocket mock_ws_router_add_websocket
#include "../examples/app_websockets/main.c"
#undef c_rest_router_add_websocket
#undef c_rest_router_init
#undef sig_handler
#undef main
#undef c_rest_router_init

static int g_multi_parser_init_fail = 0;
static c_rest_error_t mock_c_rest_multipart_parser_init(
    c_rest_multipart_parser **parser, const char *boundary,
    const struct c_rest_multipart_callbacks *callbacks, void *user_data) {
  if (g_multi_parser_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_multipart_parser_init(parser, boundary, callbacks, user_data);
}

static int g_multi_parser_exec_fail = 0;
static c_rest_error_t mock_multi_parser_exec(c_rest_multipart_parser *p,
                                             const char *d, size_t l,
                                             size_t *o) {
  if (g_multi_parser_exec_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_multipart_parser_execute(p, d, l, o);
}

static int g_multi_parser_destroy_fail = 0;
static c_rest_error_t
mock_multi_parser_destroy(c_rest_multipart_parser *p) {
  if (g_multi_parser_destroy_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_multipart_parser_destroy(p);
}

#define c_rest_multipart_parser_execute mock_multi_parser_exec
#define c_rest_multipart_parser_destroy mock_multi_parser_destroy
#define main app_full_multi_main
#define sig_handler sig_handler_full_multi
#define c_rest_multipart_parser_init mock_c_rest_multipart_parser_init
#include "../examples/app_full_multi/main.c"
#undef c_rest_multipart_parser_init
#undef sig_handler
#undef main
#undef c_rest_router_init
#undef c_rest_multipart_parser_destroy
#undef c_rest_multipart_parser_execute

static int g_jwt_sign_fail = 0;
static c_rest_error_t mock_jwt_sign(const char *p, const unsigned char *k,
                                    size_t kl, char **o) {
  if (g_jwt_sign_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_jwt_sign_hs256(p, k, kl, o);
}

static int g_jwt_router_init_fail = 0;
static c_rest_error_t mock_jwt_router_init(c_rest_router **router) {
  if (g_jwt_router_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_init(router);
}
#define c_rest_jwt_sign_hs256 mock_jwt_sign
#define c_rest_router_init mock_jwt_router_init
#define main app_jwt_json_w_main
#define sig_handler sig_handler_jwt_app
#include "../examples/app_jwt_json_w/main.c"
#undef sig_handler
#undef main
#undef c_rest_router_init
#undef c_rest_jwt_sign_hs256

static int g_graphql_router_init_fail = 0;
static int g_graphql_schema_init_fail = 0;
static c_rest_error_t mock_graphql_router_init(c_rest_router **router) {
  if (g_graphql_router_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_init(router);
}
static c_rest_error_t
mock_graphql_schema_init(struct c_rest_graphql_schema **schema) {
  if (g_graphql_schema_init_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_graphql_schema_init(schema);
}

static int g_gql_add_resolver_fail_at = 0;
static int g_gql_add_resolver_count = 0;
static c_rest_error_t
mock_gql_add_resolver(struct c_rest_graphql_schema *s, const char *f,
                      c_rest_graphql_resolver_fn r, void *u) {
  g_gql_add_resolver_count++;
  if (g_gql_add_resolver_fail_at > 0 &&
      g_gql_add_resolver_count == g_gql_add_resolver_fail_at)
    return C_REST_ERROR_GENERIC;
  return c_rest_graphql_schema_add_resolver(s, f, r, u);
}
static int g_gql_add_route_fail = 0;
static c_rest_error_t mock_gql_add_route(c_rest_router *r, const char *p,
                                         struct c_rest_graphql_schema *s) {
  if (g_gql_add_route_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_router_add_graphql(r, p, s);
}
static int g_gql_schema_free_fail = 0;
static c_rest_error_t
mock_gql_schema_free(struct c_rest_graphql_schema *s) {
  if (g_gql_schema_free_fail)
    return C_REST_ERROR_GENERIC;
  return c_rest_graphql_schema_free(s);
}

#define c_rest_graphql_schema_add_resolver mock_gql_add_resolver
#define c_rest_router_add_graphql mock_gql_add_route
#define c_rest_graphql_schema_free mock_gql_schema_free
#define main app_graphql_api_main
#define sig_handler sig_handler_graphql_app
#define logger_cb logger_cb_graphql_app
#define c_rest_router_init mock_graphql_router_init
#define c_rest_graphql_schema_init mock_graphql_schema_init
#include "../examples/app_graphql_api/main.c"
#undef c_rest_graphql_schema_init
#undef c_rest_router_init
#undef logger_cb
#undef sig_handler
#undef main
#undef c_rest_graphql_schema_free
#undef c_rest_router_add_graphql
#undef c_rest_graphql_schema_add_resolver

#undef c_rest_router_use
#undef c_rest_destroy
#undef c_rest_router_destroy
#undef c_rest_router_add
#undef c_rest_set_router
#undef c_rest_run
#undef exit
/* clang-format on */

/**
 * @brief Hook that always fails allocation.
 * @param size Requested allocation size.
 * @return NULL.
 */
static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

static int g_node_alloc_count = 0;
static int g_node_fail_at = 0;

/**
 * @brief Hook that fails on the Nth allocation call.
 * @param size Requested allocation size.
 * @return Allocated pointer or NULL on target count.
 */
static void *fail_malloc_at_n(size_t size) {
  g_node_alloc_count++;
  if (g_node_alloc_count == g_node_fail_at) {
    g_node_fail_at = 0;
    return NULL;
  }
  return malloc(size);
}

TEST test_app_node_style(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  int ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  handle_hello_world(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  req.query = "name=Alice";
  handle_echo(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  req.query = "other=Alice";
  handle_echo(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  req.query = NULL;
  handle_echo(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Test sig_handler */
  sig_handler(0);

  /* Normal execution */
  ret = app_node_style_main();
  ASSERT_EQ(0, ret);

  /* Test fail c_rest_init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_node_style_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail c_rest_router_init */
  g_node_alloc_count = 0;
  g_node_fail_at = 4;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = app_node_style_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail c_rest_run */
  g_intercept_run = 1;
  ret = app_node_style_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail c_rest_set_router */
  g_ex_set_router_fail = 1;
  ret = app_node_style_main();
  ASSERT_EQ(1, ret);
  g_ex_set_router_fail = 0;

  /* Test fail c_rest_router_add #1 */
  g_ex_router_add_count = 0;
  g_ex_router_add_fail_at = 1;
  ret = app_node_style_main();
  ASSERT_EQ(1, ret);

  /* Test fail c_rest_router_add #2 */
  g_ex_router_add_count = 0;
  g_ex_router_add_fail_at = 2;
  ret = app_node_style_main();
  ASSERT_EQ(1, ret);
  g_ex_router_add_fail_at = 0;
  g_ex_router_add_count = 0;

  /* Test fail c_rest_router_destroy */
  g_ex_router_destroy_fail = 1;
  ret = app_node_style_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;

  /* Test fail c_rest_destroy */
  g_ex_destroy_fail = 1;
  ret = app_node_style_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;

  PASS();
}

TEST test_app_threaded(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  int ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  handle_db_query(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Test sig_handler */
  sig_handler_threaded(0);

  /* Normal execution with mock run */
  g_intercept_run = 2;
  ret = app_threaded_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail c_rest_init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_threaded_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail c_rest_router_init */
  g_node_alloc_count = 0;
  g_node_fail_at = 4;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = app_threaded_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail c_rest_run */
  g_intercept_run = 1;
  ret = app_threaded_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail c_rest_set_router */
  g_ex_set_router_fail = 1;
  ret = app_threaded_main();
  ASSERT_EQ(1, ret);
  g_ex_set_router_fail = 0;

  /* Test fail c_rest_router_add */
  g_ex_router_add_count = 0;
  g_ex_router_add_fail_at = 1;
  ret = app_threaded_main();
  ASSERT_EQ(1, ret);
  g_ex_router_add_fail_at = 0;
  g_ex_router_add_count = 0;

  /* Test fail c_rest_router_destroy */
  g_intercept_run = 2;
  g_ex_router_destroy_fail = 1;
  ret = app_threaded_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;

  /* Test fail c_rest_destroy */
  g_ex_destroy_fail = 1;
  ret = app_threaded_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;
  g_intercept_run = 0;

  PASS();
}

#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
TEST test_app_multi_process(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  int ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  handle_work(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* Test sig_handler */
  sig_handler_mp(0);

  /* Normal execution */
  ret = app_multi_process_main();
  ASSERT_EQ(0, ret);

  /* Test fail c_rest_init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_multi_process_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail c_rest_router_init */
  g_node_alloc_count = 0;
  g_node_fail_at = 4;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = app_multi_process_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail c_rest_run */
  g_intercept_run = 1;
  ret = app_multi_process_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail c_rest_set_router */
  g_ex_set_router_fail = 1;
  ret = app_multi_process_main();
  ASSERT_EQ(1, ret);
  g_ex_set_router_fail = 0;

  /* Test fail c_rest_router_add */
  g_ex_router_add_count = 0;
  g_ex_router_add_fail_at = 1;
  ret = app_multi_process_main();
  ASSERT_EQ(1, ret);
  g_ex_router_add_fail_at = 0;
  g_ex_router_add_count = 0;

  /* Test fail c_rest_router_destroy */
  g_intercept_run = 2;
  g_ex_router_destroy_fail = 1;
  ret = app_multi_process_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;

  /* Test fail c_rest_destroy */
  g_ex_destroy_fail = 1;
  ret = app_multi_process_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;
  g_intercept_run = 0;

  PASS();
}
#endif

TEST test_app_http23(void) {
  int ret;

  /* Normal execution */
  ret = app_http23_main();
  ASSERT_EQ(0, ret);

  /* Test fail ctx_init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_http23_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail process */
  g_h23_process_fail = 1;
  ret = app_http23_main();
  ASSERT_EQ(1, ret);
  g_h23_process_fail = 0;

  /* Test fail get_request */
  g_h23_get_req_fail = 1;
  ret = app_http23_main();
  ASSERT_EQ(1, ret);
  g_h23_get_req_fail = 0;

  /* Test fail format_response */
  g_h23_format_res_fail = 1;
  ret = app_http23_main();
  ASSERT_EQ(1, ret);
  g_h23_format_res_fail = 0;

  /* Test fail ctx_destroy */
  g_h23_ctx_destroy_fail = 1;
  ret = app_http23_main();
  ASSERT_EQ(1, ret);
  g_h23_ctx_destroy_fail = 0;

  PASS();
}

TEST test_app_openapi(void) {
  int ret;
  struct c_rest_request req;
  struct c_rest_response res;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  handle_hello_world_openapi(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  sig_handler_openapi(0);

  ret = app_openapi_main();
  ASSERT_EQ(0, ret);

  /* Test fail router init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail swagger_ui #1 */
  g_openapi_swagger_count = 0;
  g_openapi_swagger_fail_at = 1;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);

  /* Test fail get_openapi_spec */
  g_openapi_swagger_fail_at = 0;
  g_openapi_swagger_count = 0;
  g_openapi_get_spec_fail = 1;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);
  g_openapi_get_spec_fail = 0;

  /* Test fail add_component_schema */
  g_openapi_add_schema_fail = 1;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);
  g_openapi_add_schema_fail = 0;

  /* Test fail enable_openapi #1 */
  g_openapi_enable_count = 0;
  g_openapi_enable_fail_at = 1;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);

  /* Test fail enable_openapi #2 */
  g_openapi_enable_count = 0;
  g_openapi_enable_fail_at = 2;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);
  g_openapi_enable_fail_at = 0;
  g_openapi_enable_count = 0;

  /* Test fail swagger_ui #2 */
  g_openapi_swagger_count = 0;
  g_openapi_swagger_fail_at = 2;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);
  g_openapi_swagger_fail_at = 0;
  g_openapi_swagger_count = 0;

  /* Test fail add_openapi */
  g_openapi_add_route_fail = 1;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);
  g_openapi_add_route_fail = 0;

  /* Test fail set_router */
  g_ex_set_router_fail = 1;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);
  g_ex_set_router_fail = 0;

  /* Test fail destroy(ctx) */
  g_ex_destroy_fail = 1;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;

  /* Test fail router_destroy */
  g_ex_router_destroy_fail = 1;
  ret = app_openapi_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;

  /* Test spec == NULL branch (line 63) */
  g_openapi_get_spec_null = 1;
  ret = app_openapi_main();
  ASSERT_EQ(0, ret);
  g_openapi_get_spec_null = 0;

  /* Test c_rest_init fail (line 115) */
  g_openapi_ctx_init_fail = 1;
  ret = app_openapi_main();
  ASSERT_EQ(0, ret);
  g_openapi_ctx_init_fail = 0;

  PASS();
}

TEST test_app_template_engine(void) {
  int ret;
  const char **keys = NULL;
  const char **vals = NULL;
  size_t count = 0;

  provide_template_data(NULL, &keys, &vals, &count, NULL);
  ASSERT_EQ(3, (int)count);

  sig_handler_tpl(0);

  ret = app_template_engine_main();
  ASSERT_EQ(0, ret);

  /* Test fail framework init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail router init */
  g_node_alloc_count = 0;
  g_node_fail_at = 4;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail router init + destroy fail (lines 59-60) */
  g_node_alloc_count = 0;
  g_node_fail_at = 4;
  g_crf_malloc_hook = fail_malloc_at_n;
  g_ex_destroy_fail = 1;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;
  g_crf_malloc_hook = NULL;

  /* Test fail template init */
  g_node_alloc_count = 0;
  g_node_fail_at = 7;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail template_init via mock */
  g_tpl_init_fail = 1;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_tpl_init_fail = 0;

  /* Test fail template_init + router_destroy fail (lines 71-73) */
  g_tpl_init_fail = 1;
  g_ex_router_destroy_fail = 1;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;
  g_tpl_init_fail = 0;

  /* Test fail template_init + destroy fail (lines 76-77) */
  g_tpl_init_fail = 1;
  g_ex_destroy_fail = 1;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;
  g_tpl_init_fail = 0;

  /* Test fail router_add_template */
  g_tpl_add_route_fail = 1;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_tpl_add_route_fail = 0;

  /* Test fail set_router */
  g_ex_set_router_fail = 1;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_ex_set_router_fail = 0;

  /* Test fail template_destroy */
  g_tpl_destroy_fail = 1;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_tpl_destroy_fail = 0;

  /* Test fail router_destroy */
  g_ex_router_destroy_fail = 1;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;

  /* Test fail destroy(ctx) */
  g_ex_destroy_fail = 1;
  ret = app_template_engine_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;

  PASS();
}

TEST test_app_https_server(void) {
  int ret;
  struct c_rest_request req;
  struct c_rest_response res;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  hello_handler(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  sig_handler_https(0);

  ret = app_https_server_main();
  ASSERT_EQ(0, ret);

  /* Test fail tls_init */
  g_https_tls_init_fail = 1;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_https_tls_init_fail = 0;

  /* Test fail tls_context_init */
  g_https_tls_ctx_init_fail = 1;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_https_tls_ctx_init_fail = 0;

  /* Test fail tls_context_init via malloc hook */
  g_crf_malloc_hook = fail_malloc;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail framework init */
  g_node_alloc_count = 0;
  g_node_fail_at = 2;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail framework init + tls_ctx_destroy fail (lines 47-48) */
  g_node_alloc_count = 0;
  g_node_fail_at = 2;
  g_crf_malloc_hook = fail_malloc_at_n;
  g_https_tls_ctx_destroy_fail = 1;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_https_tls_ctx_destroy_fail = 0;
  g_crf_malloc_hook = NULL;

  /* Test fail router_init */
  g_node_alloc_count = 0;
  g_node_fail_at = 4;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail set_router */
  g_ex_set_router_fail = 1;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_ex_set_router_fail = 0;

  /* Test fail router_add */
  g_ex_router_add_count = 0;
  g_ex_router_add_fail_at = 1;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_ex_router_add_fail_at = 0;
  g_ex_router_add_count = 0;

  /* Test fail destroy(ctx) */
  g_ex_destroy_fail = 1;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;

  /* Test fail tls_context_destroy */
  g_https_tls_ctx_destroy_fail = 1;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_https_tls_ctx_destroy_fail = 0;

  /* Test fail router_destroy */
  g_ex_router_destroy_fail = 1;
  ret = app_https_server_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;

  PASS();
}

#if defined(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART) &&                       \
    !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
TEST test_app_hot_reload(void) {
  int ret;
  struct c_rest_request req;
  struct c_rest_response res;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  handle_home(&req, &res, NULL);
  res.body = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  on_file_changed(NULL);
  sig_handler_hr(0);

  ASSERT_EQ(C_REST_ERROR_INVALID_ARG, mock_hr_add_watch(NULL, "a"));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            mock_hr_add_watch((c_rest_hot_reload_ctx_t *)1, NULL));

  g_intercept_run = 2;
  ret = app_hot_reload_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail platform_init */
  g_hr_platform_init_fail = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_hr_platform_init_fail = 0;

  /* Test fail framework init */
  g_node_alloc_count = 0;
  g_node_fail_at = 2;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail router_init */
  g_node_alloc_count = 0;
  g_node_fail_at = 4;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail router_add */
  g_ex_router_add_count = 0;
  g_ex_router_add_fail_at = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_ex_router_add_fail_at = 0;
  g_ex_router_add_count = 0;

  /* Test fail hot_reload_init */
  g_hr_init_fail = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_hr_init_fail = 0;

  /* Test fail add_watch 1st call, 2nd succeeds */
  g_intercept_run = 2;
  g_hr_watch_fail = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(0, ret);
  g_hr_watch_fail = 0;
  g_intercept_run = 0;

  /* Test fail add_watch 1st and 2nd calls, 3rd succeeds */
  g_intercept_run = 2;
  g_hr_watch_fail = 2;
  ret = app_hot_reload_main();
  ASSERT_EQ(0, ret);
  g_hr_watch_fail = 0;
  g_intercept_run = 0;

  /* Test fail add_watch all 3 calls fail */
  g_hr_watch_fail = 3;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_hr_watch_fail = 0;

  /* Test fail register_routes */
  g_hr_register_routes_fail = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_hr_register_routes_fail = 0;

  /* Test fail hot_reload_start (lines 106-111) */
  g_hr_start_fail = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_hr_start_fail = 0;

  /* Test fail set_router */
  g_ex_set_router_fail = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_ex_set_router_fail = 0;

  /* Test fail c_rest_run (lines 133-135) */
  g_intercept_run = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail hot_reload_destroy */
  g_intercept_run = 2;
  g_hr_destroy_fail = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_hr_destroy_fail = 0;
  g_intercept_run = 0;

  /* Test fail router_destroy */
  g_intercept_run = 2;
  g_ex_router_destroy_fail = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;
  g_intercept_run = 0;

  /* Test fail destroy(ctx) */
  g_intercept_run = 2;
  g_ex_destroy_fail = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;
  g_intercept_run = 0;

  /* Test fail platform_cleanup (lines 147-148) */
  g_intercept_run = 2;
  g_hr_platform_cleanup_fail = 1;
  ret = app_hot_reload_main();
  ASSERT_EQ(1, ret);
  g_hr_platform_cleanup_fail = 0;
  g_intercept_run = 0;

  PASS();
}
#endif

TEST test_app_server_sent_events_sse(void) {
  int ret;
  struct c_rest_request req;
  struct c_rest_response res;

  g_sse_delay_ms = 0;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* 0. Event init failure branch (lines 60-61) */
  c_rest_sse_init_response(&res);
  g_sse_event_init_fail = 1;
  my_sse_handler(&req, &res, NULL);
  g_sse_event_init_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 1. Send event failure branch (res->context is NULL, so send_event fails) */
  c_rest_sse_init_response(&res);
  my_sse_handler(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 2. Full success loop (all 5 iterations) via mocks */
  c_rest_sse_init_response(&res);
  g_sse_send_event_ret = 0;
  g_sse_keepalive_ret = 0;
  my_sse_handler(&req, &res, NULL);
  g_sse_send_event_ret = -1;
  g_sse_keepalive_ret = -1;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 3. Keepalive failure branch */
  c_rest_sse_init_response(&res);
  g_sse_send_event_ret = 0;
  g_sse_keepalive_ret = 1;
  my_sse_handler(&req, &res, NULL);
  g_sse_send_event_ret = -1;
  g_sse_keepalive_ret = -1;
  mock_c_rest_sse_send_keepalive(&res);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  sig_handler_sse(0);

  /* Normal execution */
  g_intercept_run = 2;
  ret = app_server_sent_events_sse_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail run */
  g_intercept_run = 1;
  ret = app_server_sent_events_sse_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_server_sent_events_sse_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail router init */
  g_sse_router_init_fail = 1;
  ret = app_server_sent_events_sse_main();
  ASSERT_EQ(1, ret);
  g_sse_router_init_fail = 0;

  /* Test fail add_sse */
  g_sse_add_route_fail = 1;
  ret = app_server_sent_events_sse_main();
  ASSERT_EQ(1, ret);
  g_sse_add_route_fail = 0;

  /* Test fail router_destroy */
  g_intercept_run = 2;
  g_ex_router_destroy_fail = 1;
  ret = app_server_sent_events_sse_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;
  g_intercept_run = 0;

  /* Test fail destroy(ctx) */
  g_intercept_run = 2;
  g_ex_destroy_fail = 1;
  ret = app_server_sent_events_sse_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;
  g_intercept_run = 0;

  PASS();
}

TEST test_app_websockets(void) {
  int ret;
  struct c_rest_request req;
  const unsigned char bin_msg[] = {0x01, 0x02, 0x03};
  const unsigned char txt_msg[] = "hello";

  memset(&req, 0, sizeof(req));

  /* Test callbacks */
  my_ws_on_message(&req, bin_msg, sizeof(bin_msg), 1, NULL);
  my_ws_on_message(&req, txt_msg, sizeof(txt_msg) - 1, 0, NULL);
  my_ws_on_close(&req, 1000, NULL);

  sig_handler_ws(0);

  /* Normal execution */
  g_intercept_run = 2;
  ret = app_websockets_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail run */
  g_intercept_run = 1;
  ret = app_websockets_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_websockets_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail router init */
  g_ws_router_init_fail = 1;
  ret = app_websockets_main();
  ASSERT_EQ(1, ret);
  g_ws_router_init_fail = 0;

  /* Test fail add_websocket */
  g_ws_add_route_fail = 1;
  ret = app_websockets_main();
  ASSERT_EQ(1, ret);
  g_ws_add_route_fail = 0;

  /* Test fail router_destroy */
  g_intercept_run = 2;
  g_ex_router_destroy_fail = 1;
  ret = app_websockets_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;
  g_intercept_run = 0;

  /* Test fail destroy(ctx) */
  g_intercept_run = 2;
  g_ex_destroy_fail = 1;
  ret = app_websockets_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;
  g_intercept_run = 0;

  PASS();
}

TEST test_app_full_multi(void) {
  int ret;
  struct c_rest_request req;
  struct c_rest_response res;
  const char *valid_multipart =
      "--boundary\r\nContent-Disposition: form-data; "
      "name=\"field\"\r\n\r\nvalue\r\n--boundary--\r\n";
  const char *invalid_multipart = "not_starting_with_hyphens";

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* 1. Normal valid upload */
  req.body = (char *)valid_multipart;
  req.body_len = strlen(valid_multipart);
  upload_handler(&req, &res, NULL);
  res.body = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 2. Invalid multipart payload (triggers parse_res != 0) */
  req.body = (char *)invalid_multipart;
  req.body_len = strlen(invalid_multipart);
  upload_handler(&req, &res, NULL);
  res.body = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 2b. Parser execute failure */
  req.body = (char *)valid_multipart;
  req.body_len = strlen(valid_multipart);
  g_multi_parser_exec_fail = 1;
  upload_handler(&req, &res, NULL);
  g_multi_parser_exec_fail = 0;
  res.body = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 2c. Parser destroy failure */
  req.body = (char *)valid_multipart;
  req.body_len = strlen(valid_multipart);
  g_multi_parser_destroy_fail = 1;
  upload_handler(&req, &res, NULL);
  g_multi_parser_destroy_fail = 0;
  res.body = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 3. Empty body */
  req.body = (char *)"some_body";
  req.body_len = 0;
  upload_handler(&req, &res, NULL);
  res.body = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  req.body = NULL;
  req.body_len = 0;
  upload_handler(&req, &res, NULL);
  res.body = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 4. Parser init fail branch */
  g_multi_parser_init_fail = 1;
  upload_handler(&req, &res, NULL);
  g_multi_parser_init_fail = 0;
  res.body = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  sig_handler_full_multi(0);

  /* Normal execution */
  ret = app_full_multi_main();
  ASSERT_EQ(0, ret);

  /* Test fail init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_full_multi_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail router init */
  g_node_alloc_count = 0;
  g_node_fail_at = 4;
  g_crf_malloc_hook = fail_malloc_at_n;
  ret = app_full_multi_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail router_add */
  g_ex_router_add_count = 0;
  g_ex_router_add_fail_at = 1;
  ret = app_full_multi_main();
  ASSERT_EQ(1, ret);
  g_ex_router_add_fail_at = 0;
  g_ex_router_add_count = 0;

  /* Test fail set_router */
  g_ex_set_router_fail = 1;
  ret = app_full_multi_main();
  ASSERT_EQ(1, ret);
  g_ex_set_router_fail = 0;

  /* Test fail router_destroy */
  g_ex_router_destroy_fail = 1;
  ret = app_full_multi_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;

  /* Test fail destroy(ctx) */
  g_ex_destroy_fail = 1;
  ret = app_full_multi_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;

  PASS();
}

TEST test_app_jwt_json_w(void) {
  int ret;
  struct c_rest_request req;
  struct c_rest_response res;
  const unsigned char secret[] = "my_super_secret_key";
  void *auth_ctx = NULL;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* 1. my_verify_payload matching */
  ASSERT_EQ(0, (int)my_verify_payload("{\"sub\":\"user123\"}", &auth_ctx));

  /* 2. my_verify_payload mismatch */
  ASSERT_EQ(1, (int)my_verify_payload("{\"sub\":\"other\"}", &auth_ctx));

  /* 3. protected_route_handler with auth_context == 1 */
  req.auth_context = (void *)1;
  protected_route_handler(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 4. protected_route_handler with auth_context != 1 */
  req.auth_context = NULL;
  protected_route_handler(&req, &res, NULL);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 5. generate_token_handler success */
  generate_token_handler(&req, &res, (void *)secret);
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 6. generate_token_handler failure via malloc hook */
  g_crf_malloc_hook = fail_malloc;
  generate_token_handler(&req, &res, (void *)secret);
  g_crf_malloc_hook = NULL;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 7. generate_token_handler failure via sign fail */
  g_jwt_sign_fail = 1;
  generate_token_handler(&req, &res, (void *)secret);
  g_jwt_sign_fail = 0;
  c_rest_response_cleanup(&res);
  memset(&res, 0, sizeof(res));

  /* 8. generate_token_handler failure via NULL res (lines 60-61) */
  generate_token_handler(&req, NULL, (void *)secret);

  /* 9. generate_token_handler failure via sign fail + NULL res (lines 64-65) */
  g_jwt_sign_fail = 1;
  generate_token_handler(&req, NULL, (void *)secret);
  g_jwt_sign_fail = 0;

  sig_handler_jwt_app(0);

  /* Normal execution */
  ret = app_jwt_json_w_main();
  ASSERT_EQ(0, ret);

  /* Test fail framework init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_jwt_json_w_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail router init */
  g_jwt_router_init_fail = 1;
  ret = app_jwt_json_w_main();
  ASSERT_EQ(1, ret);
  g_jwt_router_init_fail = 0;

  /* Test fail router init + destroy fail (lines 100-101) */
  g_jwt_router_init_fail = 1;
  g_ex_destroy_fail = 1;
  ret = app_jwt_json_w_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;
  g_jwt_router_init_fail = 0;

  /* Test fail router_add #1 */
  g_ex_router_add_count = 0;
  g_ex_router_add_fail_at = 1;
  ret = app_jwt_json_w_main();
  ASSERT_EQ(1, ret);

  /* Test fail router_use */
  g_ex_router_add_fail_at = 0;
  g_ex_router_add_count = 0;
  g_ex_router_use_fail = 1;
  ret = app_jwt_json_w_main();
  ASSERT_EQ(1, ret);
  g_ex_router_use_fail = 0;

  /* Test fail router_add #2 */
  g_ex_router_add_count = 0;
  g_ex_router_add_fail_at = 2;
  ret = app_jwt_json_w_main();
  ASSERT_EQ(1, ret);
  g_ex_router_add_fail_at = 0;
  g_ex_router_add_count = 0;

  /* Test fail router_destroy */
  g_ex_router_destroy_fail = 1;
  ret = app_jwt_json_w_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;

  /* Test fail destroy(ctx) */
  g_ex_destroy_fail = 1;
  ret = app_jwt_json_w_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;

  PASS();
}

TEST test_app_graphql_api(void) {
  int ret;
  char *out_json = NULL;
  size_t out_len = 0;

  /* 1. Test logger_cb */
  logger_cb_graphql_app("test log message");

  /* 2. Test resolve_user success and OOM */
  ret = (int)resolve_user("user", &out_json, &out_len, NULL);
  ASSERT_EQ(0, ret);
  CRF_FREE(out_json);
  out_json = NULL;

  g_crf_malloc_hook = fail_malloc;
  ret = (int)resolve_user("user", &out_json, &out_len, NULL);
  ASSERT_EQ((int)C_REST_ERROR_GENERIC, ret);
  g_crf_malloc_hook = NULL;

  /* 3. Test resolve_posts success and OOM */
  ret = (int)resolve_posts("posts", &out_json, &out_len, NULL);
  ASSERT_EQ(0, ret);
  CRF_FREE(out_json);
  out_json = NULL;

  g_crf_malloc_hook = fail_malloc;
  ret = (int)resolve_posts("posts", &out_json, &out_len, NULL);
  ASSERT_EQ((int)C_REST_ERROR_GENERIC, ret);
  g_crf_malloc_hook = NULL;

  sig_handler_graphql_app(0);

  /* Normal execution */
  g_intercept_run = 2;
  ret = app_graphql_api_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail run (res != 0) */
  g_intercept_run = 1;
  ret = app_graphql_api_main();
  ASSERT_EQ(0, ret);
  g_intercept_run = 0;

  /* Test fail framework init */
  g_crf_malloc_hook = fail_malloc;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);
  g_crf_malloc_hook = NULL;

  /* Test fail router init */
  g_graphql_router_init_fail = 1;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);
  g_graphql_router_init_fail = 0;

  /* Test fail router_use */
  g_ex_router_use_fail = 1;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);
  g_ex_router_use_fail = 0;

  /* Test fail schema init */
  g_graphql_schema_init_fail = 1;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);
  g_graphql_schema_init_fail = 0;

  /* Test fail add_resolver #1 */
  g_gql_add_resolver_count = 0;
  g_gql_add_resolver_fail_at = 1;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);

  /* Test fail add_resolver #2 */
  g_gql_add_resolver_count = 0;
  g_gql_add_resolver_fail_at = 2;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);
  g_gql_add_resolver_fail_at = 0;
  g_gql_add_resolver_count = 0;

  /* Test fail add_graphql */
  g_gql_add_route_fail = 1;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);
  g_gql_add_route_fail = 0;

  /* Test fail set_router */
  g_ex_set_router_fail = 1;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);
  g_ex_set_router_fail = 0;

  /* Test fail schema_free */
  g_intercept_run = 2;
  g_gql_schema_free_fail = 1;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);
  g_gql_schema_free_fail = 0;
  g_intercept_run = 0;

  /* Test fail router_destroy */
  g_intercept_run = 2;
  g_ex_router_destroy_fail = 1;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);
  g_ex_router_destroy_fail = 0;
  g_intercept_run = 0;

  /* Test fail destroy(ctx) */
  g_intercept_run = 2;
  g_ex_destroy_fail = 1;
  ret = app_graphql_api_main();
  ASSERT_EQ(1, ret);
  g_ex_destroy_fail = 0;
  g_intercept_run = 0;

  PASS();
}

int test_examples(void) {
  int failed = 0;
  failed += (test_app_node_style() != 0);
  failed += (test_app_threaded() != 0);
#if !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
  failed += (test_app_multi_process() != 0);
#endif
  failed += (test_app_http23() != 0);
  failed += (test_app_openapi() != 0);
  failed += (test_app_template_engine() != 0);
  failed += (test_app_https_server() != 0);
#if defined(C_REST_ENABLE_HOT_RELOADING_AUTO_RESTART) &&                       \
    !defined(__EMSCRIPTEN__) && !defined(CDD_DOS)
  failed += (test_app_hot_reload() != 0);
#endif
  failed += (test_app_server_sent_events_sse() != 0);
  failed += (test_app_websockets() != 0);
  failed += (test_app_full_multi() != 0);
  failed += (test_app_jwt_json_w() != 0);
  failed += (test_app_graphql_api() != 0);
  return failed;
}
