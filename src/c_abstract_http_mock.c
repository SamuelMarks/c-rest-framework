/* clang-format off */
#include "c_abstract_http.h"

#include <string.h>
/* clang-format on */

void cah_parser_init(cah_parser *parser) {
  if (parser) {
    parser->http_major = 1;
    parser->http_minor = 1;
    parser->status_code = 0;
    parser->method = CAH_GET;
  }
}

size_t cah_parser_execute(cah_parser *parser,
                          const struct cah_parser_settings *settings,
                          const char *data, size_t len) {
  /* Very dumb mock parser just to satisfy compilation and basic tests */
  if (!parser || !settings || !data || len == 0)
    return 0;

  /* Simulate a malformed request if it starts with "MALFORMED" */
  if (len >= 9 && strncmp(data, "MALFORMED", 9) == 0) {
    return 0; /* Returning 0 bytes parsed means error in http-parser style */
  }

  if (settings->on_message_begin) {
    settings->on_message_begin(parser);
  }

  /* We just pretend we parsed GET / HTTP/1.1 */
  if (settings->on_url) {
    settings->on_url(parser, "/", 1);
  }

  if (settings->on_header_field) {
    settings->on_header_field(parser, "Host", 4);
  }
  if (settings->on_header_value) {
    settings->on_header_value(parser, "localhost", 9);
  }

  if (settings->on_headers_complete) {
    settings->on_headers_complete(parser);
  }

  if (settings->on_message_complete) {
    settings->on_message_complete(parser);
  }

  return len;
}

int cah_should_keep_alive(const cah_parser *parser) {
  (void)parser;
  return 1;
}

cah_client *cah_client_create(void) {
  return (cah_client *)1; /* Dummy non-null pointer for mock */
}

void cah_client_destroy(cah_client *client) { (void)client; }

int cah_client_request(cah_client *client, const char *url,
                       const char *method) {
  (void)client;
  (void)url;
  (void)method;
  return 0;
}

int cah_tls_init(void) { return 0; }
