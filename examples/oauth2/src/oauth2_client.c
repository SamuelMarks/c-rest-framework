/* clang-format off */
#include "oauth2_client.h"
#include "c_rest_client.h"
#include "c_rest_mem.h"
#include "c_rest_string.h"

#include "parson.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* clang-format on */

static char *g_token_url = NULL;
static char *g_client_id = NULL;
static char *g_client_secret = NULL;
static c_orm_db_t *g_db = NULL;
static c_rest_client_context *g_client_ctx = NULL;

int oauth2_client_init(const char *token_url, const char *client_id,
                       const char *client_secret, c_orm_db_t *db) {
  if (token_url == NULL || client_id == NULL || client_secret == NULL) {
    return 1;
  }

  g_token_url = (char *)malloc(strlen(token_url) + 1);
  g_client_id = (char *)malloc(strlen(client_id) + 1);
  g_client_secret = (char *)malloc(strlen(client_secret) + 1);

  if (!g_token_url || !g_client_id || !g_client_secret) {
    oauth2_client_cleanup();
    return 1;
  }

#if defined(_MSC_VER)
  strcpy_s(g_token_url, strlen(token_url) + 1, token_url);
  strcpy_s(g_client_id, strlen(client_id) + 1, client_id);
  strcpy_s(g_client_secret, strlen(client_secret) + 1, client_secret);
#else
  strcpy(g_token_url, token_url);
  strcpy(g_client_id, client_id);
  strcpy(g_client_secret, client_secret);
#endif

  g_db = db;

  if (c_rest_client_init(&g_client_ctx) != 0) {
    oauth2_client_cleanup();
    return 1;
  }

  return 0;
}

int oauth2_client_password_grant(const char *username, const char *password,
                                 char **out_access_token, int *out_expires_in) {
  struct c_rest_client_form_field fields[5];
  struct c_rest_client_header headers[1];
  struct c_rest_client_response *res = NULL;
  char *auth_header = NULL;
  JSON_Value *json_val = NULL;
  JSON_Object *json_obj = NULL;
  const char *access_token = NULL;
  int expires_in = 0;
  int err = 0;

  if (username == NULL || password == NULL || out_access_token == NULL ||
      out_expires_in == NULL) {
    return 1;
  }
  if (g_client_ctx == NULL || g_token_url == NULL) {
    return 1;
  }

  fields[0].key = "grant_type";
  fields[0].value = "password";
  fields[1].key = "username";
  fields[1].value = username;
  fields[2].key = "password";
  fields[2].value = password;
  fields[3].key = "client_id";
  fields[3].value = g_client_id;
  fields[4].key = "client_secret";
  fields[4].value = g_client_secret;

  /* Basic auth header as an alternative or additional requirement depending on
   * server */
  c_rest_client_build_auth_basic(g_client_id, g_client_secret, &auth_header);

  headers[0].key = "Authorization";
  headers[0].value = auth_header ? auth_header : "";

  err = c_rest_client_post_form_sync(g_client_ctx, g_token_url, headers,
                                     auth_header ? 1 : 0, fields, 5, &res);

  if (auth_header) {
    free(auth_header);
  }

  if (err != 0 || res == NULL) {
    if (res)
      c_rest_client_response_free(res);
    return 1;
  }

  if (res->status_code != 200) {
    c_rest_client_response_free(res);
    return 1;
  }

  if (c_rest_client_response_parse_json(res, (void **)&json_val) != 0 ||
      json_val == NULL) {
    c_rest_client_response_free(res);
    return 1;
  }

  json_obj = json_value_get_object(json_val);
  if (json_obj == NULL) {
    json_value_free(json_val);
    c_rest_client_response_free(res);
    return 1;
  }

  access_token = json_object_get_string(json_obj, "access_token");
  expires_in = (int)json_object_get_number(json_obj, "expires_in");

  if (access_token == NULL) {
    json_value_free(json_val);
    c_rest_client_response_free(res);
    return 1;
  }

  *out_access_token = (char *)malloc(strlen(access_token) + 1);
  if (*out_access_token == NULL) {
    json_value_free(json_val);
    c_rest_client_response_free(res);
    return 1;
  }

#if defined(_MSC_VER)
  strcpy_s(*out_access_token, strlen(access_token) + 1, access_token);
#else
  strcpy(*out_access_token, access_token);
#endif

  *out_expires_in = expires_in;

  json_value_free(json_val);
  c_rest_client_response_free(res);

  return 0;
}

int oauth2_client_cleanup(void) {
  if (g_token_url) {
    free(g_token_url);
    g_token_url = NULL;
  }
  if (g_client_id) {
    free(g_client_id);
    g_client_id = NULL;
  }
  if (g_client_secret) {
    free(g_client_secret);
    g_client_secret = NULL;
  }
  if (g_client_ctx) {
    c_rest_client_destroy(g_client_ctx);
    g_client_ctx = NULL;
  }
  return 0;
}
