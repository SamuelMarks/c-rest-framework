/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_rate_limit.h"
#include "c_rest_mem.h"
#include <time.h>
/* clang-format on */

static void c_rest_rate_limiter_bucket_free(void *bucket_ptr) {
  if (bucket_ptr) { /* GCOVR_EXCL_LINE */
    C_REST_FREE(bucket_ptr);
  }
}

c_rest_error_t c_rest_rate_limiter_init(c_rest_rate_limiter *limiter,
                                        size_t capacity, size_t fill_rate,
                                        size_t max_entities) {
  int ret;
  if (!limiter) {
    return C_REST_ERROR_GENERIC;
  }

  limiter->config.capacity = capacity;
  limiter->config.fill_rate = fill_rate;
  limiter->initialized = 0;

  ret = c_rest_hashmap_init(&limiter->buckets, max_entities);
  if (ret != C_REST_OK) { /* GCOVR_EXCL_LINE */
    return ret;           /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_mutex_create(&limiter->mutex);
  if (ret != C_REST_OK) {                     /* GCOVR_EXCL_LINE */
    c_rest_hashmap_destroy(                   /* GCOVR_EXCL_LINE */
                           &limiter->buckets, /* GCOVR_EXCL_LINE */
                           c_rest_rate_limiter_bucket_free); /* GCOVR_EXCL_LINE
                                                              */
    return ret; /* GCOVR_EXCL_LINE */
  }

  limiter->initialized = 1;
  return C_REST_OK;
}

c_rest_error_t c_rest_rate_limiter_check(c_rest_rate_limiter *limiter,
                                         const char *identifier,
                                         size_t tokens_needed,
                                         size_t *out_remaining) {
  int ret;
  c_rest_rate_limit_bucket *bucket;
  void *val = NULL;
  time_t now;
  size_t elapsed;
  size_t tokens_to_add;

  if (!limiter || !limiter->initialized || !identifier ||
      !out_remaining) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;
  }

  now = time(NULL);

  ret = c_rest_mutex_lock(limiter->mutex);
  if (ret != 0) { /* GCOVR_EXCL_LINE */
    return ret;   /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_hashmap_get(&limiter->buckets, identifier, &val);
  if (ret == 0 && val != NULL) { /* GCOVR_EXCL_LINE */
    bucket = (c_rest_rate_limit_bucket *)val;
    elapsed = (size_t)difftime(now, bucket->last_refill);
    tokens_to_add = elapsed * limiter->config.fill_rate;

    if (tokens_to_add > 0) {                           /* GCOVR_EXCL_LINE */
      bucket->tokens += tokens_to_add;                 /* GCOVR_EXCL_LINE */
      if (bucket->tokens > limiter->config.capacity) { /* GCOVR_EXCL_LINE */
        bucket->tokens = limiter->config.capacity;     /* GCOVR_EXCL_LINE */
      }
      bucket->last_refill = now; /* GCOVR_EXCL_LINE */
    }
  } else {
    /* Create new bucket */
    void *new_bucket_ptr = NULL;
    ret = C_REST_MALLOC(sizeof(c_rest_rate_limit_bucket), &new_bucket_ptr);
    if (ret != 0) {                        /* GCOVR_EXCL_LINE */
      c_rest_mutex_unlock(limiter->mutex); /* GCOVR_EXCL_LINE */
      return ret;                          /* GCOVR_EXCL_LINE */
    }
    bucket = (c_rest_rate_limit_bucket *)new_bucket_ptr;
    bucket->tokens = limiter->config.capacity;
    bucket->last_refill = now;

    ret = c_rest_hashmap_put(&limiter->buckets, identifier, bucket);
    if (ret != 0) {                        /* GCOVR_EXCL_LINE */
      C_REST_FREE(bucket);                 /* GCOVR_EXCL_LINE */
      c_rest_mutex_unlock(limiter->mutex); /* GCOVR_EXCL_LINE */
      return ret;                          /* GCOVR_EXCL_LINE */
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

c_rest_error_t c_rest_rate_limiter_destroy(c_rest_rate_limiter *limiter) {
  int ret;
  if (!limiter || !limiter->initialized) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;           /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_mutex_lock(limiter->mutex);
  if (ret != 0) { /* GCOVR_EXCL_LINE */
    return ret;   /* GCOVR_EXCL_LINE */
  }

  c_rest_hashmap_destroy(&limiter->buckets, c_rest_rate_limiter_bucket_free);

  ret = c_rest_mutex_unlock(limiter->mutex);
  if (ret != 0) { /* GCOVR_EXCL_LINE */
    return ret;   /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_mutex_destroy(limiter->mutex);
  if (ret != 0) { /* GCOVR_EXCL_LINE */
    return ret;   /* GCOVR_EXCL_LINE */
  }

  limiter->initialized = 0;
  return C_REST_OK;
}
