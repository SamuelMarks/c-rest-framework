#ifndef C_REST_STRING_H
#define C_REST_STRING_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Dynamic string */
typedef struct c_rest_string {
  /** @brief Data payload */
  char *data;
  /** @brief Current length */
  size_t length;
  /** @brief Total capacity */
  size_t capacity;
} c_rest_string;

int c_rest_string_init(c_rest_string *str, size_t initial_capacity);
int c_rest_string_append(c_rest_string *str, const char *data, size_t len);
int c_rest_string_append_cstr(c_rest_string *str, const char *cstr);
int c_rest_string_destroy(c_rest_string *str);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_STRING_H */
