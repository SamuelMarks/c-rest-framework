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

static char g_mut_ring[16][512];
static size_t g_mut_ring_idx = 0;
static char *mut_s(const char *s) {
  char *dst = g_mut_ring[g_mut_ring_idx & 15];
  size_t len = strlen(s);
  g_mut_ring_idx++;
  if (len >= sizeof(g_mut_ring[0]))
    len = sizeof(g_mut_ring[0]) - 1;
  memcpy(dst, s, len);
  dst[len] = '\0';
  return dst;
}

TEST test_password_hashing(void) {
  char *hash = NULL;
  int res;

  res = (int)c_rest_hash_password("my_secure_password",
                                  C_REST_HASH_ALG_PBKDF2_SHA256, &hash);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(hash == NULL);

  /* Verify the prefix */
  ASSERT_FALSE(strncmp(hash, "$pbkdf2-sha256$i=100000$", 24) != 0);

  /* Verify with correct password */
  res = (int)c_rest_verify_password("my_secure_password", hash);
  ASSERT_FALSE(res != 0);

  /* Verify with incorrect password */
  res = (int)c_rest_verify_password("wrong_password", hash);
  ASSERT_FALSE(res == 0);

  /* Verify with invalid hash string */
  res =
      (int)c_rest_verify_password("my_secure_password", "invalid_hash_string");
  ASSERT_FALSE(res == 0);

  C_REST_FREE(hash);
  PASS();
}

TEST test_random_string_generation(void) {
  char *rand_str1 = NULL;
  char *rand_str2 = NULL;
  int res;

  res = (int)c_rest_random_string_generate(32, &rand_str1);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(rand_str1 == NULL);

  res = (int)c_rest_random_string_generate(32, &rand_str2);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(rand_str2 == NULL);

  /* Ensure uniqueness */
  ASSERT_FALSE(strcmp(rand_str1, rand_str2) == 0);

  /* Basic length check: 32 bytes encoded to base64url is usually ~43 characters
   */
  ASSERT_FALSE(strlen(rand_str1) < 42);

  C_REST_FREE(rand_str1);
  C_REST_FREE(rand_str2);
  PASS();
}

TEST test_oauth2_generate_access_token(void) {
  char *token1 = NULL;
  char *token2 = NULL;
  int res;

  res = (int)c_rest_oauth2_generate_access_token(&token1);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(token1 == NULL);

  res = (int)c_rest_oauth2_generate_access_token(&token2);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(token2 == NULL);

  ASSERT_FALSE(strcmp(token1, token2) == 0);
  ASSERT_FALSE(strlen(token1) < 42);

  C_REST_FREE(token1);
  C_REST_FREE(token2);
  PASS();
}

TEST test_urlencoded_parser(void) {
  struct c_rest_request req;
  const char *val = NULL;
  int res;

  memset(&req, 0, sizeof(req));
  req.body =
      mut_s("client_id=my_client&client_secret=secret123&grant_type=client_"
            "credentials");
  req.body_len = strlen(req.body);

  res = (int)c_rest_request_parse_urlencoded(&req);
  ASSERT_FALSE(res != 0);

  res = (int)c_rest_request_get_form_param(&req, "client_id", &val);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(strcmp("my_client", val) != 0);

  res = (int)c_rest_request_get_form_param(&req, "grant_type", &val);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(strcmp("client_credentials", val) != 0);

  res = (int)c_rest_request_get_form_param(&req, "missing_param", &val);
  ASSERT_FALSE(res == 0);

  req.body = NULL; /* so cleanup doesn't free string literal */
  res = (int)c_rest_request_cleanup(&req);
  ASSERT_FALSE(res != C_REST_OK);
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
  auth_hdr.key = mut_s("Authorization");
  auth_hdr.value = mut_s("Basic YWRtaW46c2VjcmV0");
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  res = (int)c_rest_request_get_auth_basic(&req, &user, &pass);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(strcmp("admin", user) != 0);
  ASSERT_FALSE(strcmp("secret", pass) != 0);

  C_REST_FREE(user);
  C_REST_FREE(pass);

  req.headers = NULL;
  res = (int)c_rest_request_cleanup(&req);
  ASSERT_FALSE(res != C_REST_OK);
  PASS();
}

TEST test_bearer_token_parser(void) {
  struct c_rest_request req;
  struct c_rest_header auth_hdr;
  char *token = NULL;
  int res;

  memset(&req, 0, sizeof(req));

  auth_hdr.key = mut_s("Authorization");
  auth_hdr.value = mut_s("Bearer abcdef1234567890");
  auth_hdr.next = NULL;
  req.headers = &auth_hdr;

  res = (int)c_rest_request_get_auth_bearer(&req, &token);
  ASSERT_FALSE(res != 0);
  ASSERT_FALSE(strcmp("abcdef1234567890", token) != 0);

  C_REST_FREE(token);

  req.headers = NULL;
  res = (int)c_rest_request_cleanup(&req);
  ASSERT_FALSE(res != C_REST_OK);
  PASS();
}

static c_rest_error_t mock_verify_bearer(const char *token,
                                         void **out_auth_context) {
  int is_valid = (strcmp(token, "valid_token") == 0);
  *out_auth_context = (void *)0x1234;
  return is_valid ? C_REST_OK : C_REST_ERROR_GENERIC;
}

static c_rest_error_t mock_verify_basic(const char *username,
                                        const char *password,
                                        void **out_auth_context) {
  int is_valid =
      (strcmp(username, "admin") == 0) * (strcmp(password, "secret") == 0);
  *out_auth_context = (void *)0x5678;
  return is_valid ? C_REST_OK : C_REST_ERROR_GENERIC;
}

TEST test_auth_middleware(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_auth_verifier verifier;
  c_rest_error_t ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  verifier.verify_bearer = mock_verify_bearer;
  verifier.verify_basic = mock_verify_basic;

  /* Test Missing Auth */
  ret = c_rest_auth_middleware(&req, &res, &verifier);
  ASSERT_FALSE(ret != 1);
  ASSERT_FALSE(res.status_code != 401);

  ASSERT_FALSE(c_rest_request_cleanup(&req) != C_REST_OK);
  ASSERT_FALSE(c_rest_response_cleanup(&res) != C_REST_OK);
  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  /* Test Invalid Bearer */
  {
    struct c_rest_header auth_hdr;
    auth_hdr.key = mut_s("Authorization");
    auth_hdr.value = mut_s("Bearer invalid_token");
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    ret = c_rest_auth_middleware(&req, &res, &verifier);
    ASSERT_FALSE(ret != 1);
    ASSERT_FALSE(res.status_code != 401);

    req.headers = NULL;
    ASSERT_FALSE(c_rest_request_cleanup(&req) != C_REST_OK);
    ASSERT_FALSE(c_rest_response_cleanup(&res) != C_REST_OK);
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
  }

  /* Test Valid Bearer */
  {
    struct c_rest_header auth_hdr;
    auth_hdr.key = mut_s("Authorization");
    auth_hdr.value = mut_s("Bearer valid_token");
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    ret = c_rest_auth_middleware(&req, &res, &verifier);
    ASSERT_FALSE(ret != 0);
    ASSERT_FALSE(req.auth_context != (void *)0x1234);

    req.headers = NULL;
    ASSERT_FALSE(c_rest_request_cleanup(&req) != C_REST_OK);
    ASSERT_FALSE(c_rest_response_cleanup(&res) != C_REST_OK);
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
  }

  /* Test Invalid Basic */
  {
    struct c_rest_header auth_hdr;
    auth_hdr.key = mut_s("Authorization");
    auth_hdr.value = mut_s("Basic YWRtaW46d3Jvbmc="); /* admin:wrong */
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    ret = c_rest_auth_middleware(&req, &res, &verifier);
    ASSERT_FALSE(ret != 1);
    ASSERT_FALSE(res.status_code != 401);

    req.headers = NULL;
    ASSERT_FALSE(c_rest_request_cleanup(&req) != C_REST_OK);
    ASSERT_FALSE(c_rest_response_cleanup(&res) != C_REST_OK);
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
  }

  /* Test Valid Basic */
  {
    struct c_rest_header auth_hdr;
    auth_hdr.key = mut_s("Authorization");
    auth_hdr.value = mut_s("Basic YWRtaW46c2VjcmV0"); /* admin:secret */
    auth_hdr.next = NULL;
    req.headers = &auth_hdr;

    ret = c_rest_auth_middleware(&req, &res, &verifier);
    ASSERT_FALSE(ret != 0);
    ASSERT_FALSE(req.auth_context != (void *)0x5678);

    req.headers = NULL;
    ASSERT_FALSE(c_rest_request_cleanup(&req) != C_REST_OK);
    ASSERT_FALSE(c_rest_response_cleanup(&res) != C_REST_OK);
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
