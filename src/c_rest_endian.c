/* clang-format off */
#include "c_rest_endian.h"
/* clang-format on */

static int is_little_endian(void) {
  unsigned int test = 1;
  unsigned char *p = (unsigned char *)&test;
  return p[0] == 1;
}

unsigned short c_rest_htons(unsigned short hostshort) {
  if (is_little_endian()) {
    return (unsigned short)(((hostshort >> 8) & 0x00FF) |
                            ((hostshort << 8) & 0xFF00));
  }
  return hostshort;
}

unsigned long c_rest_htonl(unsigned long hostlong) {
  if (is_little_endian()) {
    return ((hostlong >> 24) & 0x000000FF) | ((hostlong >> 8) & 0x0000FF00) |
           ((hostlong << 8) & 0x00FF0000) | ((hostlong << 24) & 0xFF000000);
  }
  return hostlong;
}

unsigned short c_rest_ntohs(unsigned short netshort) {
  return c_rest_htons(netshort);
}

unsigned long c_rest_ntohl(unsigned long netlong) {
  return c_rest_htonl(netlong);
}
