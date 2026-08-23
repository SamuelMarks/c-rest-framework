/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_jwt_middleware.h"
#include "c_rest_crypto.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

#ifdef C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE

static c_rest_error_t mock_verify_payload_success(const char *payload,
                                                  void **out_auth_context) {
  if (strcmp(payload, "{\"sub\":\"12345\"}") == 0) {
    *out_auth_context = (void *)1;
    return 0;
  }
  return 1;
}

static c_rest_error_t mock_verify_payload_fail(const char *payload,
                                               void **out_auth_context) {
  (void)payload;
  (void)out_auth_context;
  return 1;
}

static int test_jwt_middleware_config_init(void) {
  struct c_rest_jwt_middleware_config config;
  const unsigned char secret[] = "supersecret";

  if (c_rest_jwt_middleware_config_init(NULL, secret, sizeof(secret), NULL) ==
      0)
    return 1;
  if (c_rest_jwt_middleware_config_init(&config, NULL, sizeof(secret), NULL) ==
      0)
    return 1;
  if (c_rest_jwt_middleware_config_init(&config, secret, 0, NULL) == 0)
    return 1;

  if (c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret),
                                        mock_verify_payload_success) != 0)
    return 1;
  if (config.secret != secret)
    return 1;
  if (config.secret_len != sizeof(secret))
    return 1;
  if (config.verify_payload != mock_verify_payload_success)
    return 1;

  return 0;
}

static int test_jwt_middleware_success(void) {
  struct c_rest_jwt_middleware_config config;
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header auth_hdr;
  char *jwt_token;
  char header_val[512];
  const unsigned char secret[] = "supersecret";
  int ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  ret = c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
                                          mock_verify_payload_success);
  if (ret != 0)
    return 1;

  ret = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", secret, sizeof(secret) - 1,
                              &jwt_token);
  if (ret != 0)
    return 1;

#if defined(_MSC_VER)
  sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
  sprintf(header_val, "Bearer %s", jwt_token);
#endif

  auth_hdr.key = "Authorization";
  auth_hdr.value = header_val;
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  ret = c_rest_jwt_middleware(&req, &res, &config);
  if (ret != 0) {
    CRF_FREE(jwt_token);
    return 1;
  }

  if (req.auth_context != (void *)1) {
    CRF_FREE(jwt_token);
    return 1;
  }

  CRF_FREE(jwt_token);
  (void)!c_rest_response_cleanup(&res);
  return 0;
}

static int test_jwt_middleware_missing_token(void) {
  struct c_rest_jwt_middleware_config config;
  struct c_rest_request req;
  struct c_rest_response res;
  const unsigned char secret[] = "supersecret";
  int ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  (void)!c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
                                           mock_verify_payload_success);

  ret = c_rest_jwt_middleware(&req, &res, &config);
  if (ret == 0)
    return 1;
  if (res.status_code != 401)
    return 1;

  (void)!c_rest_response_cleanup(&res);
  return 0;
}

static int test_jwt_middleware_invalid_signature(void) {
  struct c_rest_jwt_middleware_config config;
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header auth_hdr;
  char *jwt_token;
  char header_val[512];
  const unsigned char secret[] = "supersecret";
  const unsigned char wrong_secret[] = "wrongsecret";
  int ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  (void)!c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
                                           mock_verify_payload_success);

  /* Sign with wrong secret */
  ret = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", wrong_secret,
                              sizeof(wrong_secret) - 1, &jwt_token);
  if (ret != 0)
    return 1;

#if defined(_MSC_VER)
  sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
  sprintf(header_val, "Bearer %s", jwt_token);
#endif

  auth_hdr.key = "Authorization";
  auth_hdr.value = header_val;
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  ret = c_rest_jwt_middleware(&req, &res, &config);
  if (ret == 0) {
    CRF_FREE(jwt_token);
    return 1;
  }

  if (res.status_code != 401) {
    CRF_FREE(jwt_token);
    return 1;
  }

  CRF_FREE(jwt_token);
  (void)!c_rest_response_cleanup(&res);
  return 0;
}

static int test_jwt_middleware_invalid_payload(void) {
  struct c_rest_jwt_middleware_config config;
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header auth_hdr;
  char *jwt_token;
  char header_val[512];
  const unsigned char secret[] = "supersecret";
  int ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  (void)!c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
                                           mock_verify_payload_fail);

  ret = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", secret, sizeof(secret) - 1,
                              &jwt_token);
  if (ret != 0)
    return 1;

#if defined(_MSC_VER)
  sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
  sprintf(header_val, "Bearer %s", jwt_token);
#endif

  auth_hdr.key = "Authorization";
  auth_hdr.value = header_val;
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  ret = c_rest_jwt_middleware(&req, &res, &config);
  if (ret == 0) {
    CRF_FREE(jwt_token);
    return 1;
  }

  if (res.status_code != 401) {
    CRF_FREE(jwt_token);
    return 1;
  }

  CRF_FREE(jwt_token);
  (void)!c_rest_response_cleanup(&res);
  return 0;
}

int test_jwt_json_web_tokens_authentication_middleware(void) {
  if (test_jwt_middleware_config_init() != 0)
    return 1;
  if (test_jwt_middleware_success() != 0)
    return 1;
  if (test_jwt_middleware_missing_token() != 0)
    return 1;
  if (test_jwt_middleware_invalid_signature() != 0)
    return 1;
  if (test_jwt_middleware_invalid_payload() != 0)
    return 1;

  /* Test missing verify_payload function */
  {
    struct c_rest_jwt_middleware_config config_no_verify;
    struct c_rest_request req_no_verify;
    struct c_rest_response res_no_verify;
    struct c_rest_header auth_hdr_no_verify;
    char *jwt_token_no_verify;
    char header_val_no_verify[512];
    const unsigned char secret_no_verify[] = "supersecret";

    memset(&req_no_verify, 0, sizeof(req_no_verify));
    memset(&res_no_verify, 0, sizeof(res_no_verify));

    if (c_rest_jwt_middleware_config_init(&config_no_verify, secret_no_verify,
                                          sizeof(secret_no_verify) - 1,
                                          NULL) != 0)
      return 1;

    if (c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", secret_no_verify,
                              sizeof(secret_no_verify) - 1,
                              &jwt_token_no_verify) != 0)
      return 1;

#if defined(_MSC_VER)
    sprintf_s(header_val_no_verify, sizeof(header_val_no_verify), "Bearer %s",
              jwt_token_no_verify);
#else
    sprintf(header_val_no_verify, "Bearer %s", jwt_token_no_verify);
#endif
    auth_hdr_no_verify.key = "Authorization";
    auth_hdr_no_verify.value = header_val_no_verify;
    req_no_verify.headers = &auth_hdr_no_verify;

    if (c_rest_jwt_middleware(&req_no_verify, &res_no_verify,
                              &config_no_verify) != C_REST_OK) {
      CRF_FREE(jwt_token_no_verify);
      return 1;
    }
    CRF_FREE(jwt_token_no_verify);
    c_rest_response_cleanup(&res_no_verify);
  }

  {
    struct c_rest_request req;
    struct c_rest_response res;
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    if (c_rest_jwt_middleware(NULL, &res, NULL) == C_REST_OK)
      return 1;
    if (c_rest_jwt_middleware(&req, NULL, NULL) == C_REST_OK)
      return 1;
    if (c_rest_jwt_middleware(&req, &res, NULL) == C_REST_OK)
      return 1;
    c_rest_response_cleanup(&res);
  }
  return 0;
}

#endif /* C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE */
