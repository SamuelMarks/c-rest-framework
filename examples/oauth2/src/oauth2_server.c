/* clang-format off */
#include "oauth2_server.h"
#include "c_orm_oauth2.h"
#include "c_rest_string.h"
#include "c_rest_mem.h"
#include "c_rest_openapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

int oauth2_server_init(c_rest_router *router, c_orm_db_t *db) {
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
    return 1;
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

  c_rest_router_add_openapi(router, "POST", "/api/v0/oauth/token",
                            oauth2_token_handler, (void *)db, &op_token);
  c_rest_router_add_openapi(router, "POST", "/api/v0/login",
                            oauth2_login_handler, (void *)db, &op_login);
  c_rest_router_add_openapi(router, "POST", "/api/v0/logout",
                            oauth2_logout_handler, (void *)db, &op_logout);
  c_rest_router_add_openapi(router, "GET", "/api/v0/secret",
                            oauth2_secret_handler, (void *)db, &op_secret);
  c_rest_router_add_openapi(router, "POST", "/api/v0/oauth/clients",
                            oauth2_register_client_handler, (void *)db,
                            &op_clients);
  c_rest_router_add_openapi(router, "POST", "/api/v0/users",
                            oauth2_register_user_handler, (void *)db,
                            &op_users);

  spec = c_rest_router_get_openapi_spec(router);
  if (spec) {
    spec->info.title = (char *)"OAuth2 Example API";
    spec->info.version = (char *)"1.0.0";
    spec->info.description =
        (char *)"A simple OAuth2 API example using c-rest-framework.";

    c_rest_openapi_spec_add_component_schema(spec, "c_orm_oauth2_token_t",
                                             schema_token);

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

    spec->security_schemes = (struct c_rest_openapi_security_scheme *)malloc(
        sizeof(struct c_rest_openapi_security_scheme));
    memset(spec->security_schemes, 0,
           sizeof(struct c_rest_openapi_security_scheme));

    /* Important to copy strings since c_rest_openapi.c uses free() on them when
     * tearing down */
    spec->security_schemes[0].name_key = (char *)malloc(strlen("oauth2") + 1);
    strcpy((char *)spec->security_schemes[0].name_key, "oauth2");
    spec->security_schemes[0].type = (char *)malloc(strlen("oauth2") + 1);
    strcpy((char *)spec->security_schemes[0].type, "oauth2");
    spec->security_schemes[0].description =
        (char *)malloc(strlen("OAuth2 Password Grant") + 1);
    strcpy((char *)spec->security_schemes[0].description,
           "OAuth2 Password Grant");

    spec->security_schemes[0].flows.password =
        (struct c_rest_openapi_oauth_flow *)malloc(
            sizeof(struct c_rest_openapi_oauth_flow));
    memset(spec->security_schemes[0].flows.password, 0,
           sizeof(struct c_rest_openapi_oauth_flow));
    spec->security_schemes[0].flows.password->token_url =
        (char *)malloc(strlen("/api/v0/oauth/token") + 1);
    strcpy((char *)spec->security_schemes[0].flows.password->token_url,
           "/api/v0/oauth/token");
    spec->security_schemes[0].flows.password->refresh_url =
        (char *)malloc(strlen("/api/v0/oauth/token") + 1);
    strcpy((char *)spec->security_schemes[0].flows.password->refresh_url,
           "/api/v0/oauth/token");

    spec->security_schemes[0].flows.password->scopes_keys =
        (const char **)malloc(sizeof(char *) * 2);
    spec->security_schemes[0].flows.password->scopes_values =
        (const char **)malloc(sizeof(char *) * 2);
    spec->security_schemes[0].flows.password->scopes_keys[0] =
        (char *)malloc(strlen(scope_keys[0]) + 1);
    strcpy((char *)spec->security_schemes[0].flows.password->scopes_keys[0],
           scope_keys[0]);
    spec->security_schemes[0].flows.password->scopes_values[0] =
        (char *)malloc(strlen(scope_vals[0]) + 1);
    strcpy((char *)spec->security_schemes[0].flows.password->scopes_values[0],
           scope_vals[0]);
    spec->security_schemes[0].flows.password->scopes_keys[1] =
        (char *)malloc(strlen(scope_keys[1]) + 1);
    strcpy((char *)spec->security_schemes[0].flows.password->scopes_keys[1],
           scope_keys[1]);
    spec->security_schemes[0].flows.password->scopes_values[1] =
        (char *)malloc(strlen(scope_vals[1]) + 1);
    strcpy((char *)spec->security_schemes[0].flows.password->scopes_values[1],
           scope_vals[1]);
    spec->security_schemes[0].flows.password->n_scopes = 2;
    spec->n_security_schemes = 1;

    spec->security = (struct c_rest_openapi_security_requirement *)malloc(
        sizeof(struct c_rest_openapi_security_requirement));
    spec->security[0].name = (char *)malloc(strlen("oauth2") + 1);
    strcpy((char *)spec->security[0].name, "oauth2");
    spec->security[0].scopes = NULL;
    spec->security[0].n_scopes = 0;
    spec->n_security = 1;
  }

  return 0;
}

int oauth2_token_handler(struct c_rest_request *req,
                         struct c_rest_response *res, void *user_data) {
  c_orm_db_t *db;
  const char *grant_type = NULL;
  const char *client_id = NULL;
  const char *client_secret = NULL;
  const char *username = NULL;
  const char *password = NULL;
  char *auth_user = NULL;
  char *auth_pass = NULL;
  int is_valid = 0;
  int err = 0;
  c_orm_oauth2_token_t token;
  struct c_rest_json_pair json_pairs[4];

  char *token_str = NULL;
  char *refresh_str = NULL;

  db = (c_orm_db_t *)user_data;

  if (db == NULL || req == NULL || res == NULL) {
    return 1;
  }

  /* Parse URL Encoded Body */
  if (c_rest_request_parse_urlencoded(req) != 0) {
    c_rest_response_oauth2_error(res, "invalid_request",
                                 "Failed to parse form-urlencoded body");
    return 0; /* Handler succeeded in processing (by returning error response)
               */
  }

  c_rest_request_get_form_param(req, "grant_type", &grant_type);
  c_rest_request_get_form_param(req, "client_id", &client_id);
  c_rest_request_get_form_param(req, "client_secret", &client_secret);
  c_rest_request_get_form_param(req, "username", &username);
  c_rest_request_get_form_param(req, "password", &password);

  if (grant_type == NULL) {
    c_rest_response_oauth2_error(res, "invalid_request", "Missing grant_type");
    return 0;
  }

  if (strcmp(grant_type, "password") != 0) {
    c_rest_response_oauth2_error(
        res, "unsupported_grant_type",
        "Only password grant is supported in this example");
    return 0;
  }

  /* Extract basic auth if client credentials not in body */
  if (client_id == NULL || client_secret == NULL) {
    if (c_rest_request_get_auth_basic(req, &auth_user, &auth_pass) == 0) {
      client_id = auth_user;
      client_secret = auth_pass;
    }
  }

  if (client_id == NULL || client_secret == NULL) {
    if (auth_user) {
      free(auth_user);
    }
    if (auth_pass) {
      free(auth_pass);
    }
    c_rest_response_oauth2_error(res, "invalid_client",
                                 "Missing client credentials");
    return 0;
  }

  err = c_orm_oauth2_verify_client(db, client_id, client_secret, &is_valid);
  if (err != 0 || is_valid == 0) {
    if (auth_user) {
      free(auth_user);
    }
    if (auth_pass) {
      free(auth_pass);
    }
    c_rest_response_oauth2_error(res, "invalid_client",
                                 "Invalid client credentials");
    return 0;
  }

  if (username == NULL || password == NULL) {
    if (auth_user) {
      free(auth_user);
    }
    if (auth_pass) {
      free(auth_pass);
    }
    c_rest_response_oauth2_error(res, "invalid_grant",
                                 "Missing username or password");
    return 0;
  }

  err = c_orm_user_verify_credentials(db, username, password, &is_valid);
  if (err != 0 || is_valid == 0) {
    if (auth_user) {
      free(auth_user);
    }
    if (auth_pass) {
      free(auth_pass);
    }
    c_rest_response_oauth2_error(res, "invalid_grant",
                                 "Invalid username or password");
    return 0;
  }

  /* Generate simple token (in a real system, use secure random) */
  token_str = (char *)malloc(64);
  refresh_str = (char *)malloc(64);

  if (token_str == NULL || refresh_str == NULL) {
    if (token_str) {
      free(token_str);
    }
    if (refresh_str) {
      free(refresh_str);
    }
    if (auth_user) {
      free(auth_user);
    }
    if (auth_pass) {
      free(auth_pass);
    }
    return 1;
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
  token.token_type = (char *)"bearer";
  token.expires_in = 3600;
  /* Not generating full timestamps or filling scopes for this simple mock
   * example */
  /* c-orm could save this token here */
  c_orm_oauth2_save_token(db, &token);

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

  c_rest_response_set_status(res, 200);
  c_rest_response_json_dict(res, json_pairs, 4);

  free(token_str);
  free(refresh_str);
  if (auth_user) {
    free(auth_user);
  }
  if (auth_pass) {
    free(auth_pass);
  }

  return 0;
}

int oauth2_login_handler(struct c_rest_request *req,
                         struct c_rest_response *res, void *user_data) {
  c_orm_db_t *db = (c_orm_db_t *)user_data;
  const char *username = NULL;
  const char *password = NULL;
  int is_valid = 0;
  char *token_str = NULL;
  char *refresh_str = NULL;
  c_orm_oauth2_token_t token;
  struct c_rest_json_pair json_pairs[4];

  if (db == NULL || req == NULL || res == NULL) {
    return 1;
  }

  if (c_rest_request_parse_urlencoded(req) != 0) {
    c_rest_response_oauth2_error(res, "invalid_request",
                                 "Failed to parse body");
    return 0;
  }

  c_rest_request_get_form_param(req, "username", &username);
  c_rest_request_get_form_param(req, "password", &password);

  if (username == NULL || password == NULL) {
    c_rest_response_oauth2_error(res, "invalid_request",
                                 "Missing username or password");
    return 0;
  }

  if (c_orm_user_verify_credentials(db, username, password, &is_valid) != 0 ||
      !is_valid) {
    c_rest_response_oauth2_error(res, "invalid_grant", "Invalid credentials");
    return 0;
  }

  token_str = (char *)malloc(64);
  refresh_str = (char *)malloc(64);
  if (!token_str || !refresh_str) {
    if (token_str)
      free(token_str);
    if (refresh_str)
      free(refresh_str);
    return 1;
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
  token.token_type = (char *)"bearer";
  token.expires_in = 3600;

  c_orm_oauth2_save_token(db, &token);

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

  c_rest_response_set_status(res, 200);
  c_rest_response_json_dict(res, json_pairs, 4);

  free(token_str);
  free(refresh_str);

  return 0;
}

int oauth2_logout_handler(struct c_rest_request *req,
                          struct c_rest_response *res, void *user_data) {
  c_orm_db_t *db = (c_orm_db_t *)user_data;
  char *token_str = NULL;
  struct c_rest_json_pair json_pair;

  if (c_rest_request_get_auth_bearer(req, &token_str) != 0 || !token_str) {
    c_rest_response_set_status(res, 401);
    json_pair.key = "error";
    json_pair.type = C_REST_JSON_TYPE_STRING;
    json_pair.str_val = "Missing Bearer token";
    c_rest_response_json_dict(res, &json_pair, 1);
    return 0;
  }

  c_orm_oauth2_revoke_token(db, token_str);
  free(token_str);

  c_rest_response_set_status(res, 200);
  json_pair.key = "message";
  json_pair.type = C_REST_JSON_TYPE_STRING;
  json_pair.str_val = "Successfully logged out";
  c_rest_response_json_dict(res, &json_pair, 1);
  return 0;
}

int oauth2_secret_handler(struct c_rest_request *req,
                          struct c_rest_response *res, void *user_data) {
  c_orm_db_t *db = (c_orm_db_t *)user_data;
  char *token_str = NULL;
  c_orm_oauth2_token_t db_token;
  struct c_rest_json_pair json_pair;

  if (c_rest_request_get_auth_bearer(req, &token_str) != 0 || !token_str) {
    c_rest_response_set_status(res, 401);
    json_pair.key = "error";
    json_pair.type = C_REST_JSON_TYPE_STRING;
    json_pair.str_val = "Unauthorized";
    c_rest_response_json_dict(res, &json_pair, 1);
    return 0;
  }

  memset(&db_token, 0, sizeof(db_token));
  if (c_orm_oauth2_get_token(db, token_str, &db_token) != 0) {
    c_rest_response_set_status(res, 401);
    json_pair.key = "error";
    json_pair.type = C_REST_JSON_TYPE_STRING;
    json_pair.str_val = "Invalid or expired token";
    c_rest_response_json_dict(res, &json_pair, 1);
    free(token_str);
    return 0;
  }

  if (db_token.access_token)
    free(db_token.access_token);
  if (db_token.refresh_token)
    free(db_token.refresh_token);
  if (db_token.token_type)
    free(db_token.token_type);
  if (db_token.user_id)
    free(db_token.user_id);
  if (db_token.scopes)
    free(db_token.scopes);

  free(token_str);

  c_rest_response_set_status(res, 200);
  json_pair.key = "secret_message";
  json_pair.type = C_REST_JSON_TYPE_STRING;
  json_pair.str_val = "You have accessed the protected route!";
  c_rest_response_json_dict(res, &json_pair, 1);
  return 0;
}

int oauth2_register_client_handler(struct c_rest_request *req,
                                   struct c_rest_response *res,
                                   void *user_data) {
  c_orm_db_t *db = (c_orm_db_t *)user_data;
  const char *client_id = NULL;
  const char *client_secret = NULL;
  c_orm_query_t *query = NULL;
  int has_row = 0;
  struct c_rest_json_pair pairs[2];

  if (db == NULL || req == NULL || res == NULL) {
    return 1;
  }

  if (c_rest_request_parse_urlencoded(req) != 0) {
    c_rest_response_oauth2_error(res, "invalid_request",
                                 "Failed to parse body");
    return 0;
  }

  c_rest_request_get_form_param(req, "client_id", &client_id);
  c_rest_request_get_form_param(req, "client_secret", &client_secret);

  if (client_id == NULL || client_secret == NULL) {
    c_rest_response_oauth2_error(res, "invalid_request",
                                 "Missing client_id or client_secret");
    return 0;
  }

  if (db->vtable->prepare(db, c_orm_oauth2_client_meta.query_insert, &query) !=
      0) {
    c_rest_response_oauth2_error(res, "server_error",
                                 "Failed to prepare query");
    return 0;
  }

  db->vtable->bind_string(query, 1, client_id);
  db->vtable->bind_string(query, 2, client_secret);
  db->vtable->bind_string(query, 3, "http://localhost"); /* redirect_uris */
  db->vtable->bind_string(query, 4, "password refresh_token"); /* grant_types */
  db->vtable->bind_string(query, 5, "read write");             /* scopes */

  if (db->vtable->step(query, &has_row) != 0) {
    db->vtable->finalize(query);
    c_rest_response_oauth2_error(
        res, "server_error", "Failed to insert client (maybe duplicate ID)");
    return 0;
  }
  db->vtable->finalize(query);

  pairs[0].key = "client_id";
  pairs[0].type = C_REST_JSON_TYPE_STRING;
  pairs[0].str_val = client_id;
  pairs[1].key = "client_secret";
  pairs[1].type = C_REST_JSON_TYPE_STRING;
  pairs[1].str_val = client_secret;

  c_rest_response_set_status(res, 201);
  c_rest_response_json_dict(res, pairs, 2);

  return 0;
}

int oauth2_register_user_handler(struct c_rest_request *req,
                                 struct c_rest_response *res, void *user_data) {
  c_orm_db_t *db = (c_orm_db_t *)user_data;
  const char *username = NULL;
  const char *password = NULL;
  c_orm_query_t *query = NULL;
  int has_row = 0;
  struct c_rest_json_pair pairs[1];

  if (db == NULL || req == NULL || res == NULL) {
    return 1;
  }

  if (c_rest_request_parse_urlencoded(req) != 0) {
    c_rest_response_oauth2_error(res, "invalid_request",
                                 "Failed to parse body");
    return 0;
  }

  c_rest_request_get_form_param(req, "username", &username);
  c_rest_request_get_form_param(req, "password", &password);

  if (username == NULL || password == NULL) {
    c_rest_response_oauth2_error(res, "invalid_request",
                                 "Missing username or password");
    return 0;
  }

  if (db->vtable->prepare(db, c_orm_user_meta.query_insert, &query) != 0) {
    c_rest_response_oauth2_error(res, "server_error",
                                 "Failed to prepare query");
    return 0;
  }

  db->vtable->bind_string(query, 1, username); /* Use username as ID for mock */
  db->vtable->bind_string(query, 2, username);
  db->vtable->bind_string(query, 3, password); /* mock uses plaintext */
  db->vtable->bind_string(query, 4, "salt");

  if (db->vtable->step(query, &has_row) != 0) {
    db->vtable->finalize(query);
    c_rest_response_oauth2_error(
        res, "server_error",
        "Failed to insert user (maybe duplicate username)");
    return 0;
  }
  db->vtable->finalize(query);

  pairs[0].key = "username";
  pairs[0].type = C_REST_JSON_TYPE_STRING;
  pairs[0].str_val = username;

  c_rest_response_set_status(res, 201);
  c_rest_response_json_dict(res, pairs, 1);

  return 0;
}
