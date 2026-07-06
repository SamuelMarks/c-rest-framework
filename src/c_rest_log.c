/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_log.h"
#include <stdio.h>
#include <stdarg.h>
/* clang-format on */

c_rest_error_t c_rest_framework_log_debug(const char *fmt,
                                          ...) { /* GCOVR_EXCL_LINE */
#ifdef DEBUG
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
#else
  (void)fmt;
#endif
  return C_REST_OK;
} /* GCOVR_EXCL_LINE */
