/* clang-format off */
#include "test_protos.h"
#include "test_protos.h"
#include "c_rest_websocket.h"
#include "c_rest_crypto.h"
#include "c_rest_base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_websocket(void);

static int test_websocket_generate_accept(void) {
  const char *key = "dGhlIHNhbXBsZSBub25jZQ==";
  char accept_buf[128];
  size_t accept_len = sizeof(accept_buf);
  int res;

  res = c_rest_websocket_generate_accept(key, strlen(key), accept_buf,
                                         &accept_len);
  if (res != 0) {
    printf("test_websocket_generate_accept failed to generate accept\n");
    return 1;
  }
  if (strcmp("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", accept_buf) != 0) {
    printf("test_websocket_generate_accept mismatch: %s\n", accept_buf);
    return 1;
  }
  return 0;
}

static int test_websocket_parse_frame_header(void) {
  struct c_rest_websocket_frame_header header;
  unsigned char frame1[] = {0x81, 0x05}; /* FIN, TEXT, 5 bytes */
  unsigned char frame2[] = {0x82, 0x85, 0x11, 0x22,
                            0x33, 0x44}; /* FIN, BINARY, Masked, 5 bytes */
  int res;

  res = c_rest_websocket_parse_frame_header(frame1, sizeof(frame1), &header);
  if (res != 0 || header.fin != 1 || header.opcode != C_REST_WS_OPCODE_TEXT ||
      header.masked != 0 || header.payload_length != 5 ||
      header.header_length != 2) {
    printf("test_websocket_parse_frame_header failed on frame1\n");
    return 1;
  }

  res = c_rest_websocket_parse_frame_header(frame2, sizeof(frame2), &header);
  if (res != 0 || header.fin != 1 || header.opcode != C_REST_WS_OPCODE_BINARY ||
      header.masked != 1 || header.payload_length != 5 ||
      header.header_length != 6) {
    printf("test_websocket_parse_frame_header failed on frame2\n");
    return 1;
  }
  if (header.masking_key[0] != 0x11 || header.masking_key[1] != 0x22 ||
      header.masking_key[2] != 0x33 || header.masking_key[3] != 0x44) {
    printf("test_websocket_parse_frame_header masking key mismatch\n");
    return 1;
  }
  return 0;
}

static int test_websocket_unmask_payload(void) {
  unsigned char payload[] = {0x79, 0x5F, 0x8D, 0x51, 0x28};
  unsigned char key[] = {0x37, 0xFA, 0x21, 0x3D};
  int res;

  res = c_rest_websocket_unmask_payload(payload, sizeof(payload), key);
  if (res != 0)
    return 1;
  if (payload[0] != 0x4E || payload[1] != 0xA5) {
    printf("test_websocket_unmask_payload mismatch\n");
    return 1;
  }
  return 0;
}

static int test_websocket_serialize_frame_header(void) {
  struct c_rest_websocket_frame_header header;
  unsigned char out_buf[14];
  size_t written;
  int res;

  memset(&header, 0, sizeof(header));
  header.fin = 1;
  header.opcode = C_REST_WS_OPCODE_TEXT;
  header.payload_length = 5;
  header.masked = 0;

  res = c_rest_websocket_serialize_frame_header(&header, out_buf,
                                                sizeof(out_buf), &written);
  if (res != 0 || written != 2 || out_buf[0] != 0x81 || out_buf[1] != 0x05) {
    printf("test_websocket_serialize_frame_header failed\n");
    return 1;
  }
  return 0;
}

#include "c_rest_request.h"
#include "c_rest_response.h"

static int test_websocket_upgrade(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header upgrade_hdr;
  struct c_rest_header key_hdr;
  int ret;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  upgrade_hdr.key = "Upgrade";
  upgrade_hdr.value = "websocket";
  upgrade_hdr.next = &key_hdr;

  key_hdr.key = "Sec-WebSocket-Key";
  key_hdr.value = "dGhlIHNhbXBsZSBub25jZQ==";
  key_hdr.next = NULL;

  req.headers = &upgrade_hdr;

  ret = c_rest_websocket_upgrade(&req, &res);
  if (ret != 0) {
    printf("test_websocket_upgrade failed\n");
    return 1;
  }

  if (res.status_code != 101) {
    printf("test_websocket_upgrade: wrong status code\n");
    return 1;
  }

  /* Simple check of headers_sent / cleanup */
  c_rest_response_cleanup(&res);
  return 0;
}

#include "c_rest_router.h"
/* clang-format on */

static int my_ws_on_message(struct c_rest_request *req,
                            const unsigned char *payload, size_t payload_len,
                            int is_binary, void *user_data) {
  (void)req;
  (void)payload;
  (void)payload_len;
  (void)is_binary;
  (void)user_data;
  return 0;
}

static void my_ws_on_close(struct c_rest_request *req, int status_code,
                           void *user_data) {
  (void)req;
  (void)status_code;
  (void)user_data;
}

static int test_websocket_router_registration(void) {
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header upgrade_hdr;
  struct c_rest_header key_hdr;
  int ret;

  ret = c_rest_router_init(&router);
  if (ret != 0)
    return 1;

  ret = c_rest_router_add_websocket(router, "/ws", my_ws_on_message,
                                    my_ws_on_close, NULL);
  if (ret != 0) {
    c_rest_router_destroy(router);
    return 1;
  }

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  req.method = "GET";
  req.path = "/ws";

  upgrade_hdr.key = "Upgrade";
  upgrade_hdr.value = "websocket";
  upgrade_hdr.next = &key_hdr;

  key_hdr.key = "Sec-WebSocket-Key";
  key_hdr.value = "dGhlIHNhbXBsZSBub25jZQ==";
  key_hdr.next = NULL;

  req.headers = &upgrade_hdr;

  ret = c_rest_router_dispatch(router, &req, &res);
  if (ret != 0) {
    c_rest_router_destroy(router);
    return 1;
  }

  if (res.status_code != 101) {
    printf("test_websocket_router_registration: expected 101, got %d\n",
           res.status_code);
    c_rest_response_cleanup(&res);
    c_rest_router_destroy(router);
    return 1;
  }

  c_rest_response_cleanup(&res);
  c_rest_router_destroy(router);
  return 0;
}

int test_websocket(void) {
  int res = 0;

  printf("Testing WebSocket Accept Generation...\n");
  res = test_websocket_generate_accept();
  if (res != 0)
    return res;

  printf("Testing WebSocket Upgrade...\n");
  res = test_websocket_upgrade();
  if (res != 0)
    return res;

  printf("Testing WebSocket Router Registration...\n");
  res = test_websocket_router_registration();
  if (res != 0)
    return res;

  printf("Testing WebSocket Parse Header...\n");
  res = test_websocket_parse_frame_header();
  if (res != 0)
    return res;

  printf("Testing WebSocket Unmask...\n");
  res = test_websocket_unmask_payload();
  if (res != 0)
    return res;

  printf("Testing WebSocket Serialize Header...\n");
  res = test_websocket_serialize_frame_header();
  if (res != 0)
    return res;

  printf("test_websocket finished.\n");
  return 0;
}
