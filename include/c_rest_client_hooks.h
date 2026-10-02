/**
 * @file c_rest_client_hooks.h
 * @brief HTTP client hooks.
 */
#ifndef C_REST_CLIENT_HOOKS_H
#define C_REST_CLIENT_HOOKS_H

/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_export.h"
#include <c_abstract_http/c_abstract_http.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Hooks for HTTP client. */
struct c_rest_client_hooks {
  /** @brief Init hook */
  int (*init)(struct HttpClient *client);
  /** @brief Request hook */
  int (*request)(struct HttpTransportContext *ctx, const char *url,
                 const char *method, const struct HttpHeader *headers,
                 size_t header_count, const char *body, size_t body_len,
                 struct HttpResponse *out_response);
  /** @brief Async request hook */
  int (*request_async)(struct HttpTransportContext *ctx, const char *url,
                       const char *method, const struct HttpHeader *headers,
                       size_t header_count, const char *body, size_t body_len,
                       void (*callback)(struct HttpResponse *res, void *data),
                       void *user_data);
  /** @brief Destroy hook */
  int (*destroy)(struct HttpTransportContext *ctx);
};

/** @brief Set HTTP client hooks.
 * @param hooks Pointer to hooks. */
C_REST_EXPORT extern void
c_rest_client_set_hooks(const struct c_rest_client_hooks *hooks);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_CLIENT_HOOKS_H */
