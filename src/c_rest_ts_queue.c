/* clang-format off */
#include "c_rest_ts_queue.h"

#include <stdlib.h>
/* clang-format on */

int c_rest_ts_queue_init(c_rest_ts_queue *queue) {
  if (!queue)
    return 1;
  queue->head = NULL;
  queue->tail = NULL;
  queue->size = 0;
  queue->is_closed = 0;

  if (c_rest_mutex_create(&queue->mutex) != 0) {
    return 1;
  }
  if (c_rest_cond_create(&queue->cond) != 0) {
    c_rest_mutex_destroy(queue->mutex);
    return 1;
  }

  return 0;
}

int c_rest_ts_queue_push(c_rest_ts_queue *queue, void *data) {
  c_rest_ts_queue_node *node;

  if (!queue)
    return 1;

  node = (c_rest_ts_queue_node *)malloc(sizeof(c_rest_ts_queue_node));
  if (!node)
    return 1;

  node->data = data;
  node->next = NULL;

  c_rest_mutex_lock(queue->mutex);

  if (queue->is_closed) {
    c_rest_mutex_unlock(queue->mutex);
    free(node);
    return 1;
  }

  if (queue->tail) {
    queue->tail->next = node;
  } else {
    queue->head = node;
  }
  queue->tail = node;
  queue->size++;

  c_rest_cond_signal(queue->cond);
  c_rest_mutex_unlock(queue->mutex);

  return 0;
}

void *c_rest_ts_queue_pop(c_rest_ts_queue *queue) {
  c_rest_ts_queue_node *node;
  void *data;

  if (!queue)
    return NULL;

  c_rest_mutex_lock(queue->mutex);

  while (queue->size == 0 && !queue->is_closed) {
    c_rest_cond_wait(queue->cond, queue->mutex);
  }

  if (queue->size == 0 && queue->is_closed) {
    c_rest_mutex_unlock(queue->mutex);
    return NULL;
  }

  node = queue->head;
  data = node->data;

  queue->head = node->next;
  if (!queue->head) {
    queue->tail = NULL;
  }
  queue->size--;

  c_rest_mutex_unlock(queue->mutex);
  free(node);

  return data;
}

int c_rest_ts_queue_close(c_rest_ts_queue *queue) {
  if (!queue)
    return 1;
  c_rest_mutex_lock(queue->mutex);
  queue->is_closed = 1;
  c_rest_cond_signal(queue->cond);
  c_rest_mutex_unlock(queue->mutex);
  return 0;
}

void c_rest_ts_queue_destroy(c_rest_ts_queue *queue,
                             void (*free_data)(void *)) {
  c_rest_ts_queue_node *node;
  c_rest_ts_queue_node *next;

  if (!queue)
    return;

  c_rest_ts_queue_close(queue);

  c_rest_mutex_lock(queue->mutex);
  node = queue->head;
  while (node) {
    next = node->next;
    if (free_data && node->data) {
      free_data(node->data);
    }
    free(node);
    node = next;
  }
  queue->head = NULL;
  queue->tail = NULL;
  queue->size = 0;
  c_rest_mutex_unlock(queue->mutex);

  c_rest_mutex_destroy(queue->mutex);
  c_rest_cond_destroy(queue->cond);
}
