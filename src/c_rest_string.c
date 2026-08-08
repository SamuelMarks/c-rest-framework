/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_string.h"

#include <stdlib.h>
#include <string.h>
#include "c_rest_log.h"
/* clang-format on */

#if defined(_MSC_VER)
#define SAFE_STRNCPY(dest, size, src, count) strncpy_s(dest, size, src, count)
#else
#define SAFE_STRNCPY(dest, size, src, count) strncpy(dest, src, count)
#endif

c_rest_error_t c_rest_string_init(c_rest_string *str, size_t initial_capacity) {
  void *tmp_data;
  if (!str)
    return C_REST_ERROR_GENERIC;
  if (initial_capacity == 0)
    initial_capacity = 16;
  if (C_REST_MALLOC(initial_capacity, &tmp_data)) {
    LOG_DEBUG("C_REST_MALLOC failed");
    str->data = NULL;
  } else {
    str->data = (char *)tmp_data;
  }
  if (!str->data)
    return C_REST_ERROR_GENERIC;
  str->data[0] = '\0';
  str->length = 0;
  str->capacity = initial_capacity;
  return C_REST_OK;
}

c_rest_error_t c_rest_string_append(c_rest_string *str, const char *data,
                                    size_t len) {
  size_t i;
  size_t new_cap;
  char *new_data;
  void *tmp_new_data = NULL;

  if (!str || !data || len == 0)
    return C_REST_ERROR_GENERIC;

  /* Check for integer overflow */
  if (len > ((size_t)-1) - str->length - 1) {
    return C_REST_ERROR_GENERIC;
  }

  if (str->length + len < str->capacity) {
    for (i = 0; i < len; ++i) {
      str->data[str->length + i] = data[i];
    }
    str->length += len;
    str->data[str->length] = '\0';
    return C_REST_OK;
  }

  new_cap = str->capacity == 0 ? 16 : str->capacity;

  while (str->length + len + 1 > new_cap) {
    if (new_cap >= ((size_t)-1) / 2) {
      new_cap = (size_t)-1;
      break;
    }
    new_cap *= 2;
  }

  if (C_REST_REALLOC(str->data, new_cap, &tmp_new_data)) {
    LOG_DEBUG("C_REST_REALLOC failed");
    new_data = NULL;
  } else {
    new_data = (char *)tmp_new_data;
  }
  if (!new_data)
    return C_REST_ERROR_GENERIC;
  str->data = new_data;
  str->capacity = new_cap;

  for (i = 0; i < len; ++i) {
    str->data[str->length + i] = data[i];
  }
  str->length += len;
  str->data[str->length] = '\0';
  return C_REST_OK;
}

c_rest_error_t c_rest_string_append_cstr(c_rest_string *str, const char *cstr) {
  if (!cstr)
    return C_REST_ERROR_GENERIC;
  return c_rest_string_append(str, cstr, strlen(cstr));
}

c_rest_error_t c_rest_string_destroy(c_rest_string *str) {
  if (!str)
    return C_REST_ERROR_GENERIC;
  if (str->data)
    C_REST_FREE((void *)(str->data));
  str->data = NULL;
  str->length = 0;
  str->capacity = 0;
  return C_REST_OK;
}
