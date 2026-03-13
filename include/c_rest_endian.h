#ifndef C_REST_ENDIAN_H
#define C_REST_ENDIAN_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */
#ifdef __cplusplus
extern "C" {
#endif

unsigned short c_rest_htons(unsigned short hostshort);
unsigned long c_rest_htonl(unsigned long hostlong);
unsigned short c_rest_ntohs(unsigned short netshort);
unsigned long c_rest_ntohl(unsigned long netlong);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_ENDIAN_H */
