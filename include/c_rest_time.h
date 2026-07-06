/**
 * @file c_rest_time.h
 * @brief Header file for c_rest_time.h
 */
#ifndef C_REST_TIME_H
#define C_REST_TIME_H
/* clang-format off */
#include "c_rest_error.h"

#include <stddef.h>
#include "c_rest_error.h"
#include <time.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Format a time_t value into an HTTP Date string.
 * @param t The time to format.
 * @param out_str Output buffer, must be at least 30 bytes (e.g., "Wed, 21 Oct
 * 2015 07:28:00 GMT").
 * @param out_len Length of the output buffer.
 * @return 0 on success, non-zero on failure.
 */
c_rest_error_t c_rest_http_date_format(time_t t, char *out_str, size_t out_len);

/**
 * @brief Parse an HTTP Date string into a time_t value.
 * @param date_str The date string to parse.
 * @param out_t The parsed time.
 * @return 0 on success, non-zero on failure.
 */
c_rest_error_t c_rest_http_date_parse(const char *date_str, time_t *out_t);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_TIME_H */
