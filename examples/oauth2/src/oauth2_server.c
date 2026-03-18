/* clang-format off */
#include "oauth2_server.h"
#include "c_orm_oauth2.h"
#include "c_rest_string.h"
#include "c_rest_mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

int oauth2_server_init(c_rest_router *router, c_orm_db_t *db) {
  if (router == NULL || db == NULL) {
    return 1;
  }
  c_rest_router_add(router, "POST", "/api/v0/oauth/token", oauth2_token_handler, (void *)db);
  c_rest_router_add(router, "POST", "/api/v0/login", oauth2_login_handler, (void *)db);
  c_rest_router_add(router, "POST", "/api/v0/logout", oauth2_logout_handler, (void *)db);
  c_rest_router_add(router, "GET", "/api/v0/secret", oauth2_secret_handler, (void *)db);
  c_rest_router_add(router, "POST", "/api/v0/oauth/clients", oauth2_register_client_handler, (void *)db);
  c_rest_router_add(router, "POST", "/api/v0/users", oauth2_register_user_handler, (void *)db);
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
    c_rest_response_oauth2_error(res, "invalid_request", "Failed to parse body");
    return 0;
  }

  c_rest_request_get_form_param(req, "username", &username);
  c_rest_request_get_form_param(req, "password", &password);

  if (username == NULL || password == NULL) {
    c_rest_response_oauth2_error(res, "invalid_request", "Missing username or password");
    return 0;
  }

  if (c_orm_user_verify_credentials(db, username, password, &is_valid) != 0 || !is_valid) {
    c_rest_response_oauth2_error(res, "invalid_grant", "Invalid credentials");
    return 0;
  }

  token_str = (char *)malloc(64);
  refresh_str = (char *)malloc(64);
  if (!token_str || !refresh_str) {
    if (token_str) free(token_str);
    if (refresh_str) free(refresh_str);
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

  if (db_token.access_token) free(db_token.access_token);
  if (db_token.refresh_token) free(db_token.refresh_token);
  if (db_token.token_type) free(db_token.token_type);
  if (db_token.user_id) free(db_token.user_id);
  if (db_token.scopes) free(db_token.scopes);

  free(token_str);

  c_rest_response_set_status(res, 200);
  json_pair.key = "secret_message";
  json_pair.type = C_REST_JSON_TYPE_STRING;
  json_pair.str_val = "You have accessed the protected route!";
  c_rest_response_json_dict(res, &json_pair, 1);
  return 0;
}

int oauth2_register_client_handler(struct c_rest_request *req,
                                   struct c_rest_response *res, void *user_data) {
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
    c_rest_response_oauth2_error(res, "invalid_request", "Failed to parse body");
    return 0;
  }

  c_rest_request_get_form_param(req, "client_id", &client_id);
  c_rest_request_get_form_param(req, "client_secret", &client_secret);

  if (client_id == NULL || client_secret == NULL) {
    c_rest_response_oauth2_error(res, "invalid_request", "Missing client_id or client_secret");
    return 0;
  }

  if (db->vtable->prepare(db, c_orm_oauth2_client_meta.query_insert, &query) != 0) {
    c_rest_response_oauth2_error(res, "server_error", "Failed to prepare query");
    return 0;
  }

  db->vtable->bind_string(query, 1, client_id);
  db->vtable->bind_string(query, 2, client_secret);
  db->vtable->bind_string(query, 3, "http://localhost"); /* redirect_uris */
  db->vtable->bind_string(query, 4, "password refresh_token"); /* grant_types */
  db->vtable->bind_string(query, 5, "read write"); /* scopes */

  if (db->vtable->step(query, &has_row) != 0) {
    db->vtable->finalize(query);
    c_rest_response_oauth2_error(res, "server_error", "Failed to insert client (maybe duplicate ID)");
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
    c_rest_response_oauth2_error(res, "invalid_request", "Failed to parse body");
    return 0;
  }

  c_rest_request_get_form_param(req, "username", &username);
  c_rest_request_get_form_param(req, "password", &password);

  if (username == NULL || password == NULL) {
    c_rest_response_oauth2_error(res, "invalid_request", "Missing username or password");
    return 0;
  }

  if (db->vtable->prepare(db, c_orm_user_meta.query_insert, &query) != 0) {
    c_rest_response_oauth2_error(res, "server_error", "Failed to prepare query");
    return 0;
  }

  db->vtable->bind_string(query, 1, username); /* Use username as ID for mock */
  db->vtable->bind_string(query, 2, username);
  db->vtable->bind_string(query, 3, password); /* mock uses plaintext */
  db->vtable->bind_string(query, 4, "salt");

  if (db->vtable->step(query, &has_row) != 0) {
    db->vtable->finalize(query);
    c_rest_response_oauth2_error(res, "server_error", "Failed to insert user (maybe duplicate username)");
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
