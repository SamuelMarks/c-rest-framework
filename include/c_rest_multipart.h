#ifndef C_REST_MULTIPART_H
#define C_REST_MULTIPART_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct c_rest_multipart_parser c_rest_multipart_parser;

typedef void (*c_rest_multipart_on_part_data_cb)(
    c_rest_multipart_parser *parser, const char *data, size_t len);
typedef void (*c_rest_multipart_on_part_end_cb)(
    c_rest_multipart_parser *parser);

/** @brief Multipart callbacks */
struct c_rest_multipart_callbacks {
  /** @brief Callback for part data */
  c_rest_multipart_on_part_data_cb on_part_data;
  /** @brief Callback for part end */
  c_rest_multipart_on_part_end_cb on_part_end;
};

int c_rest_multipart_parser_init(
    c_rest_multipart_parser **out_parser, const char *boundary,
    const struct c_rest_multipart_callbacks *callbacks, void *user_data);
size_t c_rest_multipart_parser_execute(c_rest_multipart_parser *parser,
                                       const char *data, size_t len);
void c_rest_multipart_parser_destroy(c_rest_multipart_parser *parser);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_MULTIPART_H */
