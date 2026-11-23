/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_log.h"
#include <cfs/cfs.h>
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
#include "c_orm_meta.h"
#endif
#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#include <c_abstract_http/http_types.h>
#else
#include "c_abstract_http.h"
#endif
#include <stdio.h>
#include <stdarg.h>
/* clang-format on */

c_rest_error_t c_rest_framework_log_debug(const char *fmt, ...) {
#ifdef DEBUG
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
#else
  (void)fmt;
#endif
  return C_REST_OK;
}

c_rest_error_t c_rest_error_from_cfs(int cfs_err) {
  switch ((cfs_errc)cfs_err) {
  case cfs_errc_success:
    return C_REST_OK;
  case cfs_errc_invalid_argument:
  case cfs_errc_argument_out_of_domain:
  case cfs_errc_argument_list_too_long:
  case cfs_errc_filename_too_long:
  case cfs_errc_illegal_byte_sequence:
    return C_REST_ERROR_INVALID_ARG;
  case cfs_errc_not_enough_memory:
  case cfs_errc_no_buffer_space:
    return C_REST_ERROR_OOM;
  case cfs_errc_no_such_file_or_directory:
  case cfs_errc_no_such_device:
  case cfs_errc_no_such_device_or_address:
    return C_REST_ERROR_NOT_FOUND;
  case cfs_errc_permission_denied:
  case cfs_errc_operation_not_permitted:
    return C_REST_ERROR_UNAUTHORIZED;
  case cfs_errc_not_supported:
  case cfs_errc_function_not_supported:
  case cfs_errc_operation_not_supported:
  case cfs_errc_protocol_not_supported:
    return C_REST_ERROR_NOT_SUPPORTED;
  case cfs_errc_timed_out:
  case cfs_errc_stream_timeout:
    return C_REST_ERROR_TIMEOUT;
  case cfs_errc_network_down:
  case cfs_errc_network_reset:
  case cfs_errc_network_unreachable:
  case cfs_errc_connection_aborted:
  case cfs_errc_connection_refused:
  case cfs_errc_connection_reset:
  case cfs_errc_host_unreachable:
  case cfs_errc_not_connected:
    return C_REST_ERROR_NETWORK;
  case cfs_errc_bad_message:
  case cfs_errc_protocol_error:
    return C_REST_ERROR_PARSE;
  case cfs_errc_io_error:
  case cfs_errc_bad_file_descriptor:
  case cfs_errc_is_a_directory:
  case cfs_errc_not_a_directory:
  case cfs_errc_file_exists:
  case cfs_errc_file_too_large:
  case cfs_errc_no_space_on_device:
  case cfs_errc_read_only_file_system:
  case cfs_errc_broken_pipe:
    return C_REST_ERROR_IO;
  default:
    return C_REST_ERROR_GENERIC;
  }
}

c_rest_error_t c_rest_error_from_orm(int orm_err) {
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
  switch ((c_orm_error_t)orm_err) {
  case C_ORM_OK:
    return C_REST_OK;
  case C_ORM_ERROR_MEMORY:
    return C_REST_ERROR_OOM;
  case C_ORM_ERROR_VALIDATION:
  case C_ORM_ERROR_TYPE_MISMATCH:
  case C_ORM_ERROR_BIND:
    return C_REST_ERROR_INVALID_ARG;
  case C_ORM_ERROR_NOT_FOUND:
    return C_REST_ERROR_NOT_FOUND;
  case C_ORM_ERROR_NOT_IMPLEMENTED:
    return C_REST_ERROR_NOT_SUPPORTED;
  case C_ORM_ERROR_EXPIRED:
    return C_REST_ERROR_UNAUTHORIZED;
  case C_ORM_ERROR_CONNECTION:
  case C_ORM_ERROR_SQL:
  case C_ORM_ERROR_STEP:
  case C_ORM_ERROR_RECURSION:
  case C_ORM_ERROR_READ_ONLY:
    return C_REST_ERROR_DB;
  case C_ORM_ERROR_UNKNOWN:
  default:
    return C_REST_ERROR_GENERIC;
  }
#else
  return (orm_err == 0) ? C_REST_OK : C_REST_ERROR_DB;
#endif
}

c_rest_error_t c_rest_error_from_cah(int cah_err) {
  switch ((enum c_abstract_http_error)cah_err) {
  case C_ABSTRACT_HTTP_SUCCESS:
    return C_REST_OK;
  case C_ABSTRACT_HTTP_ERR_INVAL:
    return C_REST_ERROR_INVALID_ARG;
  case C_ABSTRACT_HTTP_ERR_NOMEM:
    return C_REST_ERROR_OOM;
  case C_ABSTRACT_HTTP_ERR_IO:
  case C_ABSTRACT_HTTP_ERR_SYSCALL:
    return C_REST_ERROR_NETWORK;
  case C_ABSTRACT_HTTP_ERR_NOTSUP:
    return C_REST_ERROR_NOT_SUPPORTED;
  case C_ABSTRACT_HTTP_ERR_TIMEOUT:
    return C_REST_ERROR_TIMEOUT;
  default:
    return C_REST_ERROR_GENERIC;
  }
}
