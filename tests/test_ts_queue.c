/* clang-format off */
#include "c_rest_ts_queue.h"
#include "test_protos.h"
#include "c_rest_mem.h"
#include "c_rest_platform.h"
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/select.h>
#include <sys/time.h>
#endif
/* clang-format on */

static int dummy_free_count = 0;
static void dummy_free(void *ptr) {
  (void)ptr;
  dummy_free_count++;
}

static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}

#if !defined(__EMSCRIPTEN__)
static c_rest_error_t pusher_thread(void *arg) {
  c_rest_ts_queue *queue = (c_rest_ts_queue *)arg;
#if defined(_WIN32)
  Sleep(20);
#else
  {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 20000;
    select(0, NULL, NULL, NULL, &tv);
  }
#endif
  return c_rest_ts_queue_push(queue, (void *)"thread_item");
}
#endif

int test_ts_queue(void) {
  c_rest_ts_queue q;
  c_rest_ts_queue q2;
  int failed = 0;
  void *val = NULL;
  c_rest_error_t rc;
  c_rest_cond_t real_cond;
  c_rest_mutex_t real_mutex;
  const char *msgs[2];

  /* Test NULL pointer handling */
  rc = c_rest_ts_queue_init(NULL);
  failed += (rc == C_REST_OK);

  /* Test c_rest_ts_queue_init cond fail */
  g_crf_malloc_hook = fail_malloc;
  /* c_rest_cond_create fails when malloc fails */
  rc = c_rest_ts_queue_init(&q);
  g_crf_malloc_hook = NULL;
  failed += (rc == C_REST_OK);

  rc = c_rest_ts_queue_push(NULL, "test1");
  failed += (rc == C_REST_OK);

  rc = c_rest_ts_queue_pop(NULL, &val);
  failed += (rc == C_REST_OK);

  rc = c_rest_ts_queue_pop(&q, NULL);
  failed += (rc == C_REST_OK);

  rc = c_rest_ts_queue_close(NULL);
  failed += (rc == C_REST_OK);

  rc = c_rest_ts_queue_destroy(NULL, NULL);
  failed += (rc == C_REST_OK);

  /* Test c_rest_ts_queue_init malloc failure */
  g_crf_malloc_hook = fail_malloc;
  rc = c_rest_ts_queue_init(&q);
  g_crf_malloc_hook = NULL;
  failed += (rc == C_REST_OK);
  /* We might need a slightly more sophisticated mock if we want to fail
     condition creation but not mutex creation... But malloc hook failure fails
     the first allocation (mutex). So c_rest_mutex_create fails. */

  /* Normal flow */
  rc = c_rest_ts_queue_init(&q);
  failed += (rc != C_REST_OK);

  /* Test push malloc failure */
  g_crf_malloc_hook = fail_malloc;
  rc = c_rest_ts_queue_push(&q, "fail");
  g_crf_malloc_hook = NULL;
  failed += (rc == C_REST_OK);

  /* Push first item */
  rc = c_rest_ts_queue_push(&q, "test1");
  failed += (rc != C_REST_OK);

  /* Push second item to hit the queue->tail branch */
  rc = c_rest_ts_queue_push(&q, "test2");
  failed += (rc != C_REST_OK);

  /* Test pop cond wait failure */
  real_cond = q.cond;
  q.cond = (c_rest_cond_t)0;
  /* Queue is not empty right now, so it won't wait on cond! */
  /* Let's wait on cond when empty */
  q.cond = real_cond;

  /* Pop first item (head->next not NULL) */
  rc = c_rest_ts_queue_pop(&q, &val);
  failed += (rc != C_REST_OK);
  failed += (strcmp((const char *)val, "test1") != 0);

  /* Pop second item (head->next is NULL, becomes empty) */
  rc = c_rest_ts_queue_pop(&q, &val);
  failed += (rc != C_REST_OK);
  failed += (strcmp((const char *)val, "test2") != 0);

  /* Test pop from empty queue (will block unless closed or cond fails) */
  /* If cond fails, it returns error */
  real_cond = q.cond;
  q.cond = (c_rest_cond_t)0;
  rc = c_rest_ts_queue_pop(&q, &val);
  failed += (rc != C_REST_ERROR_GENERIC);
  q.cond = real_cond;
  q.cond = (c_rest_cond_t)0;
  rc = c_rest_ts_queue_push(&q, "test_cond_fail");
  failed += (rc != C_REST_ERROR_GENERIC);
  q.cond = real_cond;

  /* Although push returned error, the node was added and mutex unlocked!
     Wait, if cond signal fails, it returns error, but the item IS in the queue!
     Let's pop it! */
  rc = c_rest_ts_queue_pop(&q, &val);
  failed += (rc != C_REST_OK);
  failed += (strcmp((const char *)val, "test_cond_fail") != 0);

  /* Test pop with background thread pushing to hit cond_wait success and break
   */
#if !defined(__EMSCRIPTEN__)
  {
    c_rest_ts_queue q_bg;
    c_rest_thread_t th;
    void *bg_val = NULL;
    rc = c_rest_ts_queue_init(&q_bg);
    failed += (rc != C_REST_OK);
    rc = c_rest_thread_create(&th, pusher_thread, &q_bg);
    failed += (rc != C_REST_OK);
    rc = c_rest_ts_queue_pop(&q_bg, &bg_val);
    failed += (rc != C_REST_OK);
    failed += (strcmp((const char *)bg_val, "thread_item") != 0);
    c_rest_thread_join(th);
    c_rest_ts_queue_destroy(&q_bg, NULL);
  }
#endif

  /* Close queue */
  rc = c_rest_ts_queue_close(&q);
  failed += (rc != C_REST_OK);

  /* Test close with cond signal failure */
  rc = c_rest_ts_queue_init(&q2);
  failed += (rc != C_REST_OK);
  real_cond = q2.cond;
  q2.cond = (c_rest_cond_t)0;
  rc = c_rest_ts_queue_close(&q2);
  failed += (rc != C_REST_ERROR_GENERIC);
  q2.cond = real_cond;
  rc = c_rest_ts_queue_destroy(&q2, NULL);
  failed += (rc != C_REST_OK);

  /* Push to closed queue should fail */
  rc = c_rest_ts_queue_push(&q, "test3");
  failed += (rc == C_REST_OK);

  /* Pop from closed, empty queue should fail */
  rc = c_rest_ts_queue_pop(&q, &val);
  failed += (rc == C_REST_OK);

  /* Test mutex failures (lock) */
  real_mutex = q.mutex;
  q.mutex = (c_rest_mutex_t)0;

  rc = c_rest_ts_queue_push(&q, "fail");
  failed += (rc == C_REST_OK);

  rc = c_rest_ts_queue_pop(&q, &val);
  failed += (rc == C_REST_OK);

  rc = c_rest_ts_queue_close(&q);
  failed += (rc == C_REST_OK);

  rc = c_rest_ts_queue_destroy(&q, NULL);
  failed += (rc == C_REST_OK);

  q.mutex = real_mutex;

  /* Destroy queue */
  rc = c_rest_ts_queue_destroy(&q, NULL);
  failed += (rc != C_REST_OK);

  /* Test destroy with data */
  dummy_free_count = 0;
  rc = c_rest_ts_queue_init(&q);
  failed += (rc != C_REST_OK);

  rc = c_rest_ts_queue_push(&q, "test4");
  failed += (rc != C_REST_OK);

  rc = c_rest_ts_queue_destroy(&q, dummy_free);
  failed += (rc != C_REST_OK);
  failed += (dummy_free_count != 1);

#if !defined(_WIN32)
  {
    c_rest_ts_queue q3;
    /* Test destroy failure branches */
    rc = c_rest_ts_queue_init(&q3);
    failed += (rc != C_REST_OK);
    rc = c_rest_ts_queue_push(&q3, "test_data");
    failed += (rc != C_REST_OK);
    real_cond = q3.cond;
    q3.cond = (c_rest_cond_t)0;
    rc = c_rest_ts_queue_destroy(&q3, NULL);
    failed += (rc != C_REST_ERROR_GENERIC);
    /* The mutex was successfully destroyed by c_rest_ts_queue_destroy,
     * only the condition variable was left dangling */
    c_rest_cond_destroy(real_cond);

    rc = c_rest_ts_queue_init(&q3);
    failed += (rc != C_REST_OK);
    rc = c_rest_ts_queue_push(&q3, "test_data");
    failed += (rc != C_REST_OK);
    real_mutex = q3.mutex;
    q3.mutex = (c_rest_mutex_t)0;
    rc = c_rest_ts_queue_destroy(&q3, NULL);
    failed += (rc != C_REST_ERROR_GENERIC);
    /* Destroy what wasn't destroyed */
    c_rest_mutex_destroy(real_mutex);
    c_rest_cond_destroy(q3.cond);
  }
#endif

  msgs[0] = "test_ts_queue passed\n";
  msgs[1] = "test_ts_queue failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}
