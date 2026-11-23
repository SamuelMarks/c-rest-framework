/**
 * @file c_rest_no_discard.h
 * @brief Cross-compiler nodiscard / warn_unused_result attribute macro.
 */
#ifndef C_REST_NO_DISCARD_H
#define C_REST_NO_DISCARD_H

/* clang-format off */
#if defined(_MSC_VER) && _MSC_VER >= 1700
#include <sal.h>
#endif
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NO_DISCARD
#if defined(__cplusplus) && __cplusplus >= 201703L
/** @brief Attribute enforcing that function return values are not discarded. */
#define NO_DISCARD [[nodiscard]]
#elif defined(__GNUC__) || defined(__clang__)
/** @brief Attribute enforcing that function return values are not discarded. */
#define NO_DISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER) && _MSC_VER >= 1700
/** @brief Attribute enforcing that function return values are not discarded. */
#define NO_DISCARD _Check_return_
#else
/** @brief Attribute enforcing that function return values are not discarded. */
#define NO_DISCARD
#endif
#endif /* NO_DISCARD */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_NO_DISCARD_H */
