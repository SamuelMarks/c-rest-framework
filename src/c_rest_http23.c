/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_mem.h"
#include "c_rest_http23.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include <stdio.h>
#include <string.h>
/* clang-format on */

/** @brief RFC 9113 Section 3.4 HTTP/2 client connection preface (24 octets). */
static const unsigned char H2_CLIENT_PREFACE[24] = {
    'P', 'R', 'I',  ' ',  '*',  ' ',  'H', 'T', 'T',  'P',  '/',  '2',
    '.', '0', '\r', '\n', '\r', '\n', 'S', 'M', '\r', '\n', '\r', '\n'};

/**
 * @brief Internal state for the HTTP/2 & HTTP/3 context.
 */
struct c_rest_http23_ctx {
  c_rest_protocol_t protocol;     /**< Active HTTP/2 or HTTP/3 protocol */
  int is_ready;                   /**< Non-zero once a full request arrived */
  char *method_buf;               /**< Owned :method string buffer */
  char *path_buf;                 /**< Owned :path string buffer */
  struct c_rest_request *request; /**< Parsed framework request */
};

static NO_DISCARD c_rest_error_t h23_set_owned_str(char **slot, const char *val,
                                                   size_t len) {
  char *copy = NULL;
  c_rest_error_t rc;

  rc = C_REST_MALLOC(len + 1, &copy);
  if (rc != C_REST_OK)
    return rc;
  if (len > 0)
    memcpy(copy, val, len);
  copy[len] = '\0';
  if (*slot)
    C_REST_FREE(*slot);
  *slot = copy;
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t h23_add_header(struct c_rest_request *req,
                                                const char *k, size_t klen,
                                                const char *v, size_t vlen) {
  struct c_rest_header *hdr = NULL;
  c_rest_error_t rc;

  rc = C_REST_MALLOC(sizeof(struct c_rest_header), &hdr);
  if (rc != C_REST_OK)
    return rc;
  hdr->key = NULL;
  hdr->value = NULL;
  hdr->next = NULL;

  rc = C_REST_MALLOC(klen + 1, &hdr->key);
  if (rc != C_REST_OK) {
    C_REST_FREE(hdr);
    return rc;
  }
  rc = C_REST_MALLOC(vlen + 1, &hdr->value);
  if (rc != C_REST_OK) {
    C_REST_FREE(hdr->key);
    C_REST_FREE(hdr);
    return rc;
  }
  memcpy(hdr->key, k, klen);
  hdr->key[klen] = '\0';
  memcpy(hdr->value, v, vlen);
  hdr->value[vlen] = '\0';
  hdr->next = req->headers;
  req->headers = hdr;
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t h23_append_body(struct c_rest_request *req,
                                                 const unsigned char *data,
                                                 size_t len) {
  char *nbuf = NULL;
  c_rest_error_t rc;

  if (len == 0)
    return C_REST_OK;
  rc = C_REST_REALLOC(req->body, req->body_len + len + 1, &nbuf);
  if (rc != C_REST_OK)
    return rc;
  memcpy(nbuf + req->body_len, data, len);
  req->body_len += len;
  nbuf[req->body_len] = '\0';
  req->body = nbuf;
  return C_REST_OK;
}

static NO_DISCARD c_rest_error_t h23_parse_header_block(
    c_rest_http23_ctx_t *ctx, const unsigned char *buf, size_t len) {
  size_t i = 0;
  c_rest_error_t rc;

  while (i < len) {
    unsigned char b = buf[i];
    if ((b & 0x80U) != 0) {
      if (b == 0x82U) {
        rc = h23_set_owned_str(&ctx->method_buf, "GET", 3);
        if (rc != C_REST_OK)
          return rc;
        ctx->request->method = ctx->method_buf;
      } else if (b == 0x83U) {
        rc = h23_set_owned_str(&ctx->method_buf, "POST", 4);
        if (rc != C_REST_OK)
          return rc;
        ctx->request->method = ctx->method_buf;
      } else if (b == 0x84U) {
        rc = h23_set_owned_str(&ctx->path_buf, "/", 1);
        if (rc != C_REST_OK)
          return rc;
        ctx->request->path = ctx->path_buf;
      } else if (b == 0x85U) {
        rc = h23_set_owned_str(&ctx->path_buf, "/index.html", 11);
        if (rc != C_REST_OK)
          return rc;
        ctx->request->path = ctx->path_buf;
      } else if (b == 0x86U) {
        ctx->request->scheme = "http";
      } else if (b == 0x87U) {
        ctx->request->scheme = "https";
      } else {
        return C_REST_ERROR_PARSE;
      }
      i++;
    } else {
      size_t line_start = i;
      size_t line_end;
      size_t colon;
      size_t val_start;
      size_t val_len;
      const char *line;

      while (i < len && buf[i] != '\n')
        i++;
      line_end = i;
      if (i < len)
        i++;
      if (line_end > line_start && buf[line_end - 1] == '\r')
        line_end--;
      if (line_end == line_start)
        continue;

      line = (const char *)(buf + line_start);
      colon = (line[0] == ':') ? 1 : 0;
      while (line_start + colon < line_end && line[colon] != ':')
        colon++;
      if (line_start + colon >= line_end)
        return C_REST_ERROR_PARSE;

      val_start = colon + 1;
      while (line_start + val_start < line_end && line[val_start] == ' ')
        val_start++;
      val_len = (line_end - line_start) - val_start;

      if (colon == 7 && memcmp(line, ":method", 7) == 0) {
        rc = h23_set_owned_str(&ctx->method_buf, line + val_start, val_len);
        if (rc != C_REST_OK)
          return rc;
        ctx->request->method = ctx->method_buf;
      } else if (colon == 5 && memcmp(line, ":path", 5) == 0) {
        rc = h23_set_owned_str(&ctx->path_buf, line + val_start, val_len);
        if (rc != C_REST_OK)
          return rc;
        ctx->request->path = ctx->path_buf;
      } else {
        rc = h23_add_header(ctx->request, line, colon, line + val_start,
                            val_len);
        if (rc != C_REST_OK)
          return rc;
      }
    }
  }

  return C_REST_OK;
}

c_rest_error_t c_rest_http23_ctx_init(c_rest_protocol_t protocol,
                                      c_rest_http23_ctx_t **out_ctx) {
  c_rest_http23_ctx_t *ctx = NULL;
  c_rest_error_t rc;

  if (!out_ctx) {
    return C_REST_ERROR_INVALID_ARG;
  }

  if (protocol != C_REST_PROTOCOL_HTTP2 && protocol != C_REST_PROTOCOL_HTTP3) {
    return C_REST_ERROR_INVALID_ARG;
  }

  rc = C_REST_MALLOC(sizeof(c_rest_http23_ctx_t), &ctx);
  if (rc != C_REST_OK) {
    return rc;
  }

  ctx->protocol = protocol;
  ctx->is_ready = 0;
  ctx->method_buf = NULL;
  ctx->path_buf = NULL;
  ctx->request = NULL;

  rc = C_REST_MALLOC(sizeof(struct c_rest_request), &ctx->request);
  if (rc != C_REST_OK) {
    C_REST_FREE(ctx);
    return rc;
  }
  memset(ctx->request, 0, sizeof(struct c_rest_request));

  *out_ctx = ctx;
  return C_REST_OK;
}

c_rest_error_t c_rest_http23_ctx_destroy(c_rest_http23_ctx_t *ctx) {
  c_rest_error_t rc;
  if (!ctx) {
    return C_REST_ERROR_INVALID_ARG;
  }

  rc = c_rest_request_cleanup(ctx->request);
  if (rc != C_REST_OK) {
    return rc;
  }
  C_REST_FREE(ctx->request);
  if (ctx->method_buf)
    C_REST_FREE(ctx->method_buf);
  if (ctx->path_buf)
    C_REST_FREE(ctx->path_buf);

  C_REST_FREE(ctx);
  return C_REST_OK;
}

c_rest_error_t c_rest_http23_process(c_rest_http23_ctx_t *ctx, const char *data,
                                     size_t len, size_t *out_consumed) {
  const unsigned char *u;
  size_t pos = 0;
  c_rest_error_t rc;

  if (!ctx || !data || !out_consumed) {
    return C_REST_ERROR_INVALID_ARG;
  }

  *out_consumed = 0;
  if (len == 0) {
    return C_REST_OK;
  }

  u = (const unsigned char *)data;

  if (ctx->protocol == C_REST_PROTOCOL_HTTP2) {
    if (len >= 24 && memcmp(u, H2_CLIENT_PREFACE, 24) == 0) {
      pos = 24;
    }

    while (len - pos >= 9) {
      size_t frame_len = ((size_t)u[pos] << 16) | ((size_t)u[pos + 1] << 8) |
                         (size_t)u[pos + 2];
      unsigned char frame_type = u[pos + 3];
      unsigned char flags = u[pos + 4];
      unsigned long stream_id = (((unsigned long)(u[pos + 5] & 0x7FU)) << 24) |
                                ((unsigned long)u[pos + 6] << 16) |
                                ((unsigned long)u[pos + 7] << 8) |
                                (unsigned long)u[pos + 8];
      const unsigned char *payload;

      if (len - pos - 9 < frame_len) {
        break;
      }

      payload = u + pos + 9;
      if (frame_type == 0x04U) {
        if (stream_id != 0 || (frame_len % 6) != 0) {
          return C_REST_ERROR_PARSE;
        }
      } else if (frame_type == 0x01U) {
        if (stream_id == 0) {
          return C_REST_ERROR_PARSE;
        }
        rc = h23_parse_header_block(ctx, payload, frame_len);
        if (rc != C_REST_OK) {
          return rc;
        }
        if ((flags & 0x01U) != 0 && ctx->method_buf && ctx->path_buf) {
          ctx->is_ready = 1;
        }
      } else if (frame_type == 0x00U) {
        if (stream_id == 0) {
          return C_REST_ERROR_PARSE;
        }
        rc = h23_append_body(ctx->request, payload, frame_len);
        if (rc != C_REST_OK) {
          return rc;
        }
        if ((flags & 0x01U) != 0 && ctx->method_buf && ctx->path_buf) {
          ctx->is_ready = 1;
        }
      }

      pos += 9 + frame_len;
    }
  } else {
    while (pos < len) {
      unsigned char ftype = u[pos];
      size_t flen;
      size_t hdr_bytes;

      if ((ftype & 0xC0U) != 0) {
        return C_REST_ERROR_PARSE;
      }
      if (pos + 1 >= len) {
        break;
      }
      if ((u[pos + 1] & 0xC0U) == 0x00U) {
        flen = (size_t)(u[pos + 1] & 0x3FU);
        hdr_bytes = 2;
      } else if ((u[pos + 1] & 0xC0U) == 0x40U) {
        if (pos + 2 >= len) {
          break;
        }
        flen = (((size_t)(u[pos + 1] & 0x3FU)) << 8) | (size_t)u[pos + 2];
        hdr_bytes = 3;
      } else {
        return C_REST_ERROR_PARSE;
      }

      if (len - pos - hdr_bytes < flen) {
        break;
      }

      if (ftype == 0x01U) {
        rc = h23_parse_header_block(ctx, u + pos + hdr_bytes, flen);
        if (rc != C_REST_OK) {
          return rc;
        }
        if (ctx->method_buf && ctx->path_buf) {
          ctx->is_ready = 1;
        }
      } else if (ftype == 0x00U) {
        rc = h23_append_body(ctx->request, u + pos + hdr_bytes, flen);
        if (rc != C_REST_OK) {
          return rc;
        }
        if (ctx->method_buf && ctx->path_buf) {
          ctx->is_ready = 1;
        }
      }

      pos += hdr_bytes + flen;
    }
  }

  *out_consumed = pos;
  return C_REST_OK;
}

c_rest_error_t c_rest_http23_is_request_ready(c_rest_http23_ctx_t *ctx,
                                              int *out_ready) {
  if (!ctx || !out_ready) {
    return C_REST_ERROR_INVALID_ARG;
  }

  *out_ready = ctx->is_ready;
  return C_REST_OK;
}

c_rest_error_t c_rest_http23_get_request(c_rest_http23_ctx_t *ctx,
                                         struct c_rest_request **out_request) {
  if (!ctx || !out_request) {
    return C_REST_ERROR_INVALID_ARG;
  }

  if (!ctx->is_ready) {
    return C_REST_ERROR_NOT_FOUND;
  }

  *out_request = ctx->request;
  return C_REST_OK;
}

c_rest_error_t c_rest_http23_format_response(c_rest_http23_ctx_t *ctx,
                                             struct c_rest_response *response,
                                             char **out_buffer,
                                             size_t *out_len) {
  const char *body;
  size_t body_len;
  char status_hdr[32];
  size_t hdr_len;
  size_t extra_hdr_len = 0;
  struct c_rest_header *h;
  unsigned char *buf = NULL;
  size_t total_len;
  size_t off = 0;
  int status_code;
  c_rest_error_t rc;

  if (!ctx || !response || !out_buffer || !out_len) {
    return C_REST_ERROR_INVALID_ARG;
  }

  body = response->body;
  body_len = (body != NULL) ? response->body_len : 0;
  status_code = (response->status_code >= 100 && response->status_code <= 599)
                    ? response->status_code
                    : 200;

#if defined(_MSC_VER)
  sprintf_s(status_hdr, sizeof(status_hdr), ":status: %d\r\n", status_code);
#else
  sprintf(status_hdr, ":status: %d\r\n", status_code);
#endif
  hdr_len = strlen(status_hdr);

  for (h = response->headers; h != NULL; h = h->next) {
    if (h->key && h->value) {
      extra_hdr_len += strlen(h->key) + 2 + strlen(h->value) + 2;
    }
  }
  hdr_len += extra_hdr_len;

  if (ctx->protocol == C_REST_PROTOCOL_HTTP2) {
    total_len = 9 + hdr_len + (body_len > 0 ? (9 + body_len) : 0);
    rc = C_REST_MALLOC(total_len + 1, &buf);
    if (rc != C_REST_OK) {
      return rc;
    }

    buf[0] = (unsigned char)((hdr_len >> 16) & 0xFFU);
    buf[1] = (unsigned char)((hdr_len >> 8) & 0xFFU);
    buf[2] = (unsigned char)(hdr_len & 0xFFU);
    buf[3] = 0x01U;
    buf[4] = (unsigned char)(0x04U | (body_len == 0 ? 0x01U : 0x00U));
    buf[5] = 0x00U;
    buf[6] = 0x00U;
    buf[7] = 0x00U;
    buf[8] = 0x01U;
    off = 9;
  } else {
    if (hdr_len > 16383 || body_len > 16383) {
      return C_REST_ERROR_INVALID_ARG;
    }
    total_len = 3 + hdr_len + (body_len > 0 ? (3 + body_len) : 0);
    rc = C_REST_MALLOC(total_len + 1, &buf);
    if (rc != C_REST_OK) {
      return rc;
    }

    buf[0] = 0x01U;
    buf[1] = (unsigned char)(0x40U | ((hdr_len >> 8) & 0x3FU));
    buf[2] = (unsigned char)(hdr_len & 0xFFU);
    off = 3;
  }

  memcpy(buf + off, status_hdr, strlen(status_hdr));
  off += strlen(status_hdr);
  for (h = response->headers; h != NULL; h = h->next) {
    if (h->key && h->value) {
      size_t kl = strlen(h->key);
      size_t vl = strlen(h->value);
      memcpy(buf + off, h->key, kl);
      off += kl;
      buf[off++] = ':';
      buf[off++] = ' ';
      memcpy(buf + off, h->value, vl);
      off += vl;
      buf[off++] = '\r';
      buf[off++] = '\n';
    }
  }

  if (body_len > 0) {
    if (ctx->protocol == C_REST_PROTOCOL_HTTP2) {
      buf[off++] = (unsigned char)((body_len >> 16) & 0xFFU);
      buf[off++] = (unsigned char)((body_len >> 8) & 0xFFU);
      buf[off++] = (unsigned char)(body_len & 0xFFU);
      buf[off++] = 0x00U;
      buf[off++] = 0x01U;
      buf[off++] = 0x00U;
      buf[off++] = 0x00U;
      buf[off++] = 0x00U;
      buf[off++] = 0x01U;
    } else {
      buf[off++] = 0x00U;
      buf[off++] = (unsigned char)(0x40U | ((body_len >> 8) & 0x3FU));
      buf[off++] = (unsigned char)(body_len & 0xFFU);
    }
    memcpy(buf + off, body, body_len);
    off += body_len;
  }

  buf[off] = '\0';
  *out_buffer = (char *)buf;
  *out_len = off;
  return C_REST_OK;
}
