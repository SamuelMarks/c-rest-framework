/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "test_protos.h"
#include "c_rest_crypto.h"
#include "c_rest_request.h"
#include "c_rest_parser.h"
#include "c_rest_response.h"
#include "c_rest_middleware.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static int test_password_hashing(void) {
  char *hash = NULL;
  int res;
  int failed = 0;

  res = (int)c_rest_hash_password("my_secure_password",
                                  C_REST_HASH_ALG_PBKDF2_SHA256, &hash);
  failed += (res != 0);
  failed += (hash == NULL);

  /* Verify the prefix */
  failed += (strncmp(hash, "$pbkdf2-sha256$i=100000$", 24) != 0);

  /* Verify with correct password */
  res = (int)c_rest_verify_password("my_secure_password", hash);
  failed += (res != 0);

  /* Verify with incorrect password */
  res = (int)c_rest_verify_password("wrong_password", hash);
  failed += (res == 0);

  /* Verify with invalid hash string */
  res =
      (int)c_rest_verify_password("my_secure_password", "invalid_hash_string");
  failed += (res == 0);

  CRF_FREE(hash);
  return failed;
}

static int test_random_string_generation(void) {
  char *rand_str1 = NULL;
  char *rand_str2 = NULL;
  int res;
  int failed = 0;

  res = (int)c_rest_random_string_generate(32, &rand_str1);
  failed += (res != 0);
  failed += (rand_str1 == NULL);

  res = (int)c_rest_random_string_generate(32, &rand_str2);
  failed += (res != 0);
  failed += (rand_str2 == NULL);

  /* Ensure uniqueness */
  failed += (strcmp(rand_str1, rand_str2) == 0);

  /* Basic length check: 32 bytes encoded to base64url is usually ~43 characters
   */
  failed += (strlen(rand_str1) < 42);

  CRF_FREE(rand_str1);
  CRF_FREE(rand_str2);
  return failed;
}

static int test_oauth2_generate_access_token(void) {
  char *token1 = NULL;
  char *token2 = NULL;
  int res;
  int failed = 0;

  res = (int)c_rest_oauth2_generate_access_token(&token1);
  failed += (res != 0);
  failed += (token1 == NULL);

  res = (int)c_rest_oauth2_generate_access_token(&token2);
  failed += (res != 0);
  failed += (token2 == NULL);

  failed += (strcmp(token1, token2) == 0);
  failed += (strlen(token1) < 42);

  CRF_FREE(token1);
  CRF_FREE(token2);
  return failed;
}

static int test_urlencoded_parser(void) {
  struct c_rest_request req;
  const char *val = NULL;
  int res;
  int failed = 0;

  memset(&req, 0, sizeof(req));
  req.body = "client_id=my_client&client_secret=secret123&grant_type=client_"
             "credentials";
  req.body_len = strlen(req.body);

  res = (int)c_rest_request_parse_urlencoded(&req);
  failed += (res != 0);

  res = (int)c_rest_request_get_form_param(&req, "client_id", &val);
  failed += (res != 0);
  failed += (strcmp("my_client", val) != 0);

  res = (int)c_rest_request_get_form_param(&req, "grant_type", &val);
  failed += (res != 0);
  failed += (strcmp("client_credentials", val) != 0);

  res = (int)c_rest_request_get_form_param(&req, "missing_param", &val);
  failed += (res == 0);

  req.body = NULL; /* so cleanup doesn't free string literal */
  res = (int)c_rest_request_cleanup(&req);
  failed += (res != C_REST_OK);
  return failed;
}

static int test_basic_auth_parser(void) {
  struct c_rest_request req;
  struct c_rest_header auth_hdr;
  char *user = NULL;
  char *pass = NULL;
  int res;
  int failed = 0;

  memset(&req, 0, sizeof(req));

  /* "admin:secret" base64 is "YWRtaW46c2VjcmV0" */
  auth_hdr.key = "Authorization";
  auth_hdr.value = "Basic YWRtaW46c2VjcmV0";
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  res = (int)c_rest_request_get_auth_basic(&req, &user, &pass);
  failed += (res != 0);
  failed += (strcmp("admin", user) != 0);
  failed += (strcmp("secret", pass) != 0);

  CRF_FREE(user);
  CRF_FREE(pass);

  req.headers = NULL;
  res = (int)c_rest_request_cleanup(&req);
  failed += (res != C_REST_OK);
  return failed;
}

static int test_bearer_token_parser(void) {
  struct c_rest_request req;
  struct c_rest_header auth_hdr;
  char *token = NULL;
  int res;
  int failed = 0;

  memset(&req, 0, sizeof(req));

  auth_hdr.key = "Authorization";
  auth_hdr.value = "Bearer abcdef1234567890";
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  res = (int)c_rest_request_get_auth_bearer(&req, &token);
  failed += (res != 0);
  failed += (strcmp("abcdef1234567890", token) != 0);

  CRF_FREE(token);

  req.headers = NULL;
  res = (int)c_rest_request_cleanup(&req);
  failed += (res != C_REST_OK);
  return failed;
}

static int mock_verify_bearer(const char *token, void **out_auth_context) {
  int is_valid = (strcmp(token, "valid_token") == 0);
  *out_auth_context = (void *)0x1234;
  return is_valid ? 0 : 1;
}

static int mock_verify_basic(const char *username, const char *password,
                             void **out_auth_context) {
  int is_valid =
      (strcmp(username, "admin") == 0) * (strcmp(password, "secret") == 0);
  *out_auth_context = (void *)0x5678;
  return is_valid ? 0 : 1;
}

static int test_auth_middleware(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_auth_verifier verifier;
  c_rest_error_t ret;
  int failed = 0;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  verifier.verify_bearer = mock_verify_bearer;
  verifier.verify_basic = mock_verify_basic;

  /* Test Missing Auth */
  ret = c_rest_auth_middleware(&req, &res, &verifier);
  failed += (ret != 1);
  failed += (res.status_code != 401);

  failed += (c_rest_request_cleanup(&req) != C_REST_OK);
  failed += (c_rest_response_cleanup(&res) != C_REST_OK);
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* Test Invalid Bearer */
  {
    struct c_rest_header auth_hdr;
    auth_hdr.key = "Authorization";
    auth_hdr.value = "Bearer invalid_token";
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    ret = c_rest_auth_middleware(&req, &res, &verifier);
    failed += (ret != 1);
    failed += (res.status_code != 401);

    req.headers = NULL;
    failed += (c_rest_request_cleanup(&req) != C_REST_OK);
    failed += (c_rest_response_cleanup(&res) != C_REST_OK);
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
  }

  /* Test Valid Bearer */
  {
    struct c_rest_header auth_hdr;
    auth_hdr.key = "Authorization";
    auth_hdr.value = "Bearer valid_token";
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    ret = c_rest_auth_middleware(&req, &res, &verifier);
    failed += (ret != 0);
    failed += (req.auth_context != (void *)0x1234);

    req.headers = NULL;
    failed += (c_rest_request_cleanup(&req) != C_REST_OK);
    failed += (c_rest_response_cleanup(&res) != C_REST_OK);
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
  }

  /* Test Invalid Basic */
  {
    struct c_rest_header auth_hdr;
    auth_hdr.key = "Authorization";
    auth_hdr.value = "Basic YWRtaW46d3Jvbmc="; /* admin:wrong */
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    ret = c_rest_auth_middleware(&req, &res, &verifier);
    failed += (ret != 1);
    failed += (res.status_code != 401);

    req.headers = NULL;
    failed += (c_rest_request_cleanup(&req) != C_REST_OK);
    failed += (c_rest_response_cleanup(&res) != C_REST_OK);
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
  }

  /* Test Valid Basic */
  {
    struct c_rest_header auth_hdr;
    auth_hdr.key = "Authorization";
    auth_hdr.value = "Basic YWRtaW46c2VjcmV0"; /* admin:secret */
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    ret = c_rest_auth_middleware(&req, &res, &verifier);
    failed += (ret != 0);
    failed += (req.auth_context != (void *)0x5678);

    req.headers = NULL;
    failed += (c_rest_request_cleanup(&req) != C_REST_OK);
    failed += (c_rest_response_cleanup(&res) != C_REST_OK);
  }

  return failed;
}

int test_oauth2(void) {
  int failed = 0;
  const char *msgs[2];

  failed += test_password_hashing();
  failed += test_random_string_generation();
  failed += test_oauth2_generate_access_token();
  failed += test_urlencoded_parser();
  failed += test_basic_auth_parser();
  failed += test_bearer_token_parser();
  failed += test_auth_middleware();

  msgs[0] = "test_oauth2 passed\n";
  msgs[1] = "test_oauth2 failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}
