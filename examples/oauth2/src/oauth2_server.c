/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "oauth2_server.h"

#if defined(_MSC_VER)
#define C_REST_STRCPY(dest, size, src) strcpy_s(dest, size, src)
#else
#define C_REST_STRCPY(dest, size, src) strcpy(dest, src)
#endif

#include "c_orm_oauth2.h"
#include "c_rest_string.h"
#include "c_rest_openapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* clang-format on */

static c_rest_error_t orm_to_rest_error(c_orm_error_t err) {
  return c_rest_error_from_orm((int)err);
}

c_rest_error_t oauth2_server_init(c_rest_router *router, c_orm_db_t *db) {
  c_rest_error_t rc;
  struct c_rest_openapi_operation op_token;
  struct c_rest_openapi_operation op_login;
  struct c_rest_openapi_operation op_logout;
  struct c_rest_openapi_operation op_secret;
  struct c_rest_openapi_operation op_clients;
  struct c_rest_openapi_operation op_users;
  struct c_rest_openapi_spec *spec = NULL;
  struct c_rest_openapi_security_scheme scheme;
  const char *scope_keys[] = {"read", "write"};
  const char *scope_vals[] = {"Read access", "Write access"};
  struct c_rest_openapi_oauth_flow password_flow;

  struct c_rest_openapi_request_body req_body_token;
  struct c_rest_openapi_media_type req_mt_token;
  const char *req_keys_token[] = {"application/x-www-form-urlencoded"};
  struct c_rest_openapi_response res_token;
  struct c_rest_openapi_media_type res_mt_token;
  const char *res_keys_token[] = {"application/json"};
  struct c_rest_openapi_request_body req_body_login;
  struct c_rest_openapi_media_type req_mt_login;
  struct c_rest_openapi_response res_login;
  struct c_rest_openapi_media_type res_mt_login;
  struct c_rest_openapi_request_body req_body_clients;
  struct c_rest_openapi_media_type req_mt_clients;
  struct c_rest_openapi_request_body req_body_users;
  struct c_rest_openapi_media_type req_mt_users;
  const char *tags[] = {"OAuth2"};
  const char *schema_token =
      "{\"type\": \"object\", \"properties\": {\"access_token\": {\"type\": "
      "\"string\"}, \"refresh_token\": {\"type\": \"string\"}, \"token_type\": "
      "{\"type\": \"string\"}, \"expires_in\": {\"type\": \"integer\", "
      "\"format\": \"int32\"}, \"created_at\": {\"type\": \"integer\", "
      "\"format\": \"int64\"}, \"user_id\": {\"type\": \"string\"}, "
      "\"scopes\": {\"type\": \"string\"}}}";

  if (router == NULL || db == NULL) {
    return C_REST_ERROR_GENERIC;
  }

  memset(&op_token, 0, sizeof(op_token));
  op_token.summary = "OAuth2 Token Endpoint";
  op_token.description =
      "Exchanges credentials for an access token (Password Grant).";
  op_token.tags = tags;
  op_token.n_tags = 1;

  memset(&req_body_token, 0, sizeof(req_body_token));
  memset(&req_mt_token, 0, sizeof(req_mt_token));
  req_body_token.required = 1;
  req_body_token.content_keys = req_keys_token;
  req_body_token.content_values = &req_mt_token;
  req_body_token.n_content = 1;
  op_token.request_body = &req_body_token;

  memset(&res_token, 0, sizeof(res_token));
  memset(&res_mt_token, 0, sizeof(res_mt_token));
  res_token.status_code = "200";
  res_token.description = "Successful response";
  res_mt_token.schema.ref_name = "c_orm_oauth2_token_t";
  res_token.content_keys = res_keys_token;
  res_token.content_values = &res_mt_token;
  res_token.n_content = 1;
  op_token.responses = &res_token;
  op_token.n_responses = 1;

  memset(&op_login, 0, sizeof(op_login));
  op_login.summary = "Login Endpoint";
  op_login.description = "Creates a session token via username/password.";
  op_login.tags = tags;
  op_login.n_tags = 1;

  memset(&req_body_login, 0, sizeof(req_body_login));
  memset(&req_mt_login, 0, sizeof(req_mt_login));
  req_body_login.required = 1;
  req_body_login.content_keys = req_keys_token;
  req_body_login.content_values = &req_mt_login;
  req_body_login.n_content = 1;
  op_login.request_body = &req_body_login;

  memset(&res_login, 0, sizeof(res_login));
  memset(&res_mt_login, 0, sizeof(res_mt_login));
  res_login.status_code = "200";
  res_login.description = "Successful response";
  res_mt_login.schema.ref_name = "c_orm_oauth2_token_t";
  res_login.content_keys = res_keys_token;
  res_login.content_values = &res_mt_login;
  res_login.n_content = 1;
  op_login.responses = &res_login;
  op_login.n_responses = 1;

  memset(&op_logout, 0, sizeof(op_logout));
  op_logout.summary = "Logout Endpoint";
  op_logout.description = "Revokes the current token.";
  op_logout.tags = tags;
  op_logout.n_tags = 1;

  memset(&op_secret, 0, sizeof(op_secret));
  op_secret.summary = "Secret Endpoint";
  op_secret.description =
      "A protected endpoint requiring a valid OAuth2 token.";
  op_secret.tags = tags;
  op_secret.n_tags = 1;

  memset(&op_clients, 0, sizeof(op_clients));
  op_clients.summary = "Register Client";
  op_clients.description = "Registers a new OAuth2 client.";
  op_clients.tags = tags;
  op_clients.n_tags = 1;

  memset(&req_body_clients, 0, sizeof(req_body_clients));
  memset(&req_mt_clients, 0, sizeof(req_mt_clients));
  req_body_clients.required = 1;
  req_body_clients.content_keys = req_keys_token;
  req_body_clients.content_values = &req_mt_clients;
  req_body_clients.n_content = 1;
  op_clients.request_body = &req_body_clients;

  memset(&op_users, 0, sizeof(op_users));
  op_users.summary = "Register User";
  op_users.description = "Registers a new User.";
  op_users.tags = tags;
  op_users.n_tags = 1;

  memset(&req_body_users, 0, sizeof(req_body_users));
  memset(&req_mt_users, 0, sizeof(req_mt_users));
  req_body_users.required = 1;
  req_body_users.content_keys = req_keys_token;
  req_body_users.content_values = &req_mt_users;
  req_body_users.n_content = 1;
  op_users.request_body = &req_body_users;

  rc = c_rest_router_get_openapi_spec(router, &spec);
  if (rc != C_REST_OK) {
    if (rc == (c_rest_error_t)999999)
      return rc;
    spec = NULL;
    goto register_plain_routes;
  }

  if (spec) {
    rc = c_rest_router_add_openapi(router, "POST", "/api/v0/oauth/token",
                                   oauth2_token_handler, (void *)db, &op_token);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_router_add_openapi(router, "POST", "/api/v0/login",
                                   oauth2_login_handler, (void *)db, &op_login);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_router_add_openapi(router, "POST", "/api/v0/logout",
                                   oauth2_logout_handler, (void *)db,
                                   &op_logout);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_router_add_openapi(router, "GET", "/api/v0/secret",
                                   oauth2_secret_handler, (void *)db,
                                   &op_secret);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_router_add_openapi(router, "POST", "/api/v0/oauth/clients",
                                   oauth2_register_client_handler, (void *)db,
                                   &op_clients);
    if (rc != C_REST_OK)
      return rc;
    rc = c_rest_router_add_openapi(router, "POST", "/api/v0/users",
                                   oauth2_register_user_handler, (void *)db,
                                   &op_users);
    if (rc != C_REST_OK)
      return rc;
    goto populate_openapi_spec;
  }

register_plain_routes:;
  rc = c_rest_router_add(router, "POST", "/api/v0/oauth/token",
                         oauth2_token_handler, (void *)db);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_router_add(router, "POST", "/api/v0/login", oauth2_login_handler,
                         (void *)db);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_router_add(router, "POST", "/api/v0/logout",
                         oauth2_logout_handler, (void *)db);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_router_add(router, "GET", "/api/v0/secret", oauth2_secret_handler,
                         (void *)db);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_router_add(router, "POST", "/api/v0/oauth/clients",
                         oauth2_register_client_handler, (void *)db);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_router_add(router, "POST", "/api/v0/users",
                         oauth2_register_user_handler, (void *)db);
  if (rc != C_REST_OK)
    return rc;

populate_openapi_spec:
  if (spec) {
    if (C_REST_MALLOC(strlen("OAuth2 Example API") + 1, &spec->info.title) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY((char *)spec->info.title, strlen("OAuth2 Example API") + 1,
                  "OAuth2 Example API");

    if (C_REST_MALLOC(strlen("1.0.0") + 1, &spec->info.version) != C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY((char *)spec->info.version, strlen("1.0.0") + 1, "1.0.0");

    if (C_REST_MALLOC(
            strlen("A simple OAuth2 API example using c-rest-framework.") + 1,
            &spec->info.description) != C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY(
        (char *)spec->info.description,
        strlen("A simple OAuth2 API example using c-rest-framework.") + 1,
        "A simple OAuth2 API example using c-rest-framework.");

    {
      c_rest_error_t schema_rc = c_rest_openapi_spec_add_component_schema(
          spec, "c_orm_oauth2_token_t", schema_token);
      if (schema_rc != C_REST_OK)
        return schema_rc;
    }

    memset(&scheme, 0, sizeof(scheme));
    scheme.name_key = "oauth2";
    scheme.type = "oauth2";
    scheme.description = "OAuth2 Password Grant";

    memset(&password_flow, 0, sizeof(password_flow));
    password_flow.token_url = "/api/v0/oauth/token";
    password_flow.refresh_url = "/api/v0/oauth/token";
    password_flow.scopes_keys = scope_keys;
    password_flow.scopes_values = scope_vals;
    password_flow.n_scopes = 2;
    scheme.flows.password = &password_flow;

    if (C_REST_MALLOC(sizeof(struct c_rest_openapi_security_scheme),
                      &spec->security_schemes) != C_REST_OK)
      return C_REST_ERROR_OOM;
    memset(spec->security_schemes, 0,
           sizeof(struct c_rest_openapi_security_scheme));

    /* Important to copy strings since c_rest_openapi.c uses C_REST_FREE() on
     * them when tearing down */
    if (C_REST_MALLOC(strlen("oauth2") + 1,
                      &spec->security_schemes[0].name_key) != C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY((char *)spec->security_schemes[0].name_key,
                  strlen("oauth2") + 1, "oauth2");
    if (C_REST_MALLOC(strlen("oauth2") + 1, &spec->security_schemes[0].type) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY((char *)spec->security_schemes[0].type, strlen("oauth2") + 1,
                  "oauth2");
    if (C_REST_MALLOC(strlen("OAuth2 Password Grant") + 1,
                      &spec->security_schemes[0].description) != C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY((char *)spec->security_schemes[0].description,
                  strlen("OAuth2 Password Grant") + 1, "OAuth2 Password Grant");

    if (C_REST_MALLOC(sizeof(struct c_rest_openapi_oauth_flow),
                      &spec->security_schemes[0].flows.password) != C_REST_OK)
      return C_REST_ERROR_OOM;
    memset(spec->security_schemes[0].flows.password, 0,
           sizeof(struct c_rest_openapi_oauth_flow));
    if (C_REST_MALLOC(strlen("/api/v0/oauth/token") + 1,
                      &spec->security_schemes[0].flows.password->token_url) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY((char *)spec->security_schemes[0].flows.password->token_url,
                  strlen("/api/v0/oauth/token") + 1, "/api/v0/oauth/token");
    if (C_REST_MALLOC(strlen("/api/v0/oauth/token") + 1,
                      &spec->security_schemes[0].flows.password->refresh_url) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY((char *)spec->security_schemes[0].flows.password->refresh_url,
                  strlen("/api/v0/oauth/token") + 1, "/api/v0/oauth/token");

    if (C_REST_MALLOC(sizeof(char *) * 2,
                      &spec->security_schemes[0].flows.password->scopes_keys) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    if (C_REST_MALLOC(
            sizeof(char *) * 2,
            &spec->security_schemes[0].flows.password->scopes_values) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    if (C_REST_MALLOC(
            strlen(scope_keys[0]) + 1,
            &spec->security_schemes[0].flows.password->scopes_keys[0]) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY(
        (char *)spec->security_schemes[0].flows.password->scopes_keys[0],
        strlen(scope_keys[0]) + 1, scope_keys[0]);
    if (C_REST_MALLOC(
            strlen(scope_vals[0]) + 1,
            &spec->security_schemes[0].flows.password->scopes_values[0]) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY(
        (char *)spec->security_schemes[0].flows.password->scopes_values[0],
        strlen(scope_vals[0]) + 1, scope_vals[0]);
    if (C_REST_MALLOC(
            strlen(scope_keys[1]) + 1,
            &spec->security_schemes[0].flows.password->scopes_keys[1]) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY(
        (char *)spec->security_schemes[0].flows.password->scopes_keys[1],
        strlen(scope_keys[1]) + 1, scope_keys[1]);
    if (C_REST_MALLOC(
            strlen(scope_vals[1]) + 1,
            &spec->security_schemes[0].flows.password->scopes_values[1]) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY(
        (char *)spec->security_schemes[0].flows.password->scopes_values[1],
        strlen(scope_vals[1]) + 1, scope_vals[1]);
    spec->security_schemes[0].flows.password->n_scopes = 2;
    spec->n_security_schemes = 1;

    if (C_REST_MALLOC(sizeof(struct c_rest_openapi_security_requirement),
                      &spec->security) != C_REST_OK)
      return C_REST_ERROR_OOM;
    if (C_REST_MALLOC(strlen("oauth2") + 1, &spec->security[0].name) !=
        C_REST_OK)
      return C_REST_ERROR_OOM;
    C_REST_STRCPY((char *)spec->security[0].name, strlen("oauth2") + 1,
                  "oauth2");
    spec->security[0].scopes = NULL;
    spec->security[0].n_scopes = 0;
    spec->n_security = 1;
  }

  return C_REST_OK;
}

c_rest_error_t oauth2_token_handler(struct c_rest_request *req,
                                    struct c_rest_response *res,
                                    void *user_data) {
  c_orm_db_t *db;
  const char *grant_type = NULL;
  const char *client_id = NULL;
  const char *client_secret = NULL;
  const char *username = NULL;
  const char *password = NULL;
  char *auth_user = NULL;
  char *auth_pass = NULL;
  int is_valid = 0;
  c_orm_error_t err = C_ORM_OK;
  c_orm_oauth2_token_t token;
  struct c_rest_json_pair json_pairs[4];
  c_rest_error_t rc;
  char bearer_type[] = "bearer";

  char *token_str = NULL;
  char *refresh_str = NULL;

  db = (c_orm_db_t *)user_data;

  if (db == NULL || req == NULL || res == NULL) {
    return C_REST_ERROR_GENERIC;
  }

  /* Parse URL Encoded Body */
  rc = c_rest_request_parse_urlencoded(req);
  if (rc != C_REST_OK) {
    c_rest_error_t err_rc = c_rest_response_oauth2_error(
        res, "invalid_request", "Failed to parse form-urlencoded body");
    if (err_rc != C_REST_OK) {
      if (err_rc == (c_rest_error_t)999999)
        return err_rc;
      goto token_done_ok;
    }
    goto token_done_ok;
  }

  rc = c_rest_request_get_form_param(req, "grant_type", &grant_type);
  if (rc != C_REST_OK)
    goto after_grant_type;
after_grant_type:;
  rc = c_rest_request_get_form_param(req, "client_id", &client_id);
  if (rc != C_REST_OK)
    goto after_client_id;
after_client_id:;
  rc = c_rest_request_get_form_param(req, "client_secret", &client_secret);
  if (rc != C_REST_OK)
    goto after_client_secret;
after_client_secret:;
  rc = c_rest_request_get_form_param(req, "username", &username);
  if (rc != C_REST_OK)
    goto after_token_username;
after_token_username:;
  rc = c_rest_request_get_form_param(req, "password", &password);
  if (rc != C_REST_OK)
    goto after_token_password;
after_token_password:;

  if (grant_type == NULL) {
    return c_rest_response_oauth2_error(res, "invalid_request",
                                        "Missing grant_type");
  }

  if (strcmp(grant_type, "password") != 0) {
    rc = c_rest_response_oauth2_error(
        res, "unsupported_grant_type",
        "Only password grant is supported in this example");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_OK;
  }

  /* Extract basic auth if client credentials not in body */
  if (client_id == NULL || client_secret == NULL) {
    rc = c_rest_request_get_auth_basic(req, &auth_user, &auth_pass);
    if (rc != C_REST_OK)
      goto after_basic_auth;
    client_id = auth_user;
    client_secret = auth_pass;
  after_basic_auth:;
  }

  if (client_id == NULL || client_secret == NULL) {
    rc = c_rest_response_oauth2_error(res, "invalid_client",
                                      "Missing client credentials");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_OK;
  }

  err = c_orm_oauth2_verify_client(db, client_id, client_secret, &is_valid);
  if (err != C_ORM_OK) {
    goto invalid_client_creds;
  }
  if (is_valid == 0) {
  invalid_client_creds:
    if (auth_user)
      C_REST_FREE(auth_user);
    if (auth_pass)
      C_REST_FREE(auth_pass);
    rc = c_rest_response_oauth2_error(res, "invalid_client",
                                      "Invalid client credentials");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_OK;
  }

  if (username == NULL || password == NULL) {
    if (auth_user)
      C_REST_FREE(auth_user);
    if (auth_pass)
      C_REST_FREE(auth_pass);
    rc = c_rest_response_oauth2_error(res, "invalid_grant",
                                      "Missing username or password");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_OK;
  }

  err = c_orm_user_verify_credentials(db, username, password, &is_valid);
  if (err != C_ORM_OK) {
    goto invalid_user_creds;
  }
  if (is_valid == 0) {
  invalid_user_creds:
    if (auth_user)
      C_REST_FREE(auth_user);
    if (auth_pass)
      C_REST_FREE(auth_pass);
    rc = c_rest_response_oauth2_error(res, "invalid_grant",
                                      "Invalid username or password");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_OK;
  }

  /* Generate simple token (in a real system, use secure random) */
  token_str = NULL;
  refresh_str = NULL;
  if (C_REST_MALLOC(64, &token_str) != C_REST_OK) {
    token_str = NULL;
  }
  if (C_REST_MALLOC(64, &refresh_str) != C_REST_OK) {
    refresh_str = NULL;
  }

  if (token_str == NULL || refresh_str == NULL) {
    if (token_str)
      C_REST_FREE(token_str);
    if (refresh_str)
      C_REST_FREE(refresh_str);
    if (auth_user)
      C_REST_FREE(auth_user);
    if (auth_pass)
      C_REST_FREE(auth_pass);
    return C_REST_ERROR_GENERIC;
  }

#if defined(_MSC_VER)
  sprintf_s(token_str, 64, "access_token_demo_123");
  sprintf_s(refresh_str, 64, "refresh_token_demo_123");
#else
  sprintf(token_str, "access_token_demo_123");
  sprintf(refresh_str, "refresh_token_demo_123");
#endif

  memset(&token, 0, sizeof(token));
  token.access_token = token_str;
  token.refresh_token = refresh_str;
  token.token_type = bearer_type;
  token.expires_in = 3600;
  token.created_at = (int64_t)time(NULL);
  err = c_orm_oauth2_save_token(db, &token);
  if (err != C_ORM_OK) {
    if (err == (c_orm_error_t)999999) {
      C_REST_FREE(token_str);
      C_REST_FREE(refresh_str);
      if (auth_user)
        C_REST_FREE(auth_user);
      if (auth_pass)
        C_REST_FREE(auth_pass);
      return orm_to_rest_error(err);
    }
    goto after_token_save;
  }
after_token_save:;

  json_pairs[0].key = "access_token";
  json_pairs[0].type = C_REST_JSON_TYPE_STRING;
  json_pairs[0].str_val = token.access_token;

  json_pairs[1].key = "token_type";
  json_pairs[1].type = C_REST_JSON_TYPE_STRING;
  json_pairs[1].str_val = token.token_type;

  json_pairs[2].key = "expires_in";
  json_pairs[2].type = C_REST_JSON_TYPE_NUMBER;
  json_pairs[2].num_val = (double)token.expires_in;

  json_pairs[3].key = "refresh_token";
  json_pairs[3].type = C_REST_JSON_TYPE_STRING;
  json_pairs[3].str_val = token.refresh_token;

  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK) {
    C_REST_FREE(token_str);
    C_REST_FREE(refresh_str);
    if (auth_user)
      C_REST_FREE(auth_user);
    if (auth_pass)
      C_REST_FREE(auth_pass);
    return rc;
  }
  rc = c_rest_response_json_dict(res, json_pairs, 4);
  if (rc != C_REST_OK) {
    C_REST_FREE(token_str);
    C_REST_FREE(refresh_str);
    if (auth_user)
      C_REST_FREE(auth_user);
    if (auth_pass)
      C_REST_FREE(auth_pass);
    return rc;
  }

  C_REST_FREE(token_str);
  C_REST_FREE(refresh_str);
  if (auth_user)
    C_REST_FREE(auth_user);
  if (auth_pass)
    C_REST_FREE(auth_pass);

token_done_ok:
  rc = C_REST_OK;
  return rc;
}

c_rest_error_t oauth2_login_handler(struct c_rest_request *req,
                                    struct c_rest_response *res,
                                    void *user_data) {
  c_orm_db_t *db = (c_orm_db_t *)user_data;
  const char *username = NULL;
  const char *password = NULL;
  int is_valid = 0;
  char *token_str = NULL;
  char *refresh_str = NULL;
  c_orm_oauth2_token_t token;
  struct c_rest_json_pair json_pairs[4];
  c_rest_error_t rc;
  c_orm_error_t orm_err = C_ORM_OK;
  char bearer_type[] = "bearer";

  if (db == NULL || req == NULL || res == NULL) {
    return C_REST_ERROR_GENERIC;
  }

  rc = c_rest_request_parse_urlencoded(req);
  if (rc != C_REST_OK) {
    c_rest_error_t err_rc = c_rest_response_oauth2_error(
        res, "invalid_request", "Failed to parse body");
    if (err_rc != C_REST_OK) {
      if (err_rc == (c_rest_error_t)999999)
        return err_rc;
      goto login_done_ok;
    }
    goto login_done_ok;
  }

  rc = c_rest_request_get_form_param(req, "username", &username);
  if (rc != C_REST_OK)
    goto after_login_user;
after_login_user:;
  rc = c_rest_request_get_form_param(req, "password", &password);
  if (rc != C_REST_OK)
    goto after_login_pass;
after_login_pass:;

  if (username == NULL || password == NULL) {
    c_rest_error_t err_rc = c_rest_response_oauth2_error(
        res, "invalid_request", "Missing username or password");
    if (err_rc != C_REST_OK) {
      if (err_rc == (c_rest_error_t)999999)
        return err_rc;
      goto login_done_ok;
    }
    return C_REST_OK;
  }

  orm_err = c_orm_user_verify_credentials(db, username, password, &is_valid);
  if (orm_err != C_ORM_OK) {
    goto login_invalid_creds;
  }
  if (!is_valid) {
  login_invalid_creds:;
    return c_rest_response_oauth2_error(res, "invalid_grant",
                                        "Invalid credentials");
  }

  token_str = NULL;
  refresh_str = NULL;
  if (C_REST_MALLOC(64, &token_str) != C_REST_OK) {
    token_str = NULL;
  }
  if (C_REST_MALLOC(64, &refresh_str) != C_REST_OK) {
    refresh_str = NULL;
  }
  if (!token_str || !refresh_str) {
    if (token_str)
      C_REST_FREE(token_str);
    if (refresh_str)
      C_REST_FREE(refresh_str);
    return C_REST_ERROR_GENERIC;
  }

#if defined(_MSC_VER)
  sprintf_s(token_str, 64, "access_token_demo_123");
  sprintf_s(refresh_str, 64, "refresh_token_demo_123");
#else
  sprintf(token_str, "access_token_demo_123");
  sprintf(refresh_str, "refresh_token_demo_123");
#endif

  memset(&token, 0, sizeof(token));
  token.access_token = token_str;
  token.refresh_token = refresh_str;
  token.token_type = bearer_type;
  token.expires_in = 3600;
  token.created_at = (int64_t)time(NULL);

  orm_err = c_orm_oauth2_save_token(db, &token);
  if (orm_err != C_ORM_OK) {
    if (orm_err == (c_orm_error_t)999999) {
      C_REST_FREE(token_str);
      C_REST_FREE(refresh_str);
      return orm_to_rest_error(orm_err);
    }
    goto after_login_save;
  }
after_login_save:;

  json_pairs[0].key = "access_token";
  json_pairs[0].type = C_REST_JSON_TYPE_STRING;
  json_pairs[0].str_val = token.access_token;
  json_pairs[1].key = "token_type";
  json_pairs[1].type = C_REST_JSON_TYPE_STRING;
  json_pairs[1].str_val = token.token_type;
  json_pairs[2].key = "expires_in";
  json_pairs[2].type = C_REST_JSON_TYPE_NUMBER;
  json_pairs[2].num_val = (double)token.expires_in;
  json_pairs[3].key = "refresh_token";
  json_pairs[3].type = C_REST_JSON_TYPE_STRING;
  json_pairs[3].str_val = token.refresh_token;

  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK) {
    if (rc == (c_rest_error_t)999999) {
      C_REST_FREE(token_str);
      C_REST_FREE(refresh_str);
      return rc;
    }
    goto after_login_status;
  }
after_login_status:;
  rc = c_rest_response_json_dict(res, json_pairs, 4);
  if (rc != C_REST_OK) {
    C_REST_FREE(token_str);
    C_REST_FREE(refresh_str);
    return rc;
  }

  C_REST_FREE(token_str);
  C_REST_FREE(refresh_str);

login_done_ok:
  rc = C_REST_OK;
  return rc;
}

c_rest_error_t oauth2_logout_handler(struct c_rest_request *req,
                                     struct c_rest_response *res,
                                     void *user_data) {
  c_orm_db_t *db = (c_orm_db_t *)user_data;
  char *token_str = NULL;
  struct c_rest_json_pair json_pair;
  c_rest_error_t rc;
  c_orm_error_t orm_err = C_ORM_OK;

  rc = c_rest_request_get_auth_bearer(req, &token_str);
  if (rc != C_REST_OK) {
    c_rest_error_t s_rc = c_rest_response_set_status(res, 401);
    if (s_rc != C_REST_OK) {
      if (s_rc == (c_rest_error_t)999999)
        return s_rc;
      goto logout_missing_json;
    }
  logout_missing_json:;
    json_pair.key = "error";
    json_pair.type = C_REST_JSON_TYPE_STRING;
    json_pair.str_val = "Missing Bearer token";
    s_rc = c_rest_response_json_dict(res, &json_pair, 1);
    if (s_rc != C_REST_OK)
      return s_rc;
    goto logout_done_ok;
  }

  orm_err = c_orm_oauth2_revoke_token(db, token_str);
  if (orm_err != C_ORM_OK) {
    C_REST_FREE(token_str);
    if (orm_err == (c_orm_error_t)999999) {
      return orm_to_rest_error(orm_err);
    }
    goto after_revoke;
  }
  C_REST_FREE(token_str);
after_revoke:;

  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK) {
    if (rc == (c_rest_error_t)999999)
      return rc;
    goto logout_ok_json;
  }
logout_ok_json:;
  json_pair.key = "message";
  json_pair.type = C_REST_JSON_TYPE_STRING;
  json_pair.str_val = "Successfully logged out";
  rc = c_rest_response_json_dict(res, &json_pair, 1);
  if (rc != C_REST_OK)
    return rc;

logout_done_ok:
  rc = C_REST_OK;
  return rc;
}

c_rest_error_t oauth2_secret_handler(struct c_rest_request *req,
                                     struct c_rest_response *res,
                                     void *user_data) {
  c_orm_db_t *db = (c_orm_db_t *)user_data;
  char *token_str = NULL;
  c_orm_oauth2_token_t db_token;
  struct c_rest_json_pair json_pair;
  c_rest_error_t rc;
  c_orm_error_t orm_err = C_ORM_OK;

  rc = c_rest_request_get_auth_bearer(req, &token_str);
  if (rc != C_REST_OK) {
    c_rest_error_t s_rc = c_rest_response_set_status(res, 401);
    if (s_rc != C_REST_OK)
      return s_rc;
    json_pair.key = "error";
    json_pair.type = C_REST_JSON_TYPE_STRING;
    json_pair.str_val = "Unauthorized";
    s_rc = c_rest_response_json_dict(res, &json_pair, 1);
    if (s_rc != C_REST_OK)
      return s_rc;
    goto secret_done_ok;
  }

  memset(&db_token, 0, sizeof(db_token));
  orm_err = c_orm_oauth2_get_token(db, token_str, &db_token);
  if (orm_err != C_ORM_OK) {
    c_rest_error_t s_rc;
    C_REST_FREE(token_str);
    s_rc = c_rest_response_set_status(res, 401);
    if (s_rc != C_REST_OK)
      return s_rc;
    json_pair.key = "error";
    json_pair.type = C_REST_JSON_TYPE_STRING;
    json_pair.str_val = "Invalid or expired token";
    s_rc = c_rest_response_json_dict(res, &json_pair, 1);
    if (s_rc != C_REST_OK)
      return s_rc;
    goto secret_done_ok;
  }

  orm_err = c_orm_system_free(db_token.access_token);
  if (orm_err != C_ORM_OK) {
    C_REST_FREE(token_str);
    return orm_to_rest_error(orm_err);
  }
  if (db_token.refresh_token) {
    orm_err = c_orm_system_free(db_token.refresh_token);
    if (orm_err != C_ORM_OK) {
      C_REST_FREE(token_str);
      return orm_to_rest_error(orm_err);
    }
  }
  if (db_token.token_type) {
    orm_err = c_orm_system_free(db_token.token_type);
    if (orm_err != C_ORM_OK) {
      C_REST_FREE(token_str);
      return orm_to_rest_error(orm_err);
    }
  }
  if (db_token.user_id) {
    orm_err = c_orm_system_free(db_token.user_id);
    if (orm_err != C_ORM_OK) {
      C_REST_FREE(token_str);
      return orm_to_rest_error(orm_err);
    }
  }
  if (db_token.scopes) {
    orm_err = c_orm_system_free(db_token.scopes);
    if (orm_err != C_ORM_OK) {
      C_REST_FREE(token_str);
      return orm_to_rest_error(orm_err);
    }
  }

  C_REST_FREE(token_str);

  rc = c_rest_response_set_status(res, 200);
  if (rc != C_REST_OK)
    return rc;
  json_pair.key = "secret_message";
  json_pair.type = C_REST_JSON_TYPE_STRING;
  json_pair.str_val = "You have accessed the protected route!";
  rc = c_rest_response_json_dict(res, &json_pair, 1);
  if (rc != C_REST_OK)
    return rc;

secret_done_ok:
  rc = C_REST_OK;
  return rc;
}

c_rest_error_t oauth2_register_client_handler(struct c_rest_request *req,
                                              struct c_rest_response *res,
                                              void *user_data) {
  c_orm_db_t *db = (c_orm_db_t *)user_data;
  const char *client_id = NULL;
  const char *client_secret = NULL;
  c_orm_query_t *query = NULL;
  int has_row = 0;
  struct c_rest_json_pair pairs[2];
  c_rest_error_t rc;
  c_orm_error_t orm_err = C_ORM_OK;

  if (db == NULL || req == NULL || res == NULL) {
    return C_REST_ERROR_GENERIC;
  }

  rc = c_rest_request_parse_urlencoded(req);
  if (rc != C_REST_OK) {
    c_rest_error_t err_rc = c_rest_response_oauth2_error(
        res, "invalid_request", "Failed to parse body");
    if (err_rc != C_REST_OK) {
      if (err_rc == (c_rest_error_t)999999)
        return err_rc;
      goto reg_client_done_ok;
    }
    goto reg_client_done_ok;
  }

  rc = c_rest_request_get_form_param(req, "client_id", &client_id);
  if (rc != C_REST_OK)
    goto after_reg_cid;
after_reg_cid:;
  rc = c_rest_request_get_form_param(req, "client_secret", &client_secret);
  if (rc != C_REST_OK)
    goto after_reg_csec;
after_reg_csec:;

  if (client_id == NULL || client_secret == NULL) {
    rc = c_rest_response_oauth2_error(res, "invalid_request",
                                      "Missing client_id or client_secret");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_OK;
  }

  orm_err =
      db->vtable->prepare(db, c_orm_oauth2_client_meta.query_insert, &query);
  if (orm_err != C_ORM_OK) {
    rc = c_rest_response_oauth2_error(res, "server_error",
                                      "Failed to prepare query");
    if (rc != C_REST_OK)
      return rc;
    goto reg_client_done_ok;
  }

  orm_err = db->vtable->bind_string(query, 1, client_id);
  if (orm_err != C_ORM_OK)
    goto reg_client_step_err;
  orm_err = db->vtable->bind_string(query, 2, client_secret);
  if (orm_err != C_ORM_OK)
    goto reg_client_step_err;
  orm_err =
      db->vtable->bind_string(query, 3, "http://localhost"); /* redirect_uris */
  if (orm_err != C_ORM_OK)
    goto reg_client_step_err;
  orm_err = db->vtable->bind_string(query, 4,
                                    "password refresh_token"); /* grant_types */
  if (orm_err != C_ORM_OK)
    goto reg_client_step_err;
  orm_err = db->vtable->bind_string(query, 5, "read write"); /* scopes */
  if (orm_err != C_ORM_OK)
    goto reg_client_step_err;

  orm_err = db->vtable->step(query, &has_row);
  if (orm_err != C_ORM_OK) {
  reg_client_step_err:;
    {
      c_orm_error_t fin_err = db->vtable->finalize(query);
      if (fin_err != C_ORM_OK) {
        return orm_to_rest_error(fin_err);
      }
    }
    rc = c_rest_response_oauth2_error(
        res, "server_error", "Failed to insert client (maybe duplicate ID)");
    if (rc != C_REST_OK)
      return rc;
    goto reg_client_done_ok;
  }
  orm_err = db->vtable->finalize(query);
  if (orm_err != C_ORM_OK) {
    return orm_to_rest_error(orm_err);
  }

  pairs[0].key = "client_id";
  pairs[0].type = C_REST_JSON_TYPE_STRING;
  pairs[0].str_val = client_id;
  pairs[1].key = "client_secret";
  pairs[1].type = C_REST_JSON_TYPE_STRING;
  pairs[1].str_val = client_secret;

  rc = c_rest_response_set_status(res, 201);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_json_dict(res, pairs, 2);
  if (rc != C_REST_OK)
    return rc;

reg_client_done_ok:
  rc = C_REST_OK;
  return rc;
}

c_rest_error_t oauth2_register_user_handler(struct c_rest_request *req,
                                            struct c_rest_response *res,
                                            void *user_data) {
  c_orm_db_t *db = (c_orm_db_t *)user_data;
  const char *username = NULL;
  const char *password = NULL;
  c_orm_query_t *query = NULL;
  int has_row = 0;
  struct c_rest_json_pair pairs[1];
  c_rest_error_t rc;
  c_orm_error_t orm_err = C_ORM_OK;

  if (db == NULL || req == NULL || res == NULL) {
    return C_REST_ERROR_GENERIC;
  }

  rc = c_rest_request_parse_urlencoded(req);
  if (rc != C_REST_OK) {
    c_rest_error_t err_rc = c_rest_response_oauth2_error(
        res, "invalid_request", "Failed to parse body");
    if (err_rc != C_REST_OK) {
      if (err_rc == (c_rest_error_t)999999)
        return err_rc;
      goto reg_user_done_ok;
    }
    goto reg_user_done_ok;
  }

  rc = c_rest_request_get_form_param(req, "username", &username);
  if (rc != C_REST_OK)
    goto after_reg_uname;
after_reg_uname:;
  rc = c_rest_request_get_form_param(req, "password", &password);
  if (rc != C_REST_OK)
    goto after_reg_upass;
after_reg_upass:;

  if (username == NULL || password == NULL) {
    rc = c_rest_response_oauth2_error(res, "invalid_request",
                                      "Missing username or password");
    if (rc != C_REST_OK)
      return rc;
    return C_REST_OK;
  }

  orm_err = db->vtable->prepare(db, c_orm_user_meta.query_insert, &query);
  if (orm_err != C_ORM_OK) {
    rc = c_rest_response_oauth2_error(res, "server_error",
                                      "Failed to prepare query");
    if (rc != C_REST_OK)
      return rc;
    goto reg_user_done_ok;
  }

  orm_err = db->vtable->bind_string(query, 1,
                                    username); /* Use username as ID for mock */
  if (orm_err != C_ORM_OK)
    goto reg_user_step_err;
  orm_err = db->vtable->bind_string(query, 2, username);
  if (orm_err != C_ORM_OK)
    goto reg_user_step_err;
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
  {
    char *pw_hash = NULL;
    c_orm_error_t free_err;
    orm_err = c_orm_user_hash_password(password, "salt", &pw_hash);
    if (orm_err != C_ORM_OK)
      goto reg_user_step_err;
    orm_err = db->vtable->bind_string(query, 3, pw_hash);
    if (orm_err != C_ORM_OK) {
      free_err = c_orm_system_free(pw_hash);
      if (free_err != C_ORM_OK)
        goto reg_user_step_err;
      goto reg_user_step_err;
    }
    free_err = c_orm_system_free(pw_hash);
    if (free_err != C_ORM_OK)
      goto reg_user_step_err;
  }
#else
  orm_err =
      db->vtable->bind_string(query, 3, password); /* mock uses plaintext */
  if (orm_err != C_ORM_OK)
    goto reg_user_step_err;
#endif
  orm_err = db->vtable->bind_string(query, 4, "salt");
  if (orm_err != C_ORM_OK)
    goto reg_user_step_err;

  orm_err = db->vtable->step(query, &has_row);
  if (orm_err != C_ORM_OK) {
  reg_user_step_err:;
    {
      c_orm_error_t fin_err = db->vtable->finalize(query);
      if (fin_err != C_ORM_OK) {
        return orm_to_rest_error(fin_err);
      }
    }
    rc = c_rest_response_oauth2_error(
        res, "server_error",
        "Failed to insert user (maybe duplicate username)");
    if (rc != C_REST_OK)
      return rc;
    goto reg_user_done_ok;
  }
  orm_err = db->vtable->finalize(query);
  if (orm_err != C_ORM_OK) {
    return orm_to_rest_error(orm_err);
  }

  pairs[0].key = "username";
  pairs[0].type = C_REST_JSON_TYPE_STRING;
  pairs[0].str_val = username;

  rc = c_rest_response_set_status(res, 201);
  if (rc != C_REST_OK)
    return rc;
  rc = c_rest_response_json_dict(res, pairs, 1);
  if (rc != C_REST_OK)
    return rc;

reg_user_done_ok:
  rc = C_REST_OK;
  return rc;
}
