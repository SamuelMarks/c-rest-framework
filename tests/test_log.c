/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_log.h"
#include "test_protos.h"
#include <cfs/cfs.h>
#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
#include "c_orm_meta.h"
#endif
#ifdef C_REST_FRAMEWORK_USE_REAL_CAH
#include <c_abstract_http/http_types.h>
#else
#include "c_abstract_http.h"
#endif
/* clang-format on */

TEST test_log_debug_calls(void) {
  ASSERT_EQ(C_REST_OK, LOG_DEBUG("This is a test log: %d", 42));
  ASSERT_EQ(C_REST_OK,
            c_rest_framework_log_debug(
                "Direct call to c_rest_framework_log_debug: %d", 43));
  PASS();
}

TEST test_error_mapping_fidelity(void) {
  /* cfs_errc mappings */
  ASSERT_EQ(C_REST_OK, c_rest_error_from_cfs((int)cfs_errc_success));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_error_from_cfs((int)cfs_errc_invalid_argument));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_error_from_cfs((int)cfs_errc_argument_out_of_domain));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_error_from_cfs((int)cfs_errc_argument_list_too_long));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_error_from_cfs((int)cfs_errc_filename_too_long));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_error_from_cfs((int)cfs_errc_illegal_byte_sequence));

  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_error_from_cfs((int)cfs_errc_not_enough_memory));
  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_error_from_cfs((int)cfs_errc_no_buffer_space));

  ASSERT_EQ(C_REST_ERROR_NOT_FOUND,
            c_rest_error_from_cfs((int)cfs_errc_no_such_file_or_directory));
  ASSERT_EQ(C_REST_ERROR_NOT_FOUND,
            c_rest_error_from_cfs((int)cfs_errc_no_such_device));
  ASSERT_EQ(C_REST_ERROR_NOT_FOUND,
            c_rest_error_from_cfs((int)cfs_errc_no_such_device_or_address));

  ASSERT_EQ(C_REST_ERROR_UNAUTHORIZED,
            c_rest_error_from_cfs((int)cfs_errc_permission_denied));
  ASSERT_EQ(C_REST_ERROR_UNAUTHORIZED,
            c_rest_error_from_cfs((int)cfs_errc_operation_not_permitted));

  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            c_rest_error_from_cfs((int)cfs_errc_not_supported));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            c_rest_error_from_cfs((int)cfs_errc_function_not_supported));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            c_rest_error_from_cfs((int)cfs_errc_operation_not_supported));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            c_rest_error_from_cfs((int)cfs_errc_protocol_not_supported));

  ASSERT_EQ(C_REST_ERROR_TIMEOUT,
            c_rest_error_from_cfs((int)cfs_errc_timed_out));
  ASSERT_EQ(C_REST_ERROR_TIMEOUT,
            c_rest_error_from_cfs((int)cfs_errc_stream_timeout));

  ASSERT_EQ(C_REST_ERROR_NETWORK,
            c_rest_error_from_cfs((int)cfs_errc_network_down));
  ASSERT_EQ(C_REST_ERROR_NETWORK,
            c_rest_error_from_cfs((int)cfs_errc_network_reset));
  ASSERT_EQ(C_REST_ERROR_NETWORK,
            c_rest_error_from_cfs((int)cfs_errc_network_unreachable));
  ASSERT_EQ(C_REST_ERROR_NETWORK,
            c_rest_error_from_cfs((int)cfs_errc_connection_aborted));
  ASSERT_EQ(C_REST_ERROR_NETWORK,
            c_rest_error_from_cfs((int)cfs_errc_connection_refused));
  ASSERT_EQ(C_REST_ERROR_NETWORK,
            c_rest_error_from_cfs((int)cfs_errc_connection_reset));
  ASSERT_EQ(C_REST_ERROR_NETWORK,
            c_rest_error_from_cfs((int)cfs_errc_host_unreachable));
  ASSERT_EQ(C_REST_ERROR_NETWORK,
            c_rest_error_from_cfs((int)cfs_errc_not_connected));

  ASSERT_EQ(C_REST_ERROR_PARSE,
            c_rest_error_from_cfs((int)cfs_errc_bad_message));
  ASSERT_EQ(C_REST_ERROR_PARSE,
            c_rest_error_from_cfs((int)cfs_errc_protocol_error));

  ASSERT_EQ(C_REST_ERROR_IO, c_rest_error_from_cfs((int)cfs_errc_io_error));
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_error_from_cfs((int)cfs_errc_bad_file_descriptor));
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_error_from_cfs((int)cfs_errc_is_a_directory));
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_error_from_cfs((int)cfs_errc_not_a_directory));
  ASSERT_EQ(C_REST_ERROR_IO, c_rest_error_from_cfs((int)cfs_errc_file_exists));
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_error_from_cfs((int)cfs_errc_file_too_large));
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_error_from_cfs((int)cfs_errc_no_space_on_device));
  ASSERT_EQ(C_REST_ERROR_IO,
            c_rest_error_from_cfs((int)cfs_errc_read_only_file_system));
  ASSERT_EQ(C_REST_ERROR_IO, c_rest_error_from_cfs((int)cfs_errc_broken_pipe));

  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_error_from_cfs((int)cfs_errc_unknown_error));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_error_from_cfs(99999));

#ifdef C_REST_FRAMEWORK_USE_REAL_C_ORM
  ASSERT_EQ(C_REST_OK, c_rest_error_from_orm((int)C_ORM_OK));
  ASSERT_EQ(C_REST_ERROR_OOM, c_rest_error_from_orm((int)C_ORM_ERROR_MEMORY));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_error_from_orm((int)C_ORM_ERROR_VALIDATION));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_error_from_orm((int)C_ORM_ERROR_TYPE_MISMATCH));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_error_from_orm((int)C_ORM_ERROR_BIND));
  ASSERT_EQ(C_REST_ERROR_NOT_FOUND,
            c_rest_error_from_orm((int)C_ORM_ERROR_NOT_FOUND));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            c_rest_error_from_orm((int)C_ORM_ERROR_NOT_IMPLEMENTED));
  ASSERT_EQ(C_REST_ERROR_UNAUTHORIZED,
            c_rest_error_from_orm((int)C_ORM_ERROR_EXPIRED));
  ASSERT_EQ(C_REST_ERROR_DB,
            c_rest_error_from_orm((int)C_ORM_ERROR_CONNECTION));
  ASSERT_EQ(C_REST_ERROR_DB, c_rest_error_from_orm((int)C_ORM_ERROR_SQL));
  ASSERT_EQ(C_REST_ERROR_DB, c_rest_error_from_orm((int)C_ORM_ERROR_STEP));
  ASSERT_EQ(C_REST_ERROR_DB, c_rest_error_from_orm((int)C_ORM_ERROR_RECURSION));
  ASSERT_EQ(C_REST_ERROR_DB, c_rest_error_from_orm((int)C_ORM_ERROR_READ_ONLY));
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            c_rest_error_from_orm((int)C_ORM_ERROR_UNKNOWN));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_error_from_orm(99999));
#else
  ASSERT_EQ(C_REST_OK, c_rest_error_from_orm(0));
  ASSERT_EQ(C_REST_ERROR_DB, c_rest_error_from_orm(1));
#endif

  /* c_abstract_http_error mappings */
  ASSERT_EQ(C_REST_OK, c_rest_error_from_cah((int)C_ABSTRACT_HTTP_SUCCESS));
  ASSERT_EQ(C_REST_ERROR_INVALID_ARG,
            c_rest_error_from_cah((int)C_ABSTRACT_HTTP_ERR_INVAL));
  ASSERT_EQ(C_REST_ERROR_OOM,
            c_rest_error_from_cah((int)C_ABSTRACT_HTTP_ERR_NOMEM));
  ASSERT_EQ(C_REST_ERROR_NETWORK,
            c_rest_error_from_cah((int)C_ABSTRACT_HTTP_ERR_IO));
  ASSERT_EQ(C_REST_ERROR_NETWORK,
            c_rest_error_from_cah((int)C_ABSTRACT_HTTP_ERR_SYSCALL));
  ASSERT_EQ(C_REST_ERROR_NOT_SUPPORTED,
            c_rest_error_from_cah((int)C_ABSTRACT_HTTP_ERR_NOTSUP));
  ASSERT_EQ(C_REST_ERROR_TIMEOUT,
            c_rest_error_from_cah((int)C_ABSTRACT_HTTP_ERR_TIMEOUT));
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_error_from_cah(-9999));

  PASS();
}

SUITE(log_suite) {
  RUN_TEST(test_log_debug_calls);
  RUN_TEST(test_error_mapping_fidelity);
}
