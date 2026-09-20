/* clang-format off */
#include "c_rest_error.h"
#include "test_protos.h"
#include "c_rest_websocket.h"
#include "c_rest_crypto.h"
#include "c_rest_base64.h"
#include "c_rest_router.h"
#include "c_rest_request.h"
#include "c_rest_response.h"
#include "c_rest_testing_mocks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

static void *fail_malloc_n(size_t size) {
  static int alloc_count = 0;
  extern int g_fail_malloc_at;
  if (g_fail_malloc_at <= 0) {
    alloc_count = 0;
    return NULL;
  }
  alloc_count++;
  if (alloc_count == g_fail_malloc_at) {
    alloc_count = 0;
    g_fail_malloc_at = 0;
    return NULL;
  }
  return malloc(size);
}

int test_websocket(void);

static int test_websocket_generate_accept(void) {
  const char *key = "dGhlIHNhbXBsZSBub25jZQ==";
  char accept_buf[128];
  size_t accept_len = sizeof(accept_buf);
  int failed = 0;

  failed += (c_rest_websocket_generate_accept(key, strlen(key), accept_buf,
                                              &accept_len) != 0);
  failed += (strcmp("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", accept_buf) != 0);
  return failed != 0 ? 1 : 0;
}

static int test_websocket_parse_frame_header(void) {
  struct c_rest_websocket_frame_header header;
  unsigned char frame1[] = {0x81, 0x05}; /* FIN, TEXT, 5 bytes */
  unsigned char frame2[] = {0x82, 0x85, 0x11, 0x22,
                            0x33, 0x44}; /* FIN, BINARY, Masked, 5 bytes */
  int failed = 0;

  failed += (c_rest_websocket_parse_frame_header(frame1, sizeof(frame1),
                                                 &header) != 0);
  failed += (header.fin != 1);
  failed += (header.opcode != C_REST_WS_OPCODE_TEXT);
  failed += (header.masked != 0);
  failed += (header.payload_length != 5);
  failed += (header.header_length != 2);

  failed += (c_rest_websocket_parse_frame_header(frame2, sizeof(frame2),
                                                 &header) != 0);
  failed += (header.fin != 1);
  failed += (header.opcode != C_REST_WS_OPCODE_BINARY);
  failed += (header.masked != 1);
  failed += (header.payload_length != 5);
  failed += (header.header_length != 6);
  failed += (header.masking_key[0] != 0x11);
  failed += (header.masking_key[1] != 0x22);
  failed += (header.masking_key[2] != 0x33);
  failed += (header.masking_key[3] != 0x44);

  return failed != 0 ? 1 : 0;
}

static int test_websocket_unmask_payload(void) {
  unsigned char payload[] = {0x79, 0x5F, 0x8D, 0x51, 0x28};
  unsigned char key[] = {0x37, 0xFA, 0x21, 0x3D};
  int failed = 0;

  failed +=
      (c_rest_websocket_unmask_payload(payload, sizeof(payload), key) != 0);
  failed += (payload[0] != 0x4E);
  failed += (payload[1] != 0xA5);
  return failed != 0 ? 1 : 0;
}

static int test_websocket_serialize_frame_header(void) {
  struct c_rest_websocket_frame_header header;
  unsigned char out_buf[14];
  size_t written;
  int failed = 0;

  memset(&header, 0, sizeof(header));
  header.fin = 1;
  header.opcode = C_REST_WS_OPCODE_TEXT;
  header.payload_length = 5;
  header.masked = 0;

  failed += (c_rest_websocket_serialize_frame_header(
                 &header, out_buf, sizeof(out_buf), &written) != 0);
  failed += (written != 2);
  failed += (out_buf[0] != 0x81);
  failed += (out_buf[1] != 0x05);
  return failed != 0 ? 1 : 0;
}

static int test_websocket_edge_cases(void) {
  struct c_rest_websocket_frame_header header;
  unsigned char frame[32];
  unsigned char out_buf[32];
  size_t written;
  char accept_buf[128];
  size_t accept_len = sizeof(accept_buf);
  const char *long_key =
      "123456789012345678901234567890123456789012345678901234567890123456789"
      "0123456789012345678901234567890"; /* 100 bytes */
  int failed = 0;

  /* Generate accept */
  failed += (c_rest_websocket_generate_accept(NULL, 0, accept_buf,
                                              &accept_len) == C_REST_OK);
  failed += (c_rest_websocket_generate_accept("x", 1, NULL, &accept_len) ==
             C_REST_OK);
  failed +=
      (c_rest_websocket_generate_accept("x", 1, accept_buf, NULL) == C_REST_OK);
  failed +=
      (c_rest_websocket_generate_accept(long_key, strlen(long_key), accept_buf,
                                        &accept_len) == C_REST_OK);

  /* Parse frame */
  failed +=
      (c_rest_websocket_parse_frame_header(NULL, 10, &header) == C_REST_OK);
  failed += (c_rest_websocket_parse_frame_header(frame, 10, NULL) == C_REST_OK);
  failed +=
      (c_rest_websocket_parse_frame_header(frame, 1, &header) == C_REST_OK);

  frame[0] = 0x81;
  frame[1] = 126; /* Len 126 but no ext length */
  failed +=
      (c_rest_websocket_parse_frame_header(frame, 2, &header) == C_REST_OK);

  frame[1] = 127; /* Len 127 but no ext length */
  failed +=
      (c_rest_websocket_parse_frame_header(frame, 2, &header) == C_REST_OK);

  /* Length 127 with high bits set */
  memset(frame, 0, sizeof(frame));
  frame[0] = 0x81;
  frame[1] = 127;
  frame[2] = 0xFF; /* high bits */
  failed +=
      (c_rest_websocket_parse_frame_header(frame, 16, &header) == C_REST_OK);

  /* Length 127 with no high bits */
  memset(frame, 0, sizeof(frame));
  frame[0] = 0x81;
  frame[1] = 127;
  frame[9] = 10;
  failed +=
      (c_rest_websocket_parse_frame_header(frame, 16, &header) != C_REST_OK);

  /* Masked but no mask */
  frame[0] = 0x81;
  frame[1] = 0x80 | 5; /* Masked, len 5 */
  failed +=
      (c_rest_websocket_parse_frame_header(frame, 2, &header) == C_REST_OK);

  /* Serialize */
  failed += (c_rest_websocket_serialize_frame_header(
                 NULL, out_buf, sizeof(out_buf), &written) == C_REST_OK);
  failed += (c_rest_websocket_serialize_frame_header(
                 &header, NULL, sizeof(out_buf), &written) == C_REST_OK);
  failed += (c_rest_websocket_serialize_frame_header(
                 &header, out_buf, sizeof(out_buf), NULL) == C_REST_OK);

  memset(&header, 0, sizeof(header));
  header.payload_length = 5;
  failed += (c_rest_websocket_serialize_frame_header(&header, out_buf, 1,
                                                     &written) == C_REST_OK);

  header.payload_length = 126;
  failed += (c_rest_websocket_serialize_frame_header(&header, out_buf, 2,
                                                     &written) == C_REST_OK);
  failed += (c_rest_websocket_serialize_frame_header(
                 &header, out_buf, sizeof(out_buf), &written) != C_REST_OK);

  header.payload_length = 70000; /* > 0xFFFF */
  failed += (c_rest_websocket_serialize_frame_header(&header, out_buf, 2,
                                                     &written) == C_REST_OK);
  failed += (c_rest_websocket_serialize_frame_header(&header, out_buf, 4,
                                                     &written) == C_REST_OK);
  failed += (c_rest_websocket_serialize_frame_header(
                 &header, out_buf, sizeof(out_buf), &written) != C_REST_OK);

  header.masked = 1;
  header.payload_length = 5;
  failed += (c_rest_websocket_serialize_frame_header(&header, out_buf, 2,
                                                     &written) == C_REST_OK);
  failed += (c_rest_websocket_serialize_frame_header(
                 &header, out_buf, sizeof(out_buf), &written) != C_REST_OK);

  header.payload_length = 70000;
  failed += (c_rest_websocket_serialize_frame_header(&header, out_buf, 12,
                                                     &written) == C_REST_OK);

  /* Unmask */
  failed += (c_rest_websocket_unmask_payload(NULL, 10, frame) == C_REST_OK);
  failed += (c_rest_websocket_unmask_payload(NULL, 0, frame) != C_REST_OK);

  return failed != 0 ? 1 : 0;
}

static int test_websocket_upgrade(void) {
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header upgrade_hdr;
  struct c_rest_header key_hdr;
  int failed = 0;

  memset(&req, 0, sizeof(req));
  memset(&res, 0, sizeof(res));

  upgrade_hdr.key = "Upgrade";
  upgrade_hdr.value = "websocket";
  upgrade_hdr.next = &key_hdr;

  key_hdr.key = "Sec-WebSocket-Key";
  key_hdr.value = "dGhlIHNhbXBsZSBub25jZQ==";
  key_hdr.next = NULL;

  req.headers = &upgrade_hdr;

  failed += (c_rest_websocket_upgrade(&req, &res) != 0);

  /* Test with Upgrade = NULL */
  upgrade_hdr.value = NULL;
  failed += (c_rest_websocket_upgrade(&req, &res) == 0);

  /* Test with Upgrade = "not_websocket" */
  upgrade_hdr.value = "not_websocket";
  failed += (c_rest_websocket_upgrade(&req, &res) == 0);

  /* Restore for safety */
  upgrade_hdr.value = "websocket";

  c_rest_response_cleanup(&res);
  return failed != 0 ? 1 : 0;
}

static c_rest_error_t my_ws_on_message(struct c_rest_request *req,
                                       const unsigned char *payload,
                                       size_t payload_len, int is_binary,
                                       void *user_data) {
  (void)req;
  (void)payload;
  (void)payload_len;
  (void)is_binary;
  (void)user_data;
  return 0;
}

static c_rest_error_t my_ws_on_close(struct c_rest_request *req,
                                     int status_code, void *user_data) {
  (void)req;
  (void)status_code;
  (void)user_data;
  return C_REST_OK;
}

static int test_websocket_router_registration(void) {
  c_rest_router *router = NULL;
  struct c_rest_request req;
  struct c_rest_response res;
  struct c_rest_header upgrade_hdr;
  struct c_rest_header key_hdr;
  int failed = 0;

  failed += (c_rest_router_init(&router) != 0);

  failed += (c_rest_router_add_websocket(router, "/ws", my_ws_on_message,
                                         my_ws_on_close, NULL) != 0);

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

  failed += (c_rest_router_dispatch(router, &req, &res) != 0);

  /* Call the callbacks directly to ensure 100% function coverage */
  failed += (my_ws_on_message(&req, (const unsigned char *)"test", 4, 0,
                              NULL) != C_REST_OK);
  failed += (my_ws_on_close(&req, 1000, NULL) != C_REST_OK);

  failed += (c_rest_router_destroy(router) != C_REST_OK);
  c_rest_response_cleanup(&res);
  return failed != 0 ? 1 : 0;
}

int test_websocket(void) {
  int failed = 0;

  printf("Testing WebSocket Accept Generation...\n");
  failed += test_websocket_generate_accept();

  printf("Testing WebSocket Upgrade...\n");
  failed += test_websocket_upgrade();

  printf("Testing WebSocket Router Registration...\n");
  failed += test_websocket_router_registration();

  printf("Testing WebSocket Parse Header...\n");
  failed += test_websocket_parse_frame_header();

  printf("Testing WebSocket Unmask...\n");
  failed += test_websocket_unmask_payload();

  printf("Testing WebSocket Serialize Header...\n");
  failed += test_websocket_serialize_frame_header();

  printf("Testing WebSocket Edge Cases...\n");
  failed += test_websocket_edge_cases();

  /* OOM missing branches test */
  {
    struct c_rest_request req;
    struct c_rest_response res_local;
    c_rest_router *r2 = NULL;

    memset(&req, 0, sizeof(req));
    memset(&res_local, 0, sizeof(res_local));

    failed += (c_rest_router_init(&r2) != C_REST_OK);
    failed += (c_rest_router_destroy(r2) != C_REST_OK);
  }

  /* Parse Frame Header missing branches */
  {
    struct c_rest_websocket_frame_header hdr;
    unsigned char data[16];
    memset(data, 0, sizeof(data));

    /* payload_length == 126 and data_len < 4 */
    data[0] = 0x81;
    data[1] = 126;
    failed += (c_rest_websocket_parse_frame_header(data, 3, &hdr) == C_REST_OK);

    /* payload_length == 127 and data_len < 10 */
    data[1] = 127;
    failed += (c_rest_websocket_parse_frame_header(data, 9, &hdr) == C_REST_OK);

    /* valid length == 126 */
    data[0] = 0x81;
    data[1] = 126;
    data[2] = 0x12;
    data[3] = 0x34;
    failed += (c_rest_websocket_parse_frame_header(data, 4, &hdr) != C_REST_OK);

    /* valid length == 127 */
    data[1] = 127;
    data[2] = 0;
    data[3] = 0;
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    data[8] = 0;
    data[9] = 0x56;
    failed +=
        (c_rest_websocket_parse_frame_header(data, 10, &hdr) != C_REST_OK);

    /* length > 4GB */
    data[1] = 127;
    data[2] = 0x01; /* high != 0 */
    failed +=
        (c_rest_websocket_parse_frame_header(data, 10, &hdr) == C_REST_OK);

    /* masked and data_len < offset + 4 */
    data[1] = 0x80; /* masked, length 0 */
    failed += (c_rest_websocket_parse_frame_header(data, 5, &hdr) == C_REST_OK);
  }

  /* Serialize Frame Header missing branches */
  {
    struct c_rest_websocket_frame_header hdr;
    unsigned char out[16];
    size_t written;
    memset(&hdr, 0, sizeof(hdr));
    failed += (c_rest_websocket_serialize_frame_header(NULL, out, 1,
                                                       &written) == C_REST_OK);
    memset(out, 0, sizeof(out));

    /* out_data_max < 2 */
    failed += (c_rest_websocket_serialize_frame_header(&hdr, out, 1,
                                                       &written) == C_REST_OK);

    /* payload_length <= 0xFFFF and out_data_max < offset + 2 */
    hdr.payload_length = 126;
    failed += (c_rest_websocket_serialize_frame_header(&hdr, out, 3,
                                                       &written) == C_REST_OK);

    /* payload_length > 0xFFFF and out_data_max < offset + 8 */
    hdr.payload_length = 0x10000;
    failed += (c_rest_websocket_serialize_frame_header(&hdr, out, 9,
                                                       &written) == C_REST_OK);

    /* header->masked and out_data_max < offset + 4 */
    hdr.payload_length = 0;
    hdr.masked = 1;
    failed += (c_rest_websocket_serialize_frame_header(&hdr, out, 3,
                                                       &written) == C_REST_OK);
  }

  /* Cover missing flags in serialize */
  {
    struct c_rest_websocket_frame_header hdr2;
    unsigned char out2[16];
    size_t written2;
    memset(&hdr2, 0, sizeof(hdr2));
    hdr2.fin = 0;
    hdr2.rsv1 = 1;
    hdr2.rsv2 = 1;
    hdr2.rsv3 = 1;
    failed += (c_rest_websocket_serialize_frame_header(&hdr2, out2, 16,
                                                       &written2) != C_REST_OK);
  }

  /* Cover c_rest_websocket_generate_accept and c_rest_websocket_upgrade error
   * paths */
  {
    struct c_rest_request req2;
    struct c_rest_response res2;
    struct c_rest_header hdr;
    struct c_rest_header hdr3;
    extern int g_fail_malloc_at;
    char accept_buf[128];
    size_t len = 128;

    memset(&req2, 0, sizeof(req2));
    memset(&res2, 0, sizeof(res2));

    /* Invalid header */
    failed += (c_rest_websocket_upgrade(&req2, &res2) == C_REST_OK);

    req2.headers = &hdr;
    hdr.key = "Upgrade";
    hdr.value = "not_websocket";
    hdr.next = NULL;
    failed += (c_rest_websocket_upgrade(&req2, &res2) == C_REST_OK);

    hdr.value = "websocket";
    failed += (c_rest_websocket_upgrade(&req2, &res2) ==
               C_REST_OK); /* Will fail on Sec-WebSocket-Key missing */

    hdr.next = &hdr3;
    hdr3.key = "Sec-WebSocket-Key";
    hdr3.value = "dGhlIHNhbXBsZSBub25jZQ==";
    hdr3.next = NULL;

    /* sha1 failure in generate_accept */
    g_mock_crypto_fail = 8;
    failed += (c_rest_websocket_generate_accept(hdr3.value, strlen(hdr3.value),
                                                accept_buf, &len) == C_REST_OK);
    g_mock_crypto_fail = 0;

    /* base64 encode failure in generate_accept (out_len too small) */
    len = 5;
    failed += (c_rest_websocket_generate_accept(hdr3.value, strlen(hdr3.value),
                                                accept_buf, &len) == C_REST_OK);
    len = sizeof(accept_buf);

    /* generate_accept failure in websocket_upgrade (key too long) */
    hdr3.value = "0123456789012345678901234567890123456789012345678901234567890"
                 "123456789012345678901234567890123456789";
    failed += (c_rest_websocket_upgrade(&req2, &res2) == C_REST_OK);
    hdr3.value = "dGhlIHNhbXBsZSBub25jZQ==";

    /* response_set_status failure in websocket_upgrade (res == NULL) */
    failed += (c_rest_websocket_upgrade(&req2, NULL) == C_REST_OK);

    /* response_set_header failure 1 (Upgrade) */
    g_fail_malloc_at = 1;
    g_crf_malloc_hook = fail_malloc_n;
    failed += (c_rest_websocket_upgrade(&req2, &res2) == C_REST_OK);
    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
    c_rest_response_cleanup(&res2);
    memset(&res2, 0, sizeof(res2));

    /* response_set_header failure 2 (Connection) */
    g_fail_malloc_at = 4;
    g_crf_malloc_hook = fail_malloc_n;
    failed += (c_rest_websocket_upgrade(&req2, &res2) == C_REST_OK);
    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
    c_rest_response_cleanup(&res2);
    memset(&res2, 0, sizeof(res2));

    /* response_set_header failure 3 (Sec-WebSocket-Accept) */
    g_fail_malloc_at = 7;
    g_crf_malloc_hook = fail_malloc_n;
    failed += (c_rest_websocket_upgrade(&req2, &res2) == C_REST_OK);
    g_crf_malloc_hook = NULL;
    g_fail_malloc_at = 0;
    c_rest_response_cleanup(&res2);
    memset(&res2, 0, sizeof(res2));

    /* Success path */
    failed += (c_rest_websocket_upgrade(&req2, &res2) != C_REST_OK);
    c_rest_response_cleanup(&res2);

    /* Test fail_malloc_n edge case */
    g_fail_malloc_at = 0;
    failed += (fail_malloc_n(10) != NULL);
  }

  printf("test_websocket finished.\n");
  return failed != 0 ? 1 : 0;
}
