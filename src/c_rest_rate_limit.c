/* clang-format off */
#include "c_rest_rate_limit.h"
#include "c_rest_mem.h"
#include <time.h>
/* clang-format on */

static void c_rest_rate_limiter_free_bucket(void *bucket_ptr) {
  if (bucket_ptr) {
    C_REST_FREE(bucket_ptr);
  }
}

int c_rest_rate_limiter_init(c_rest_rate_limiter *limiter, size_t capacity,
                             size_t fill_rate, size_t max_entities) {
  int ret;
  if (!limiter) {
    return 1;
  }

  limiter->config.capacity = capacity;
  limiter->config.fill_rate = fill_rate;
  limiter->initialized = 0;

  ret = c_rest_hashmap_init(&limiter->buckets, max_entities);
  if (ret != 0) {
    return ret;
  }

  ret = c_rest_mutex_create(&limiter->mutex);
  if (ret != 0) {
    c_rest_hashmap_destroy(&limiter->buckets, c_rest_rate_limiter_free_bucket);
    return ret;
  }

  limiter->initialized = 1;
  return 0;
}

int c_rest_rate_limiter_check(c_rest_rate_limiter *limiter,
                              const char *identifier, size_t tokens_needed,
                              size_t *out_remaining) {
  int ret;
  c_rest_rate_limit_bucket *bucket;
  void *val = NULL;
  time_t now;
  size_t elapsed;
  size_t tokens_to_add;

  if (!limiter || !limiter->initialized || !identifier || !out_remaining) {
    return 1;
  }

  now = time(NULL);

  ret = c_rest_mutex_lock(limiter->mutex);
  if (ret != 0) {
    return ret;
  }

  ret = c_rest_hashmap_get(&limiter->buckets, identifier, &val);
  if (ret == 0 && val != NULL) {
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
  } else {
    /* Create new bucket */
    void *new_bucket_ptr = NULL;
    ret = C_REST_MALLOC(sizeof(c_rest_rate_limit_bucket), &new_bucket_ptr);
    if (ret != 0) {
      c_rest_mutex_unlock(limiter->mutex);
      return ret;
    }
    bucket = (c_rest_rate_limit_bucket *)new_bucket_ptr;
    bucket->tokens = limiter->config.capacity;
    bucket->last_refill = now;

    ret = c_rest_hashmap_put(&limiter->buckets, identifier, bucket);
    if (ret != 0) {
      C_REST_FREE(bucket);
      c_rest_mutex_unlock(limiter->mutex);
      return ret;
    }
  }

  if (bucket->tokens >= tokens_needed) {
    bucket->tokens -= tokens_needed;
    *out_remaining = bucket->tokens;
    ret = 0;
  } else {
    *out_remaining = bucket->tokens;
    ret = 1; /* Rate limited */
  }

  c_rest_mutex_unlock(limiter->mutex);
  return ret;
}

int c_rest_rate_limiter_destroy(c_rest_rate_limiter *limiter) {
  int ret;
  if (!limiter || !limiter->initialized) {
    return 1;
  }

  ret = c_rest_mutex_lock(limiter->mutex);
  if (ret != 0) {
    return ret;
  }

  c_rest_hashmap_destroy(&limiter->buckets, c_rest_rate_limiter_free_bucket);

  ret = c_rest_mutex_unlock(limiter->mutex);
  if (ret != 0) {
    return ret;
  }

  ret = c_rest_mutex_destroy(limiter->mutex);
  if (ret != 0) {
    return ret;
  }

  limiter->initialized = 0;
  return 0;
}