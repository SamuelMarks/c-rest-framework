/* clang-format off */
#include "c_rest_endian.h"
#include "c_rest_log.h"
/* clang-format on */

static int is_little_endian(void) {
  unsigned int test = 1;
  unsigned char *p = (unsigned char *)&test;
  return p[0] == 1;
}

int c_rest_htons(unsigned short hostshort, unsigned short *out_netshort) {
  if (!out_netshort) {
    LOG_DEBUG("c_rest_htons: invalid argument out_netshort is NULL");
    return 1;
  }
  if (is_little_endian()) {
    *out_netshort = (unsigned short)(((hostshort >> 8) & 0x00FF) |
                                     ((hostshort << 8) & 0xFF00));
  } else {
    *out_netshort = hostshort;
  }
  return 0;
}

int c_rest_htonl(unsigned long hostlong, unsigned long *out_netlong) {
  if (!out_netlong) {
    LOG_DEBUG("c_rest_htonl: invalid argument out_netlong is NULL");
    return 1;
  }
  if (is_little_endian()) {
    *out_netlong =
        ((hostlong >> 24) & 0x000000FF) | ((hostlong >> 8) & 0x0000FF00) |
        ((hostlong << 8) & 0x00FF0000) | ((hostlong << 24) & 0xFF000000);
  } else {
    *out_netlong = hostlong;
  }
  return 0;
}

int c_rest_ntohs(unsigned short netshort, unsigned short *out_hostshort) {
  int rc;
  if (!out_hostshort) {
    LOG_DEBUG("c_rest_ntohs: invalid argument out_hostshort is NULL");
    return 1;
  }
  rc = c_rest_htons(netshort, out_hostshort);
  if (rc != 0) {
    LOG_DEBUG("c_rest_ntohs: c_rest_htons failed with %d", rc);
    return rc;
  }
  return 0;
}

int c_rest_ntohl(unsigned long netlong, unsigned long *out_hostlong) {
  int rc;
  if (!out_hostlong) {
    LOG_DEBUG("c_rest_ntohl: invalid argument out_hostlong is NULL");
    return 1;
  }
  rc = c_rest_htonl(netlong, out_hostlong);
  if (rc != 0) {
    LOG_DEBUG("c_rest_ntohl: c_rest_htonl failed with %d", rc);
    return rc;
  }
  return 0;
}
