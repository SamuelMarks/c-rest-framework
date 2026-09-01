/* clang-format off */
#include "c_rest_error.h"
#include "greatest.h"
#include <string.h>

#undef C_REST_EXPORT
#define C_REST_EXPORT

#include "c_rest_mem.h"
#include "c_rest_ts_queue.h"

static int g_mock_mutex_unlock_countdown = -1;
static int g_mock_mutex_lock_countdown = -1;
static int g_mock_mutex_destroy_countdown = -1;
static int g_mock_cond_create_countdown = -1;
static int g_mock_cond_wait_countdown = -1;

extern c_rest_error_t c_rest_mutex_unlock(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_mutex_lock(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_mutex_destroy(c_rest_mutex_t mutex);
extern c_rest_error_t c_rest_cond_create(c_rest_cond_t *cond);
extern c_rest_error_t c_rest_cond_wait(c_rest_cond_t cond, c_rest_mutex_t mutex);

static c_rest_error_t mock_c_rest_mutex_unlock(c_rest_mutex_t mutex) {
    if (g_mock_mutex_unlock_countdown >= 0) {
        if (g_mock_mutex_unlock_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_mutex_unlock_countdown--;
    }
    return c_rest_mutex_unlock(mutex);
}

static c_rest_error_t mock_c_rest_mutex_lock(c_rest_mutex_t mutex) {
    if (g_mock_mutex_lock_countdown >= 0) {
        if (g_mock_mutex_lock_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_mutex_lock_countdown--;
    }
    return c_rest_mutex_lock(mutex);
}

static c_rest_error_t mock_c_rest_mutex_destroy(c_rest_mutex_t mutex) {
    if (g_mock_mutex_destroy_countdown >= 0) {
        if (g_mock_mutex_destroy_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_mutex_destroy_countdown--;
    }
    return c_rest_mutex_destroy(mutex);
}

static c_rest_error_t mock_c_rest_cond_create(c_rest_cond_t *cond) {
    if (g_mock_cond_create_countdown >= 0) {
        if (g_mock_cond_create_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_cond_create_countdown--;
    }
    return c_rest_cond_create(cond);
}

static c_rest_error_t mock_c_rest_cond_wait(c_rest_cond_t cond, c_rest_mutex_t mutex) {
    if (g_mock_cond_wait_countdown >= 0) {
        if (g_mock_cond_wait_countdown == 0) return C_REST_ERROR_GENERIC;
        g_mock_cond_wait_countdown--;
    }
    return c_rest_cond_wait(cond, mutex);
}

#define c_rest_mutex_unlock mock_c_rest_mutex_unlock
#define c_rest_mutex_lock mock_c_rest_mutex_lock
#define c_rest_mutex_destroy mock_c_rest_mutex_destroy
#define c_rest_cond_create mock_c_rest_cond_create
#define c_rest_cond_wait mock_c_rest_cond_wait

#define c_rest_mem_tracker_init test_c_rest_mem_tracker_init
#define c_rest_mem_malloc test_c_rest_mem_malloc
#define c_rest_mem_free test_c_rest_mem_free
#define c_rest_mem_tracker_print_leaks test_c_rest_mem_tracker_print_leaks
#define c_rest_mem_tracker_cleanup test_c_rest_mem_tracker_cleanup

#define c_rest_ts_queue_init test_c_rest_ts_queue_init
#define c_rest_ts_queue_push test_c_rest_ts_queue_push
#define c_rest_ts_queue_pop test_c_rest_ts_queue_pop
#define c_rest_ts_queue_close test_c_rest_ts_queue_close
#define c_rest_ts_queue_destroy test_c_rest_ts_queue_destroy

/* Include the source files directly */
#include "../src/c_rest_mem.c"
#include "../src/c_rest_ts_queue.c"

#undef c_rest_mutex_unlock
#undef c_rest_mutex_lock
#undef c_rest_mutex_destroy
#undef c_rest_cond_create
#undef c_rest_cond_wait

/* clang-format on */

static void reset_mocks(void *data) {
  (void)data;
  g_mock_mutex_unlock_countdown = -1;
  g_mock_mutex_lock_countdown = -1;
  g_mock_mutex_destroy_countdown = -1;
  g_mock_cond_create_countdown = -1;
  g_mock_cond_wait_countdown = -1;
}

TEST test_mem_error_branches(void) {
  void *ptr = NULL;

  test_c_rest_mem_tracker_init();

  /* test c_rest_mutex_unlock failure in malloc */
  g_mock_mutex_unlock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_mem_malloc(10, "file", 1, &ptr));

  /* test c_rest_mutex_lock failure in free */
  test_c_rest_mem_malloc(10, "file", 1, &ptr);
  g_mock_mutex_lock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_mem_free(ptr));
  test_c_rest_mem_free(ptr); /* actually free it */

  /* test c_rest_mutex_unlock failure in free */
  test_c_rest_mem_malloc(10, "file", 1, &ptr);
  g_mock_mutex_unlock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_mem_free(ptr));

  /* test lock and unlock failure in print_leaks */
  g_mock_mutex_lock_countdown = 0;
  test_c_rest_mem_tracker_print_leaks();
  g_mock_mutex_unlock_countdown = 0;
  test_c_rest_mem_tracker_print_leaks();

  /* test c_rest_mutex_unlock failure in cleanup */
  g_mock_mutex_unlock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_mem_tracker_cleanup());

  /* test c_rest_mutex_destroy failure in cleanup */
  g_mock_mutex_destroy_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_mem_tracker_cleanup());
  test_c_rest_mem_tracker_cleanup(); /* actually clean up */

  PASS();
}

TEST test_ts_queue_error_branches(void) {
  c_rest_ts_queue *q = malloc(sizeof(c_rest_ts_queue));
  void *item = NULL;
  memset(q, 0, sizeof(*q));

  /* test cond_create failure in init */
  g_mock_cond_create_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_ts_queue_init(q));

  test_c_rest_ts_queue_init(q);

  /* test c_rest_mutex_unlock failure in push */
  g_mock_mutex_unlock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_ts_queue_push(q, (void *)1));

  /* test c_rest_cond_wait failure in pop */
  g_mock_cond_wait_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_ts_queue_pop(q, &item));

  /* test c_rest_mutex_unlock failure in pop */
  g_mock_mutex_unlock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_ts_queue_pop(q, &item));

  /* test c_rest_mutex_unlock failure in close */
  g_mock_mutex_unlock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_ts_queue_close(q));

  /* test c_rest_mutex_unlock failure in destroy */
  g_mock_mutex_unlock_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_ts_queue_destroy(q, NULL));

  /* test c_rest_mutex_destroy failure in destroy */
  g_mock_mutex_destroy_countdown = 0;
  ASSERT_EQ(C_REST_ERROR_GENERIC, test_c_rest_ts_queue_destroy(q, NULL));

  test_c_rest_ts_queue_destroy(q, NULL); /* clean up */
  free(q);

  PASS();
}

SUITE(mem_mock_suite) {
  SET_SETUP(reset_mocks, NULL);
  RUN_TEST(test_mem_error_branches);
  RUN_TEST(test_ts_queue_error_branches);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(mem_mock_suite);
  GREATEST_MAIN_END();
}
