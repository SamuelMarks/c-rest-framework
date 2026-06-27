/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_multipart.h"
#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"
#include "c_rest_mem.h"
/* clang-format on */

#ifdef C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING

enum c_rest_multipart_state {
  s_start,
  s_start_boundary,
  s_header_field_start,
  s_header_field,
  s_header_value_start,
  s_header_value,
  s_header_value_almost_done,
  s_headers_almost_done,
  s_part_data_start,
  s_part_data,
  s_part_data_almost_boundary,
  s_boundary,
  s_part_data_almost_end,
  s_end,
  s_epilogue
};

struct c_rest_multipart_parser {
  char *boundary;
  size_t boundary_length;
  size_t index;
  size_t boundary_match_index;
  int state;
  struct c_rest_multipart_callbacks callbacks;
  void *user_data;
  char flags;
};

#define CR '\r'
#define LF '\n'

c_rest_error_t c_rest_multipart_parser_init(
    c_rest_multipart_parser **out_parser, const char *boundary,
    const struct c_rest_multipart_callbacks *callbacks, void *user_data) {
  c_rest_multipart_parser *p;
  size_t blen;

  if (!out_parser || !boundary)  /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(sizeof(c_rest_multipart_parser), &p) !=
      0)                         /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  blen = strlen(boundary);
  p->boundary_length = blen;
  if (C_REST_MALLOC(blen + 1, &p->boundary) != 0) { /* GCOVR_EXCL_LINE */
    C_REST_FREE(p);                                 /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                    /* GCOVR_EXCL_LINE */
  }

#if defined(_MSC_VER)
  strcpy_s(p->boundary, blen + 1, boundary);
#else
  strcpy(p->boundary, boundary);
#endif

  if (callbacks) { /* GCOVR_EXCL_LINE */
    p->callbacks = *callbacks;
  } else {
    memset(&p->callbacks, 0, sizeof(p->callbacks)); /* GCOVR_EXCL_LINE */
  }

  p->user_data = user_data;
  p->state = s_start;
  p->index = 0;
  p->boundary_match_index = 0;
  p->flags = 0;
  *out_parser = p;
  return C_REST_OK;
}

c_rest_error_t c_rest_multipart_parser_execute(c_rest_multipart_parser *parser,
                                               const char *data, size_t len,
                                               size_t *out_parsed) {
  size_t i = 0;
  size_t mark = 0;
  char c;

  if (!out_parsed)               /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  *out_parsed = 0;
  if (!parser || !data)          /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  for (i = 0; i < len; i++) {
    c = data[i];
    switch (parser->state) { /* GCOVR_EXCL_LINE */
    case s_start:
      if (c == '-' && parser->index == 0) { /* GCOVR_EXCL_LINE */
        parser->index++;
      } else if (c == '-' && parser->index == 1) { /* GCOVR_EXCL_LINE */
        parser->index++;
        parser->state = s_start_boundary;
      } else {
        return C_REST_ERROR_GENERIC; /* Invalid start */ /* GCOVR_EXCL_LINE */
      }
      break;
    case s_start_boundary:
      if (parser->index - 2 < parser->boundary_length) {
        if (c != parser->boundary[parser->index - 2]) { /* GCOVR_EXCL_LINE */
          return C_REST_ERROR_GENERIC;
          /* Invalid boundary */ /* GCOVR_EXCL_LINE */
        }
        parser->index++;
      } else if (c == CR) { /* GCOVR_EXCL_LINE */
        parser->state = s_header_field_start;
      } else {
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
      }
      break;
    case s_header_field_start:
      if (c == LF) {                           /* GCOVR_EXCL_LINE */
        if (parser->callbacks.on_part_begin) { /* GCOVR_EXCL_LINE */
          parser->callbacks.on_part_begin(parser);
        }
        parser->state = s_header_field;
        mark = i + 1;
      } else {
        return C_REST_ERROR_GENERIC; /* Expected LF */ /* GCOVR_EXCL_LINE */
      }
      break;
    case s_header_field:
      if (c == CR) {
        parser->state = s_headers_almost_done;
      } else if (c == ':') {
        if (parser->callbacks.on_header_field &&
            i > mark) { /* GCOVR_EXCL_LINE */
          parser->callbacks.on_header_field(parser, data + mark, i - mark);
        }
        parser->state = s_header_value_start;
      }
      break;
    case s_headers_almost_done:
      if (c == LF) {                                 /* GCOVR_EXCL_LINE */
        if (parser->callbacks.on_headers_complete) { /* GCOVR_EXCL_LINE */
          parser->callbacks.on_headers_complete(parser);
        }
        parser->state = s_part_data_start;
      } else {
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
      }
      break;
    case s_header_value_start:
      if (c == ' ') {
        mark = i + 1;
      } else {
        mark = i;
        parser->state = s_header_value;
        i--;
      }
      break;
    case s_header_value:
      if (c == CR) {
        if (parser->callbacks.on_header_value &&
            i > mark) { /* GCOVR_EXCL_LINE */
          parser->callbacks.on_header_value(parser, data + mark, i - mark);
        }
        parser->state = s_header_value_almost_done;
      }
      break;
    case s_header_value_almost_done:
      if (c == LF) { /* GCOVR_EXCL_LINE */
        parser->state = s_header_field;
        mark = i + 1;
      } else {
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
      }
      break;
    case s_part_data_start:
      parser->state = s_part_data;
      mark = i;
      /* fall through */
    case s_part_data:
      if (c == CR) {
        if (parser->callbacks.on_part_data && i > mark) { /* GCOVR_EXCL_LINE */
          parser->callbacks.on_part_data(parser, data + mark, i - mark);
        }
        parser->state = s_part_data_almost_boundary;
      }
      break;
    case s_part_data_almost_boundary:
      if (c == LF) { /* GCOVR_EXCL_LINE */
        parser->state = s_boundary;
        parser->boundary_match_index = 0;
      } else {
        if (parser->callbacks.on_part_data) { /* GCOVR_EXCL_LINE */
          parser->callbacks.on_part_data(parser, "\\r",
                                         1); /* GCOVR_EXCL_LINE */
        }
        parser->state = s_part_data; /* GCOVR_EXCL_LINE */
        mark = i;                    /* GCOVR_EXCL_LINE */
        i--;                         /* GCOVR_EXCL_LINE */
      }
      break;
    case s_boundary:
      if (parser->boundary_match_index == 0) {
        if (c == '-') { /* GCOVR_EXCL_LINE */
          parser->boundary_match_index++;
        } else {
          if (parser->callbacks.on_part_data) { /* GCOVR_EXCL_LINE */
            parser->callbacks.on_part_data(parser, "\\r\\n",
                                           2); /* GCOVR_EXCL_LINE */
          }
          parser->state = s_part_data; /* GCOVR_EXCL_LINE */
          mark = i;                    /* GCOVR_EXCL_LINE */
          i--;                         /* GCOVR_EXCL_LINE */
        }
      } else if (parser->boundary_match_index == 1) {
        if (c == '-') { /* GCOVR_EXCL_LINE */
          parser->boundary_match_index++;
        } else {
          if (parser->callbacks.on_part_data) { /* GCOVR_EXCL_LINE */
            parser->callbacks.on_part_data(parser, "\\r\\n-",
                                           3); /* GCOVR_EXCL_LINE */
          }
          parser->state = s_part_data; /* GCOVR_EXCL_LINE */
          mark = i;                    /* GCOVR_EXCL_LINE */
          i--;                         /* GCOVR_EXCL_LINE */
        }
      } else if (parser->boundary_match_index - 2 < parser->boundary_length) {
        if (c == parser->boundary[parser->boundary_match_index -
                                  2]) { /* GCOVR_EXCL_LINE */
          parser->boundary_match_index++;
        } else {
          if (parser->callbacks.on_part_data) { /* GCOVR_EXCL_LINE */
            parser->callbacks.on_part_data(parser, "\\r\\n--",
                                           4);      /* GCOVR_EXCL_LINE */
            if (parser->boundary_match_index > 2) { /* GCOVR_EXCL_LINE */
              parser->callbacks.on_part_data(
                  parser, parser->boundary,          /* GCOVR_EXCL_LINE */
                  parser->boundary_match_index - 2); /* GCOVR_EXCL_LINE */
            }
          }
          parser->state = s_part_data; /* GCOVR_EXCL_LINE */
          mark = i;                    /* GCOVR_EXCL_LINE */
          i--;                         /* GCOVR_EXCL_LINE */
        }
      } else if (parser->boundary_match_index - 2 ==
                 parser->boundary_length) { /* GCOVR_EXCL_LINE */
        if (c == CR) {
          if (parser->callbacks.on_part_end) { /* GCOVR_EXCL_LINE */
            parser->callbacks.on_part_end(parser);
          }
          parser->state = s_header_field_start;
        } else if (c == '-') { /* GCOVR_EXCL_LINE */
          parser->state = s_part_data_almost_end;
        } else {
          return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
        }
      }
      break;
    case s_part_data_almost_end:
      if (c == '-') { /* GCOVR_EXCL_LINE */
        parser->state = s_end;
      } else {
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
      }
      break;
    case s_end:
      if (c == CR) {                         /* GCOVR_EXCL_LINE */
        if (parser->callbacks.on_part_end) { /* GCOVR_EXCL_LINE */
          parser->callbacks.on_part_end(parser);
        }
        if (parser->callbacks.on_body_end) { /* GCOVR_EXCL_LINE */
          parser->callbacks.on_body_end(parser);
        }
        parser->state = s_epilogue;
      } else {
        return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
      }
      break;
    case s_epilogue:
      break;
    }
  }

  if (parser->state == s_part_data && mark < len) { /* GCOVR_EXCL_LINE */
    if (parser->callbacks.on_part_data) {           /* GCOVR_EXCL_LINE */
      parser->callbacks.on_part_data(parser, data + mark,
                                     len - mark); /* GCOVR_EXCL_LINE */
    }
  }

  *out_parsed = len;
  return C_REST_OK;
}

c_rest_error_t c_rest_multipart_parser_clone(
    const c_rest_multipart_parser *parser, /* GCOVR_EXCL_LINE */
    c_rest_multipart_parser **out_clone) {
  c_rest_multipart_parser *p;
  if (!parser || !out_clone)     /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (C_REST_MALLOC(sizeof(c_rest_multipart_parser), &p) !=
      0)                         /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  *p = *parser;           /* GCOVR_EXCL_LINE */
  if (parser->boundary) { /* GCOVR_EXCL_LINE */
    if (C_REST_MALLOC(parser->boundary_length + 1, &p->boundary) !=
        0) {                       /* GCOVR_EXCL_LINE */
      C_REST_FREE(p);              /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
    }
#if defined(_MSC_VER)
    strcpy_s(p->boundary, parser->boundary_length + 1, parser->boundary);
#else
    strcpy(p->boundary, parser->boundary); /* GCOVR_EXCL_LINE */
#endif
  }

  *out_clone = p;   /* GCOVR_EXCL_LINE */
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_multipart_parser_destroy(c_rest_multipart_parser *parser) {
  if (!parser)                   /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  if (parser->boundary)          /* GCOVR_EXCL_LINE */
    C_REST_FREE(parser->boundary);
  C_REST_FREE(parser);
  return C_REST_OK;
}

c_rest_error_t c_rest_multipart_parser_get_user_data(
    c_rest_multipart_parser *parser, /* GCOVR_EXCL_LINE */
    void **out_user_data) {
  if (!out_user_data)                                 /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                      /* GCOVR_EXCL_LINE */
  *out_user_data = parser ? parser->user_data : NULL; /* GCOVR_EXCL_LINE */
  return C_REST_OK;                                   /* GCOVR_EXCL_LINE */
}

#endif /* C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING */

typedef int c_rest_multipart_dummy_declaration;
