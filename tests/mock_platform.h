#ifndef MOCK_PLATFORM_H
#define MOCK_PLATFORM_H

/* clang-format off */
#include "c_rest_modality.h"
#include "c_rest_platform.h"
#include "c_rest_testing_mocks.h"
#include "c_rest_tls.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

extern const struct c_rest_platform_vtable mock_platform_vtable;

c_rest_error_t mock_c_rest_socket_create(c_rest_socket_t *sock);
c_rest_error_t mock_c_rest_socket_bind(c_rest_socket_t sock, const char *host,
                                       unsigned short port);
c_rest_error_t mock_c_rest_socket_listen(c_rest_socket_t sock, int backlog);
c_rest_error_t mock_c_rest_tls_accept(struct c_rest_tls_context *ctx,
                                      c_rest_socket_t sock,
                                      struct c_rest_tls_connection **out_conn);
c_rest_error_t mock_c_rest_tls_close(struct c_rest_tls_connection *conn);

c_rest_error_t mock_c_rest_process_create(c_rest_process_t *out_proc,
                                          const char *executable,
                                          char *const argv[]);
c_rest_error_t mock_c_rest_process_wait(c_rest_process_t proc,
                                        int *out_exit_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MOCK_PLATFORM_H */
