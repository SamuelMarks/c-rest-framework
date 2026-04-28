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

/**
 * @brief Initializes a thread-safe queue.
 *
 * @param queue The queue to initialize.
 * @return 0 on success, non-zero on error.
 */
int c_rest_ts_queue_init(c_rest_ts_queue *queue);

/**
 * @brief Pushes data into a thread-safe queue.
 *
 * @param queue The queue.
 * @param data The data to push.
 * @return 0 on success, non-zero on error.
 */
int c_rest_ts_queue_push(c_rest_ts_queue *queue, void *data);

/**
 * @brief Pops data from a thread-safe queue.
 *
 * @param queue The queue.
 * @param out_data Pointer to hold the popped data.
 * @return 0 on success, non-zero on error.
 */
int c_rest_ts_queue_pop(c_rest_ts_queue *queue, void **out_data);

/**
 * @brief Closes a thread-safe queue.
 *
 * @param queue The queue.
 * @return 0 on success, non-zero on error.
 */
int c_rest_ts_queue_close(c_rest_ts_queue *queue);

/**
 * @brief Destroys a thread-safe queue.
 *
 * @param queue The queue to destroy.
 * @param free_data Function to free the data in the queue nodes.
 * @return 0 on success, non-zero on error.
 */
int c_rest_ts_queue_destroy(c_rest_ts_queue *queue, void (*free_data)(void *));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_TS_QUEUE_H */
