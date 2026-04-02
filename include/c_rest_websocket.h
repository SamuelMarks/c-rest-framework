#ifndef C_REST_WEBSOCKET_H
#define C_REST_WEBSOCKET_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WebSocket Opcode values as defined in RFC 6455.
 */
enum c_rest_websocket_opcode {
  C_REST_WS_OPCODE_CONT = 0x0,
  C_REST_WS_OPCODE_TEXT = 0x1,
  C_REST_WS_OPCODE_BINARY = 0x2,
  C_REST_WS_OPCODE_CLOSE = 0x8,
  C_REST_WS_OPCODE_PING = 0x9,
  C_REST_WS_OPCODE_PONG = 0xA
};

/**
 * @brief Status codes for WebSocket close frames.
 */
enum c_rest_websocket_status {
  C_REST_WS_STATUS_NORMAL = 1000,
  C_REST_WS_STATUS_GOING_AWAY = 1001,
  C_REST_WS_STATUS_PROTOCOL_ERROR = 1002,
  C_REST_WS_STATUS_UNSUPPORTED = 1003,
  C_REST_WS_STATUS_NO_STATUS = 1005,
  C_REST_WS_STATUS_ABNORMAL = 1006,
  C_REST_WS_STATUS_INVALID_DATA = 1007,
  C_REST_WS_STATUS_POLICY_VIOLATION = 1008,
  C_REST_WS_STATUS_TOO_LARGE = 1009,
  C_REST_WS_STATUS_EXTENSION_REQ = 1010,
  C_REST_WS_STATUS_UNEXPECTED_COND = 1011
};

/**
 * @brief Represents a parsed WebSocket frame header.
 */
struct c_rest_websocket_frame_header {
  int fin;                             /**< FIN bit (1 if final fragment). */
  int rsv1;                            /**< RSV1 bit. */
  int rsv2;                            /**< RSV2 bit. */
  int rsv3;                            /**< RSV3 bit. */
  enum c_rest_websocket_opcode opcode; /**< Frame opcode. */
  int masked;                   /**< Mask bit (1 if payload is masked). */
  unsigned char masking_key[4]; /**< 32-bit masking key. */
  size_t payload_length;        /**< Length of the payload. */
  size_t header_length;         /**< Total length of the frame header. */
};

struct c_rest_request;
struct c_rest_response;

/**
 * @brief Computes the Sec-WebSocket-Accept header value.
 * @param ws_key The client's Sec-WebSocket-Key.
 * @param ws_key_len The length of the client's key.
 * @param out_accept The output buffer for the accept string.
 * @param out_accept_len Size of the output buffer, updated to actual length.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_websocket_generate_accept(const char *ws_key, size_t ws_key_len,
                                     char *out_accept, size_t *out_accept_len);

/**
 * @brief Process an HTTP request and upgrade it to a WebSocket connection if
 * requested.
 * @param req The incoming HTTP request.
 * @param res The outgoing HTTP response.
 * @return 0 on successful upgrade (status code 101 set), non-zero on failure.
 */
int c_rest_websocket_upgrade(struct c_rest_request *req,
                             struct c_rest_response *res);

/**
 * @brief Parses a WebSocket frame header from a buffer.
 * @param data The incoming data buffer.
 * @param data_len The length of the incoming data.
 * @param out_header The parsed frame header.
 * @return 0 on success (full header parsed), non-zero on failure or incomplete.
 */
int c_rest_websocket_parse_frame_header(
    const unsigned char *data, size_t data_len,
    struct c_rest_websocket_frame_header *out_header);

/**
 * @brief Unmasks WebSocket payload data in-place.
 * @param payload The payload data.
 * @param payload_len The length of the payload.
 * @param masking_key The 4-byte masking key.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_websocket_unmask_payload(unsigned char *payload, size_t payload_len,
                                    const unsigned char masking_key[4]);

/**
 * @brief Serializes a WebSocket frame header to a buffer.
 * @param header The frame header struct.
 * @param out_data The output buffer.
 * @param out_data_max Valid size of out_data.
 * @param out_written The number of bytes written to out_data.
 * @return 0 on success, non-zero on failure.
 */
int c_rest_websocket_serialize_frame_header(
    const struct c_rest_websocket_frame_header *header, unsigned char *out_data,
    size_t out_data_max, size_t *out_written);

/* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* C_REST_WEBSOCKET_H */
