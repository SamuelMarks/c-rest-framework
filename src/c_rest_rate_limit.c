/* clang-format off */
#include "c_rest_testing_mocks.h"
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_rate_limit.h"
#include <time.h>
/* clang-format on */

#ifdef C_REST_TESTING_MALLOC_HOOK
#ifndef c_rest_mutex_create
#define c_rest_mutex_create crf_seam_rl_mutex_create
#endif
#ifndef c_rest_mutex_lock
#define c_rest_mutex_lock crf_seam_rl_mutex_lock
#endif
#ifndef c_rest_hashmap_put
#define c_rest_hashmap_put crf_seam_rl_hashmap_put
#endif
#ifndef c_rest_mutex_unlock
#define c_rest_mutex_unlock crf_seam_rl_mutex_unlock
#endif
#ifndef c_rest_hashmap_destroy
#define c_rest_hashmap_destroy crf_seam_rl_hashmap_destroy
#endif
#ifndef c_rest_mutex_destroy
#define c_rest_mutex_destroy crf_seam_rl_mutex_destroy
#endif
#endif

static NO_DISCARD c_rest_error_t
c_rest_rate_limiter_bucket_free(void *bucket_ptr) {
  C_REST_FREE(bucket_ptr);
  return C_REST_OK;
}

c_rest_error_t c_rest_rate_limiter_init(c_rest_rate_limiter *limiter,
                                        size_t capacity, size_t fill_rate,
                                        size_t max_entities) {
  c_rest_error_t rc;
  c_rest_error_t ret;
  if (!limiter) {
    return C_REST_ERROR_GENERIC;
  }

  limiter->config.capacity = capacity;
  limiter->config.fill_rate = fill_rate;
  limiter->config.max_entities = max_entities;
  limiter->initialized = 0;

  ret = c_rest_hashmap_init(&limiter->buckets, max_entities);

  if (ret != C_REST_OK) {
    return ret;
  }

  ret = c_rest_mutex_create(&limiter->mutex);

  if (ret != C_REST_OK) {
    rc = c_rest_hashmap_destroy(&limiter->buckets,
                                c_rest_rate_limiter_bucket_free);
    if (rc != C_REST_OK)
      return rc;
    return ret;
  }

  limiter->initialized = 1;
  return C_REST_OK;
}

c_rest_error_t c_rest_rate_limiter_check(c_rest_rate_limiter *limiter,
                                         const char *identifier,
                                         size_t tokens_needed,
                                         size_t *out_remaining) {
  c_rest_error_t rc;
  c_rest_error_t ret;
  c_rest_rate_limit_bucket *bucket;
  void *val = NULL;
  time_t now;
  size_t elapsed;
  size_t tokens_to_add;

  if (!limiter) {
    return C_REST_ERROR_GENERIC;
  }
  if (!limiter->initialized) {
    return C_REST_ERROR_GENERIC;
  }
  if (!identifier) {
    return C_REST_ERROR_GENERIC;
  }
  if (!out_remaining) {
    return C_REST_ERROR_GENERIC;
  }

  now = time(NULL);

  rc = c_rest_mutex_lock(limiter->mutex);
  if (rc != C_REST_OK)
    return rc;

  ret = c_rest_hashmap_get(&limiter->buckets, identifier, &val);
  if (ret != C_REST_OK) {
    /* Create new bucket */
    void *new_bucket_ptr = NULL;
    c_rest_error_t put_rc;
    if (limiter->buckets.size >= limiter->config.max_entities) {
      *out_remaining = 0;
      rc = c_rest_mutex_unlock(limiter->mutex);
      if (rc != C_REST_OK)
        return rc;
      return C_REST_ERROR_GENERIC;
    }
    if (C_REST_MALLOC(sizeof(c_rest_rate_limit_bucket), &new_bucket_ptr) != 0) {
      rc = c_rest_mutex_unlock(limiter->mutex);
      if (rc != C_REST_OK)
        return rc;
      return C_REST_ERROR_OOM;
    }

    bucket = (c_rest_rate_limit_bucket *)new_bucket_ptr;
    bucket->tokens = limiter->config.capacity;
    bucket->last_refill = now;

    put_rc = c_rest_hashmap_put(&limiter->buckets, identifier, bucket);
    if (put_rc != C_REST_OK) {
      C_REST_FREE(bucket);
      rc = c_rest_mutex_unlock(limiter->mutex);
      if (rc != C_REST_OK)
        return rc;
      return put_rc;
    }
    goto check_tokens;
  } else {
    bucket = (c_rest_rate_limit_bucket *)val;
    elapsed = (size_t)difftime(now, bucket->last_refill);
    tokens_to_add = elapsed * limiter->config.fill_rate;

    if (tokens_to_add > 0) {
      bucket->tokens += tokens_to_add;
      if (bucket->tokens > limiter->config.capacity) {
        bucket->tokens = limiter->config.capacity;
      }
      bucket->last_refill = now;
    }
  }

check_tokens:
  if (bucket->tokens >= tokens_needed) {
    bucket->tokens -= tokens_needed;
    *out_remaining = bucket->tokens;
    ret = C_REST_OK;
  } else {
    *out_remaining = bucket->tokens;
    ret = C_REST_ERROR_GENERIC; /* Rate limited */
  }

  {
    rc = c_rest_mutex_unlock(limiter->mutex);
    if (rc != C_REST_OK)
      return rc;
  }
  return ret;
}

c_rest_error_t c_rest_rate_limiter_destroy(c_rest_rate_limiter *limiter) {
  c_rest_error_t rc;
  c_rest_error_t unlock_rc;
  if (!limiter) {
    return C_REST_ERROR_GENERIC;
  }
  if (!limiter->initialized) {
    return C_REST_ERROR_GENERIC;
  }

  rc = c_rest_mutex_lock(limiter->mutex);
  if (rc != C_REST_OK)
    return rc;

  rc = c_rest_hashmap_destroy(&limiter->buckets,
                              c_rest_rate_limiter_bucket_free);
  if (rc != C_REST_OK) {
    unlock_rc = c_rest_mutex_unlock(limiter->mutex);
    if (unlock_rc != C_REST_OK)
      return unlock_rc;
    return rc;
  }

  rc = c_rest_mutex_unlock(limiter->mutex);
  if (rc != C_REST_OK)
    return rc;

  rc = c_rest_mutex_destroy(limiter->mutex);
  if (rc != C_REST_OK)
    return rc;

  limiter->initialized = 0;
  return C_REST_OK;
}
