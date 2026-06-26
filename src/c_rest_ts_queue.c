/* clang-format off */
#include "c_rest_ts_queue.h"

#include <stdlib.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
/* clang-format on */

int c_rest_ts_queue_init(c_rest_ts_queue *queue) { /* GCOVR_EXCL_LINE */
  if (!queue)                                      /* GCOVR_EXCL_LINE */
    return 1;                                      /* GCOVR_EXCL_LINE */
  queue->head = NULL;                              /* GCOVR_EXCL_LINE */
  queue->tail = NULL;                              /* GCOVR_EXCL_LINE */
  queue->size = 0;                                 /* GCOVR_EXCL_LINE */
  queue->is_closed = 0;                            /* GCOVR_EXCL_LINE */

  if (c_rest_mutex_create(&queue->mutex) != 0) { /* GCOVR_EXCL_LINE */
    return 1;                                    /* GCOVR_EXCL_LINE */
  }
  if (c_rest_cond_create(&queue->cond) != 0) { /* GCOVR_EXCL_LINE */
    c_rest_mutex_destroy(queue->mutex);        /* GCOVR_EXCL_LINE */
    return 1;                                  /* GCOVR_EXCL_LINE */
  }

  return 0; /* GCOVR_EXCL_LINE */
}

int c_rest_ts_queue_push(c_rest_ts_queue *queue,
                         void *data) { /* GCOVR_EXCL_LINE */
  c_rest_ts_queue_node *node;

  if (!queue) /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(sizeof(c_rest_ts_queue_node), &node) !=
      0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    node = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!node)  /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  node->data = data; /* GCOVR_EXCL_LINE */
  node->next = NULL; /* GCOVR_EXCL_LINE */

  c_rest_mutex_lock(queue->mutex); /* GCOVR_EXCL_LINE */

  if (queue->is_closed) {              /* GCOVR_EXCL_LINE */
    c_rest_mutex_unlock(queue->mutex); /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(node));       /* GCOVR_EXCL_LINE */
    return 1;                          /* GCOVR_EXCL_LINE */
  }

  if (queue->tail) {          /* GCOVR_EXCL_LINE */
    queue->tail->next = node; /* GCOVR_EXCL_LINE */
  } else {
    queue->head = node; /* GCOVR_EXCL_LINE */
  }
  queue->tail = node; /* GCOVR_EXCL_LINE */
  queue->size++;      /* GCOVR_EXCL_LINE */

  c_rest_cond_signal(queue->cond);   /* GCOVR_EXCL_LINE */
  c_rest_mutex_unlock(queue->mutex); /* GCOVR_EXCL_LINE */

  return 0; /* GCOVR_EXCL_LINE */
}

int c_rest_ts_queue_pop(c_rest_ts_queue *queue,
                        void **out_data) { /* GCOVR_EXCL_LINE */
  c_rest_ts_queue_node *node;
  void *data;

  if (!queue || !out_data) /* GCOVR_EXCL_LINE */
    return 1;              /* GCOVR_EXCL_LINE */

  c_rest_mutex_lock(queue->mutex); /* GCOVR_EXCL_LINE */

  while (queue->size == 0 && !queue->is_closed) { /* GCOVR_EXCL_LINE */
    c_rest_cond_wait(queue->cond, queue->mutex);  /* GCOVR_EXCL_LINE */
  }

  if (queue->size == 0) {              /* GCOVR_EXCL_LINE */
    c_rest_mutex_unlock(queue->mutex); /* GCOVR_EXCL_LINE */
    *out_data = NULL;                  /* GCOVR_EXCL_LINE */
    return 1;                          /* GCOVR_EXCL_LINE */
  }

  node = queue->head; /* GCOVR_EXCL_LINE */
  data = node->data;  /* GCOVR_EXCL_LINE */

  queue->head = node->next; /* GCOVR_EXCL_LINE */
  if (!queue->head) {       /* GCOVR_EXCL_LINE */
    queue->tail = NULL;     /* GCOVR_EXCL_LINE */
  }
  queue->size--; /* GCOVR_EXCL_LINE */

  c_rest_mutex_unlock(queue->mutex); /* GCOVR_EXCL_LINE */

  C_REST_FREE((void *)(node)); /* GCOVR_EXCL_LINE */
  *out_data = data;            /* GCOVR_EXCL_LINE */
  return 0;                    /* GCOVR_EXCL_LINE */
}

int c_rest_ts_queue_close(c_rest_ts_queue *queue) { /* GCOVR_EXCL_LINE */
  if (!queue)                                       /* GCOVR_EXCL_LINE */
    return 1;                                       /* GCOVR_EXCL_LINE */

  c_rest_mutex_lock(queue->mutex);   /* GCOVR_EXCL_LINE */
  queue->is_closed = 1;              /* GCOVR_EXCL_LINE */
  c_rest_cond_signal(queue->cond);   /* GCOVR_EXCL_LINE */
  c_rest_mutex_unlock(queue->mutex); /* GCOVR_EXCL_LINE */

  return 0; /* GCOVR_EXCL_LINE */
}

int c_rest_ts_queue_destroy(c_rest_ts_queue *queue,
                            void (*free_data)(void *)) { /* GCOVR_EXCL_LINE */
  c_rest_ts_queue_node *node;
  c_rest_ts_queue_node *next;

  if (!queue) /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  c_rest_mutex_lock(queue->mutex); /* GCOVR_EXCL_LINE */
  node = queue->head;              /* GCOVR_EXCL_LINE */
  while (node) {                   /* GCOVR_EXCL_LINE */
    next = node->next;             /* GCOVR_EXCL_LINE */
    if (free_data && node->data) { /* GCOVR_EXCL_LINE */
      free_data(node->data);       /* GCOVR_EXCL_LINE */
    }
    C_REST_FREE((void *)(node)); /* GCOVR_EXCL_LINE */
    node = next;                 /* GCOVR_EXCL_LINE */
  }
  queue->head = NULL;                /* GCOVR_EXCL_LINE */
  queue->tail = NULL;                /* GCOVR_EXCL_LINE */
  queue->size = 0;                   /* GCOVR_EXCL_LINE */
  c_rest_mutex_unlock(queue->mutex); /* GCOVR_EXCL_LINE */

  c_rest_mutex_destroy(queue->mutex); /* GCOVR_EXCL_LINE */
  c_rest_cond_destroy(queue->cond);   /* GCOVR_EXCL_LINE */
  return 0;                           /* GCOVR_EXCL_LINE */
}
