#ifndef C_REST_MIDDLEWARE_H
#define C_REST_MIDDLEWARE_H

/* clang-format off */
#include "c_rest_router.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Built-in CORS Middleware
 */
int c_rest_cors_middleware(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data);

/*
 * Built-in Logger Middleware
 */
int c_rest_logger_middleware(struct c_rest_request *req,
                             struct c_rest_response *res, void *user_data);

/*
 * Built-in Static File Middleware
 * user_data should be a string representing the physical root path.
 */
int c_rest_static_middleware(struct c_rest_request *req,
                             struct c_rest_response *res, void *user_data);

/*
 * Built-in HSTS (HTTP Strict Transport Security) Middleware
 */
int c_rest_hsts_middleware(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data);

/*
 * Built-in HTTP-to-HTTPS redirect Middleware
 */
int c_rest_https_redirect_middleware(struct c_rest_request *req,
                                     struct c_rest_response *res,
                                     void *user_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_MIDDLEWARE_H */
