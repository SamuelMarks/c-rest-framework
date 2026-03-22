#ifndef C_REST_ROUTER_H
#define C_REST_ROUTER_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

struct c_rest_request;
struct c_rest_response;

/* Handler function signature */
typedef int (*c_rest_handler_fn)(struct c_rest_request *req,
                                 struct c_rest_response *res, void *user_data);

/* Middleware function signature */
typedef int (*c_rest_middleware_fn)(struct c_rest_request *req,
                                    struct c_rest_response *res,
                                    void *user_data);

typedef struct c_rest_router c_rest_router;

int c_rest_router_init(c_rest_router **out_router);
int c_rest_router_destroy(c_rest_router *router);

/* Route addition */
int c_rest_router_add(c_rest_router *router, const char *method,
                      const char *path, c_rest_handler_fn handler,
                      void *user_data);

struct c_rest_openapi_operation;
struct c_rest_openapi_spec;

/* Route addition with OpenAPI metadata */
int c_rest_router_add_openapi(c_rest_router *router, const char *method,
                              const char *path, c_rest_handler_fn handler,
                              void *user_data,
                              const struct c_rest_openapi_operation *op_meta);

/* Get the underlying OpenAPI spec */
int c_rest_router_get_openapi_spec(c_rest_router *router, struct c_rest_openapi_spec **out_spec);

/* Middleware registration (Pre-handler) */
int c_rest_router_use(c_rest_router *router, const char *path_prefix,
                      c_rest_middleware_fn middleware, void *user_data);

/* Middleware registration (Post-handler) */
int c_rest_router_use_post(c_rest_router *router, const char *path_prefix,
                           c_rest_middleware_fn middleware, void *user_data);

/* Dispatch */
int c_rest_router_dispatch(c_rest_router *router, struct c_rest_request *req,
                           struct c_rest_response *res);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_ROUTER_H */
