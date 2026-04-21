#ifndef C_REST_COMPRESSION_H
#define C_REST_COMPRESSION_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Compression types */
typedef enum {
  C_REST_COMPRESSION_NONE = 0,
  C_REST_COMPRESSION_GZIP,
  C_REST_COMPRESSION_BROTLI
} c_rest_compression_type_t;

/** @brief Opaque compression context */
typedef struct c_rest_compression_ctx c_rest_compression_ctx_t;

/**
 * @brief Initialize a compression context.
 * @param ctx Pointer to the context pointer to be initialized.
 * @param type Compression type to use.
 * @return 0 on success, non-zero on error.
 */
int c_rest_compression_ctx_init(c_rest_compression_ctx_t **ctx,
                                c_rest_compression_type_t type);

/**
 * @brief Destroy a compression context.
 * @param ctx The context to destroy.
 * @return 0 on success, non-zero on error.
 */
int c_rest_compression_ctx_destroy(c_rest_compression_ctx_t *ctx);

/**
 * @brief Compress a chunk of data.
 * @param ctx The initialized compression context.
 * @param in_data The input data buffer.
 * @param in_len The length of the input data.
 * @param out_data Pointer to a buffer where the compressed data will be written
 * (allocated by function, must be freed by caller).
 * @param out_len Pointer to a size_t where the length of the compressed data
 * will be stored.
 * @return 0 on success, non-zero on error.
 */
int c_rest_compress_data(c_rest_compression_ctx_t *ctx,
                         const unsigned char *in_data, size_t in_len,
                         unsigned char **out_data, size_t *out_len);

/**
 * @brief Finish compression stream and flush remaining data.
 * @param ctx The initialized compression context.
 * @param out_data Pointer to a buffer where the remaining compressed data will
 * be written (allocated by function, must be freed by caller).
 * @param out_len Pointer to a size_t where the length of the remaining
 * compressed data will be stored.
 * @return 0 on success, non-zero on error.
 */
int c_rest_compress_finish(c_rest_compression_ctx_t *ctx,
                           unsigned char **out_data, size_t *out_len);

/**
 * @brief Helper for one-shot compression of an entire buffer.
 * @param type The compression type.
 * @param in_data The input data buffer.
 * @param in_len The length of the input data.
 * @param out_data Pointer to a buffer where compressed data will be written
 * (must be freed).
 * @param out_len Pointer to a size_t where the length of the compressed data
 * will be stored.
 * @return 0 on success, non-zero on error.
 */
int c_rest_compress_buffer(c_rest_compression_type_t type,
                           const unsigned char *in_data, size_t in_len,
                           unsigned char **out_data, size_t *out_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_COMPRESSION_H */
