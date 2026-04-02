#ifndef C_REST_MULTIPART_H
#define C_REST_MULTIPART_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING

typedef struct c_rest_multipart_parser c_rest_multipart_parser;

typedef int (*c_rest_multipart_cb)(c_rest_multipart_parser *parser);
typedef int (*c_rest_multipart_data_cb)(c_rest_multipart_parser *parser,
                                        const char *at, size_t length);

/** @brief Multipart callbacks */
struct c_rest_multipart_callbacks {
  c_rest_multipart_cb on_part_begin;
  c_rest_multipart_data_cb on_header_field;
  c_rest_multipart_data_cb on_header_value;
  c_rest_multipart_cb on_headers_complete;
  c_rest_multipart_data_cb on_part_data;
  c_rest_multipart_cb on_part_end;
  c_rest_multipart_cb on_body_end;
};

/**
 * @brief Initialize a multipart parser.
 * @param out_parser Pointer to the parser pointer to initialize.
 * @param boundary The boundary string to match.
 * @param callbacks The callbacks to invoke during parsing.
 * @param user_data User data to associate with the parser.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_multipart_parser_init(
    c_rest_multipart_parser **out_parser, const char *boundary,
    const struct c_rest_multipart_callbacks *callbacks, void *user_data);

/**
 * @brief Execute the multipart parser on a chunk of data.
 * @param parser The parser to execute.
 * @param data The data chunk to parse.
 * @param len The length of the data chunk.
 * @param out_parsed Pointer to store the number of bytes parsed.
 * @return 0 on success, non-zero on error.
 */
int c_rest_multipart_parser_execute(c_rest_multipart_parser *parser,
                                    const char *data, size_t len,
                                    size_t *out_parsed);

/**
 * @brief Destroy a multipart parser and free its resources.
 * @param parser The parser to destroy.
 * @return 0 on success, non-zero on error.
 */
int c_rest_multipart_parser_destroy(c_rest_multipart_parser *parser);

/**
 * @brief Clone a multipart parser context.
 * @param parser The parser to clone.
 * @param out_clone Pointer to store the cloned parser.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_multipart_parser_clone(const c_rest_multipart_parser *parser,
                                  c_rest_multipart_parser **out_clone);

void *c_rest_multipart_parser_get_user_data(c_rest_multipart_parser *parser);

#endif /* C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING */

/* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_MULTIPART_H */
