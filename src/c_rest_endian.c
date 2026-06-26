/* clang-format off */
#include "c_rest_endian.h"
#include "c_rest_log.h"
/* clang-format on */

static int is_little_endian(int *out_is_little) { /* GCOVR_EXCL_LINE */
  unsigned int test = 1;                          /* GCOVR_EXCL_LINE */
  unsigned char *p = (unsigned char *)&test;      /* GCOVR_EXCL_LINE */
  *out_is_little = (p[0] == 1);                   /* GCOVR_EXCL_LINE */
  return 0;                                       /* GCOVR_EXCL_LINE */
}

int c_rest_htons(unsigned short hostshort,
                 unsigned short *out_netshort) { /* GCOVR_EXCL_LINE */
  int little_endian;
  if (!out_netshort) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_htons: invalid argument out_netshort is NULL");
    return 1; /* GCOVR_EXCL_LINE */
  }
  is_little_endian(&little_endian); /* GCOVR_EXCL_LINE */
  if (little_endian) {              /* GCOVR_EXCL_LINE */
    *out_netshort =
        (unsigned short)(((hostshort >> 8) & 0x00FF) | /* GCOVR_EXCL_LINE */
                         ((hostshort << 8) & 0xFF00));
  } else {
    *out_netshort = hostshort; /* GCOVR_EXCL_LINE */
  }
  return 0; /* GCOVR_EXCL_LINE */
}

int c_rest_htonl(unsigned long hostlong,
                 unsigned long *out_netlong) { /* GCOVR_EXCL_LINE */
  int little_endian;
  if (!out_netlong) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_htonl: invalid argument out_netlong is NULL");
    return 1; /* GCOVR_EXCL_LINE */
  }
  is_little_endian(&little_endian); /* GCOVR_EXCL_LINE */
  if (little_endian) {              /* GCOVR_EXCL_LINE */
    *out_netlong =                  /* GCOVR_EXCL_LINE */
        ((hostlong >> 24) & 0x000000FF) |
        ((hostlong >> 8) & 0x0000FF00) | /* GCOVR_EXCL_LINE */
        ((hostlong << 8) & 0x00FF0000) |
        ((hostlong << 24) & 0xFF000000); /* GCOVR_EXCL_LINE */
  } else {
    *out_netlong = hostlong; /* GCOVR_EXCL_LINE */
  }
  return 0; /* GCOVR_EXCL_LINE */
}

int c_rest_ntohs(unsigned short netshort,
                 unsigned short *out_hostshort) { /* GCOVR_EXCL_LINE */
  int rc;
  if (!out_hostshort) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_ntohs: invalid argument out_hostshort is NULL");
    return 1; /* GCOVR_EXCL_LINE */
  }
  rc = c_rest_htons(netshort, out_hostshort); /* GCOVR_EXCL_LINE */
  if (rc != 0) {                              /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_ntohs: c_rest_htons failed with %d", rc);
    return rc; /* GCOVR_EXCL_LINE */
  }
  return 0; /* GCOVR_EXCL_LINE */
}

int c_rest_ntohl(unsigned long netlong,
                 unsigned long *out_hostlong) { /* GCOVR_EXCL_LINE */
  int rc;
  if (!out_hostlong) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_ntohl: invalid argument out_hostlong is NULL");
    return 1; /* GCOVR_EXCL_LINE */
  }
  rc = c_rest_htonl(netlong, out_hostlong); /* GCOVR_EXCL_LINE */
  if (rc != 0) {                            /* GCOVR_EXCL_LINE */
    LOG_DEBUG("c_rest_ntohl: c_rest_htonl failed with %d", rc);
    return rc; /* GCOVR_EXCL_LINE */
  }
  return 0; /* GCOVR_EXCL_LINE */
}
