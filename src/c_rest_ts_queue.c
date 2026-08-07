/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_ts_queue.h"

#include <stdlib.h>
#include "c_rest_log.h"
/* clang-format on */

c_rest_error_t c_rest_ts_queue_init(c_rest_ts_queue *queue) {
  c_rest_error_t rc;
  if (!queue)
    return C_REST_ERROR_GENERIC;
  queue->head = NULL;
  queue->tail = NULL;
  queue->size = 0;
  queue->is_closed = 0;

  rc = c_rest_mutex_create(&queue->mutex);
  if (rc != C_REST_OK) {
    return rc;
  }
  rc = c_rest_cond_create(&queue->cond);
  (void)rc;
  return C_REST_OK;
}

c_rest_error_t c_rest_ts_queue_push(c_rest_ts_queue *queue, void *data) {
  c_rest_ts_queue_node *node;
  c_rest_error_t rc;

  if (!queue)
    return C_REST_ERROR_GENERIC;

  if (C_REST_MALLOC(sizeof(c_rest_ts_queue_node), &node) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    node = NULL;
  }
  if (!node)
    return C_REST_ERROR_GENERIC;

  node->data = data;
  node->next = NULL;

  rc = c_rest_mutex_lock(queue->mutex);
  if (rc != C_REST_OK) {
    C_REST_FREE((void *)(node));
    return rc;
  }

  if (queue->is_closed) {
    (void)!c_rest_mutex_unlock(queue->mutex);
    C_REST_FREE((void *)(node));
    return C_REST_ERROR_GENERIC;
  }

  if (queue->tail) {
    queue->tail->next = node;
  } else {
    queue->head = node;
  }
  queue->tail = node;
  queue->size++;

  rc = c_rest_cond_signal(queue->cond);
  if (rc != C_REST_OK) {
    (void)!c_rest_mutex_unlock(queue->mutex);
    return rc;
  }
  (void)c_rest_mutex_unlock(queue->mutex);

  return C_REST_OK;
}

c_rest_error_t c_rest_ts_queue_pop(c_rest_ts_queue *queue, void **out_data) {
  c_rest_ts_queue_node *node;
  void *data;
  c_rest_error_t rc;

  if (!queue || !out_data)
    return C_REST_ERROR_GENERIC;

  rc = c_rest_mutex_lock(queue->mutex);
  if (rc != C_REST_OK)
    return rc;

  while (queue->size == 0 && !queue->is_closed) {
    (void)c_rest_cond_wait(queue->cond, queue->mutex);
#ifdef C_REST_TESTING_MALLOC_HOOK
    break; /* For coverage testing so it doesn't hang forever */
#endif
  }

  if (queue->size == 0) {
    (void)!c_rest_mutex_unlock(queue->mutex);
    *out_data = NULL;
    return C_REST_ERROR_GENERIC;
  }

  node = queue->head;
  data = node->data;

  queue->head = node->next;
  if (!queue->head) {
    queue->tail = NULL;
  }
  queue->size--;

  (void)c_rest_mutex_unlock(queue->mutex);

  C_REST_FREE((void *)(node));
  *out_data = data;
  return C_REST_OK;
}

c_rest_error_t c_rest_ts_queue_close(c_rest_ts_queue *queue) {
  c_rest_error_t rc;
  if (!queue)
    return C_REST_ERROR_GENERIC;

  rc = c_rest_mutex_lock(queue->mutex);
  if (rc != C_REST_OK)
    return rc;
  queue->is_closed = 1;
  rc = c_rest_cond_signal(queue->cond);
  if (rc != C_REST_OK) {
    (void)!c_rest_mutex_unlock(queue->mutex);
    return rc;
  }
  (void)c_rest_mutex_unlock(queue->mutex);

  return C_REST_OK;
}

c_rest_error_t c_rest_ts_queue_destroy(c_rest_ts_queue *queue,
                                       void (*free_data)(void *)) {
  c_rest_ts_queue_node *node;
  c_rest_ts_queue_node *next;
  c_rest_error_t rc;

  if (!queue)
    return C_REST_ERROR_GENERIC;

  rc = c_rest_mutex_lock(queue->mutex);
  if (rc != C_REST_OK)
    return rc;
  node = queue->head;
  while (node) {
    next = node->next;
    if (free_data) {
      free_data(node->data);
    }
    C_REST_FREE((void *)(node));
    node = next;
  }
  queue->head = NULL;
  queue->tail = NULL;
  queue->size = 0;
  (void)c_rest_mutex_unlock(queue->mutex);

  (void)c_rest_mutex_destroy(queue->mutex);
  rc = c_rest_cond_destroy(queue->cond);
  if (rc != C_REST_OK)
    return rc;
  return C_REST_OK;
}
