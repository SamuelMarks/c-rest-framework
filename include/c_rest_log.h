/**
 * @file c_rest_log.h
 * @brief Header file for c_rest_log.h
 */
#ifndef C_REST_LOG_H
#define C_REST_LOG_H

/* clang-format off */
#include <stdio.h>
#include "c_rest_error.h"
#include "c_rest_export.h"
#include "c_rest_no_discard.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Logs a debug message with formatting.
 * @param fmt The format string.
 * @param ... The arguments for the format string.
 * @return C_REST_OK on success.
 */
NO_DISCARD C_REST_EXPORT extern c_rest_error_t
c_rest_framework_log_debug(const char *fmt, ...);

#ifndef LOG_DEBUG
/**
 * @brief Macro for debug logging.
 */
#define LOG_DEBUG c_rest_framework_log_debug
#endif /* !LOG_DEBUG */

/**
 * @brief Logs a debug message and returns its integer status code.
 * @param msg Debug message string literal.
 */
#define C_REST_LOG_MSG(msg) ((int)c_rest_framework_log_debug(msg))

/**
 * @brief Logs a debug message and combines its return status with an error
 * code.
 * @param err Primary c_rest_error_t code to return.
 * @param msg Debug message string literal.
 */
#define C_REST_LOG_ERR(err, msg)                                               \
  ((c_rest_error_t)((int)(err) | C_REST_LOG_MSG(msg)))

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_LOG_H */
