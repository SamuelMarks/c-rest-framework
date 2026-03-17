/* clang-format off */
#include "greatest.h"
#include "c_rest_crypto.h"
#include "c_rest_request.h"
#include "c_rest_parser.h"

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

SUITE(oauth2_suite) {
  RUN_TEST(test_password_hashing);
  RUN_TEST(test_random_string_generation);
  RUN_TEST(test_urlencoded_parser);
  RUN_TEST(test_basic_auth_parser);
  RUN_TEST(test_bearer_token_parser);
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
