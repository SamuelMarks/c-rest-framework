/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_jwt_middleware.h"

#ifdef C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE

#include "c_rest_crypto.h"
#include <stdlib.h>
#include <string.h>

#ifdef C_REST_TESTING_MALLOC_HOOK
C_REST_EXPORT int g_mock_jwt_fail = 0;
#ifndef c_rest_response_set_status
#define c_rest_response_set_status(r, s) \
  ((g_mock_jwt_fail == 1 || g_mock_jwt_fail == 4 || g_mock_jwt_fail == 7 || g_mock_jwt_fail == 10) \
       ? C_REST_ERROR_GENERIC \
       : c_rest_response_set_status(r, s))
#endif
#ifndef c_rest_response_html
#define c_rest_response_html(r, h) \
  ((g_mock_jwt_fail == 2 || g_mock_jwt_fail == 3 || g_mock_jwt_fail == 6 || g_mock_jwt_fail == 9 || g_mock_jwt_fail == 12) \
       ? C_REST_ERROR_GENERIC \
       : c_rest_response_html(r, h))
#endif
#ifndef c_rest_response_set_header
#define c_rest_response_set_header(r, k, v) \
  ((g_mock_jwt_fail == 5 || g_mock_jwt_fail == 8 || g_mock_jwt_fail == 11) \
       ? C_REST_ERROR_GENERIC \
       : c_rest_response_set_header(r, k, v))
#endif
#endif
#include "c_rest_log.h"
/* clang-format on */

c_rest_error_t c_rest_jwt_middleware_config_init(
    struct c_rest_jwt_middleware_config *config, const unsigned char *secret,
    size_t secret_len,
    c_rest_error_t (*verify_payload)(const char *, void **)) {
  if (!config || !secret || secret_len == 0) {
    return C_REST_ERROR_GENERIC;
  }
  config->secret = secret;
  config->secret_len = secret_len;
  config->verify_payload = verify_payload;
  return C_REST_OK;
}

c_rest_error_t c_rest_jwt_middleware(struct c_rest_request *req,
                                     struct c_rest_response *res,
                                     void *user_data) {
  struct c_rest_jwt_middleware_config *config;
  char *token;
  char *payload;
  void *auth_ctx;
  c_rest_error_t verify_res;
  c_rest_error_t rc;

  token = NULL;
  payload = NULL;
  auth_ctx = NULL;

  if (!req || !res) {
    return C_REST_ERROR_GENERIC;
  }

  if (!user_data) {
    rc = c_rest_response_set_status(res, 500);
    if (rc != C_REST_OK) {
      return rc;
    }
    rc = c_rest_response_html(res, "Internal Server Error: Missing JWT config");
    if (rc != C_REST_OK) {
      return rc;
    }
    return C_REST_ERROR_GENERIC;
  }

  config = (struct c_rest_jwt_middleware_config *)user_data;

  rc = c_rest_request_get_auth_bearer(req, &token);
  if (rc != C_REST_OK) {
    rc = c_rest_response_set_status(res, 401);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_set_header(res, "WWW-Authenticate",
                                    "Bearer realm=\"API\"");
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_html(res, "Unauthorized: Missing Bearer token");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  verify_res = c_rest_jwt_verify_hs256(token, config->secret,
                                       config->secret_len, &payload);
  if (verify_res != C_REST_OK) {
    C_REST_FREE((void *)(token));
    rc = c_rest_response_set_status(res, 401);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_set_header(
        res, "WWW-Authenticate",
        "Bearer realm=\"API\", error=\"invalid_token\"");
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_response_html(res, "Unauthorized: Invalid token signature");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_ERROR_GENERIC;
  }

  if (config->verify_payload) {
    if (config->verify_payload(payload, &auth_ctx) != 0) {
      C_REST_FREE((void *)(token));
      C_REST_FREE((void *)(payload));
      rc = c_rest_response_set_status(res, 401);
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_response_set_header(
          res, "WWW-Authenticate",
          "Bearer realm=\"API\", error=\"invalid_token\"");
      if (rc != C_REST_OK)
        return rc;
      rc = c_rest_response_html(res, "Unauthorized: Invalid token payload");
      if (rc != C_REST_OK)
        return rc;
      return C_REST_ERROR_GENERIC;
    }
  } else {
    /* If no verification callback, we could just pass the payload as
       auth_context, but memory management becomes an issue. We just leave it
       NULL or user should provide a callback. */
  }

  req->auth_context = auth_ctx;

  C_REST_FREE((void *)(token));
  C_REST_FREE((void *)(payload));
  return C_REST_OK;
}

#endif /* C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE */

typedef int c_rest_jwt_middleware_dummy_declaration;
