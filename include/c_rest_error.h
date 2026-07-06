/**
 * @file c_rest_error.h
 * @brief Error codes for the C REST framework.
 */
#ifndef C_REST_ERROR_H
#define C_REST_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error codes returned by most functions in the framework.
 */
typedef enum c_rest_error {
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
  C_REST_ERROR_DB = 12
} c_rest_error_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_ERROR_H */
