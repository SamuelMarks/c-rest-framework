/* clang-format off */
#define C_REST_MEM_TRACK 1
#include "test_protos.h"
#include "c_rest_mem.h"
#include "c_rest_platform.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

extern void *(*g_crf_malloc_hook)(size_t);
extern void *(*g_crf_calloc_hook)(size_t, size_t);
extern void *(*g_crf_realloc_hook)(void *, size_t);
extern char *(*g_crf_strdup_hook)(const char *);
extern c_rest_mutex_t *g_crf_mem_mutex_ptr;
extern int *g_crf_mem_initialized_ptr;

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

int test_mem(void) {
  void *ptr1 = NULL;
  void *ptr2 = NULL;
  void *ptr3 = NULL;
  char *str1 = NULL;
  int failed = 0;
  c_rest_error_t rc;

  void *ptr_leak = NULL;

  /* Test cleanup before init */
  (void)!c_rest_mem_tracker_cleanup();
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();
  (void)!c_rest_mem_tracker_cleanup();
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();
  (void)!c_rest_mem_tracker_print_leaks();

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

  if (ptr1) {
    free(ptr1);
    ptr1 = NULL;
  }

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
  failed += (rc != C_REST_OK | ptr_leak == NULL);

  /* Another leak */
  {
    void *ptr_leak2 = NULL;
    rc = C_REST_MALLOC(50, &ptr_leak2);
    failed += (rc != C_REST_OK | ptr_leak2 == NULL);
    (void)C_REST_FREE(ptr_leak2);
  }

  rc = C_REST_MALLOC(100, &ptr1);
  failed += (rc != C_REST_OK | ptr1 == NULL);

  rc = C_REST_CALLOC(10, 10, &ptr2);
  failed += (rc != C_REST_OK | ptr2 == NULL);

  /* Realloc invalid pointer when empty (or when no elements match) */
  g_crf_realloc_hook = fail_realloc;
  rc = C_REST_REALLOC(&failed, 10, &ptr3);
  g_crf_realloc_hook = NULL;
  failed += (rc != C_REST_ERROR_OOM);

  rc = c_rest_mem_strdup("test_str", "test.c", 100, &str1);
  failed += (rc != C_REST_OK | str1 == NULL);

  /* Realloc an untracked pointer successfully */
  {
    void *untracked = malloc(10);
    rc = C_REST_REALLOC(untracked, 20, &untracked);
    failed += (rc != C_REST_OK);
    free(untracked);
  }

  /* Realloc with non-null pointer */
  rc = C_REST_REALLOC(ptr1, 200, &ptr1);
  failed += (rc != C_REST_OK | ptr1 == NULL);

  /* Realloc with NULL pointer (should malloc) */
  rc = C_REST_REALLOC(NULL, 30, &ptr3);
  failed += (rc != C_REST_OK | ptr3 == NULL);

  /* Realloc with size 0 (should free) */
  rc = C_REST_REALLOC(ptr3, 0, &ptr3);
  failed += (rc != C_REST_OK | ptr3 != NULL);

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
    void *p = NULL;
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
  (void)!c_rest_mem_tracker_print_leaks();

  /* Now there should be ONE leak */
  rc = c_rest_mem_tracker_print_leaks();

  (void)C_REST_FREE(ptr_leak);
  failed += (rc == C_REST_OK);

  /* Test remove_node uninitialized state */
  *g_crf_mem_initialized_ptr = 0;
  rc = C_REST_FREE(
      &failed); /* Just a valid ptr, but not tracked/uninitialized */
  failed += (rc != C_REST_ERROR_GENERIC);
  *g_crf_mem_initialized_ptr = 1;
  *g_crf_mem_initialized_ptr = 1;

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
    void *leak_for_cleanup = NULL;
    C_REST_MALLOC(10, &leak_for_cleanup);
    C_REST_MALLOC(10, &leak_for_cleanup);
  }

  (void)!c_rest_mem_tracker_cleanup();
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();
  (void)!c_rest_mem_tracker_cleanup();
  c_rest_mem_tracker_init();
  c_rest_mem_tracker_cleanup();

  if (failed) {
    printf("test_mem failed\n");
  } else {
    printf("test_mem passed\n");
  }

  return failed;
}
