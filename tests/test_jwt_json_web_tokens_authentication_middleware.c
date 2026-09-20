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

/**
 * @brief Mock verification callback that succeeds for matching subject.
 * @param payload JWT payload json.
 * @param out_auth_context Output authentication context pointer.
 * @return C_REST_OK on match, error code otherwise.
 */
static c_rest_error_t mock_verify_payload_success(const char *payload,
                                                  void **out_auth_context) {
  int match;
  match = strcmp(payload, "{\"sub\":\"12345\"}");
  *out_auth_context = (void *)(size_t)(match == 0);
  return (match == 0) ? C_REST_OK : C_REST_ERROR_GENERIC;
}

/**
 * @brief Mock verification callback that always fails.
 * @param payload JWT payload json.
 * @param out_auth_context Output authentication context pointer.
 * @return C_REST_ERROR_GENERIC.
 */
static c_rest_error_t mock_verify_payload_fail(const char *payload,
                                               void **out_auth_context) {
  (void)payload;
  (void)out_auth_context;
  return C_REST_ERROR_GENERIC;
}

/**
 * @brief Test suite runner for JWT middleware.
 * @return 0 on success, non-zero on failure.
 */
int test_jwt_json_web_tokens_authentication_middleware(void) {
  int failed = 0;
  struct c_rest_jwt_middleware_config config;
  const unsigned char secret[] = "supersecret";
  const unsigned char wrong_secret[] = "wrongsecret";
  c_rest_error_t rc;
  const char *msgs[2];

  /* 1. c_rest_jwt_middleware_config_init tests */
  rc = c_rest_jwt_middleware_config_init(NULL, secret, sizeof(secret), NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_jwt_middleware_config_init(&config, NULL, sizeof(secret), NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_jwt_middleware_config_init(&config, secret, 0, NULL);
  failed += (rc != C_REST_ERROR_GENERIC);

  rc = c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret),
                                         mock_verify_payload_success);
  failed += (rc != C_REST_OK);
  failed += (config.secret != secret);
  failed += (config.secret_len != sizeof(secret));
  failed += (config.verify_payload != mock_verify_payload_success);

  /* 2. c_rest_jwt_middleware NULL argument tests */
  {
    struct c_rest_request req;
    struct c_rest_response res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    rc = c_rest_jwt_middleware(NULL, &res, NULL);
    failed += (rc != C_REST_ERROR_GENERIC);

    rc = c_rest_jwt_middleware(&req, NULL, NULL);
    failed += (rc != C_REST_ERROR_GENERIC);

    rc = c_rest_jwt_middleware(&req, &res, NULL);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 500);

    rc = c_rest_response_cleanup(&res);
    failed += (rc != C_REST_OK);
  }

  /* 3. Missing bearer token */
  {
    struct c_rest_request req;
    struct c_rest_response res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    rc = c_rest_jwt_middleware(&req, &res, &config);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 401);

    rc = c_rest_response_cleanup(&res);
    failed += (rc != C_REST_OK);
  }

  /* 4. Success path */
  {
    struct c_rest_request req;
    struct c_rest_response res;
    struct c_rest_header auth_hdr;
    char *jwt_token = NULL;
    char header_val[512];

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    rc = c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
                                           mock_verify_payload_success);
    failed += (rc != C_REST_OK);

    rc = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", secret,
                               sizeof(secret) - 1, &jwt_token);
    failed += (rc != C_REST_OK);

#if defined(_MSC_VER)
    sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
    sprintf(header_val, "Bearer %s", jwt_token);
#endif

    auth_hdr.key = "Authorization";
    auth_hdr.value = header_val;
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    rc = c_rest_jwt_middleware(&req, &res, &config);
    failed += (rc != C_REST_OK);
    failed += (req.auth_context != (void *)1);

    CRF_FREE(jwt_token);
    rc = c_rest_response_cleanup(&res);
    failed += (rc != C_REST_OK);
  }

  /* 5. Invalid signature */
  {
    struct c_rest_request req;
    struct c_rest_response res;
    struct c_rest_header auth_hdr;
    char *jwt_token = NULL;
    char header_val[512];

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    rc = c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
                                           mock_verify_payload_success);
    failed += (rc != C_REST_OK);

    rc = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", wrong_secret,
                               sizeof(wrong_secret) - 1, &jwt_token);
    failed += (rc != C_REST_OK);

#if defined(_MSC_VER)
    sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
    sprintf(header_val, "Bearer %s", jwt_token);
#endif

    auth_hdr.key = "Authorization";
    auth_hdr.value = header_val;
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    rc = c_rest_jwt_middleware(&req, &res, &config);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 401);

    CRF_FREE(jwt_token);
    rc = c_rest_response_cleanup(&res);
    failed += (rc != C_REST_OK);
  }

  /* 6. Invalid payload */
  {
    struct c_rest_request req;
    struct c_rest_response res;
    struct c_rest_header auth_hdr;
    char *jwt_token = NULL;
    char header_val[512];

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    rc = c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
                                           mock_verify_payload_fail);
    failed += (rc != C_REST_OK);

    rc = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", secret,
                               sizeof(secret) - 1, &jwt_token);
    failed += (rc != C_REST_OK);

#if defined(_MSC_VER)
    sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
    sprintf(header_val, "Bearer %s", jwt_token);
#endif

    auth_hdr.key = "Authorization";
    auth_hdr.value = header_val;
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    rc = c_rest_jwt_middleware(&req, &res, &config);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 401);

    CRF_FREE(jwt_token);
    rc = c_rest_response_cleanup(&res);
    failed += (rc != C_REST_OK);
  }

  /* 7. Payload verification callback returning failure via mismatch in
   * mock_verify_payload_success */
  {
    struct c_rest_request req;
    struct c_rest_response res;
    struct c_rest_header auth_hdr;
    char *jwt_token = NULL;
    char header_val[512];

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    rc = c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret) - 1,
                                           mock_verify_payload_success);
    failed += (rc != C_REST_OK);

    rc = c_rest_jwt_sign_hs256("{\"sub\":\"mismatch\"}", secret,
                               sizeof(secret) - 1, &jwt_token);
    failed += (rc != C_REST_OK);

#if defined(_MSC_VER)
    sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
    sprintf(header_val, "Bearer %s", jwt_token);
#endif

    auth_hdr.key = "Authorization";
    auth_hdr.value = header_val;
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    rc = c_rest_jwt_middleware(&req, &res, &config);
    failed += (rc != C_REST_ERROR_GENERIC);
    failed += (res.status_code != 401);

    CRF_FREE(jwt_token);
    rc = c_rest_response_cleanup(&res);
    failed += (rc != C_REST_OK);
  }

  /* 8. Missing verify_payload function */
  {
    struct c_rest_jwt_middleware_config config_no_verify;
    struct c_rest_request req_no_verify;
    struct c_rest_response res_no_verify;
    struct c_rest_header auth_hdr_no_verify;
    char *jwt_token_no_verify = NULL;
    char header_val_no_verify[512];
    const unsigned char secret_no_verify[] = "supersecret";

    memset(&req_no_verify, 0, sizeof(req_no_verify));
    memset(&res_no_verify, 0, sizeof(res_no_verify));

    rc = c_rest_jwt_middleware_config_init(&config_no_verify, secret_no_verify,
                                           sizeof(secret_no_verify) - 1, NULL);
    failed += (rc != C_REST_OK);

    rc = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", secret_no_verify,
                               sizeof(secret_no_verify) - 1,
                               &jwt_token_no_verify);
    failed += (rc != C_REST_OK);

#if defined(_MSC_VER)
    sprintf_s(header_val_no_verify, sizeof(header_val_no_verify), "Bearer %s",
              jwt_token_no_verify);
#else
    sprintf(header_val_no_verify, "Bearer %s", jwt_token_no_verify);
#endif
    auth_hdr_no_verify.key = "Authorization";
    auth_hdr_no_verify.value = header_val_no_verify;
    auth_hdr_no_verify.next = NULL;
    req_no_verify.headers = &auth_hdr_no_verify;

    rc = c_rest_jwt_middleware(&req_no_verify, &res_no_verify,
                               &config_no_verify);
    failed += (rc != C_REST_OK);

    CRF_FREE(jwt_token_no_verify);
    rc = c_rest_response_cleanup(&res_no_verify);
    failed += (rc != C_REST_OK);
  }

  msgs[0] = "test_jwt_json_web_tokens_authentication_middleware passed\n";
  msgs[1] = "test_jwt_json_web_tokens_authentication_middleware failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}

#endif /* C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE */
