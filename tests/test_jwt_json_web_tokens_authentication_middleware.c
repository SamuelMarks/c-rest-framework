/* clang-format off */
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

static int mock_verify_payload_success(const char *payload,
                                       void **out_auth_context) {
  if (strcmp(payload, "{\"sub\":\"12345\"}") == 0) {
    *out_auth_context = (void *)1;
    return 0;
  }
  return 1;
}

static int mock_verify_payload_fail(const char *payload,
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
    free(jwt_token);
    return 1;
  }

  if (req.auth_context != (void *)1) {
    free(jwt_token);
    return 1;
  }

  free(jwt_token);
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

  c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
                                    mock_verify_payload_success);

  ret = c_rest_jwt_middleware(&req, &res, &config);
  if (ret == 0)
    return 1;
  if (res.status_code != 401)
    return 1;

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

  c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
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
    free(jwt_token);
    return 1;
  }

  if (res.status_code != 401) {
    free(jwt_token);
    return 1;
  }

  free(jwt_token);
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

  c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
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
    free(jwt_token);
    return 1;
  }

  if (res.status_code != 401) {
    free(jwt_token);
    return 1;
  }

  free(jwt_token);
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
  return 0;
}

#endif /* C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE */
