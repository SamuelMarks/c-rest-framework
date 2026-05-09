#ifndef C_REST_LOG_H
#define C_REST_LOG_H

/* clang-format off */
#include <stdio.h>
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
void c_rest_framework_log_debug(const char *fmt, ...);
#define LOG_DEBUG c_rest_framework_log_debug
#else
/**
 * @brief Logs a debug message with formatting.
 * @param fmt The format string.
 * @param ... The arguments for the format string.
 */
void c_rest_framework_log_debug(const char *fmt, ...);
#define LOG_DEBUG 1 ? (void)0 : c_rest_framework_log_debug
#endif /* DEBUG */
#endif /* !LOG_DEBUG */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_LOG_H */
