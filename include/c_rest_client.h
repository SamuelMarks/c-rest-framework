#ifndef C_REST_CLIENT_H
#define C_REST_CLIENT_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct c_rest_client_context c_rest_client_context;

/*
 * Expose c-abstract-http client functionality.
 */
int c_rest_client_init(c_rest_client_context **out_client);
int c_rest_client_destroy(c_rest_client_context *client);

int c_rest_client_request_sync(c_rest_client_context *client, const char *url,
                               const char *method);
int c_rest_client_request_async(c_rest_client_context *client, const char *url,
                                const char *method,
                                void (*callback)(void *data), void *user_data);

/*
 * Reverse proxy utility using the client.
 */
int c_rest_proxy_request(const char *target_url, void *req, void *res);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_CLIENT_H */
