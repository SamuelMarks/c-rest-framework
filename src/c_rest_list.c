/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_list.h"

#include <stdlib.h>
#include "c_rest_log.h"
/* clang-format on */

c_rest_error_t c_rest_list_init(c_rest_list *list) {
  if (!list)
    return C_REST_ERROR_GENERIC;
  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
  return C_REST_OK;
}

c_rest_error_t c_rest_list_push_back(c_rest_list *list, void *data) {
  c_rest_list_node *node;
  if (!list)
    return C_REST_ERROR_GENERIC;

  if (C_REST_MALLOC(sizeof(c_rest_list_node), &node) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    node = NULL;
  }
  if (!node)
    return C_REST_ERROR_GENERIC;

  node->data = data;
  node->next = NULL;

  if (list->tail) {
    list->tail->next = node;
  } else {
    list->head = node;
  }
  list->tail = node;
  list->size++;

  return C_REST_OK;
}

c_rest_error_t c_rest_list_pop_front(c_rest_list *list, void **out_data) {
  c_rest_list_node *node;
  void *data;

  if (!list || !out_data)
    return C_REST_ERROR_GENERIC;

  if (!list->head) {
    *out_data = NULL;
    return C_REST_ERROR_GENERIC;
  }

  node = list->head;
  data = node->data;

  list->head = node->next;
  if (!list->head) {
    list->tail = NULL;
  }

  C_REST_FREE((void *)(node));
  list->size--;

  *out_data = data;
  return C_REST_OK;
}

c_rest_error_t c_rest_list_destroy(c_rest_list *list,
                                   void (*free_data)(void *)) {
  c_rest_list_node *node;
  c_rest_list_node *next;

  if (!list)
    return C_REST_ERROR_GENERIC;

  node = list->head;
  while (node) {
    next = node->next;
    if (free_data && node->data) {
      free_data(node->data);
    }
    C_REST_FREE((void *)(node));
    node = next;
  }

  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
  return C_REST_OK;
}
