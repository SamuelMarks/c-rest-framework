/**
 * @file c_rest_log.h
 * @brief Header file for c_rest_log.h
 */
/* clang-format off */
#include "c_rest_error.h"
#ifndef C_REST_LOG_H
#define C_REST_LOG_H

#include <stdio.h>
#include "c_rest_export.h"
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOG_DEBUG
#ifdef DEBUG
/**
 * @brief Logs a debug message with formatting.
 * @param fmt The format string.
 * @param ... The arguments for the format string.
 */
C_REST_EXPORT extern c_rest_error_t c_rest_framework_log_debug(const char *fmt,
                                                               ...);
/**
 * @brief Macro for debug logging.
 */
#define LOG_DEBUG (void)!c_rest_framework_log_debug
#else
/**
 * @brief Logs a debug message with formatting.
 * @param fmt The format string.
 * @param ... The arguments for the format string.
 */
C_REST_EXPORT extern c_rest_error_t c_rest_framework_log_debug(const char *fmt,
                                                               ...);
/**
 * @brief Macro for debug logging (disabled but evaluates args to avoid branch
 * coverage issues).
 */
#define LOG_DEBUG (void)!c_rest_framework_log_debug
#endif /* DEBUG */
#endif /* !LOG_DEBUG */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_LOG_H */
