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

/**
 * @brief Initializes a linked list.
 *
 * @param list The list to initialize.
 * @return 0 on success, non-zero on error.
 */
int c_rest_list_init(c_rest_list *list);

/**
 * @brief Pushes data to the back of the linked list.
 *
 * @param list The linked list.
 * @param data The data to push.
 * @return 0 on success, non-zero on error.
 */
int c_rest_list_push_back(c_rest_list *list, void *data);

/**
 * @brief Pops data from the front of the linked list.
 *
 * @param list The linked list.
 * @param out_data Pointer to hold the popped data.
 * @return 0 on success, non-zero on error.
 */
int c_rest_list_pop_front(c_rest_list *list, void **out_data);

/**
 * @brief Destroys the linked list.
 *
 * @param list The list to destroy.
 * @param free_data Function to free the data in the list nodes.
 * @return 0 on success, non-zero on error.
 */
int c_rest_list_destroy(c_rest_list *list, void (*free_data)(void *));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_LIST_H */
