/* clang-format off */
#include "c_rest_string.h"

#include <stdlib.h>
#include <string.h>
/* clang-format on */

#if defined(_MSC_VER)
#define SAFE_STRNCPY(dest, size, src, count) strncpy_s(dest, size, src, count)
#else
#define SAFE_STRNCPY(dest, size, src, count) strncpy(dest, src, count)
#endif

int c_rest_string_init(c_rest_string *str, size_t initial_capacity) {
  if (!str)
    return 1;
  if (initial_capacity == 0)
    initial_capacity = 16;
  str->data = (char *)malloc(initial_capacity);
  if (!str->data)
    return 1;
  str->data[0] = '\0';
  str->length = 0;
  str->capacity = initial_capacity;
  return 0;
}

int c_rest_string_append(c_rest_string *str, const char *data, size_t len) {
  if (!str || !data || len == 0)
    return 1;
  if (str->length + len + 1 > str->capacity) {
    size_t new_cap = str->capacity > 0 ? str->capacity * 2 : 16;
    char *new_data;
    while (str->length + len + 1 > new_cap) {
      new_cap *= 2;
    }
    new_data = (char *)realloc(str->data, new_cap);
    if (!new_data)
      return 1;
    str->data = new_data;
    str->capacity = new_cap;
  }
  memcpy(str->data + str->length, data, len);
  str->length += len;
  str->data[str->length] = '\0';
  return 0;
}

int c_rest_string_append_cstr(c_rest_string *str, const char *cstr) {
  if (!cstr)
    return 1;
  return c_rest_string_append(str, cstr, strlen(cstr));
}

int c_rest_string_destroy(c_rest_string *str) {
  if (!str)
    return 1;
  if (str->data)
    free(str->data);
  str->data = NULL;
  str->length = 0;
  str->capacity = 0;
  return 0;
}
