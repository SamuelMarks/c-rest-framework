#ifndef C_REST_JWT_MIDDLEWARE_H
#define C_REST_JWT_MIDDLEWARE_H

/* clang-format off */
#include "c_rest_request.h"
#include "c_rest_response.h"
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE

/**
 * @brief JWT Middleware configuration context.
 */
struct c_rest_jwt_middleware_config {
  const unsigned char *secret;
  size_t secret_len;
  int (*verify_payload)(const char *payload, void **out_auth_context);
};

/**
 * @brief Initialize a JWT middleware config.
 * @param config The configuration to initialize.
 * @param secret The HMAC secret key.
 * @param secret_len Length of the secret.
 * @param verify_payload Optional callback to verify the payload and create auth
 * context.
 * @return 0 on success, non-zero on error.
 */
int c_rest_jwt_middleware_config_init(
    struct c_rest_jwt_middleware_config *config, const unsigned char *secret,
    size_t secret_len, int (*verify_payload)(const char *, void **));

/**
 * @brief Built-in JWT Authentication Middleware.
 * user_data should be a pointer to a struct c_rest_jwt_middleware_config.
 * @param req The request.
 * @param res The response.
 * @param user_data Pointer to the struct c_rest_jwt_middleware_config.
 * @return 0 to continue, non-zero to stop (e.g., sending 401).
 */
int c_rest_jwt_middleware(struct c_rest_request *req,
                          struct c_rest_response *res, void *user_data);

#endif /* C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE */

/* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_JWT_MIDDLEWARE_H */
