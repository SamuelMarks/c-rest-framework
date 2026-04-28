/* clang-format off */
#include "c_rest_request.h"
#include "c_rest_str_utils.h"
#include <parson.h>

#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"

#include <ctype.h>

static int c_rest_stricmp(const char *s1, const char *s2) {
  while (*s1 && *s2) {
    int c1 = tolower((unsigned char)*s1);
    int c2 = tolower((unsigned char)*s2);
    if (c1 != c2) {
      return c1 - c2;
    }
    s1++;
    s2++;
  }
  return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int c_rest_request_get_header(struct c_rest_request *req, const char *key,
                              const char **out_value) {
  struct c_rest_header *h;
  if (!req || !key || !out_value) {
    return 1;
  }
  for (h = req->headers; h != NULL; h = h->next) {
    if (c_rest_stricmp(h->key, key) == 0) {
      *out_value = h->value;
      return 0;
    }
  }
  return 1;
}

static int parse_cookies_if_needed(struct c_rest_request *req) {
  const char *cookie_str;
  const char *p;
  if (!req || req->cookies) {
    return 0;
  }
  if (c_rest_request_get_header(req, "Cookie", &cookie_str) != 0) {
    return 1;
  }

  p = cookie_str;
  while (*p) {
    const char *eq;
    const char *semi;
    size_t key_len, val_len;
    struct c_rest_header *cp;

    while (*p == ' ')
      p++; /* Skip leading spaces */
    if (*p == '\0')
      break;

    eq = strchr(p, '=');
    semi = strchr(p, ';');

    if (!semi) {
      semi = p + strlen(p);
    }

    if (!eq || eq > semi) {
      /* Invalid cookie format, skip to next */
      p = semi;
      if (*p == ';')
        p++;
      continue;
    }

    key_len = (size_t)(eq - p);
    val_len = (size_t)(semi - eq - 1);

    if (C_REST_MALLOC(sizeof(struct c_rest_header), (void **)&cp) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); cp = NULL; }
    if (cp) {
      if (C_REST_MALLOC(key_len + 1, (void **)&cp->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); cp->key = NULL; }
      if (C_REST_MALLOC(val_len + 1, (void **)&cp->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); cp->value = NULL; }
      if (cp->key && cp->value) {
        memcpy(cp->key, p, key_len);
        cp->key[key_len] = '\0';
        memcpy(cp->value, eq + 1, val_len);
        cp->value[val_len] = '\0';

        cp->next = req->cookies;
        req->cookies = cp;
      } else {
        if (cp->key)
          C_REST_FREE((void *)(cp->key));
        if (cp->value)
          C_REST_FREE((void *)(cp->value));
        C_REST_FREE((void *)(cp));
      }
    }

    p = semi;
    if (*p == ';')
      p++;
  }
  return 0;
}

int c_rest_request_get_cookie(struct c_rest_request *req, const char *key,
                              const char **out_value) {
  struct c_rest_header *cp;
  if (!req || !key || !out_value) {
    return 1;
  }
  parse_cookies_if_needed(req);
  for (cp = req->cookies; cp != NULL; cp = cp->next) {
    if (strcmp(cp->key, key) == 0) {
      *out_value = cp->value;
      return 0;
    }
  }
  return 1;
}

static int parse_query_if_needed(struct c_rest_request *req) {
  const char *p;
  if (!req || !req->query || req->query_params) {
    return 0; /* Already parsed or no query string */
  }

  p = req->query;
  while (*p) {
    const char *eq = strchr(p, '=');
    const char *amp = strchr(p, '&');
    size_t key_len, val_len;
    struct c_rest_header *qp;

    if (!amp) {
      amp = p + strlen(p);
    }

    if (C_REST_MALLOC(sizeof(struct c_rest_header), (void **)&qp) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp = NULL; }
    if (!qp) {
      break; /* Out of memory */
    }
    qp->key = NULL;
    qp->value = NULL;
    qp->next = NULL;

    if (eq && eq < amp) {
      key_len = (size_t)(eq - p);
      val_len = (size_t)(amp - eq - 1);

      if (C_REST_MALLOC(key_len + 1, (void **)&qp->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->key = NULL; }
      if (qp->key) {
        c_rest_url_decode(qp->key, p, key_len);
      }

      if (C_REST_MALLOC(val_len + 1, (void **)&qp->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->value = NULL; }
      if (qp->value) {
        c_rest_url_decode(qp->value, eq + 1, val_len);
      }
    } else {
      key_len = (size_t)(amp - p);
      if (C_REST_MALLOC(key_len + 1, (void **)&qp->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->key = NULL; }
      if (qp->key) {
        c_rest_url_decode(qp->key, p, key_len);
      }
      if (C_REST_MALLOC(1, (void **)&qp->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->value = NULL; }
      if (qp->value) {
        qp->value[0] = '\0';
      }
    }

    if (!qp->key || !qp->value) {
      C_REST_FREE((void *)(qp->key));
      C_REST_FREE((void *)(qp->value));
      C_REST_FREE((void *)(qp));
    } else {
      qp->next = req->query_params;
      req->query_params = qp;
    }

    if (*amp == '\0') {
      break;
    }
    p = amp + 1;
  }
  return 0;
}

int c_rest_request_get_query(struct c_rest_request *req, const char *key,
                             const char **out_value) {
  struct c_rest_header *qp;

  if (!req || !key || !out_value) {
    return 1;
  }

  parse_query_if_needed(req);

  for (qp = req->query_params; qp != NULL; qp = qp->next) {
    if (strcmp(qp->key, key) == 0) {
      *out_value = qp->value;
      return 0;
    }
  }

  return 1;
}

int c_rest_request_parse_urlencoded(struct c_rest_request *req) {
  const char *p;
  if (!req || req->form_params) {
    return 0; /* Already parsed or no body */
  }

  if (!req->body || req->body_len == 0) {
    return 0;
  }

  p = req->body;
  while (*p) {
    const char *eq = strchr(p, '=');
    const char *amp = strchr(p, '&');
    size_t key_len, val_len;
    struct c_rest_header *qp;

    if (!amp) {
      amp = p + strlen(p);
    }

    if (C_REST_MALLOC(sizeof(struct c_rest_header), (void **)&qp) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp = NULL; }
    if (!qp) {
      break; /* Out of memory */
    }
    qp->key = NULL;
    qp->value = NULL;
    qp->next = NULL;

    if (eq && eq < amp) {
      key_len = (size_t)(eq - p);
      val_len = (size_t)(amp - eq - 1);

      if (C_REST_MALLOC(key_len + 1, (void **)&qp->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->key = NULL; }
      if (qp->key) {
        c_rest_url_decode(qp->key, p, key_len);
      }

      if (C_REST_MALLOC(val_len + 1, (void **)&qp->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->value = NULL; }
      if (qp->value) {
        c_rest_url_decode(qp->value, eq + 1, val_len);
      }
    } else {
      key_len = (size_t)(amp - p);
      if (C_REST_MALLOC(key_len + 1, (void **)&qp->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->key = NULL; }
      if (qp->key) {
        c_rest_url_decode(qp->key, p, key_len);
      }
      if (C_REST_MALLOC(1, (void **)&qp->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->value = NULL; }
      if (qp->value) {
        qp->value[0] = '\0';
      }
    }

    if (!qp->key || !qp->value) {
      C_REST_FREE((void *)(qp->key));
      C_REST_FREE((void *)(qp->value));
      C_REST_FREE((void *)(qp));
    } else {
      qp->next = req->form_params;
      req->form_params = qp;
    }

    if (*amp == '\0') {
      break;
    }
    p = amp + 1;
  }
  return 0;
}

int c_rest_request_get_form_param(struct c_rest_request *req, const char *key,
                                  const char **out_value) {
  struct c_rest_header *qp;

  if (!req || !key || !out_value) {
    return 1;
  }

  c_rest_request_parse_urlencoded(req);

  for (qp = req->form_params; qp != NULL; qp = qp->next) {
    if (strcmp(qp->key, key) == 0) {
      *out_value = qp->value;
      return 0;
    }
  }

  return 1;
}

int c_rest_request_read_body(struct c_rest_request *req, char **body,
                             size_t *body_len) {
  if (!req || !body || !body_len) {
    return 1;
  }
  *body = req->body;
  *body_len = req->body_len;
  return 0;
}

int c_rest_request_accepts_encoding(struct c_rest_request *req,
                                    const char *encoding) {
  const char *accept_enc;
  if (!req || !encoding) {
    return 0;
  }
  if (c_rest_request_get_header(req, "Accept-Encoding", &accept_enc) != 0) {
    return 0;
  }
  /* Simple substring search */
  if (strstr(accept_enc, encoding) != NULL) {
    return 1;
  }
  return 0;
}

int c_rest_request_parse_json(struct c_rest_request *req, void **json_obj) {
  if (!req || !json_obj) {
    return 1;
  }
  *json_obj = NULL;
  if (!req->body || req->body_len == 0) {
    return 1;
  }

  *json_obj = (void *)json_parse_string(req->body);
  if (!*json_obj) {
    return 1;
  }

  return 0;
}

int c_rest_request_cleanup(struct c_rest_request *req) {
  struct c_rest_header *h;
  struct c_rest_header *next_h;

  if (!req) {
    return 1;
  }

  h = req->headers;
  while (h) {
    next_h = h->next;
    C_REST_FREE((void *)(h->key));
    C_REST_FREE((void *)(h->value));
    C_REST_FREE((void *)(h));
    h = next_h;
  }
  req->headers = NULL;

  h = req->query_params;
  while (h) {
    next_h = h->next;
    C_REST_FREE((void *)(h->key));
    C_REST_FREE((void *)(h->value));
    C_REST_FREE((void *)(h));
    h = next_h;
  }
  req->query_params = NULL;

  h = req->form_params;
  while (h) {
    next_h = h->next;
    C_REST_FREE((void *)(h->key));
    C_REST_FREE((void *)(h->value));
    C_REST_FREE((void *)(h));
    h = next_h;
  }
  req->form_params = NULL;

  h = req->cookies;
  while (h) {
    next_h = h->next;
    C_REST_FREE((void *)(h->key));
    C_REST_FREE((void *)(h->value));
    C_REST_FREE((void *)(h));
    h = next_h;
  }
  req->cookies = NULL;

  {
    struct c_rest_path_var *pv = req->path_vars;
    struct c_rest_path_var *next_pv;
    while (pv) {
      next_pv = pv->next;
      C_REST_FREE((void *)(pv->name));
      C_REST_FREE((void *)(pv->value));
      C_REST_FREE((void *)(pv));
      pv = next_pv;
    }
    req->path_vars = NULL;
  }

  if (req->body) {
    C_REST_FREE((void *)(req->body));
    req->body = NULL;
  }
  return 0;
}

int c_rest_request_get_auth_bearer(struct c_rest_request *req,
                                   char **out_token) {
  const char *auth_val;
  if (!req || !out_token)
    return 1;

  if (c_rest_request_get_header(req, "Authorization", &auth_val) != 0) {
    return 1;
  }

  if (strncmp(auth_val, "Bearer ", 7) != 0) {
    return 1;
  }

  if (C_REST_MALLOC(strlen(auth_val + 7) + 1, (void **)out_token) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); *out_token = NULL; }
  if (!*out_token)
    return 1;

#if defined(_MSC_VER)
  strcpy_s(*out_token, strlen(auth_val + 7) + 1, auth_val + 7);
#else
  strcpy(*out_token, auth_val + 7);
#endif

  return 0;
}

#include "c_rest_base64.h"
/* clang-format on */

int c_rest_request_get_auth_basic(struct c_rest_request *req,
                                  char **out_username, char **out_password) {
  const char *auth_val;
  char *decoded;
  size_t decoded_len;
  char *colon;
  size_t auth_len;

  if (!req || !out_username || !out_password)
    return 1;

  if (c_rest_request_get_header(req, "Authorization", &auth_val) != 0) {
    return 1;
  }

  if (strncmp(auth_val, "Basic ", 6) != 0) {
    return 1;
  }

  auth_val += 6;
  auth_len = strlen(auth_val);

  if (c_rest_base64_decode(auth_val, auth_len, NULL, &decoded_len) != 0) {
    return 1;
  }

  if (C_REST_MALLOC(decoded_len + 1, (void **)&decoded) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    decoded = NULL;
  }
  if (!decoded)
    return 1;

  if (c_rest_base64_decode(auth_val, auth_len, (unsigned char *)decoded,
                           &decoded_len) != 0) {
    C_REST_FREE((void *)(decoded));
    return 1;
  }
  decoded[decoded_len] = '\0';

  colon = strchr(decoded, ':');
  if (!colon) {
    C_REST_FREE((void *)(decoded));
    return 1;
  }

  *colon = '\0';
  if (C_REST_MALLOC(strlen(decoded) + 1, (void **)out_username) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    *out_username = NULL;
  }
  if (*out_username) {
#if defined(_MSC_VER)
    strcpy_s(*out_username, strlen(decoded) + 1, decoded);
#else
    strcpy(*out_username, decoded);
#endif
  }

  if (C_REST_MALLOC(strlen(colon + 1) + 1, (void **)out_password) != 0) {
    LOG_DEBUG("C_REST_MALLOC failed");
    *out_password = NULL;
  }
  if (*out_password) {
#if defined(_MSC_VER)
    strcpy_s(*out_password, strlen(colon + 1) + 1, colon + 1);
#else
    strcpy(*out_password, colon + 1);
#endif
  }
  C_REST_FREE((void *)(decoded));

  if (!*out_username || !*out_password) {
    if (*out_username)
      C_REST_FREE((void *)(*out_username));
    if (*out_password)
      C_REST_FREE((void *)(*out_password));
    return 1;
  }

  return 0;
}
