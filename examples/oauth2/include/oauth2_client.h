#ifndef OAUTH2_CLIENT_H
#define OAUTH2_CLIENT_H

/* clang-format off */
#include "c_orm_db.h"
#include "c_rest_client.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize an OAuth2 client.
 *
 * @param token_url The OAuth2 token endpoint URL.
 * @param client_id The OAuth2 client ID.
 * @param client_secret The OAuth2 client secret.
 * @param db c-orm database connection to store credentials locally (optional).
 * @return 0 on success, non-zero on failure.
 */
int oauth2_client_init(const char *token_url, const char *client_id,
                       const char *client_secret, c_orm_db_t *db);

/**
 * @brief Perform a password grant flow to retrieve a token.
 *
 * @param username The user's username.
 * @param password The user's password.
 * @param out_access_token Pointer to store the newly allocated access token
 * string.
 * @param out_expires_in Pointer to store the expiration time in seconds.
 * @return 0 on success, non-zero on failure.
 */
int oauth2_client_password_grant(const char *username, const char *password,
                                 char **out_access_token, int *out_expires_in);

/**
 * @brief Clean up the OAuth2 client.
 * @return 0 on success.
 */
int oauth2_client_cleanup(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* OAUTH2_CLIENT_H */
