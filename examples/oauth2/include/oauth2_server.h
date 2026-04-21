#ifndef OAUTH2_SERVER_H
#define OAUTH2_SERVER_H

/* clang-format off */
#include "c_rest_router.h"
#include "c_orm_db.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the OAuth2 server routes on a given router.
 *
 * @param router The REST framework router.
 * @param db The c-orm database connection.
 * @return 0 on success, non-zero on failure.
 */
int oauth2_server_init(c_rest_router *router, c_orm_db_t *db);

/**
 * @brief Token endpoint handler for OAuth2 (RFC 6749).
 * Supports password grant flow.
 *
 * @param req The incoming HTTP request.
 * @param res The outgoing HTTP response.
 * @param user_data Pointer to c_orm_db_t.
 * @return 0 on success, non-zero on failure.
 */
int oauth2_token_handler(struct c_rest_request *req,
                         struct c_rest_response *res, void *user_data);

/**
 * @brief Login handler that creates a token from username and password.
 */
int oauth2_login_handler(struct c_rest_request *req,
                         struct c_rest_response *res, void *user_data);

/**
 * @brief Logout handler that revokes a given token.
 */
int oauth2_logout_handler(struct c_rest_request *req,
                          struct c_rest_response *res, void *user_data);

/**
 * @brief Secret protected endpoint that requires a valid token.
 */
int oauth2_secret_handler(struct c_rest_request *req,
                          struct c_rest_response *res, void *user_data);

int oauth2_register_client_handler(struct c_rest_request *req,
                                   struct c_rest_response *res,
                                   void *user_data);
int oauth2_register_user_handler(struct c_rest_request *req,
                                 struct c_rest_response *res, void *user_data);

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* OAUTH2_SERVER_H */
