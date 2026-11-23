/* clang-format off */
#include "c_rest_testing_mocks.h"
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_jwt_middleware.h"

#ifdef C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE

#include "c_rest_crypto.h"
#include <stdlib.h>
#include <string.h>
#include "c_rest_log.h"
/* clang-format on */

#ifdef C_REST_TESTING_MALLOC_HOOK
#ifndef c_rest_response_set_status
#define c_rest_response_set_status crf_seam_jwt_set_status
#endif
#ifndef c_rest_response_html
#define c_rest_response_html crf_seam_jwt_html
#endif
#ifndef c_rest_response_set_header
#define c_rest_response_set_header crf_seam_jwt_set_header
#endif
#endif

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

  {
    c_rest_error_t auth_rc = c_rest_request_get_auth_bearer(req, &token);
    if (auth_rc != C_REST_OK) {
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
      return auth_rc;
    }
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
    return verify_res;
  }

  if (config->verify_payload) {
    c_rest_error_t cb_rc = config->verify_payload(payload, &auth_ctx);
    if (cb_rc != C_REST_OK) {
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
      return cb_rc;
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
