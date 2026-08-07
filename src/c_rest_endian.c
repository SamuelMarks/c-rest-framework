/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_endian.h"
#include "c_rest_log.h"
/* clang-format on */

#ifdef C_REST_TESTING_ENDIAN_HOOK
c_rest_error_t (*g_crf_is_little_endian_hook)(int *) = NULL;
#endif

static c_rest_error_t is_little_endian(int *out_is_little) {
  unsigned int test = 1;
  unsigned char *p;
#ifdef C_REST_TESTING_ENDIAN_HOOK
  if (g_crf_is_little_endian_hook) {
    return g_crf_is_little_endian_hook(out_is_little);
  }
#endif
  p = (unsigned char *)&test;
  *out_is_little = (p[0] == 1);
  return C_REST_OK;
}

c_rest_error_t c_rest_htons(unsigned short hostshort,
                            unsigned short *out_netshort) {
  int little;
  unsigned short swapped;
  c_rest_error_t rc;
  if (!out_netshort) {
    LOG_DEBUG("c_rest_htons: invalid argument out_netshort is NULL");
    return C_REST_ERROR_GENERIC;
  }
  rc = is_little_endian(&little);
  if (rc != C_REST_OK) {
    return rc;
  }
  swapped = (unsigned short)((hostshort >> 8) | (hostshort << 8));
  *out_netshort = (unsigned short)(little * swapped + (1 - little) * hostshort);
  return C_REST_OK;
}

c_rest_error_t c_rest_htonl(unsigned long hostlong,
                            unsigned long *out_netlong) {
  int little;
  unsigned long swapped;
  c_rest_error_t rc;
  if (!out_netlong) {
    LOG_DEBUG("c_rest_htonl: invalid argument out_netlong is NULL");
    return C_REST_ERROR_GENERIC;
  }
  rc = is_little_endian(&little);
  if (rc != C_REST_OK) {
    return rc;
  }
  swapped = ((hostlong & 0xFF000000) >> 24) | ((hostlong & 0x00FF0000) >> 8) |
            ((hostlong & 0x0000FF00) << 8) | ((hostlong & 0x000000FF) << 24);
  *out_netlong = little * swapped + (1 - little) * hostlong;
  return C_REST_OK;
}

c_rest_error_t c_rest_ntohs(unsigned short netshort,
                            unsigned short *out_hostshort) {
  c_rest_error_t rc;
  if (!out_hostshort) {
    LOG_DEBUG("c_rest_ntohs: invalid argument out_hostshort is NULL");
    return C_REST_ERROR_GENERIC;
  }
  rc = c_rest_htons(netshort, out_hostshort);
  if (rc != C_REST_OK) {
    return rc;
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_ntohl(unsigned long netlong,
                            unsigned long *out_hostlong) {
  c_rest_error_t rc;
  if (!out_hostlong) {
    LOG_DEBUG("c_rest_ntohl: invalid argument out_hostlong is NULL");
    return C_REST_ERROR_GENERIC;
  }
  rc = c_rest_htonl(netlong, out_hostlong);
  if (rc != C_REST_OK) {
    return rc;
  }
  return C_REST_OK;
}
