/* clang-format off */
#include "c_rest_platform.h"
#include "c_rest_tls.h"
/* clang-format on */

static const struct c_rest_platform_vtable default_platform_vtable = {
    c_rest_socket_create, c_rest_socket_bind,   c_rest_socket_listen,
    c_rest_socket_accept, c_rest_socket_close,  c_rest_socket_send,
    c_rest_socket_recv,   c_rest_thread_create, c_rest_thread_join,
    c_rest_tls_accept,    c_rest_tls_close};

c_rest_error_t c_rest_get_default_platform_vtable(
    const struct c_rest_platform_vtable **out_vtable) {
  if (!out_vtable)
    return C_REST_ERROR_INVALID_ARG;
  *out_vtable = &default_platform_vtable;
  return C_REST_OK;
}
