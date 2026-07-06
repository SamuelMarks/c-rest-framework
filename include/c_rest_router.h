/**
 * @file c_rest_router.h
 * @brief Header file for c_rest_router.h
 */
#ifndef C_REST_ROUTER_H
#define C_REST_ROUTER_H
/* clang-format off */
#include "c_rest_error.h"

#include <stddef.h>
#include "c_rest_error.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

struct c_rest_request;
struct c_rest_response;

/** @brief Handler function signature.
 * @param req Request.
 * @param res Response.
 * @param user_data User data.
 * @return 0 on success. */
typedef c_rest_error_t (*c_rest_handler_fn)(struct c_rest_request *req,
                                            struct c_rest_response *res,
                                            void *user_data);

/** @brief Middleware function signature.
 * @param req Request.
 * @param res Response.
 * @param user_data User data.
 * @return 0 on success. */
typedef c_rest_error_t (*c_rest_middleware_fn)(struct c_rest_request *req,
                                               struct c_rest_response *res,
                                               void *user_data);

/** @brief Opaque router context. */
typedef struct c_rest_router c_rest_router;

/** @brief Initialize router.
 * @param out_router Pointer to store router.
 * @return 0 on success. */
c_rest_error_t c_rest_router_init(c_rest_router **out_router);
/** @brief Destroy router.
 * @param router Router context.
 * @return 0 on success. */
c_rest_error_t c_rest_router_destroy(c_rest_router *router);

/* Route addition */
/** @brief Add a route.
 * @param router Router.
 * @param method Method.
 * @param path Path.
 * @param handler Handler function.
 * @param user_data User data.
 * @return 0 on success. */
c_rest_error_t c_rest_router_add(c_rest_router *router, const char *method,
                                 const char *path, c_rest_handler_fn handler,
                                 void *user_data);

struct c_rest_openapi_operation;
struct c_rest_openapi_spec;

/* WebSocket callbacks */
/** @brief WebSocket message callback.
 * @param req Request.
 * @param payload Payload data.
 * @param payload_len Payload length.
 * @param is_binary 1 if binary.
 * @param user_data User data.
 * @return 0 on success. */
typedef c_rest_error_t (*c_rest_websocket_on_message_fn)(
    struct c_rest_request *req, const unsigned char *payload,
    size_t payload_len, int is_binary, void *user_data);

/** @brief WebSocket close callback.
 * @param req Request.
 * @param status_code Status code.
 * @param user_data User data.
 * @return 0 on success. */
typedef c_rest_error_t (*c_rest_websocket_on_close_fn)(
    struct c_rest_request *req, int status_code, void *user_data);

/* Route addition with OpenAPI metadata */
/** @brief Add a route with OpenAPI metadata.
 * @param router Router.
 * @param method Method.
 * @param path Path.
 * @param handler Handler.
 * @param user_data User data.
 * @param op_meta OpenAPI metadata.
 * @return 0 on success. */
c_rest_error_t
c_rest_router_add_openapi(c_rest_router *router, const char *method,
                          const char *path, c_rest_handler_fn handler,
                          void *user_data,
                          const struct c_rest_openapi_operation *op_meta);

/* Route addition for WebSockets */
/** @brief Add a WebSocket route.
 * @param router Router.
 * @param path Path.
 * @param on_message Message callback.
 * @param on_close Close callback.
 * @param user_data User data.
 * @return 0 on success. */
c_rest_error_t
c_rest_router_add_websocket(c_rest_router *router, const char *path,
                            c_rest_websocket_on_message_fn on_message,
                            c_rest_websocket_on_close_fn on_close,
                            void *user_data);

/* Route addition for WebSockets with OpenAPI metadata */
/** @brief Add a WebSocket route with OpenAPI metadata.
 * @param router Router.
 * @param path Path.
 * @param on_message Message callback.
 * @param on_close Close callback.
 * @param user_data User data.
 * @param op_meta OpenAPI metadata.
 * @return 0 on success. */
c_rest_error_t c_rest_router_add_websocket_openapi(
    c_rest_router *router, const char *path,
    c_rest_websocket_on_message_fn on_message,
    c_rest_websocket_on_close_fn on_close, void *user_data,
    const struct c_rest_openapi_operation *op_meta);

/* Route addition for Server-Sent Events (SSE) */
/** @brief Add an SSE route.
 * @param router Router.
 * @param path Path.
 * @param handler Handler.
 * @param user_data User data.
 * @return 0 on success. */
c_rest_error_t c_rest_router_add_sse(c_rest_router *router, const char *path,
                                     c_rest_handler_fn handler,
                                     void *user_data);

/* Route addition for Server-Sent Events (SSE) with OpenAPI metadata */
/** @brief Add an SSE route with OpenAPI metadata.
 * @param router Router.
 * @param path Path.
 * @param handler Handler.
 * @param user_data User data.
 * @param op_meta OpenAPI metadata.
 * @return 0 on success. */
c_rest_error_t
c_rest_router_add_sse_openapi(c_rest_router *router, const char *path,
                              c_rest_handler_fn handler, void *user_data,
                              const struct c_rest_openapi_operation *op_meta);

#ifdef C_REST_FRAMEWORK_ENABLE_GRAPHQL
struct c_rest_graphql_schema;

/* Route addition for GraphQL endpoints */
c_rest_error_t c_rest_router_add_graphql(c_rest_router *router,
                                         const char *path,
                                         struct c_rest_graphql_schema *schema);

/* Route addition for GraphQL endpoints with OpenAPI metadata */
c_rest_error_t c_rest_router_add_graphql_openapi(
    c_rest_router *router, const char *path,
    struct c_rest_graphql_schema *schema,
    const struct c_rest_openapi_operation *op_meta);
#endif

#ifdef C_REST_ENABLE_SERVER_SIDE_TEMPLATE_ENGINE_HTML_RENDERING
struct c_rest_template_context;

/* Callback to provide data for the template at request time */
typedef c_rest_error_t (*c_rest_template_data_fn)(struct c_rest_request *req,
                                                  const char ***out_keys,
                                                  const char ***out_values,
                                                  size_t *out_count,
                                                  void *user_data);

/* Route addition for Template endpoints */
c_rest_error_t c_rest_router_add_template(
    c_rest_router *router, const char *method, const char *path,
    const struct c_rest_template_context *ctx,
    c_rest_template_data_fn data_provider, void *user_data);

/* Route addition for Template endpoints with OpenAPI metadata */
c_rest_error_t c_rest_router_add_template_openapi(
    c_rest_router *router, const char *method, const char *path,
    const struct c_rest_template_context *ctx,
    c_rest_template_data_fn data_provider, void *user_data,
    const struct c_rest_openapi_operation *op_meta);
#endif

/* Get the underlying OpenAPI spec */
/** @brief Get OpenAPI spec from router.
 * @param router Router.
 * @param out_spec Pointer to store spec.
 * @return 0 on success. */
c_rest_error_t
c_rest_router_get_openapi_spec(c_rest_router *router,
                               struct c_rest_openapi_spec **out_spec);

/* Middleware registration (Pre-handler) */
/** @brief Add a pre-handler middleware.
 * @param router Router.
 * @param path_prefix Path prefix.
 * @param middleware Middleware.
 * @param user_data User data.
 * @return 0 on success. */
c_rest_error_t c_rest_router_use(c_rest_router *router, const char *path_prefix,
                                 c_rest_middleware_fn middleware,
                                 void *user_data);

/* Middleware registration (Post-handler) */
/** @brief Add a post-handler middleware.
 * @param router Router.
 * @param path_prefix Path prefix.
 * @param middleware Middleware.
 * @param user_data User data.
 * @return 0 on success. */
c_rest_error_t c_rest_router_use_post(c_rest_router *router,
                                      const char *path_prefix,
                                      c_rest_middleware_fn middleware,
                                      void *user_data);

/* Dispatch */
/** @brief Dispatch a request.
 * @param router Router.
 * @param req Request.
 * @param res Response.
 * @return 0 on success. */
c_rest_error_t c_rest_router_dispatch(c_rest_router *router,
                                      struct c_rest_request *req,
                                      struct c_rest_response *res);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_ROUTER_H */
