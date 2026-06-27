/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_jwt_middleware.h"

#ifdef C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE

#include "c_rest_crypto.h"
#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
/* clang-format on */

c_rest_error_t c_rest_jwt_middleware_config_init(
    struct c_rest_jwt_middleware_config *config, const unsigned char *secret,
    size_t secret_len, int (*verify_payload)(const char *, void **)) {
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
  int verify_res;

  token = NULL;
  payload = NULL;
  auth_ctx = NULL;

  if (!req || !res) {            /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  if (!user_data) {                       /* GCOVR_EXCL_LINE */
    c_rest_response_set_status(res, 500); /* GCOVR_EXCL_LINE */
    c_rest_response_html(
        res, "Internal Server Error: Missing JWT config"); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                           /* GCOVR_EXCL_LINE */
  }

  config = (struct c_rest_jwt_middleware_config *)user_data;

  if (c_rest_request_get_auth_bearer(req, &token) != 0) {
    c_rest_response_set_status(res, 401);
    c_rest_response_set_header(res, "WWW-Authenticate", "Bearer realm=\"API\"");
    c_rest_response_html(res, "Unauthorized: Missing Bearer token");
    return C_REST_ERROR_GENERIC;
  }

  verify_res = c_rest_jwt_verify_hs256(token, config->secret,
                                       config->secret_len, &payload);
  if (verify_res != 0) {
    C_REST_FREE((void *)(token));
    c_rest_response_set_status(res, 401);
    c_rest_response_set_header(res, "WWW-Authenticate",
                               "Bearer realm=\"API\", error=\"invalid_token\"");
    c_rest_response_html(res, "Unauthorized: Invalid token signature");
    return C_REST_ERROR_GENERIC;
  }

  if (config->verify_payload) { /* GCOVR_EXCL_LINE */
    if (config->verify_payload(payload, &auth_ctx) != 0) {
      C_REST_FREE((void *)(token));
      C_REST_FREE((void *)(payload));
      c_rest_response_set_status(res, 401);
      c_rest_response_set_header(
          res, "WWW-Authenticate",
          "Bearer realm=\"API\", error=\"invalid_token\"");
      c_rest_response_html(res, "Unauthorized: Invalid token payload");
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
