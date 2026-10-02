#include "c_rest_export.h"
C_REST_EXPORT extern int mem_initialized;
/* clang-format off */
#include "c_rest_platform.h"

#define C_REST_MEM_TRACK 1
#include "test_protos.h"
#include "c_rest_mem.h"
#include "c_rest_platform.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

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

int test_mem(void) {
  void *ptr1 = NULL;
  void *ptr2 = NULL;
  void *ptr3 = NULL;
  char *str1 = NULL;
  int failed = 0;
  c_rest_error_t rc;
  const char *msgs[2];

  void *ptr_leak = NULL;

  /* Test cleanup before init */
  rc = c_rest_mem_tracker_cleanup();
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();
  rc = c_rest_mem_tracker_cleanup();
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();
  rc = c_rest_mem_tracker_print_leaks();

  /* Null checks */
  rc = C_REST_MALLOC(50, NULL);
  failed += (rc == C_REST_OK);
  rc = C_REST_CALLOC(5, 5, NULL);
  failed += (rc == C_REST_OK);
  rc = C_REST_REALLOC(NULL, 10, NULL);
  failed += (rc == C_REST_OK);

  /* Realloc invalid pointer when empty will be tested after init */

  rc = c_rest_mem_strdup("test", "f", 1, NULL);
  failed += (rc == C_REST_OK);
  rc = c_rest_mem_strdup(NULL, "f", 1, &str1);
  failed += (rc == C_REST_OK);

  /* Failure when mem_initialized is 0 */
  rc = C_REST_MALLOC(50, &ptr_leak);
  failed += (rc != C_REST_ERROR_GENERIC);

  /* Test c_rest_mem_realloc when mem_initialized is 0 (it should just use
   * standard realloc) */
  rc = C_REST_REALLOC(NULL, 10, &ptr1);
  failed += (rc != C_REST_OK);

  g_crf_realloc_hook = fail_realloc;
  rc = C_REST_REALLOC(NULL, 10, &ptr2);
  g_crf_realloc_hook = NULL;
  failed += (rc != C_REST_ERROR_OOM);

  free(ptr1);
  ptr1 = NULL;

  /* Init fail */
  g_crf_malloc_hook = fail_malloc;
  rc = c_rest_mem_tracker_init();
  g_crf_malloc_hook = NULL;
  failed += (rc == C_REST_OK);

  /* Test init success */
  rc = c_rest_mem_tracker_init();
  failed += (rc != C_REST_OK);

  /* Tracker is empty here. Test Realloc invalid ptr when empty! */
  g_crf_realloc_hook = fail_realloc;
  rc = C_REST_REALLOC(&failed, 10, &ptr3);
  g_crf_realloc_hook = NULL;
  failed += (rc != C_REST_ERROR_OOM);

  /* Test second init */
  rc = c_rest_mem_tracker_init();
  failed += (rc != C_REST_OK);

  /* Test 0 leaks */
  rc = c_rest_mem_tracker_print_leaks();
  failed += (rc != C_REST_OK);

  /* Allocations */
  rc = C_REST_MALLOC(50, &ptr_leak);
  failed += (rc != C_REST_OK);
  failed += (ptr_leak == NULL);

  /* Another leak */
  {
    void *ptr_leak2 = NULL;
    rc = C_REST_MALLOC(50, &ptr_leak2);
    failed += (rc != C_REST_OK);
    failed += (ptr_leak2 == NULL);
    rc = C_REST_FREE(ptr_leak2);
    failed += (rc != C_REST_OK);
  }

  rc = C_REST_MALLOC(100, &ptr1);
  failed += (rc != C_REST_OK);
  failed += (ptr1 == NULL);

  rc = C_REST_CALLOC(10, 10, &ptr2);
  failed += (rc != C_REST_OK);
  failed += (ptr2 == NULL);

  /* Realloc invalid pointer when empty (or when no elements match) */
  g_crf_realloc_hook = fail_realloc;
  rc = C_REST_REALLOC(&failed, 10, &ptr3);
  g_crf_realloc_hook = NULL;
  failed += (rc != C_REST_ERROR_OOM);

  rc = c_rest_mem_strdup("test_str", "test.c", 100, &str1);
  failed += (rc != C_REST_OK);
  failed += (str1 == NULL);

  /* Realloc an untracked pointer successfully */
  {
    void *untracked = malloc(10);
    rc = C_REST_REALLOC(untracked, 20, &untracked);
    failed += (rc != C_REST_OK);
    free(untracked);
  }

  /* Realloc with non-null pointer */
  rc = C_REST_REALLOC(ptr1, 200, &ptr1);
  failed += (rc != C_REST_OK);
  failed += (ptr1 == NULL);

  /* Realloc with NULL pointer (should malloc) */
  rc = C_REST_REALLOC(NULL, 30, &ptr3);
  failed += (rc != C_REST_OK);
  failed += (ptr3 == NULL);

  /* Realloc with size 0 (should free) */
  rc = C_REST_REALLOC(ptr3, 0, &ptr3);
  failed += (rc != C_REST_OK);
  failed += (ptr3 != NULL);

  /* Free */
  rc = C_REST_FREE(ptr1);
  failed += (rc != C_REST_OK);

  rc = C_REST_FREE(ptr2);
  failed += (rc != C_REST_OK);

  rc = C_REST_FREE(str1);
  failed += (rc != C_REST_OK);

  rc = C_REST_FREE(NULL);
  failed += (rc != C_REST_OK);

  /* Test hooks / failure paths when mem_initialized = 1 */
  {
    void *p = NULL;
    char *s = NULL;

    /* Malloc failure */
    g_crf_malloc_hook = fail_malloc;
    rc = C_REST_MALLOC(10, &p);
    g_crf_malloc_hook = NULL;
    failed += (rc != C_REST_ERROR_OOM);

    /* Calloc node failure */
    g_crf_malloc_hook = fail_malloc;
    rc = C_REST_CALLOC(10, 10, &p);
    g_crf_malloc_hook = NULL;
    failed += (rc != C_REST_ERROR_OOM);

    /* Calloc hook failure */
    g_crf_calloc_hook = fail_calloc;
    rc = C_REST_CALLOC(10, 10, &p);
    g_crf_calloc_hook = NULL;
    failed += (rc != C_REST_ERROR_OOM);

    /* Realloc with NULL ptr node failure */
    g_crf_malloc_hook = fail_malloc;
    rc = C_REST_REALLOC(NULL, 10, &p);
    g_crf_malloc_hook = NULL;
    failed += (rc != C_REST_ERROR_OOM);

    /* Realloc failure */
    rc = C_REST_MALLOC(10, &p);
    failed += (rc != C_REST_OK);
    g_crf_realloc_hook = fail_realloc;
    {
      void *new_p = p;
      rc = C_REST_REALLOC(p, 20, &new_p);
      g_crf_realloc_hook = NULL;
      failed += (rc != C_REST_ERROR_OOM);
    }
    C_REST_FREE(p);

    /* strdup failure */
    g_crf_strdup_hook = fail_strdup;
    rc = c_rest_mem_strdup("test", "f", 1, &s);
    g_crf_strdup_hook = NULL;
    failed += (rc != C_REST_ERROR_OOM);
  }

  /* Test add_node failures */
  {
    c_rest_mutex_t real_mutex = *g_crf_mem_mutex_ptr;

    /* Malloc for node fails */
    g_crf_malloc_hook = fail_malloc;
    /* But wait, we can't easily fail the add_node malloc without failing the
       ptr malloc if it's C_REST_MALLOC! Actually, C_REST_MALLOC uses
       g_crf_malloc_hook. Let's test strdup where strdup succeeds but add_node
       fails. */
    g_crf_malloc_hook = fail_malloc;
    rc = c_rest_mem_strdup("test", "f", 1, &str1);
    g_crf_malloc_hook = NULL;
    failed += (rc != C_REST_ERROR_OOM);

    /* Mutex lock fails in add_node */
    *g_crf_mem_mutex_ptr = (c_rest_mutex_t)0;
    rc = c_rest_mem_strdup("test", "f", 1, &str1);
    failed += (rc == C_REST_OK);
    *g_crf_mem_mutex_ptr = real_mutex;
  }

  /* Test remove_node failure */
  {
    void *p = NULL;
    c_rest_mutex_t real_mutex = *g_crf_mem_mutex_ptr;
    rc = C_REST_MALLOC(10, &p);

    *g_crf_mem_mutex_ptr = (c_rest_mutex_t)0;
    rc = C_REST_FREE(p);
    failed += (rc == C_REST_OK);
    *g_crf_mem_mutex_ptr = real_mutex;

    C_REST_FREE(p);
  }

  /* Test free not in tracker */
  {
    void *fake_ptr = malloc(10);
    rc = C_REST_FREE(fake_ptr);
  }

  /* Test print_leaks failure */
  {
    c_rest_mutex_t real_mutex = *g_crf_mem_mutex_ptr;
    *g_crf_mem_mutex_ptr = (c_rest_mutex_t)0;
    rc = c_rest_mem_tracker_print_leaks();
    failed += (rc == C_REST_OK);
    *g_crf_mem_mutex_ptr = real_mutex;
  }

  /* Test cleanup failure */
  {
    c_rest_mutex_t real_mutex = *g_crf_mem_mutex_ptr;
    *g_crf_mem_mutex_ptr = (c_rest_mutex_t)0;
    rc = c_rest_mem_tracker_cleanup();
    c_rest_mem_tracker_init();
    c_rest_mem_tracker_cleanup();
    failed += (rc == C_REST_OK);
    *g_crf_mem_mutex_ptr = real_mutex;
  }

  /* Test realloc mutex failures */
  {
    void *p = NULL;
    c_rest_mutex_t real_mutex = *g_crf_mem_mutex_ptr;
    rc = C_REST_MALLOC(10, &p);

    /* Test lock in realloc with invalid mutex (ignored, but hits the void
     * casts) */
    *g_crf_mem_mutex_ptr = (c_rest_mutex_t)0;
    rc = C_REST_REALLOC(p, 20, &p);
    *g_crf_mem_mutex_ptr = real_mutex;

    /* Since we ignored lock failure, it succeeded. Free it. */
    C_REST_FREE(p);
  }

  /* Test leaks */
  rc = c_rest_mem_tracker_print_leaks();

  /* Now there should be ONE leak */
  rc = c_rest_mem_tracker_print_leaks();

  C_REST_FREE(ptr_leak);
  failed += (rc == C_REST_OK);

  /* Test remove_node uninitialized state */
  mem_initialized = 0;
  rc = C_REST_FREE(
      &failed); /* Just a valid ptr, but not tracked/uninitialized */
  failed += (rc != C_REST_ERROR_GENERIC);
  mem_initialized = 1;
  mem_initialized = 1;

  /* Test empty cleanup */
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();

  /* Test empty cleanup */
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();

  c_rest_mem_tracker_init();
  {
    void *leak1 = NULL;
    void *leak2 = NULL;
    rc = C_REST_MALLOC(10, &leak1);
    failed += (rc != C_REST_OK);
    rc = C_REST_MALLOC(10, &leak2);
    failed += (rc != C_REST_OK);

    rc = c_rest_mem_tracker_cleanup();
    failed += (rc != C_REST_OK);
    C_REST_FREE(leak1);
    C_REST_FREE(leak2);
  }

  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();
  rc = c_rest_mem_tracker_cleanup();
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();

#ifdef C_REST_TESTING_MALLOC_HOOK
  {
    extern int g_mock_mem_fail;
    char *dup_str = NULL;
    void *ptr = NULL;

    /* c_rest_internal_strdup */
    c_rest_internal_strdup(NULL, &dup_str);
    c_rest_internal_strdup("test", NULL);
    c_rest_internal_strdup("test", &dup_str);
    free(dup_str);

    printf("MEM STEP 1\n");
    fflush(stdout);
    /* 1: add_node unlock fail */
    c_rest_mem_tracker_init();
    g_mock_mem_fail = 1;
    c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 0;
    c_rest_mutex_unlock(*g_crf_mem_mutex_ptr);
    c_rest_mem_free(ptr);
    c_rest_mem_tracker_cleanup();

    printf("MEM STEP 2\n");
    fflush(stdout);
    /* 2: remove_node unlock fail */
    c_rest_mem_tracker_init();
    c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 2;
    c_rest_mem_free(ptr);
    g_mock_mem_fail = 0;
    c_rest_mutex_unlock(*g_crf_mem_mutex_ptr);
    c_rest_mem_tracker_cleanup();

    printf("MEM STEP 4\n");
    fflush(stdout);
    /* 4: realloc first unlock fail */
    c_rest_mem_tracker_init();
    c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 4;
    c_rest_mem_realloc(ptr, 20, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 0;
    c_rest_mutex_unlock(*g_crf_mem_mutex_ptr);
    c_rest_mem_free(ptr);
    c_rest_mem_tracker_cleanup();

    printf("MEM STEP 5\n");
    fflush(stdout);
    /* 5: realloc second lock fail */
    c_rest_mem_tracker_init();
    c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 5;
    c_rest_mem_realloc(ptr, 20, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 0;
    c_rest_mem_tracker_cleanup();
    free(ptr);

    printf("MEM STEP 6\n");
    fflush(stdout);
    /* 6: realloc second unlock fail */
    c_rest_mem_tracker_init();
    c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 6;
    c_rest_mem_realloc(ptr, 20, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 0;
    c_rest_mutex_unlock(*g_crf_mem_mutex_ptr);
    c_rest_mem_free(ptr);
    c_rest_mem_tracker_cleanup();

    printf("MEM STEP 8\n");
    fflush(stdout);
    /* 8: print_leaks unlock fail */
    c_rest_mem_tracker_init();
    g_mock_mem_fail = 8;
    c_rest_mem_tracker_print_leaks();
    g_mock_mem_fail = 0;
    c_rest_mutex_unlock(*g_crf_mem_mutex_ptr);
    c_rest_mem_tracker_cleanup();

    printf("MEM STEP 10\n");
    fflush(stdout);
    /* 10: cleanup unlock fail */
    c_rest_mem_tracker_init();
    g_mock_mem_fail = 10;
    c_rest_mem_tracker_cleanup();
    g_mock_mem_fail = 0;
    c_rest_mutex_unlock(*g_crf_mem_mutex_ptr);
    c_rest_mem_tracker_cleanup();

    printf("MEM STEP 11\n");
    fflush(stdout);
    /* 11: cleanup mutex_destroy fail */
    c_rest_mem_tracker_init();
    g_mock_mem_fail = 11;
    c_rest_mem_tracker_cleanup();
    g_mock_mem_fail = 0;
    c_rest_mutex_destroy(*g_crf_mem_mutex_ptr);
    mem_initialized = 0;

    /* realloc size 0 */
    c_rest_mem_tracker_init();
    c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr);
    /* Test realloc size 0 failure when free fails */
    g_mock_mem_fail = 2;
    c_rest_mem_realloc(ptr, 0, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 0;
    c_rest_mem_free(ptr);
    c_rest_mem_tracker_cleanup();

    /* normal realloc size 0 */
    c_rest_mem_tracker_init();
    c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr);
    c_rest_mem_realloc(ptr, 0, __FILE__, __LINE__, &ptr);
    c_rest_mem_tracker_cleanup();

    /* realloc second lock fail with curr */
    c_rest_mem_tracker_init();
    c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 12; /* fail on second lock only (line 267) */
    c_rest_mem_realloc(ptr, 20, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 0;
    c_rest_mem_free(ptr);
    c_rest_mem_tracker_cleanup();

    /* realloc second unlock fail with curr (line 270) */
    c_rest_mem_tracker_init();
    c_rest_mem_malloc(10, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 14; /* first lock succeeds, unlock succeeds, second lock
                             succeeds and sets fail=6 (line 270 unlock fails) */
    c_rest_mem_realloc(ptr, 20, __FILE__, __LINE__, &ptr);
    g_mock_mem_fail = 0;
    c_rest_mem_free(ptr);
    c_rest_mem_tracker_cleanup();

    printf("MEM SUB: /* strdup without hook (normal branch) */\n");
    fflush(stdout);
    /* strdup without hook (normal branch) */
    c_rest_mem_tracker_init();
    g_crf_strdup_hook = NULL;
    c_rest_mem_strdup("test", __FILE__, __LINE__, &dup_str);
    c_rest_mem_free(dup_str);
    c_rest_mem_tracker_cleanup();

    /* strdup with hook returning non-null */
    c_rest_mem_tracker_init();
    g_crf_strdup_hook = success_strdup;
    c_rest_mem_strdup("test", __FILE__, __LINE__, &dup_str);
    g_crf_strdup_hook = NULL;
    c_rest_mem_free(dup_str);
    c_rest_mem_tracker_cleanup();
  }
#endif

  msgs[0] = "test_mem passed\n";
  msgs[1] = "test_mem failed\n";
  printf("%s", msgs[failed != 0]);

  return failed;
}
