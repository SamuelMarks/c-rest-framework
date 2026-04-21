#ifndef C_REST_RATE_LIMIT_H
#define C_REST_RATE_LIMIT_H

/* clang-format off */
#include "c_rest_hashmap.h"
#include "c_rest_platform.h"
#include <stddef.h>
#include <time.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Rate limiting token bucket configuration.
 */
typedef struct c_rest_rate_limit_config {
  /** @brief Maximum tokens the bucket can hold */
  size_t capacity;
  /** @brief Tokens added per second */
  size_t fill_rate;
} c_rest_rate_limit_config;

/**
 * @brief Represents a single rate limit context for an entity (e.g. IP).
 */
typedef struct c_rest_rate_limit_bucket {
  /** @brief Current number of tokens */
  size_t tokens;
  /** @brief Last time the bucket was refilled */
  time_t last_refill;
} c_rest_rate_limit_bucket;

/**
 * @brief The global rate limiter middleware state.
 */
typedef struct c_rest_rate_limiter {
  /** @brief Map of identifiers (e.g., IP addresses) to their token buckets */
  c_rest_hashmap buckets;
  /** @brief Configuration for token bucket behavior */
  c_rest_rate_limit_config config;
  /** @brief Mutex to ensure thread-safe updates to the buckets map */
  c_rest_mutex_t mutex;
  /** @brief Whether the limiter is fully initialized */
  int initialized;
} c_rest_rate_limiter;

/**
 * @brief Initialize the global rate limiter.
 * @param limiter Pointer to the limiter to initialize.
 * @param capacity Maximum number of tokens per entity.
 * @param fill_rate Tokens replenished per second.
 * @param max_entities The expected maximum number of distinct entities to
 * track.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_rate_limiter_init(c_rest_rate_limiter *limiter, size_t capacity,
                             size_t fill_rate, size_t max_entities);

/**
 * @brief Check if a request from the given identifier should be allowed.
 * @param limiter Pointer to the rate limiter.
 * @param identifier The entity string (e.g., IP address or API key).
 * @param tokens_needed The number of tokens required for this request.
 * @param out_remaining Outputs the number of remaining tokens if allowed.
 * @return 0 if the request is allowed (enough tokens), non-zero if rate limited
 * or on error.
 */
int c_rest_rate_limiter_check(c_rest_rate_limiter *limiter,
                              const char *identifier, size_t tokens_needed,
                              size_t *out_remaining);

/**
 * @brief Free resources associated with the rate limiter.
 * @param limiter Pointer to the rate limiter to destroy.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_rate_limiter_destroy(c_rest_rate_limiter *limiter);

/* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_RATE_LIMIT_H */
