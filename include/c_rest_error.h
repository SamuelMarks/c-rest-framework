#ifndef C_REST_ERROR_H
#define C_REST_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum c_rest_error {
  C_REST_OK = 0,
  C_REST_ERROR_GENERIC = 1,
  C_REST_ERROR_OOM = 2,
  C_REST_ERROR_INVALID_ARG = 3,
  C_REST_ERROR_NOT_FOUND = 4,
  C_REST_ERROR_IO = 5,
  C_REST_ERROR_NETWORK = 6,
  C_REST_ERROR_TIMEOUT = 7,
  C_REST_ERROR_UNAUTHORIZED = 8,
  C_REST_ERROR_NOT_SUPPORTED = 9,
  C_REST_ERROR_PARSE = 10,
  C_REST_ERROR_TLS = 11,
  C_REST_ERROR_DB = 12
} c_rest_error_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
