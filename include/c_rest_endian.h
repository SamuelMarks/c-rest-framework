#ifndef C_REST_ENDIAN_H
#define C_REST_ENDIAN_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Converts a 16-bit unsigned integer from host byte order to network
 * byte order.
 *
 * @param hostshort The 16-bit integer in host byte order.
 * @param out_netshort Pointer to hold the converted 16-bit integer in network
 * byte order.
 * @return 0 on success, non-zero on error.
 */
int c_rest_htons(unsigned short hostshort, unsigned short *out_netshort);

/**
 * @brief Converts a 32-bit unsigned integer from host byte order to network
 * byte order.
 *
 * @param hostlong The 32-bit integer in host byte order.
 * @param out_netlong Pointer to hold the converted 32-bit integer in network
 * byte order.
 * @return 0 on success, non-zero on error.
 */
int c_rest_htonl(unsigned long hostlong, unsigned long *out_netlong);

/**
 * @brief Converts a 16-bit unsigned integer from network byte order to host
 * byte order.
 *
 * @param netshort The 16-bit integer in network byte order.
 * @param out_hostshort Pointer to hold the converted 16-bit integer in host
 * byte order.
 * @return 0 on success, non-zero on error.
 */
int c_rest_ntohs(unsigned short netshort, unsigned short *out_hostshort);

/**
 * @brief Converts a 32-bit unsigned integer from network byte order to host
 * byte order.
 *
 * @param netlong The 32-bit integer in network byte order.
 * @param out_hostlong Pointer to hold the converted 32-bit integer in host byte
 * order.
 * @return 0 on success, non-zero on error.
 */
int c_rest_ntohl(unsigned long netlong, unsigned long *out_hostlong);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_ENDIAN_H */
