/* clang-format off */
#include "greatest.h"
#include "c_rest_crypto.h"
#include "c_rest_request.h"
#include "c_rest_parser.h"
#include "c_rest_response.h"
#include "c_rest_middleware.h"

#include <stdlib.h>
#include <string.h>
/* clang-format on */

GREATEST_MAIN_DEFS();

TEST test_password_hashing(void) {
  char *hash = NULL;
  int res;

  res = c_rest_hash_password("my_secure_password",
                             C_REST_HASH_ALG_PBKDF2_SHA256, &hash);
  ASSERT_EQ(0, res);
  ASSERT_NEQ(NULL, hash);

  /* Verify the prefix */
  ASSERT(strncmp(hash, "$pbkdf2-sha256$i=100000$", 24) == 0);

  /* Verify with correct password */
  res = c_rest_verify_password("my_secure_password", hash);
  ASSERT_EQ(0, res);

  /* Verify with incorrect password */
  res = c_rest_verify_password("wrong_password", hash);
  ASSERT_NEQ(0, res);

  /* Verify with invalid hash string */
  res = c_rest_verify_password("my_secure_password", "invalid_hash_string");
  ASSERT_NEQ(0, res);

  free(hash);
  PASS();
}

TEST test_random_string_generation(void) {
  char *rand_str1 = NULL;
  char *rand_str2 = NULL;
  int res;

  res = c_rest_random_string_generate(32, &rand_str1);
  ASSERT_EQ(0, res);
  ASSERT_NEQ(NULL, rand_str1);

  res = c_rest_random_string_generate(32, &rand_str2);
  ASSERT_EQ(0, res);
  ASSERT_NEQ(NULL, rand_str2);

  /* Ensure uniqueness */
  ASSERT(strcmp(rand_str1, rand_str2) != 0);

  /* Basic length check: 32 bytes encoded to base64url is usually ~43 characters
   */
  ASSERT(strlen(rand_str1) >= 42);

  free(rand_str1);
  free(rand_str2);
  PASS();
}

TEST test_oauth2_generate_access_token(void) {
  char *token1 = NULL;
  char *token2 = NULL;
  int res;

  res = c_rest_oauth2_generate_access_token(&token1);
  ASSERT_EQ(0, res);
  ASSERT_NEQ(NULL, token1);

  res = c_rest_oauth2_generate_access_token(&token2);
  ASSERT_EQ(0, res);
  ASSERT_NEQ(NULL, token2);

  ASSERT(strcmp(token1, token2) != 0);
  ASSERT(strlen(token1) >= 42);

  free(token1);
  free(token2);
  PASS();
}

TEST test_urlencoded_parser(void) {
  struct c_rest_request req;
  const char *val = NULL;
  int res;

  memset(&req, 0, sizeof(req));
  req.body = "client_id=my_client&client_secret=secret123&grant_type=client_"
             "credentials";
  req.body_len = strlen(req.body);

  res = c_rest_request_parse_urlencoded(&req);
  ASSERT_EQ(0, res);

  res = c_rest_request_get_form_param(&req, "client_id", &val);
  ASSERT_EQ(0, res);
  ASSERT_STR_EQ("my_client", val);

  res = c_rest_request_get_form_param(&req, "grant_type", &val);
  ASSERT_EQ(0, res);
  ASSERT_STR_EQ("client_credentials", val);

  res = c_rest_request_get_form_param(&req, "missing_param", &val);
  ASSERT_NEQ(0, res);

  req.body = NULL; /* so cleanup doesn't free string literal */
  c_rest_request_cleanup(&req);
  PASS();
}

TEST test_basic_auth_parser(void) {
  struct c_rest_request req;
  struct c_rest_header auth_hdr;
  char *user = NULL;
  char *pass = NULL;
  int res;

  memset(&req, 0, sizeof(req));

  /* "admin:secret" base64 is "YWRtaW46c2VjcmV0" */
  auth_hdr.key = "Authorization";
  auth_hdr.value = "Basic YWRtaW46c2VjcmV0";
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  res = c_rest_request_get_auth_basic(&req, &user, &pass);
  ASSERT_EQ(0, res);
  ASSERT_STR_EQ("admin", user);
  ASSERT_STR_EQ("secret", pass);

  free(user);
  free(pass);

  req.headers = NULL;
  c_rest_request_cleanup(&req);
  PASS();
}

TEST test_bearer_token_parser(void) {
  struct c_rest_request req;
  struct c_rest_header auth_hdr;
  char *token = NULL;
  int res;

  memset(&req, 0, sizeof(req));

  auth_hdr.key = "Authorization";
  auth_hdr.value = "Bearer abcdef1234567890";
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  res = c_rest_request_get_auth_bearer(&req, &token);
  ASSERT_EQ(0, res);
  ASSERT_STR_EQ("abcdef1234567890", token);

  free(token);

  req.headers = NULL;
  c_rest_request_cleanup(&req);
  PASS();
}

static int mock_verify_bearer(const char *token, void **out_auth_context) {
  if (strcmp(token, "valid_token") == 0) {
    *out_auth_context = (void *)0x1234;
    return 0;
  }
  return 1;
}

static int mock_verify_basic(const char *username, const char *password,
                             void **out_auth_context) {
  if (strcmp(username, "admin") == 0 && strcmp(password, "secret") == 0) {
    *out_auth_context = (void *)0x5678;
    return 0;
  }
  return 1;
}

TEST test_auth_middleware(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_auth_verifier verifier;
  int ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  verifier.verify_bearer = mock_verify_bearer;
  verifier.verify_basic = mock_verify_basic;

  /* Test Missing Auth */
  ret = c_rest_auth_middleware(&req, &res, &verifier);
  ASSERT_EQ(1, ret);
  ASSERT_EQ(401, res.status_code);

  c_rest_request_cleanup(&req);
  c_rest_response_cleanup(&res);
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
    ASSERT_EQ(1, ret);
    ASSERT_EQ(401, res.status_code);

    req.headers = NULL;
    c_rest_request_cleanup(&req);
    c_rest_response_cleanup(&res);
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
    ASSERT_EQ(0, ret);
    ASSERT_EQ((void *)0x1234, req.auth_context);

    req.headers = NULL;
    c_rest_request_cleanup(&req);
    c_rest_response_cleanup(&res);
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
    ASSERT_EQ(1, ret);
    ASSERT_EQ(401, res.status_code);

    req.headers = NULL;
    c_rest_request_cleanup(&req);
    c_rest_response_cleanup(&res);
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
    ASSERT_EQ(0, ret);
    ASSERT_EQ((void *)0x5678, req.auth_context);

    req.headers = NULL;
    c_rest_request_cleanup(&req);
    c_rest_response_cleanup(&res);
  }
  PASS();
}

SUITE(oauth2_suite) {
  RUN_TEST(test_password_hashing);
  RUN_TEST(test_random_string_generation);
  RUN_TEST(test_oauth2_generate_access_token);
  RUN_TEST(test_urlencoded_parser);
  RUN_TEST(test_basic_auth_parser);
  RUN_TEST(test_bearer_token_parser);
  RUN_TEST(test_auth_middleware);
}

int test_oauth2(void) {
  int argc = 1;
  char *argv[] = {"test_oauth2"};
  (void)argc;
  (void)argv;
  GREATEST_INIT();
  RUN_SUITE(oauth2_suite);
  GREATEST_PRINT_REPORT();
  return greatest_info.failed > 0 ? 1 : 0;
}
