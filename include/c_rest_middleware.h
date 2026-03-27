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

/**
 * @brief Authentication verifiers for middleware.
 */
struct c_rest_auth_verifier {
  /** @brief Verify a bearer token. Return 0 if valid. */
  int (*verify_bearer)(const char *token, void **out_auth_context);
  /** @brief Verify basic auth credentials. Return 0 if valid. */
  int (*verify_basic)(const char *username, const char *password,
                      void **out_auth_context);
};

/**
 * @brief Universal Authentication Middleware.
 * Handles both Bearer and Basic authentication, safely injecting the verified
 * credential object into req->auth_context.
 * user_data should be a pointer to a struct c_rest_auth_verifier.
 * @param req The request.
 * @param res The response.
 * @param user_data Pointer to the struct c_rest_auth_verifier.
 * @return 0 to continue, non-zero to stop (e.g., sending 401).
 */
int c_rest_auth_middleware(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data);

/**
 * @brief OAuth2 Bearer Token Middleware verification callback.
 * @param token The extracted bearer token.
 * @param out_auth_context Pointer to store the resolved user context.
 * @return 0 if token is valid, non-zero otherwise.
 */
typedef int (*c_rest_oauth2_verify_fn)(const char *token,
                                       void **out_auth_context);

/**
 * @brief Built-in OAuth2 Middleware.
 * user_data should be a pointer to a c_rest_oauth2_verify_fn.
 * @param req The request.
 * @param res The response.
 * @param user_data Pointer to the verification function.
 * @return 0 to continue, non-zero to stop (e.g., sending 401).
 */
int c_rest_oauth2_middleware(struct c_rest_request *req,
                             struct c_rest_response *res, void *user_data);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* C_REST_MIDDLEWARE_H */
