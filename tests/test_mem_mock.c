/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include "greatest_clean.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_mem.h"
#include "c_rest_ts_queue.h"

static int g_mock_mutex_unlock_countdown = -1;
static int g_mock_mutex_lock_countdown = -1;
static int g_mock_mutex_unlock_fail = 0;
static int g_mock_mutex_lock_fail = 0;
static int g_mock_mutex_destroy_fail = 0;
static int g_mock_cond_create_fail = 0;
static int g_mock_cond_wait_fail = 0;
static int g_mock_cond_signal_fail = 0;

static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

static char *dummy_fail_strdup(const char *str) {
  (void)str;
  return NULL;
}

static char *dummy_success_strdup(const char *str) {
  size_t len = strlen(str) + 1;
  char *d = (char *)malloc(len);
#if defined(_MSC_VER)
  strcpy_s(d, len, str);
#else
  memcpy(d, str, len);
#endif
  return d;
}

extern c_rest_error_t c_rest_mutex_unlock(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_mutex_lock(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_mutex_destroy(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_cond_create(c_rest_cond_t *cond);
extern c_rest_error_t c_rest_cond_wait(c_rest_cond_t cond, c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_cond_signal(c_rest_cond_t cond);

static c_rest_error_t mock_c_rest_cond_signal(c_rest_cond_t cond) {
  if (g_mock_cond_signal_fail) {
    g_mock_cond_signal_fail = 0;
    return C_REST_ERROR_GENERIC;
  }
  return c_rest_cond_signal(cond);
}

static c_rest_error_t mock_c_rest_mutex_unlock(c_rest_mutex_t mutex) {
    if (g_mock_mutex_unlock_fail) {
        g_mock_mutex_unlock_fail = 0;
        c_rest_mutex_unlock(mutex);
        return C_REST_ERROR_GENERIC;
    }
    if (g_mock_mutex_unlock_countdown >= 0) {
        if (g_mock_mutex_unlock_countdown == 0) {
            g_mock_mutex_unlock_countdown = -1;
            c_rest_mutex_unlock(mutex);
            return C_REST_ERROR_GENERIC;
        }
        g_mock_mutex_unlock_countdown--;
    }
    return c_rest_mutex_unlock(mutex);
}

static c_rest_error_t mock_c_rest_mutex_lock(c_rest_mutex_t mutex) {
    if (g_mock_mutex_lock_fail) {
        g_mock_mutex_lock_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    if (g_mock_mutex_lock_countdown >= 0) {
        if (g_mock_mutex_lock_countdown == 0) {
            g_mock_mutex_lock_countdown = -1;
            return C_REST_ERROR_GENERIC;
        }
        g_mock_mutex_lock_countdown--;
    }
    return c_rest_mutex_lock(mutex);
}

static c_rest_error_t mock_c_rest_mutex_destroy(c_rest_mutex_t mutex) {
    if (g_mock_mutex_destroy_fail) {
        g_mock_mutex_destroy_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return c_rest_mutex_destroy(mutex);
}

static c_rest_error_t mock_c_rest_cond_create(c_rest_cond_t *cond) {
    if (g_mock_cond_create_fail) {
        g_mock_cond_create_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return c_rest_cond_create(cond);
}

static c_rest_error_t mock_c_rest_cond_wait(c_rest_cond_t cond, c_rest_mutex_t mutex) {
    if (g_mock_cond_wait_fail) {
        g_mock_cond_wait_fail = 0;
        return C_REST_ERROR_GENERIC;
    }
    return c_rest_cond_wait(cond, mutex);
}

#define c_rest_mutex_unlock mock_c_rest_mutex_unlock
#define c_rest_mutex_lock mock_c_rest_mutex_lock
#define c_rest_mutex_destroy mock_c_rest_mutex_destroy
#define c_rest_cond_create mock_c_rest_cond_create
#define c_rest_cond_wait mock_c_rest_cond_wait
#define c_rest_cond_signal mock_c_rest_cond_signal

/* Include the source files directly */
#include "../src/c_rest_mem.c"
#include "../src/c_rest_ts_queue.c"

#undef c_rest_mutex_unlock
#undef c_rest_mutex_lock
#undef c_rest_mutex_destroy
#undef c_rest_cond_create
#undef c_rest_cond_wait
#undef c_rest_cond_signal

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_mutex_unlock_fail = 0;
  g_mock_mutex_lock_fail = 0;
  g_mock_mutex_destroy_fail = 0;
  g_mock_cond_create_fail = 0;
  g_mock_cond_wait_fail = 0;
  g_mock_mutex_unlock_countdown = -1;
  g_mock_mutex_lock_countdown = -1;
}

TEST test_mem_error_branches(void) {
  void *ptr = NULL;

  c_rest_mem_tracker_init();

  /* test c_rest_mutex_unlock failure in malloc */
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_malloc(10, "file", 1, &ptr));

  /* test c_rest_mutex_lock failure in free */
  c_rest_mem_malloc(10, "file", 1, &ptr);
  g_mock_mutex_lock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_free(ptr));
  c_rest_mem_free(ptr); /* actually free it */

  /* test c_rest_mutex_unlock failure in free */
  c_rest_mem_malloc(10, "file", 1, &ptr);
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_free(ptr));

  /* test lock and unlock failure in print_leaks */
  g_mock_mutex_lock_fail = 1;
  c_rest_mem_tracker_print_leaks();
  g_mock_mutex_unlock_fail = 1;
  c_rest_mem_tracker_print_leaks();

  /* test c_rest_mutex_unlock failure in cleanup */
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_tracker_cleanup());

  /* test c_rest_mutex_destroy failure in cleanup */
  g_mock_mutex_destroy_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_tracker_cleanup());
  c_rest_mem_tracker_cleanup(); /* actually clean up */

  /* test c_rest_internal_strdup with NULL, valid string, and malloc failure */
  {
    char *dup_str = NULL;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_internal_strdup(NULL, &dup_str));
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_internal_strdup("test", NULL));
    ASSERT_EQ(C_REST_OK, c_rest_internal_strdup("test", &dup_str));
    ASSERT(dup_str != NULL);
    free(dup_str);

    dup_str = NULL;
    g_crf_malloc_hook = fail_malloc;
    ASSERT_EQ(C_REST_ERROR_OOM, c_rest_internal_strdup("test", &dup_str));
    ASSERT_EQ(NULL, dup_str);
    g_crf_malloc_hook = NULL;

    dup_str = NULL;
    g_crf_strdup_hook = dummy_fail_strdup;
    ASSERT_EQ(C_REST_ERROR_OOM,
              c_rest_mem_strdup("test", __FILE__, __LINE__, &dup_str));
    ASSERT_EQ(NULL, dup_str);
    g_crf_strdup_hook = NULL;

    dup_str = NULL;
    c_rest_mem_tracker_init();
    g_crf_strdup_hook = dummy_success_strdup;
    ASSERT_EQ(C_REST_OK,
              c_rest_mem_strdup("test", __FILE__, __LINE__, &dup_str));
    ASSERT(dup_str != NULL);
    c_rest_mem_free(dup_str);
    g_crf_strdup_hook = NULL;
  }

  /* test c_rest_mem_realloc with size=0 and mutex_unlock failure in free */
  c_rest_mem_tracker_init();
  c_rest_mem_malloc(10, "file", 1, &ptr);
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_realloc(ptr, 0, "file", 1, &ptr));
  c_rest_mem_free(ptr);

  /* test c_rest_mem_realloc with mutex_lock failure on search */
  c_rest_mem_malloc(10, "file", 1, &ptr);
  g_mock_mutex_lock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_realloc(ptr, 20, "file", 1, &ptr));

  /* test c_rest_mem_realloc with mutex_unlock failure on search */
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_realloc(ptr, 20, "file", 1, &ptr));

  /* test c_rest_mem_realloc with mutex_lock failure on update (countdown 1) */
  g_mock_mutex_lock_countdown = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_realloc(ptr, 20, "file", 1, &ptr));

  /* test c_rest_mem_realloc with mutex_unlock failure on update (countdown 1)
   */
  g_mock_mutex_unlock_countdown = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_realloc(ptr, 30, "file", 1, &ptr));

  c_rest_mem_free(ptr);
  c_rest_mem_tracker_cleanup();

  PASS();
}

TEST test_ts_queue_error_branches(void) {
  c_rest_ts_queue *q = malloc(sizeof(c_rest_ts_queue));
  void *item = NULL;
  memset(q, 0, sizeof(*q));

  /* test cond_create failure and mutex_destroy failure in init */
  g_mock_cond_create_fail = 1;
  g_mock_mutex_destroy_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_init(q));

  /* test cond_create failure in init */
  g_mock_cond_create_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_init(q));

  c_rest_ts_queue_init(q);

  /* test push to closed queue with unlock failure */
  q->is_closed = 1;
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_push(q, (void *)1));
  q->is_closed = 0;

  /* test cond_signal failure and unlock failure in push */
  g_mock_cond_signal_fail = 1;
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_push(q, (void *)1));

  /* test c_rest_mutex_unlock failure in push */
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_push(q, (void *)1));

  /* Pop all items so queue is empty for cond_wait test */
  while (q->size > 0) {
    c_rest_ts_queue_pop(q, &item);
  }

  /* test c_rest_cond_wait failure and unlock failure in pop */
  g_mock_cond_wait_fail = 1;
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_pop(q, &item));

  /* test c_rest_cond_wait failure in pop */
  g_mock_cond_wait_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_pop(q, &item));

  /* test pop on empty queue with unlock failure */
  q->is_closed = 1;
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_pop(q, &item));
  q->is_closed = 0;

  /* Test mock_c_rest_cond_wait return when fail is 0 */
  ASSERT_EQ(C_REST_ERROR_GENERIC,
            mock_c_rest_cond_wait((c_rest_cond_t)0, (c_rest_mutex_t)0));

  /* Push an item so queue is not empty when testing unlock failure in pop */
  c_rest_ts_queue_push(q, (void *)2);

  /* test c_rest_mutex_unlock failure in pop */
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_pop(q, &item));

  /* test cond_signal failure and unlock failure in close */
  g_mock_cond_signal_fail = 1;
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_close(q));

  /* test c_rest_mutex_unlock failure in close */
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_close(q));

  /* test c_rest_mutex_unlock failure in destroy */
  g_mock_mutex_unlock_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_destroy(q, NULL));

  /* test c_rest_mutex_destroy failure in destroy */
  g_mock_mutex_destroy_fail = 1;
  ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_destroy(q, NULL));

  c_rest_ts_queue_destroy(q, NULL); /* clean up */
  free(q);

  PASS();
}

SUITE_EXTERN(mem_mock_suite);
SUITE(mem_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_mem_error_branches);
  RUN_TEST(test_ts_queue_error_branches);
}
