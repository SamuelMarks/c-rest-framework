/* clang-format off */
#include "c_rest_export.h"
#include "c_rest_platform.h"

#define C_REST_MEM_TRACK 1
#include "test_protos.h"
#include "c_rest_mem.h"
#include "c_rest_platform.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

C_REST_EXPORT extern int mem_initialized;

static void *fail_malloc(size_t size) {
  (void)size;
  return NULL;
}
static void *fail_calloc(size_t n, size_t s) {
  (void)n;
  (void)s;
  return NULL;
}
static void *fail_realloc(void *p, size_t s) {
  (void)p;
  (void)s;
  return NULL;
}
static char *fail_strdup(const char *s) {
  (void)s;
  return NULL;
}

static char *success_strdup(const char *s) {
  size_t len = strlen(s) + 1;
  char *d = (char *)malloc(len);
  if (d) {
#if defined(_MSC_VER)
    strcpy_s(d, len, s);
#else
    memcpy(d, s, len);
#endif
  }
  return d;
}

TEST test_mem_cases(void) {
  void *ptr1 = NULL;
  void *ptr2 = NULL;
  void *ptr3 = NULL;
  char *str1 = NULL;
  int dummy_anchor = 0;
  c_rest_error_t rc;
  void *ptr_leak = NULL;

  /* Test cleanup before init */
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_print_leaks());

  /* Null checks */
  rc = C_REST_MALLOC(50, NULL);
  ASSERT_FALSE(rc == C_REST_OK);
  rc = C_REST_CALLOC(5, 5, NULL);
  ASSERT_FALSE(rc == C_REST_OK);
  rc = c_rest_mem_calloc((size_t)-1, 2, __FILE__, __LINE__, &ptr1);
  ASSERT_FALSE(rc != C_REST_ERROR_OOM);
  rc = c_rest_mem_calloc(0, 5, __FILE__, __LINE__, &ptr1);
  if (rc == C_REST_OK && ptr1) {
    free(ptr1);
    ptr1 = NULL;
  }
  rc = C_REST_REALLOC(NULL, 10, NULL);
  ASSERT_FALSE(rc == C_REST_OK);

  /* Realloc invalid pointer when empty will be tested after init */

  rc = c_rest_mem_strdup("test", "f", 1, NULL);
  ASSERT_FALSE(rc == C_REST_OK);
  rc = c_rest_mem_strdup(NULL, "f", 1, &str1);
  ASSERT_FALSE(rc == C_REST_OK);

  /* Failure when mem_initialized is 0 */
  rc = C_REST_MALLOC(50, &ptr_leak);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);

  /* Test c_rest_mem_realloc when mem_initialized is 0 (it should just use
   * standard realloc) */
  rc = C_REST_REALLOC(NULL, 10, &ptr1);
  ASSERT_FALSE(rc != C_REST_OK);

  g_crf_realloc_hook = fail_realloc;
  rc = C_REST_REALLOC(NULL, 10, &ptr2);
  g_crf_realloc_hook = NULL;
  ASSERT_FALSE(rc != C_REST_ERROR_OOM);

  free(ptr1);
  ptr1 = NULL;

  /* Init fail */
  g_crf_malloc_hook = fail_malloc;
  rc = c_rest_mem_tracker_init();
  g_crf_malloc_hook = NULL;
  ASSERT_FALSE(rc == C_REST_OK);

  /* Test init success */
  rc = c_rest_mem_tracker_init();
  ASSERT_FALSE(rc != C_REST_OK);

  /* Tracker is empty here. Test Realloc invalid ptr when empty! */
  g_crf_realloc_hook = fail_realloc;
  rc = C_REST_REALLOC(&dummy_anchor, 10, &ptr3);
  g_crf_realloc_hook = NULL;
  ASSERT_FALSE(rc != C_REST_ERROR_OOM);

  /* Test second init */
  rc = c_rest_mem_tracker_init();
  ASSERT_FALSE(rc != C_REST_OK);

  /* Test 0 leaks */
  rc = c_rest_mem_tracker_print_leaks();
  ASSERT_FALSE(rc != C_REST_OK);

  /* Allocations */
  rc = C_REST_MALLOC(50, &ptr_leak);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(ptr_leak == NULL);

  /* Another leak */
  {
    void *ptr_leak2 = NULL;
    rc = C_REST_MALLOC(50, &ptr_leak2);
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_FALSE(ptr_leak2 == NULL);
    rc = C_REST_FREE(ptr_leak2);
    ASSERT_FALSE(rc != C_REST_OK);
  }

  rc = C_REST_MALLOC(100, &ptr1);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(ptr1 == NULL);

  rc = C_REST_CALLOC(10, 10, &ptr2);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(ptr2 == NULL);

  /* Realloc invalid pointer when empty (or when no elements match) */
  g_crf_realloc_hook = fail_realloc;
  rc = C_REST_REALLOC(&dummy_anchor, 10, &ptr3);
  g_crf_realloc_hook = NULL;
  ASSERT_FALSE(rc != C_REST_ERROR_OOM);

  rc = c_rest_mem_strdup("test_str", "test.c", 100, &str1);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(str1 == NULL);

  /* Realloc an untracked pointer successfully */
  {
    void *untracked = malloc(10);
    void *untracked_c = calloc(1, 10);
    untracked_c = realloc(untracked_c, 20);
    free(untracked_c);
    rc = C_REST_REALLOC(untracked, 20, &untracked);
    ASSERT_FALSE(rc != C_REST_OK);
    free(untracked);
  }

  /* Realloc with non-null pointer */
  rc = C_REST_REALLOC(ptr1, 200, &ptr1);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(ptr1 == NULL);

  /* Realloc with NULL pointer (should malloc) */
  rc = C_REST_REALLOC(NULL, 30, &ptr3);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(ptr3 == NULL);

  /* Realloc with size 0 (should free) */
  rc = C_REST_REALLOC(ptr3, 0, &ptr3);
  ASSERT_FALSE(rc != C_REST_OK);
  ASSERT_FALSE(ptr3 != NULL);

  /* Free */
  rc = C_REST_FREE(ptr1);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = C_REST_FREE(ptr2);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = C_REST_FREE(str1);
  ASSERT_FALSE(rc != C_REST_OK);

  rc = C_REST_FREE(NULL);
  ASSERT_FALSE(rc != C_REST_OK);

  /* Test hooks / failure paths when mem_initialized = 1 */
  {
    void *p = NULL;
    char *s = NULL;

    /* Malloc failure */
    g_crf_malloc_hook = fail_malloc;
    rc = C_REST_MALLOC(10, &p);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(rc != C_REST_ERROR_OOM);

    /* Calloc node failure */
    g_crf_malloc_hook = fail_malloc;
    rc = C_REST_CALLOC(10, 10, &p);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(rc != C_REST_ERROR_OOM);

    /* Calloc hook failure */
    g_crf_calloc_hook = fail_calloc;
    rc = C_REST_CALLOC(10, 10, &p);
    g_crf_calloc_hook = NULL;
    ASSERT_FALSE(rc != C_REST_ERROR_OOM);

    /* Realloc with NULL ptr node failure */
    g_crf_malloc_hook = fail_malloc;
    rc = C_REST_REALLOC(NULL, 10, &p);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(rc != C_REST_ERROR_OOM);

    /* Realloc failure */
    rc = C_REST_MALLOC(10, &p);
    ASSERT_FALSE(rc != C_REST_OK);
    g_crf_realloc_hook = fail_realloc;
    {
      void *new_p = p;
      rc = C_REST_REALLOC(p, 20, &new_p);
      g_crf_realloc_hook = NULL;
      ASSERT_FALSE(rc != C_REST_ERROR_OOM);
    }
    ASSERT_EQ(C_REST_OK, C_REST_FREE(p));

    /* strdup failure */
    g_crf_strdup_hook = fail_strdup;
    rc = c_rest_mem_strdup("test", "f", 1, &s);
    g_crf_strdup_hook = NULL;
    ASSERT_FALSE(rc != C_REST_ERROR_OOM);
  }

  /* Test add_node failures */
  {
    c_rest_mutex_t real_mutex = *g_crf_mem_mutex_ptr;

    g_crf_malloc_hook = fail_malloc;
    rc = c_rest_mem_strdup("test", "f", 1, &str1);
    g_crf_malloc_hook = NULL;
    ASSERT_FALSE(rc != C_REST_ERROR_OOM);

    /* Mutex lock fails in add_node */
    *g_crf_mem_mutex_ptr = (c_rest_mutex_t)0;
    rc = c_rest_mem_strdup("test", "f", 1, &str1);
    ASSERT_FALSE(rc == C_REST_OK);
    *g_crf_mem_mutex_ptr = real_mutex;
  }

  /* Test remove_node failure */
  {
    void *p = NULL;
    c_rest_mutex_t real_mutex = *g_crf_mem_mutex_ptr;
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(10, &p));

    *g_crf_mem_mutex_ptr = (c_rest_mutex_t)0;
    rc = C_REST_FREE(p);
    ASSERT_FALSE(rc == C_REST_OK);
    *g_crf_mem_mutex_ptr = real_mutex;

    ASSERT_EQ(C_REST_OK, C_REST_FREE(p));
  }

  /* Test free not in tracker */
  {
    void *fake_ptr = malloc(10);
    ASSERT_EQ(C_REST_OK, C_REST_FREE(fake_ptr));
  }

  /* Test print_leaks failure */
  {
    c_rest_mutex_t real_mutex = *g_crf_mem_mutex_ptr;
    *g_crf_mem_mutex_ptr = (c_rest_mutex_t)0;
    rc = c_rest_mem_tracker_print_leaks();
    ASSERT_FALSE(rc == C_REST_OK);
    *g_crf_mem_mutex_ptr = real_mutex;
  }

  /* Test cleanup failure */
  {
    c_rest_mutex_t real_mutex = *g_crf_mem_mutex_ptr;
    *g_crf_mem_mutex_ptr = (c_rest_mutex_t)0;
    rc = c_rest_mem_tracker_cleanup();
    ASSERT_FALSE(rc == C_REST_OK);
    *g_crf_mem_mutex_ptr = real_mutex;
  }

  /* Test realloc mutex failures */
  {
    void *p = NULL;
    c_rest_mutex_t real_mutex = *g_crf_mem_mutex_ptr;
    ASSERT_EQ(C_REST_OK, C_REST_MALLOC(10, &p));

    *g_crf_mem_mutex_ptr = (c_rest_mutex_t)0;
    rc = C_REST_REALLOC(p, 20, &p);
    *g_crf_mem_mutex_ptr = real_mutex;
    if (rc != C_REST_OK) {
      ASSERT_EQ(C_REST_OK, C_REST_FREE(p));
    } else {
      ASSERT_EQ(C_REST_OK, C_REST_FREE(p));
    }
  }

  /* Test leaks */
  rc = c_rest_mem_tracker_print_leaks();
  rc = c_rest_mem_tracker_print_leaks();

  ASSERT_EQ(C_REST_OK, C_REST_FREE(ptr_leak));
  ASSERT_FALSE(rc == C_REST_OK);

  /* Test remove_node uninitialized state */
  mem_initialized = 0;
  rc = C_REST_FREE(&dummy_anchor);
  ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
  mem_initialized = 1;

  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
  {
    void *leak1 = NULL;
    void *leak2 = NULL;
    rc = C_REST_MALLOC(10, &leak1);
    ASSERT_FALSE(rc != C_REST_OK);
    rc = C_REST_MALLOC(10, &leak2);
    ASSERT_FALSE(rc != C_REST_OK);

    rc = c_rest_mem_tracker_cleanup();
    ASSERT_FALSE(rc != C_REST_OK);
    ASSERT_EQ(C_REST_ERROR_GENERIC, C_REST_FREE(leak1));
    ASSERT_EQ(C_REST_ERROR_GENERIC, C_REST_FREE(leak2));
    free(leak1);
    free(leak2);
  }

  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
  ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

#ifdef C_REST_TESTING_MALLOC_HOOK
  {
    char *dup_str = NULL;
    void *ptr = NULL;

    /* c_rest_internal_strdup */
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_internal_strdup(NULL, &dup_str));
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_internal_strdup("test", NULL));
    ASSERT_EQ(C_REST_OK, c_rest_internal_strdup("test", &dup_str));
    free(dup_str);
    dup_str = NULL;

    /* 1: add_node unlock fail (unlinks and frees node cleanly) */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    g_mock_mem_fail = 1;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr));
    g_mock_mem_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

    /* 2: remove_node unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    ASSERT_EQ(C_REST_OK, c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr));
    g_mock_mem_fail = 2;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_free(ptr));
    g_mock_mem_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

    /* 4: realloc unlock fail on success */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    ASSERT_EQ(C_REST_OK, c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr));
    g_mock_mem_fail = 4;
    rc = c_rest_mem_realloc(ptr, 20, __FILE__, __LINE__, &ptr);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    g_mock_mem_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mem_free(ptr));
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

    /* 4b: realloc unlock fail on OOM */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    ASSERT_EQ(C_REST_OK, c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr));
    g_crf_realloc_hook = fail_realloc;
    g_mock_mem_fail = 4;
    {
      void *oom_out = NULL;
      rc = c_rest_mem_realloc(ptr, 20, __FILE__, __LINE__, &oom_out);
      ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    }
    g_crf_realloc_hook = NULL;
    g_mock_mem_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mem_free(ptr));
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

    /* 5: realloc lock fail */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    ASSERT_EQ(C_REST_OK, c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr));
    g_mock_mem_fail = 5;
    rc = c_rest_mem_realloc(ptr, 20, __FILE__, __LINE__, &ptr);
    ASSERT_FALSE(rc != C_REST_ERROR_GENERIC);
    g_mock_mem_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mem_free(ptr));
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

    /* 8: print_leaks unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    g_mock_mem_fail = 8;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_tracker_print_leaks());
    g_mock_mem_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

    /* 10: cleanup unlock fail */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    g_mock_mem_fail = 10;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_tracker_cleanup());
    g_mock_mem_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

    /* 11: cleanup mutex_destroy fail */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    g_mock_mem_fail = 11;
    ASSERT_EQ(C_REST_ERROR_GENERIC, c_rest_mem_tracker_cleanup());
    g_mock_mem_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mutex_destroy(*g_crf_mem_mutex_ptr));
    mem_initialized = 0;

    /* realloc size 0 */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    ASSERT_EQ(C_REST_OK, c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr));
    g_mock_mem_fail = 2;
    ASSERT_EQ(C_REST_ERROR_GENERIC,
              c_rest_mem_realloc(ptr, 0, __FILE__, __LINE__, &ptr));
    g_mock_mem_fail = 0;
    ASSERT_EQ(C_REST_OK, c_rest_mem_free(ptr));
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

    /* normal realloc size 0 */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    ASSERT_EQ(C_REST_OK, c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr));
    ASSERT_EQ(C_REST_OK, c_rest_mem_realloc(ptr, 0, __FILE__, __LINE__, &ptr));
    /* calloc size 0 and overflow */
    rc = c_rest_mem_calloc(1, 0, __FILE__, __LINE__, &ptr);
    if (rc == C_REST_OK && ptr) {
      ASSERT_EQ(C_REST_OK, c_rest_mem_free(ptr));
      ptr = NULL;
    }
    rc = c_rest_mem_calloc(((size_t)-1), 2, __FILE__, __LINE__, &ptr);
    ASSERT_FALSE(rc != C_REST_ERROR_OOM);
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

    /* strdup without hook (normal branch) */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    g_crf_strdup_hook = NULL;
    ASSERT_EQ(C_REST_OK,
              c_rest_mem_strdup("test", __FILE__, __LINE__, &dup_str));
    ASSERT_EQ(C_REST_OK, c_rest_mem_free(dup_str));
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());

    /* strdup with hook returning non-null */
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_init());
    g_crf_strdup_hook = success_strdup;
    ASSERT_EQ(C_REST_OK,
              c_rest_mem_strdup("test", __FILE__, __LINE__, &dup_str));
    g_crf_strdup_hook = NULL;
    ASSERT_EQ(C_REST_OK, c_rest_mem_free(dup_str));
    ASSERT_EQ(C_REST_OK, c_rest_mem_tracker_cleanup());
  }
#endif

  PASS();
}

SUITE(mem_suite) { RUN_TEST(test_mem_cases); }
