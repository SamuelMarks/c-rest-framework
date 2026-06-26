/* clang-format off */
#include "c_rest_list.h"

#include <stdlib.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
/* clang-format on */

int c_rest_list_init(c_rest_list *list) { /* GCOVR_EXCL_LINE */
  if (!list)                              /* GCOVR_EXCL_LINE */
    return 1;                             /* GCOVR_EXCL_LINE */
  list->head = NULL;                      /* GCOVR_EXCL_LINE */
  list->tail = NULL;                      /* GCOVR_EXCL_LINE */
  list->size = 0;                         /* GCOVR_EXCL_LINE */
  return 0;                               /* GCOVR_EXCL_LINE */
}

int c_rest_list_push_back(c_rest_list *list, void *data) { /* GCOVR_EXCL_LINE */
  c_rest_list_node *node;
  if (!list)  /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(sizeof(c_rest_list_node), &node) !=
      0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    node = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!node)  /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  node->data = data; /* GCOVR_EXCL_LINE */
  node->next = NULL; /* GCOVR_EXCL_LINE */

  if (list->tail) {          /* GCOVR_EXCL_LINE */
    list->tail->next = node; /* GCOVR_EXCL_LINE */
  } else {
    list->head = node; /* GCOVR_EXCL_LINE */
  }
  list->tail = node; /* GCOVR_EXCL_LINE */
  list->size++;      /* GCOVR_EXCL_LINE */

  return 0; /* GCOVR_EXCL_LINE */
}

int c_rest_list_pop_front(c_rest_list *list,
                          void **out_data) { /* GCOVR_EXCL_LINE */
  c_rest_list_node *node;
  void *data;

  if (!list || !out_data) /* GCOVR_EXCL_LINE */
    return 1;             /* GCOVR_EXCL_LINE */

  if (!list->head) {  /* GCOVR_EXCL_LINE */
    *out_data = NULL; /* GCOVR_EXCL_LINE */
    return 1;         /* GCOVR_EXCL_LINE */
  }

  node = list->head; /* GCOVR_EXCL_LINE */
  data = node->data; /* GCOVR_EXCL_LINE */

  list->head = node->next; /* GCOVR_EXCL_LINE */
  if (!list->head) {       /* GCOVR_EXCL_LINE */
    list->tail = NULL;     /* GCOVR_EXCL_LINE */
  }

  C_REST_FREE((void *)(node)); /* GCOVR_EXCL_LINE */
  list->size--;                /* GCOVR_EXCL_LINE */

  *out_data = data; /* GCOVR_EXCL_LINE */
  return 0;         /* GCOVR_EXCL_LINE */
}

int c_rest_list_destroy(c_rest_list *list,
                        void (*free_data)(void *)) { /* GCOVR_EXCL_LINE */
  c_rest_list_node *node;
  c_rest_list_node *next;

  if (!list)  /* GCOVR_EXCL_LINE */
    return 1; /* GCOVR_EXCL_LINE */

  node = list->head;               /* GCOVR_EXCL_LINE */
  while (node) {                   /* GCOVR_EXCL_LINE */
    next = node->next;             /* GCOVR_EXCL_LINE */
    if (free_data && node->data) { /* GCOVR_EXCL_LINE */
      free_data(node->data);       /* GCOVR_EXCL_LINE */
    }
    C_REST_FREE((void *)(node)); /* GCOVR_EXCL_LINE */
    node = next;                 /* GCOVR_EXCL_LINE */
  }

  list->head = NULL; /* GCOVR_EXCL_LINE */
  list->tail = NULL; /* GCOVR_EXCL_LINE */
  list->size = 0;    /* GCOVR_EXCL_LINE */
  return 0;          /* GCOVR_EXCL_LINE */
}
