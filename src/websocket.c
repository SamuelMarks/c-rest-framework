/* clang-format off */
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

int c_rest_websocket_upgrade(struct c_rest_request *req,
                             struct c_rest_response *res) {
  const char *upgrade = NULL;
  const char *ws_key = NULL;
  char accept_buf[128];
  size_t accept_len = sizeof(accept_buf);
  int ret;

  if (!req || !res)
    return 1;

  ret = c_rest_request_get_header(req, "Upgrade", &upgrade);
  if (ret != 0 || !upgrade || strcmp(upgrade, "websocket") != 0) {
    return 1;
  }

  ret = c_rest_request_get_header(req, "Sec-WebSocket-Key", &ws_key);
  if (ret != 0 || !ws_key) {
    return 1;
  }

  ret = c_rest_websocket_generate_accept(ws_key, strlen(ws_key), accept_buf,
                                         &accept_len);
  if (ret != 0) {
    return 1;
  }

  c_rest_response_set_status(res, 101);
  c_rest_response_set_header(res, "Upgrade", "websocket");
  c_rest_response_set_header(res, "Connection", "Upgrade");
  c_rest_response_set_header(res, "Sec-WebSocket-Accept", accept_buf);

  return 0;
}

int c_rest_websocket_generate_accept(const char *ws_key, size_t ws_key_len,
                                     char *out_accept, size_t *out_accept_len) {
  char concat_buf[128];
  unsigned char sha1_hash[20];
  size_t concat_len;
  int res;

  if (!ws_key || !out_accept || !out_accept_len) {
    return 1;
  }

  concat_len = ws_key_len + C_REST_WS_MAGIC_LEN;
  if (concat_len >= sizeof(concat_buf)) {
    return 1; /* Key too long */
  }

  /* Strict C89: Variables at top */
#if defined(_MSC_VER)
  /* safe CRT */
  memcpy_s(concat_buf, sizeof(concat_buf), ws_key, ws_key_len);
  memcpy_s(concat_buf + ws_key_len, sizeof(concat_buf) - ws_key_len,
           C_REST_WS_MAGIC_STRING, C_REST_WS_MAGIC_LEN);
#else
  memcpy(concat_buf, ws_key, ws_key_len);
  memcpy(concat_buf + ws_key_len, C_REST_WS_MAGIC_STRING, C_REST_WS_MAGIC_LEN);
#endif

  res = c_rest_sha1((const unsigned char *)concat_buf, concat_len, sha1_hash);
  if (res != 0) {
    return res;
  }

  res = c_rest_base64_encode(sha1_hash, sizeof(sha1_hash), out_accept,
                             out_accept_len);
  if (res != 0) {
    return res;
  }

  return 0;
}

int c_rest_websocket_parse_frame_header(
    const unsigned char *data, size_t data_len,
    struct c_rest_websocket_frame_header *out_header) {
  unsigned char b1, b2;
  size_t offset = 2;
  unsigned short ext_len_16;
  /* 64-bit lengths would need abstraction, keeping it basic to C89 but standard
   * WS allows 64-bit */
  /* Since c-rest-framework focuses on embedded/C89, let's just handle
   * reasonable sizes */

  if (!data || data_len < 2 || !out_header) {
    return 1; /* Need at least 2 bytes */
  }

  b1 = data[0];
  b2 = data[1];

  out_header->fin = (b1 & 0x80) != 0;
  out_header->rsv1 = (b1 & 0x40) != 0;
  out_header->rsv2 = (b1 & 0x20) != 0;
  out_header->rsv3 = (b1 & 0x10) != 0;
  out_header->opcode = (enum c_rest_websocket_opcode)(b1 & 0x0F);

  out_header->masked = (b2 & 0x80) != 0;
  out_header->payload_length = (size_t)(b2 & 0x7F);

  if (out_header->payload_length == 126) {
    if (data_len < 4) {
      return 1; /* Need 2 more bytes */
    }
    ext_len_16 = (unsigned short)((data[2] << 8) | data[3]);
    out_header->payload_length = ext_len_16;
    offset += 2;
  } else if (out_header->payload_length == 127) {
    if (data_len < 10) {
      return 1; /* Need 8 more bytes */
    }
    /* C89 has no standard 64-bit integer, but size_t might be 32-bit. */
    /* If payload length exceeds size_t max, we'll return error. */
    /* For now, just grab the lower 32 bits if size_t is 32-bit, but properly
     * parse 64. */
    {
      unsigned long high =
          ((unsigned long)data[2] << 24) | ((unsigned long)data[3] << 16) |
          ((unsigned long)data[4] << 8) | ((unsigned long)data[5]);
      unsigned long low =
          ((unsigned long)data[6] << 24) | ((unsigned long)data[7] << 16) |
          ((unsigned long)data[8] << 8) | ((unsigned long)data[9]);

      if (high != 0) {
        /* We don't support > 4GB payloads on 32-bit systems, and we keep it
         * safe. */
        if (sizeof(size_t) <= 4) {
          return 1;
        }
        /* If size_t is 64-bit, we could do this, but C89 lacks standard
           uint64_t. To stay perfectly standard C89 without assuming unsigned
           long long exists: */
        return 1; /* Payload too large for strictly standard C89 fallback. */
      }
      out_header->payload_length = (size_t)low;
      offset += 8;
    }
  }

  if (out_header->masked) {
    if (data_len < offset + 4) {
      return 1; /* Need 4 bytes for masking key */
    }
    out_header->masking_key[0] = data[offset++];
    out_header->masking_key[1] = data[offset++];
    out_header->masking_key[2] = data[offset++];
    out_header->masking_key[3] = data[offset++];
  } else {
    out_header->masking_key[0] = 0;
    out_header->masking_key[1] = 0;
    out_header->masking_key[2] = 0;
    out_header->masking_key[3] = 0;
  }

  out_header->header_length = offset;
  return 0;
}

int c_rest_websocket_unmask_payload(unsigned char *payload, size_t payload_len,
                                    const unsigned char masking_key[4]) {
  size_t i;
  if (!payload && payload_len > 0) {
    return 1;
  }
  for (i = 0; i < payload_len; i++) {
    payload[i] ^= masking_key[i % 4];
  }
  return 0;
}

int c_rest_websocket_serialize_frame_header(
    const struct c_rest_websocket_frame_header *header, unsigned char *out_data,
    size_t out_data_max, size_t *out_written) {
  size_t offset = 0;
  unsigned char b1, b2;

  if (!header || !out_data || !out_written) {
    return 1;
  }

  if (out_data_max < 2) {
    return 1;
  }

  b1 = (unsigned char)header->opcode;
  if (header->fin)
    b1 |= 0x80;
  if (header->rsv1)
    b1 |= 0x40;
  if (header->rsv2)
    b1 |= 0x20;
  if (header->rsv3)
    b1 |= 0x10;

  out_data[offset++] = b1;

  b2 = 0;
  if (header->masked) {
    b2 |= 0x80;
  }

  if (header->payload_length < 126) {
    b2 |= (unsigned char)header->payload_length;
    out_data[offset++] = b2;
  } else if (header->payload_length <= 0xFFFF) {
    b2 |= 126;
    out_data[offset++] = b2;
    if (out_data_max < offset + 2)
      return 1;
    out_data[offset++] = (unsigned char)((header->payload_length >> 8) & 0xFF);
    out_data[offset++] = (unsigned char)(header->payload_length & 0xFF);
  } else {
    /* Assume 32-bit payload for C89 safety */
    b2 |= 127;
    out_data[offset++] = b2;
    if (out_data_max < offset + 8)
      return 1;
    out_data[offset++] = 0;
    out_data[offset++] = 0;
    out_data[offset++] = 0;
    out_data[offset++] = 0;
    out_data[offset++] = (unsigned char)((header->payload_length >> 24) & 0xFF);
    out_data[offset++] = (unsigned char)((header->payload_length >> 16) & 0xFF);
    out_data[offset++] = (unsigned char)((header->payload_length >> 8) & 0xFF);
    out_data[offset++] = (unsigned char)(header->payload_length & 0xFF);
  }

  if (header->masked) {
    if (out_data_max < offset + 4)
      return 1;
    out_data[offset++] = header->masking_key[0];
    out_data[offset++] = header->masking_key[1];
    out_data[offset++] = header->masking_key[2];
    out_data[offset++] = header->masking_key[3];
  }

  *out_written = offset;
  return 0;
}
