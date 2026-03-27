#ifndef C_REST_TS_QUEUE_H
#define C_REST_TS_QUEUE_H

/* clang-format off */
#include "c_rest_platform.h"
#include <stddef.h>
/* clang-format on */
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Thread-safe queue node */
typedef struct c_rest_ts_queue_node {
  /** @brief Data payload */
  void *data;
  /** @brief Pointer to next element */
  struct c_rest_ts_queue_node *next;
} c_rest_ts_queue_node;

/** @brief Thread-safe queue */
typedef struct c_rest_ts_queue {
  /** @brief Pointer to head node */
  c_rest_ts_queue_node *head;
  /** @brief Pointer to tail node */
  c_rest_ts_queue_node *tail;
  /** @brief Mutex for thread safety */
  c_rest_mutex_t mutex;
  /** @brief Condition variable */
  c_rest_cond_t cond;
  /** @brief Current size */
  size_t size;
  /** @brief Queue closed flag */
  int is_closed;
} c_rest_ts_queue;

int c_rest_ts_queue_init(c_rest_ts_queue *queue);
int c_rest_ts_queue_push(c_rest_ts_queue *queue, void *data);
int c_rest_ts_queue_pop(c_rest_ts_queue *queue, void **out_data);
int c_rest_ts_queue_close(c_rest_ts_queue *queue);
int c_rest_ts_queue_destroy(c_rest_ts_queue *queue, void (*free_data)(void *));

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* C_REST_TS_QUEUE_H */
