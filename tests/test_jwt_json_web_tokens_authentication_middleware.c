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
TEST test_jwt_middleware_cases(void) {
  struct c_rest_jwt_middleware_config config;
  const unsigned char secret[] = "supersecret";
  const unsigned char wrong_secret[] = "wrongsecret";
  c_rest_error_t rc;

  /* 1. c_rest_jwt_middleware_config_init tests */
  rc = c_rest_jwt_middleware_config_init(NULL, secret, sizeof(secret), NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_jwt_middleware_config_init(&config, NULL, sizeof(secret), NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_jwt_middleware_config_init(&config, secret, 0, NULL);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  rc = c_rest_jwt_middleware_config_init(&config, secret, sizeof(secret),
                                         mock_verify_payload_success);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(config.secret != secret);
  ASSERT_FALSE(config.secret_len != sizeof(secret));
  ASSERT_FALSE(config.verify_payload != mock_verify_payload_success);

  /* 2. c_rest_jwt_middleware NULL argument tests */
  {
    struct c_rest_request req;
    struct c_rest_response res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    rc = c_rest_jwt_middleware(NULL, &res, NULL);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

    rc = c_rest_jwt_middleware(&req, NULL, NULL);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

    rc = c_rest_jwt_middleware(&req, &res, NULL);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 500);

    rc = c_rest_response_cleanup(&res);
    ASSERT_FALSE(rc != C_REST_OK);
  }

  /* 3. Missing bearer token */
  {
    struct c_rest_request req;
    struct c_rest_response res;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    rc = c_rest_jwt_middleware(&req, &res, &config);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 401);

    rc = c_rest_response_cleanup(&res);
    ASSERT_FALSE(rc != C_REST_OK);
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
    ASSERT_FALSE(rc != C_REST_OK);

    rc = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", secret,
                               sizeof(secret) - 1, &jwt_token);
    ASSERT_FALSE(rc != C_REST_OK);

#if defined(_MSC_VER)
    sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
    sprintf(header_val, "Bearer %s", jwt_token);
#endif

    {
      static char k_auth[] = "Authorization";
      auth_hdr.key = k_auth;
    }
    auth_hdr.value = header_val;
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    rc = c_rest_jwt_middleware(&req, &res, &config);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_FALSE(req.auth_context != (void *)1);

    C_REST_FREE(jwt_token);
    rc = c_rest_response_cleanup(&res);
    ASSERT_FALSE(rc != C_REST_OK);
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
    ASSERT_FALSE(rc != C_REST_OK);

    rc = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", wrong_secret,
                               sizeof(wrong_secret) - 1, &jwt_token);
    ASSERT_FALSE(rc != C_REST_OK);

#if defined(_MSC_VER)
    sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
    sprintf(header_val, "Bearer %s", jwt_token);
#endif

    {
      static char k_auth[] = "Authorization";
      auth_hdr.key = k_auth;
    }
    auth_hdr.value = header_val;
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    rc = c_rest_jwt_middleware(&req, &res, &config);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 401);

    C_REST_FREE(jwt_token);
    rc = c_rest_response_cleanup(&res);
    ASSERT_FALSE(rc != C_REST_OK);
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
    ASSERT_FALSE(rc != C_REST_OK);

    rc = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", secret,
                               sizeof(secret) - 1, &jwt_token);
    ASSERT_FALSE(rc != C_REST_OK);

#if defined(_MSC_VER)
    sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
    sprintf(header_val, "Bearer %s", jwt_token);
#endif

    {
      static char k_auth[] = "Authorization";
      auth_hdr.key = k_auth;
    }
    auth_hdr.value = header_val;
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    rc = c_rest_jwt_middleware(&req, &res, &config);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 401);

    C_REST_FREE(jwt_token);
    rc = c_rest_response_cleanup(&res);
    ASSERT_FALSE(rc != C_REST_OK);
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
    ASSERT_FALSE(rc != C_REST_OK);

    rc = c_rest_jwt_sign_hs256("{\"sub\":\"mismatch\"}", secret,
                               sizeof(secret) - 1, &jwt_token);
    ASSERT_FALSE(rc != C_REST_OK);

#if defined(_MSC_VER)
    sprintf_s(header_val, sizeof(header_val), "Bearer %s", jwt_token);
#else
    sprintf(header_val, "Bearer %s", jwt_token);
#endif

    {
      static char k_auth[] = "Authorization";
      auth_hdr.key = k_auth;
    }
    auth_hdr.value = header_val;
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    rc = c_rest_jwt_middleware(&req, &res, &config);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_FALSE(res.status_code != 401);

    C_REST_FREE(jwt_token);
    rc = c_rest_response_cleanup(&res);
    ASSERT_FALSE(rc != C_REST_OK);
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
    ASSERT_FALSE(rc != C_REST_OK);

    rc = c_rest_jwt_sign_hs256("{\"sub\":\"12345\"}", secret_no_verify,
                               sizeof(secret_no_verify) - 1,
                               &jwt_token_no_verify);
    ASSERT_FALSE(rc != C_REST_OK);

#if defined(_MSC_VER)
    sprintf_s(header_val_no_verify, sizeof(header_val_no_verify), "Bearer %s",
              jwt_token_no_verify);
#else
    sprintf(header_val_no_verify, "Bearer %s", jwt_token_no_verify);
#endif
    {
      static char k_auth_nv[] = "Authorization";
      auth_hdr_no_verify.key = k_auth_nv;
    }
    auth_hdr_no_verify.value = header_val_no_verify;
    auth_hdr_no_verify.next = NULL;
    req_no_verify.headers = &auth_hdr_no_verify;

    rc = c_rest_jwt_middleware(&req_no_verify, &res_no_verify,
                               &config_no_verify);
    ASSERT_FALSE(rc != C_REST_OK);

    C_REST_FREE(jwt_token_no_verify);
    rc = c_rest_response_cleanup(&res_no_verify);
    ASSERT_FALSE(rc != C_REST_OK);
  }

#ifdef C_REST_TESTING_MALLOC_HOOK
  {
    struct c_rest_request mock_req;
    struct c_rest_response mock_res;
    struct c_rest_header mock_hdr;
    char *mock_tok = NULL;
    char mock_hdr_val[512];
    int k;

    ASSERT_EQ(C_REST_OK, c_rest_jwt_sign_hs256("{\"sub\":\"123\"}", secret,
                                               sizeof(secret) - 1, &mock_tok));
#if defined(_MSC_VER)
    sprintf_s(mock_hdr_val, sizeof(mock_hdr_val), "Bearer %s", mock_tok);
#else
    sprintf(mock_hdr_val, "Bearer %s", mock_tok);
#endif
    mock_hdr.key = (char *)"Authorization";
    mock_hdr.value = mock_hdr_val;
    mock_hdr.next = NULL;

    for (k = 1; k <= 12; k++) {
      g_mock_jwt_fail = k;
      memset(&mock_req, 0, sizeof(mock_req));
      memset(&mock_res, 0, sizeof(mock_res));
      mock_req.headers = &mock_hdr;

      if (k <= 2) {
        /* fail on NULL config */
        ASSERT_FALSE(c_rest_jwt_middleware(&mock_req, &mock_res, NULL) ==
                     C_REST_OK);
      } else if (k <= 5) {
        /* fail on missing token */
        mock_req.headers = NULL;
        config.verify_payload = mock_verify_payload_fail;
        ASSERT_FALSE(c_rest_jwt_middleware(&mock_req, &mock_res, &config) ==
                     C_REST_OK);
        config.verify_payload = mock_verify_payload_success;
      } else if (k <= 8) {
        /* fail on invalid token signature */
        mock_hdr.value = (char *)"Bearer invalid.signature.token";
        ASSERT_FALSE(c_rest_jwt_middleware(&mock_req, &mock_res, &config) ==
                     C_REST_OK);
        mock_hdr.value = mock_hdr_val;
      } else {
        /* fail on verify_payload callback */
        ASSERT_FALSE(c_rest_jwt_middleware(&mock_req, &mock_res, &config) ==
                     C_REST_OK);
      }
      ASSERT_EQ(C_REST_OK, c_rest_response_cleanup(&mock_res));
    }
    g_mock_jwt_fail = 0;
    C_REST_FREE(mock_tok);
  }
#endif

  PASS();
}

SUITE(jwt_middleware_suite) { RUN_TEST(test_jwt_middleware_cases); }

#endif /* C_REST_ENABLE_JWT_JSON_WEB_TOKENS_AUTHENTICATION_MIDDLEWARE */
