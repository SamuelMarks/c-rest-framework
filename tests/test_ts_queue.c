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

#if !defined(__EMSCRIPTEN__) && !defined(C_REST_NO_THREADS)
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

TEST test_ts_queue_cases(void) {
  c_rest_ts_queue q;
  c_rest_ts_queue q2;
  void *val = NULL;
  c_rest_error_t rc;
  c_rest_cond_t real_cond;
  c_rest_mutex_t real_mutex;
  char test1[] = "test1";
  char test2[] = "test2";
  char test3[] = "test3";
  char test4[] = "test4";
  char test_cond_fail[] = "test_cond_fail";
  char fail_str[] = "fail";
#if !defined(_WIN32)
  char test_data[] = "test_data";
#endif

  /* Test NULL pointer handling */
  rc = c_rest_ts_queue_init(NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  /* Test c_rest_ts_queue_init cond fail */
  g_crf_malloc_hook = fail_malloc;
  /* c_rest_cond_create fails when malloc fails */
  rc = c_rest_ts_queue_init(&q);
  g_crf_malloc_hook = NULL;
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_ts_queue_push(NULL, test1);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_ts_queue_pop(NULL, &val);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_ts_queue_pop(&q, NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_ts_queue_close(NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_ts_queue_destroy(NULL, NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  /* Test c_rest_ts_queue_init malloc failure */
  g_crf_malloc_hook = fail_malloc;
  rc = c_rest_ts_queue_init(&q);
  g_crf_malloc_hook = NULL;
  ASSERT_FALSE(rc == C_REST_OK);

  /* Normal flow */
  rc = c_rest_ts_queue_init(&q);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Test push malloc failure */
  g_crf_malloc_hook = fail_malloc;
  rc = c_rest_ts_queue_push(&q, fail_str);
  g_crf_malloc_hook = NULL;
  ASSERT_FALSE(rc == C_REST_OK);

  /* Push first item */
  rc = c_rest_ts_queue_push(&q, test1);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Push second item to hit the queue->tail branch */
  rc = c_rest_ts_queue_push(&q, test2);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Test pop cond wait failure */
  real_cond = q.cond;
  q.cond = (c_rest_cond_t)0;
  /* Queue is not empty right now, so it won't wait on cond! */
  /* Let's wait on cond when empty */
  q.cond = real_cond;

  /* Pop first item (head->next not NULL) */
  rc = c_rest_ts_queue_pop(&q, &val);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(strcmp((const char *)val, "test1") != 0);

  /* Pop second item (head->next is NULL, becomes empty) */
  rc = c_rest_ts_queue_pop(&q, &val);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(strcmp((const char *)val, "test2") != 0);

  /* Test pop from empty queue (will block unless closed or cond fails) */
  /* If cond fails, it returns error */
  real_cond = q.cond;
  q.cond = (c_rest_cond_t)0;
  rc = c_rest_ts_queue_pop(&q, &val);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  q.cond = real_cond;
  q.cond = (c_rest_cond_t)0;
  rc = c_rest_ts_queue_push(&q, test_cond_fail);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  q.cond = real_cond;

  rc = c_rest_ts_queue_pop(&q, &val);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(strcmp((const char *)val, "test_cond_fail") != 0);

#if !defined(__EMSCRIPTEN__) && !defined(C_REST_NO_THREADS)
  {
    c_rest_ts_queue q_bg;
    c_rest_thread_t th;
    void *bg_val = NULL;
    rc = c_rest_ts_queue_init(&q_bg);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_thread_create(&th, pusher_thread, &q_bg);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_ts_queue_pop(&q_bg, &bg_val);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_FALSE(strcmp((const char *)bg_val, "thread_item") != 0);
    ASSERT_EQ(C_REST_OK, c_rest_thread_join(th));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&q_bg, NULL));
  }
#endif

  /* Close queue */
  rc = c_rest_ts_queue_close(&q);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Test close with cond signal failure */
  rc = c_rest_ts_queue_init(&q2);
  ASSERT_FALSE(rc != C_REST_OK);
  real_cond = q2.cond;
  q2.cond = (c_rest_cond_t)0;
  rc = c_rest_ts_queue_close(&q2);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  q2.cond = real_cond;
  rc = c_rest_ts_queue_destroy(&q2, NULL);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Push to closed queue should fail */
  rc = c_rest_ts_queue_push(&q, test3);
  ASSERT_FALSE(rc == C_REST_OK);

  /* Pop from closed, empty queue should fail */
  rc = c_rest_ts_queue_pop(&q, &val);
  ASSERT_FALSE(rc == C_REST_OK);

  /* Test mutex failures (lock) */
  real_mutex = q.mutex;
  q.mutex = (c_rest_mutex_t)0;

  rc = c_rest_ts_queue_push(&q, fail_str);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_ts_queue_pop(&q, &val);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_ts_queue_close(&q);
  ASSERT_FALSE(rc == C_REST_OK);

  rc = c_rest_ts_queue_destroy(&q, NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  q.mutex = real_mutex;

  /* Destroy queue */
  rc = c_rest_ts_queue_destroy(&q, NULL);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Test destroy with data */
  dummy_free_count = 0;
  rc = c_rest_ts_queue_init(&q);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_ts_queue_push(&q, test4);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = c_rest_ts_queue_destroy(&q, dummy_free);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(dummy_free_count != 1);

#if !defined(_WIN32)
  {
    c_rest_ts_queue q3;
    /* Test destroy failure branches */
    rc = c_rest_ts_queue_init(&q3);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_ts_queue_push(&q3, test_data);
    ASSERT_FALSE(rc != C_REST_OK);
    real_cond = q3.cond;
    q3.cond = (c_rest_cond_t)0;
    rc = c_rest_ts_queue_destroy(&q3, NULL);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_cond_destroy(real_cond));

    rc = c_rest_ts_queue_init(&q3);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = c_rest_ts_queue_push(&q3, test_data);
    ASSERT_FALSE(rc != C_REST_OK);
    real_mutex = q3.mutex;
    q3.mutex = (c_rest_mutex_t)0;
    rc = c_rest_ts_queue_destroy(&q3, NULL);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    ASSERT_EQ(C_REST_OK, c_rest_mutex_destroy(real_mutex));
    ASSERT_EQ(C_REST_OK, c_rest_cond_destroy(q3.cond));
  }
#endif

#ifdef C_REST_TESTING_MALLOC_HOOK
  {
    c_rest_ts_queue tq;
    void *out_val = NULL;

    /* 1: cond_create fail with mutex_destroy success */
    g_mock_tsq_fail = 1;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_init(&tq));

    /* 2: cond_create fail with mutex_destroy fail */
    g_mock_tsq_fail = 2;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_destroy(tq.mutex));

    /* 3: push mutex_lock fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 3;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_push(&tq, (void *)1));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* is_closed unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_close(&tq));
    g_mock_tsq_fail = 5;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_push(&tq, (void *)1));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_unlock(tq.mutex));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 4: push cond_signal fail, unlock ok */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 4;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_push(&tq, (void *)1));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 5: push cond_signal fail, unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 5;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_push(&tq, (void *)1));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_unlock(tq.mutex));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 6: push normal unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 6;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_push(&tq, (void *)1));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_unlock(tq.mutex));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 7: pop cond_wait fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 7;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_pop(&tq, &out_val));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_unlock(tq.mutex));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 8: pop size==0 unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_close(&tq));
    g_mock_tsq_fail = 8;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_pop(&tq, &out_val));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_unlock(tq.mutex));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 9: close cond_signal fail, unlock ok */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 9;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_close(&tq));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 10: close cond_signal fail, unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 10;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_close(&tq));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_unlock(tq.mutex));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 11: close normal unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 11;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_close(&tq));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_unlock(tq.mutex));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 12: destroy unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 12;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_destroy(&tq, NULL));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_unlock(tq.mutex));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 13: destroy mutex_destroy fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 13;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_destroy(&tq, NULL));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_destroy(tq.mutex));
    ASSERT_EQ(C_REST_OK, c_rest_cond_destroy(tq.cond));

    /* 14: pop normal unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_push(&tq, (void *)1));
    g_mock_tsq_fail = 14;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_pop(&tq, &out_val));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_unlock(tq.mutex));
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_destroy(&tq, NULL));

    /* 15: destroy cond_destroy fail */
    ASSERT_EQ(C_REST_OK, c_rest_ts_queue_init(&tq));
    g_mock_tsq_fail = 15;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_ts_queue_destroy(&tq, NULL));
    g_mock_tsq_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_cond_destroy(tq.cond));
  }
#endif

  PASS();
}

SUITE(ts_queue_suite) { RUN_TEST(test_ts_queue_cases); }
