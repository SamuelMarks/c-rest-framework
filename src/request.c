/* clang-format off */
#include "c_rest_request.h"

#include <stdlib.h>
#include <string.h>
/* clang-format on */

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#endif

int c_rest_request_get_header(struct c_rest_request *req, const char *key,
                              const char **out_value) {
  struct c_rest_header *h;
  if (!req || !key || !out_value) {
    return 1;
  }
  for (h = req->headers; h != NULL; h = h->next) {
    if (strcasecmp(h->key, key) == 0) {
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

    cp = (struct c_rest_header *)malloc(sizeof(struct c_rest_header));
    if (cp) {
      cp->key = (char *)malloc(key_len + 1);
      cp->value = (char *)malloc(val_len + 1);
      if (cp->key && cp->value) {
        memcpy(cp->key, p, key_len);
        cp->key[key_len] = '\0';
        memcpy(cp->value, eq + 1, val_len);
        cp->value[val_len] = '\0';

        cp->next = req->cookies;
        req->cookies = cp;
      } else {
        if (cp->key)
          free(cp->key);
        if (cp->value)
          free(cp->value);
        free(cp);
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

    qp = (struct c_rest_header *)malloc(sizeof(struct c_rest_header));
    if (!qp) {
      break; /* Out of memory */
    }
    qp->key = NULL;
    qp->value = NULL;
    qp->next = NULL;

    if (eq && eq < amp) {
      key_len = (size_t)(eq - p);
      val_len = (size_t)(amp - eq - 1);

      qp->key = (char *)malloc(key_len + 1);
      if (qp->key) {
        memcpy(qp->key, p, key_len);
        qp->key[key_len] = '\0';
      }

      qp->value = (char *)malloc(val_len + 1);
      if (qp->value) {
        memcpy(qp->value, eq + 1, val_len);
        qp->value[val_len] = '\0';
      }
    } else {
      key_len = (size_t)(amp - p);
      qp->key = (char *)malloc(key_len + 1);
      if (qp->key) {
        memcpy(qp->key, p, key_len);
        qp->key[key_len] = '\0';
      }
      qp->value = (char *)malloc(1);
      if (qp->value) {
        qp->value[0] = '\0';
      }
    }

    if (!qp->key || !qp->value) {
      free(qp->key);
      free(qp->value);
      free(qp);
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
  /* Stub: JSON parsing wrapper will be integrated here later. */
  *json_obj = NULL;
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
    free(h->key);
    free(h->value);
    free(h);
    h = next_h;
  }
  req->headers = NULL;

  h = req->query_params;
  while (h) {
    next_h = h->next;
    free(h->key);
    free(h->value);
    free(h);
    h = next_h;
  }
  req->query_params = NULL;

  h = req->cookies;
  while (h) {
    next_h = h->next;
    free(h->key);
    free(h->value);
    free(h);
    h = next_h;
  }
  req->cookies = NULL;

  {
    struct c_rest_path_var *pv = req->path_vars;
    struct c_rest_path_var *next_pv;
    while (pv) {
      next_pv = pv->next;
      free(pv->name);
      free(pv->value);
      free(pv);
      pv = next_pv;
    }
    req->path_vars = NULL;
  }

  if (req->body) {
    free(req->body);
    req->body = NULL;
  }
  return 0;
}
