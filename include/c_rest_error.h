/**
 * @file c_rest_error.h
 * @brief Error codes for the C REST framework.
 */
#ifndef C_REST_ERROR_H
#define C_REST_ERROR_H

/* clang-format off */
#include "c_rest_export.h"
#include "c_rest_no_discard.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error codes returned by most functions in the framework.
 */
typedef enum c_rest_error {
  /** @brief Internal sentinel to ensure signed enum underlying type. */
  C_REST_ERROR_INTERNAL = -1,
  /** @brief Success, no error. */
  C_REST_OK = 0,
  /** @brief Generic or unknown error. */
  C_REST_ERROR_GENERIC = 1,
  /** @brief Out of memory error. */
  C_REST_ERROR_OOM = 2,
  /** @brief Invalid argument passed to a function. */
  C_REST_ERROR_INVALID_ARG = 3,
  /** @brief Resource not found. */
  C_REST_ERROR_NOT_FOUND = 4,
  /** @brief I/O error. */
  C_REST_ERROR_IO = 5,
  /** @brief Network communication error. */
  C_REST_ERROR_NETWORK = 6,
  /** @brief Operation timed out. */
  C_REST_ERROR_TIMEOUT = 7,
  /** @brief Unauthorized access. */
  C_REST_ERROR_UNAUTHORIZED = 8,
  /** @brief Operation not supported. */
  C_REST_ERROR_NOT_SUPPORTED = 9,
  /** @brief Parsing error. */
  C_REST_ERROR_PARSE = 10,
  /** @brief TLS/SSL error. */
  C_REST_ERROR_TLS = 11,
  /** @brief Database error. */
  C_REST_ERROR_DB = 12,
  /** @brief Intentional middleware/handler short-circuit (response ready). */
  C_REST_SHORT_CIRCUIT = 13
} c_rest_error_t;

/**
 * @brief Translate a c-fs error code (cfs_errc) to c_rest_error_t.
 * @param cfs_err The cfs_errc value cast to int.
 * @return Corresponding c_rest_error_t code.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
c_rest_error_from_cfs(int cfs_err);

/**
 * @brief Translate a c-orm error code (c_orm_error_t) to c_rest_error_t.
 * @param orm_err The c_orm_error_t value cast to int.
 * @return Corresponding c_rest_error_t code.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
c_rest_error_from_orm(int orm_err);

/**
 * @brief Translate a c-abstract-http error code to c_rest_error_t.
 * @param cah_err The c_abstract_http_error value cast to int.
 * @return Corresponding c_rest_error_t code.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
c_rest_error_from_cah(int cah_err);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_ERROR_H */
