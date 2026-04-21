#ifndef C_REST_LIST_H
#define C_REST_LIST_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Linked list node */
typedef struct c_rest_list_node {
  /** @brief Data payload */
  void *data;
  /** @brief Pointer to next element */
  struct c_rest_list_node *next;
} c_rest_list_node;

/** @brief Linked list */
typedef struct c_rest_list {
  /** @brief Pointer to head node */
  c_rest_list_node *head;
  /** @brief Pointer to tail node */
  c_rest_list_node *tail;
  /** @brief Current size */
  size_t size;
} c_rest_list;

int c_rest_list_init(c_rest_list *list);
int c_rest_list_push_back(c_rest_list *list, void *data);
int c_rest_list_pop_front(c_rest_list *list, void **out_data);
int c_rest_list_destroy(c_rest_list *list, void (*free_data)(void *));

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* C_REST_LIST_H */
