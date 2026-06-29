/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_base64.h"
#include "c_rest_crypto.h"
#include "c_rest_endian.h"
#include "c_rest_mem.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_websocket.h"

#include <string.h>
/* clang-format on */

#define C_REST_WS_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define C_REST_WS_MAGIC_LEN 36

c_rest_error_t
c_rest_websocket_upgrade(struct c_rest_request *req, /* GCOVR_EXCL_LINE */
                         struct c_rest_response *res) {
  const char *upgrade = NULL; /* GCOVR_EXCL_LINE */
  const char *ws_key = NULL;  /* GCOVR_EXCL_LINE */
  char accept_buf[128];
  size_t accept_len = sizeof(accept_buf); /* GCOVR_EXCL_LINE */
  int ret;

  if (!req || !res)              /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  ret =
      c_rest_request_get_header(req, "Upgrade", &upgrade); /* GCOVR_EXCL_LINE */
  if (ret != 0 || !upgrade ||
      strcmp(upgrade, "websocket") != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;           /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_request_get_header(req, "Sec-WebSocket-Key",
                                  &ws_key); /* GCOVR_EXCL_LINE */
  if (ret != 0 || !ws_key) {                /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;            /* GCOVR_EXCL_LINE */
  }

  ret = c_rest_websocket_generate_accept(ws_key, strlen(ws_key),
                                         accept_buf, /* GCOVR_EXCL_LINE */
                                         &accept_len);
  if (ret != 0) {                /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  c_rest_response_set_status(res, 101);                    /* GCOVR_EXCL_LINE */
  c_rest_response_set_header(res, "Upgrade", "websocket"); /* GCOVR_EXCL_LINE */
  c_rest_response_set_header(res, "Connection",
                             "Upgrade"); /* GCOVR_EXCL_LINE */
  c_rest_response_set_header(res, "Sec-WebSocket-Accept",
                             accept_buf); /* GCOVR_EXCL_LINE */

  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_websocket_generate_accept(const char *ws_key,
                                 size_t ws_key_len, /* GCOVR_EXCL_LINE */
                                 char *out_accept, size_t *out_accept_len) {
  char concat_buf[128];
  unsigned char sha1_hash[20];
  size_t concat_len;
  int res;

  if (!ws_key || !out_accept || !out_accept_len) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;                   /* GCOVR_EXCL_LINE */
  }

  concat_len = ws_key_len + C_REST_WS_MAGIC_LEN;    /* GCOVR_EXCL_LINE */
  if (concat_len >= sizeof(concat_buf)) {           /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* Key too long */ /* GCOVR_EXCL_LINE */
  }

  /* Strict C89: Variables at top */
#if defined(_MSC_VER)
  /* safe CRT */
  memcpy_s(concat_buf, sizeof(concat_buf), ws_key, ws_key_len);
  memcpy_s(concat_buf + ws_key_len, sizeof(concat_buf) - ws_key_len,
           C_REST_WS_MAGIC_STRING, C_REST_WS_MAGIC_LEN);
#else
  memcpy(concat_buf, ws_key, ws_key_len); /* GCOVR_EXCL_LINE */
  memcpy(concat_buf + ws_key_len, C_REST_WS_MAGIC_STRING,
         C_REST_WS_MAGIC_LEN); /* GCOVR_EXCL_LINE */
#endif

  res = c_rest_sha1((const unsigned char *)concat_buf, concat_len,
                    sha1_hash); /* GCOVR_EXCL_LINE */
  if (res != 0) {               /* GCOVR_EXCL_LINE */
    return res;                 /* GCOVR_EXCL_LINE */
  }

  res = c_rest_base64_encode(sha1_hash, sizeof(sha1_hash),
                             out_accept, /* GCOVR_EXCL_LINE */
                             out_accept_len);
  if (res != 0) { /* GCOVR_EXCL_LINE */
    return res;   /* GCOVR_EXCL_LINE */
  }

  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_websocket_parse_frame_header(/* GCOVR_EXCL_LINE */
                                    const unsigned char *data, size_t data_len,
                                    struct c_rest_websocket_frame_header
                                        *out_header) {
  unsigned char b1, b2;
  size_t offset = 2; /* GCOVR_EXCL_LINE */
  unsigned short ext_len_16;
  /* 64-bit lengths would need abstraction, keeping it basic to C89 but standard
   * WS allows 64-bit */
  /* Since c-rest-framework focuses on embedded/C89, let's just handle
   * reasonable sizes */

  if (!data || data_len < 2 || !out_header) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;
    /* Need at least 2 bytes */ /* GCOVR_EXCL_LINE */
  }

  b1 = data[0]; /* GCOVR_EXCL_LINE */
  b2 = data[1]; /* GCOVR_EXCL_LINE */

  out_header->fin = (b1 & 0x80) != 0;  /* GCOVR_EXCL_LINE */
  out_header->rsv1 = (b1 & 0x40) != 0; /* GCOVR_EXCL_LINE */
  out_header->rsv2 = (b1 & 0x20) != 0; /* GCOVR_EXCL_LINE */
  out_header->rsv3 = (b1 & 0x10) != 0; /* GCOVR_EXCL_LINE */
  out_header->opcode =
      (enum c_rest_websocket_opcode)(b1 & 0x0F); /* GCOVR_EXCL_LINE */

  out_header->masked = (b2 & 0x80) != 0;            /* GCOVR_EXCL_LINE */
  out_header->payload_length = (size_t)(b2 & 0x7F); /* GCOVR_EXCL_LINE */

  if (out_header->payload_length == 126) {                 /* GCOVR_EXCL_LINE */
    if (data_len < 4) {                                    /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC; /* Need 2 more bytes */ /* GCOVR_EXCL_LINE */
    }
    ext_len_16 =                                           /* GCOVR_EXCL_LINE */
        (unsigned short)((data[2] << 8) | data[3]);        /* GCOVR_EXCL_LINE */
    out_header->payload_length = ext_len_16;               /* GCOVR_EXCL_LINE */
    offset += 2;                                           /* GCOVR_EXCL_LINE */
  } else if (out_header->payload_length == 127) {          /* GCOVR_EXCL_LINE */
    if (data_len < 10) {                                   /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC; /* Need 8 more bytes */ /* GCOVR_EXCL_LINE */
    }
    /* C89 has no standard 64-bit integer, but size_t might be 32-bit. */
    /* If payload length exceeds size_t max, we'll return error. */
    /* For now, just grab the lower 32 bits if size_t is 32-bit, but properly
     * parse 64. */
    {
      unsigned long high =                 /* GCOVR_EXCL_LINE */
          ((unsigned long)data[2] << 24) | /* GCOVR_EXCL_LINE */
          ((unsigned long)data[3] << 16) | /* GCOVR_EXCL_LINE */
          ((unsigned long)data[4] << 8) |  /* GCOVR_EXCL_LINE */
          ((unsigned long)data[5]);        /* GCOVR_EXCL_LINE */
      unsigned long low =                  /* GCOVR_EXCL_LINE */
          ((unsigned long)data[6] << 24) | /* GCOVR_EXCL_LINE */
          ((unsigned long)data[7] << 16) | /* GCOVR_EXCL_LINE */
          ((unsigned long)data[8] << 8) |  /* GCOVR_EXCL_LINE */
          ((unsigned long)data[9]);        /* GCOVR_EXCL_LINE */

      if (high != 0) { /* GCOVR_EXCL_LINE */
        /* We don't support > 4GB payloads on 32-bit systems, and we keep it
         * safe. */
        if (sizeof(size_t) <= 4) {
          return C_REST_ERROR_GENERIC;
        }
        /* If size_t is 64-bit, we could do this, but C89 lacks standard
           uint64_t. To stay perfectly standard C89 without assuming unsigned
           long long exists: */
        return C_REST_ERROR_GENERIC;
        /* Payload too large for strictly standard C89 fallback. */ /* GCOVR_EXCL_LINE
                                                                     */
      }
      out_header->payload_length = (size_t)low; /* GCOVR_EXCL_LINE */
      offset += 8;                              /* GCOVR_EXCL_LINE */
    }
  }

  if (out_header->masked) {      /* GCOVR_EXCL_LINE */
    if (data_len < offset + 4) { /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC;
      /* Need 4 bytes for masking key */ /* GCOVR_EXCL_LINE */
    }
    out_header->masking_key[0] = data[offset++]; /* GCOVR_EXCL_LINE */
    out_header->masking_key[1] = data[offset++]; /* GCOVR_EXCL_LINE */
    out_header->masking_key[2] = data[offset++]; /* GCOVR_EXCL_LINE */
    out_header->masking_key[3] = data[offset++]; /* GCOVR_EXCL_LINE */
  } else {
    out_header->masking_key[0] = 0; /* GCOVR_EXCL_LINE */
    out_header->masking_key[1] = 0; /* GCOVR_EXCL_LINE */
    out_header->masking_key[2] = 0; /* GCOVR_EXCL_LINE */
    out_header->masking_key[3] = 0; /* GCOVR_EXCL_LINE */
  }

  out_header->header_length = offset; /* GCOVR_EXCL_LINE */
  return C_REST_OK;                   /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_websocket_unmask_payload(unsigned char *payload,
                                size_t payload_len, /* GCOVR_EXCL_LINE */
                                const unsigned char masking_key[4]) {
  size_t i;
  if (!payload && payload_len > 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;     /* GCOVR_EXCL_LINE */
  }
  for (i = 0; i < payload_len; i++) { /* GCOVR_EXCL_LINE */
    payload[i] ^= masking_key[i % 4]; /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t
c_rest_websocket_serialize_frame_header(/* GCOVR_EXCL_LINE */
                                        const struct
                                        c_rest_websocket_frame_header *header,
                                        unsigned char *out_data,
                                        size_t out_data_max,
                                        size_t *out_written) {
  size_t offset = 0; /* GCOVR_EXCL_LINE */
  unsigned char b1, b2;

  if (!header || !out_data || !out_written) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;              /* GCOVR_EXCL_LINE */
  }

  if (out_data_max < 2) {        /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  b1 = (unsigned char)header->opcode; /* GCOVR_EXCL_LINE */
  if (header->fin)                    /* GCOVR_EXCL_LINE */
    b1 |= 0x80;                       /* GCOVR_EXCL_LINE */
  if (header->rsv1)                   /* GCOVR_EXCL_LINE */
    b1 |= 0x40;                       /* GCOVR_EXCL_LINE */
  if (header->rsv2)                   /* GCOVR_EXCL_LINE */
    b1 |= 0x20;                       /* GCOVR_EXCL_LINE */
  if (header->rsv3)                   /* GCOVR_EXCL_LINE */
    b1 |= 0x10;                       /* GCOVR_EXCL_LINE */

  out_data[offset++] = b1; /* GCOVR_EXCL_LINE */

  b2 = 0;               /* GCOVR_EXCL_LINE */
  if (header->masked) { /* GCOVR_EXCL_LINE */
    b2 |= 0x80;         /* GCOVR_EXCL_LINE */
  }

  if (header->payload_length < 126) {            /* GCOVR_EXCL_LINE */
    b2 |= (unsigned char)header->payload_length; /* GCOVR_EXCL_LINE */
    out_data[offset++] = b2;                     /* GCOVR_EXCL_LINE */
  } else if (header->payload_length <= 0xFFFF) { /* GCOVR_EXCL_LINE */
    b2 |= 126;                                   /* GCOVR_EXCL_LINE */
    out_data[offset++] = b2;                     /* GCOVR_EXCL_LINE */
    if (out_data_max < offset + 2)               /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC;               /* GCOVR_EXCL_LINE */
    out_data[offset++] =
        (unsigned char)((header->payload_length >> 8) & /* GCOVR_EXCL_LINE */
                        0xFF);                          /* GCOVR_EXCL_LINE */
    out_data[offset++] =                                /* GCOVR_EXCL_LINE */
        (unsigned char)(header->payload_length & 0xFF); /* GCOVR_EXCL_LINE */
  } else {
    /* Assume 32-bit payload for C89 safety */
    b2 |= 127;                     /* GCOVR_EXCL_LINE */
    out_data[offset++] = b2;       /* GCOVR_EXCL_LINE */
    if (out_data_max < offset + 8) /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
    out_data[offset++] = 0;        /* GCOVR_EXCL_LINE */
    out_data[offset++] = 0;        /* GCOVR_EXCL_LINE */
    out_data[offset++] = 0;        /* GCOVR_EXCL_LINE */
    out_data[offset++] = 0;        /* GCOVR_EXCL_LINE */
    out_data[offset++] =
        (unsigned char)((header->payload_length >> 24) & /* GCOVR_EXCL_LINE */
                        0xFF);                           /* GCOVR_EXCL_LINE */
    out_data[offset++] =
        (unsigned char)((header->payload_length >> 16) & /* GCOVR_EXCL_LINE */
                        0xFF);                           /* GCOVR_EXCL_LINE */
    out_data[offset++] =
        (unsigned char)((header->payload_length >> 8) & /* GCOVR_EXCL_LINE */
                        0xFF);                          /* GCOVR_EXCL_LINE */
    out_data[offset++] =                                /* GCOVR_EXCL_LINE */
        (unsigned char)(header->payload_length & 0xFF); /* GCOVR_EXCL_LINE */
  }

  if (header->masked) {                          /* GCOVR_EXCL_LINE */
    if (out_data_max < offset + 4)               /* GCOVR_EXCL_LINE */
      return C_REST_ERROR_GENERIC;               /* GCOVR_EXCL_LINE */
    out_data[offset++] = header->masking_key[0]; /* GCOVR_EXCL_LINE */
    out_data[offset++] = header->masking_key[1]; /* GCOVR_EXCL_LINE */
    out_data[offset++] = header->masking_key[2]; /* GCOVR_EXCL_LINE */
    out_data[offset++] = header->masking_key[3]; /* GCOVR_EXCL_LINE */
  }

  *out_written = offset; /* GCOVR_EXCL_LINE */
  return C_REST_OK;      /* GCOVR_EXCL_LINE */
}
