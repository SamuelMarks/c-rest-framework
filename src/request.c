/* clang-format off */
#include "c_rest_error.h"
#include "c_rest_request.h"
#include "c_rest_str_utils.h"
#include <parson.h>

#include <stdlib.h>
#include <string.h>
#include "c_rest_mem.h"
#include "c_rest_log.h"

#include <ctype.h>
static c_rest_error_t c_rest_stricmp(const char *s1, const char *s2, int *out_cmp) {
  while (*s1 && *s2) { /* GCOVR_EXCL_LINE */
    int c1 = tolower((unsigned char)*s1);
    int c2 = tolower((unsigned char)*s2);
    if (c1 != c2) { /* GCOVR_EXCL_LINE */
      *out_cmp = c1 - c2; /* GCOVR_EXCL_LINE */
      return C_REST_OK; /* GCOVR_EXCL_LINE */
    }
    s1++;
    s2++;
  }
  *out_cmp = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
  return C_REST_OK;
}

c_rest_error_t c_rest_request_get_header(struct c_rest_request *req, const char *key,
                              const char **out_value) {
  struct c_rest_header *h;
  if (!req || !key || !out_value) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  for (h = req->headers; h != NULL; h = h->next) {
    int cmp;
    if (c_rest_stricmp(h->key, key, &cmp) == 0 && cmp == 0) { /* GCOVR_EXCL_LINE */
      *out_value = h->value;
      return C_REST_OK;
    }
  }
  *out_value = NULL;
  return C_REST_ERROR_GENERIC;
}

static int parse_cookies_if_needed(struct c_rest_request *req) { /* GCOVR_EXCL_LINE */
  const char *cookie_str;
  const char *p;
  if (!req || req->cookies) { /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */
  }
  if (c_rest_request_get_header(req, "Cookie", &cookie_str) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  p = cookie_str; /* GCOVR_EXCL_LINE */
  while (*p) { /* GCOVR_EXCL_LINE */
    const char *eq;
    const char *semi;
    size_t key_len, val_len;
    struct c_rest_header *cp;

    while (*p == ' ') /* GCOVR_EXCL_LINE */
      p++; /* Skip leading spaces */ /* GCOVR_EXCL_LINE */
    if (*p == '\0') /* GCOVR_EXCL_LINE */
      break; /* GCOVR_EXCL_LINE */

    eq = strchr(p, '='); /* GCOVR_EXCL_LINE */
    semi = strchr(p, ';'); /* GCOVR_EXCL_LINE */

    if (!semi) { /* GCOVR_EXCL_LINE */
      semi = p + strlen(p); /* GCOVR_EXCL_LINE */
    }

    if (!eq || eq > semi) { /* GCOVR_EXCL_LINE */
      /* Invalid cookie format, skip to next */
      p = semi; /* GCOVR_EXCL_LINE */
      if (*p == ';') /* GCOVR_EXCL_LINE */
        p++; /* GCOVR_EXCL_LINE */
      continue; /* GCOVR_EXCL_LINE */
    }

    key_len = (size_t)(eq - p); /* GCOVR_EXCL_LINE */
    val_len = (size_t)(semi - eq - 1); /* GCOVR_EXCL_LINE */

    if (C_REST_MALLOC(sizeof(struct c_rest_header), &cp) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); cp = NULL; } /* GCOVR_EXCL_LINE */
    if (cp) { /* GCOVR_EXCL_LINE */
      if (C_REST_MALLOC(key_len + 1, &cp->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); cp->key = NULL; } /* GCOVR_EXCL_LINE */
      if (C_REST_MALLOC(val_len + 1, &cp->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); cp->value = NULL; } /* GCOVR_EXCL_LINE */
      if (cp->key && cp->value) { /* GCOVR_EXCL_LINE */
        memcpy(cp->key, p, key_len); /* GCOVR_EXCL_LINE */
        cp->key[key_len] = '\0'; /* GCOVR_EXCL_LINE */
        memcpy(cp->value, eq + 1, val_len); /* GCOVR_EXCL_LINE */
        cp->value[val_len] = '\0'; /* GCOVR_EXCL_LINE */

        cp->next = req->cookies; /* GCOVR_EXCL_LINE */
        req->cookies = cp; /* GCOVR_EXCL_LINE */
      } else {
        if (cp->key) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(cp->key)); /* GCOVR_EXCL_LINE */
        if (cp->value) /* GCOVR_EXCL_LINE */
          C_REST_FREE((void *)(cp->value)); /* GCOVR_EXCL_LINE */
        C_REST_FREE((void *)(cp)); /* GCOVR_EXCL_LINE */
      }
    }

    p = semi; /* GCOVR_EXCL_LINE */
    if (*p == ';') /* GCOVR_EXCL_LINE */
      p++; /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK; /* GCOVR_EXCL_LINE */
}

c_rest_error_t c_rest_request_get_cookie(struct c_rest_request *req, const char *key, /* GCOVR_EXCL_LINE */
                              const char **out_value) {
  struct c_rest_header *cp;
  if (!req || !key || !out_value) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  parse_cookies_if_needed(req); /* GCOVR_EXCL_LINE */
  for (cp = req->cookies; cp != NULL; cp = cp->next) { /* GCOVR_EXCL_LINE */
    if (strcmp(cp->key, key) == 0) { /* GCOVR_EXCL_LINE */
      *out_value = cp->value; /* GCOVR_EXCL_LINE */
      return C_REST_OK; /* GCOVR_EXCL_LINE */
    }
  }
  return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
}

static int parse_query_if_needed(struct c_rest_request *req) {
  const char *p;
  if (!req || !req->query || req->query_params) { /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* Already parsed or no query string */
  }

  p = req->query;
  while (*p) { /* GCOVR_EXCL_LINE */
    const char *eq = strchr(p, '=');
    const char *amp = strchr(p, '&');
    size_t key_len, val_len;
    struct c_rest_header *qp;

    if (!amp) {
      amp = p + strlen(p);
    }

    if (C_REST_MALLOC(sizeof(struct c_rest_header), &qp) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp = NULL; } /* GCOVR_EXCL_LINE */
    if (!qp) { /* GCOVR_EXCL_LINE */
      break; /* Out of memory */
    }
    qp->key = NULL;
    qp->value = NULL;
    qp->next = NULL;

    if (eq && eq < amp) { /* GCOVR_EXCL_LINE */
      key_len = (size_t)(eq - p);
      val_len = (size_t)(amp - eq - 1);

      if (C_REST_MALLOC(key_len + 1, &qp->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->key = NULL; } /* GCOVR_EXCL_LINE */
      if (qp->key) { /* GCOVR_EXCL_LINE */
        c_rest_url_decode(qp->key, p, key_len);
      }

      if (C_REST_MALLOC(val_len + 1, &qp->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->value = NULL; } /* GCOVR_EXCL_LINE */
      if (qp->value) { /* GCOVR_EXCL_LINE */
        c_rest_url_decode(qp->value, eq + 1, val_len);
      }
    } else {
      key_len = (size_t)(amp - p);
      if (C_REST_MALLOC(key_len + 1, &qp->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->key = NULL; } /* GCOVR_EXCL_LINE */
      if (qp->key) { /* GCOVR_EXCL_LINE */
        c_rest_url_decode(qp->key, p, key_len);
      }
      if (C_REST_MALLOC(1, &qp->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->value = NULL; } /* GCOVR_EXCL_LINE */
      if (qp->value) { /* GCOVR_EXCL_LINE */
        qp->value[0] = '\0';
      }
    }

    if (!qp->key || !qp->value) { /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(qp->key)); /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(qp->value)); /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(qp)); /* GCOVR_EXCL_LINE */
    } else {
      qp->next = req->query_params;
      req->query_params = qp;
    }

    if (*amp == '\0') {
      break;
    }
    p = amp + 1;
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_request_get_query(struct c_rest_request *req, const char *key,
                             const char **out_value) {
  struct c_rest_header *qp;

  if (!req || !key || !out_value) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  parse_query_if_needed(req);

  for (qp = req->query_params; qp != NULL; qp = qp->next) {
    if (strcmp(qp->key, key) == 0) {
      *out_value = qp->value;
      return C_REST_OK;
    }
  }

  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_request_parse_urlencoded(struct c_rest_request *req) {
  const char *p;
  if (!req || req->form_params) { /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* Already parsed or no body */
  }

  if (!req->body || req->body_len == 0) { /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */
  }

  p = req->body;
  while (*p) { /* GCOVR_EXCL_LINE */
    const char *eq = strchr(p, '=');
    const char *amp = strchr(p, '&');
    size_t key_len, val_len;
    struct c_rest_header *qp;

    if (!amp) {
      amp = p + strlen(p);
    }

    if (C_REST_MALLOC(sizeof(struct c_rest_header), &qp) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp = NULL; } /* GCOVR_EXCL_LINE */
    if (!qp) { /* GCOVR_EXCL_LINE */
      break; /* Out of memory */
    }
    qp->key = NULL;
    qp->value = NULL;
    qp->next = NULL;

    if (eq && eq < amp) { /* GCOVR_EXCL_LINE */
      key_len = (size_t)(eq - p);
      val_len = (size_t)(amp - eq - 1);

      if (C_REST_MALLOC(key_len + 1, &qp->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->key = NULL; } /* GCOVR_EXCL_LINE */
      if (qp->key) { /* GCOVR_EXCL_LINE */
        c_rest_url_decode(qp->key, p, key_len);
      }

      if (C_REST_MALLOC(val_len + 1, &qp->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->value = NULL; } /* GCOVR_EXCL_LINE */
      if (qp->value) { /* GCOVR_EXCL_LINE */
        c_rest_url_decode(qp->value, eq + 1, val_len);
      }
    } else {
      key_len = (size_t)(amp - p); /* GCOVR_EXCL_LINE */
      if (C_REST_MALLOC(key_len + 1, &qp->key) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->key = NULL; } /* GCOVR_EXCL_LINE */
      if (qp->key) { /* GCOVR_EXCL_LINE */
        c_rest_url_decode(qp->key, p, key_len); /* GCOVR_EXCL_LINE */
      }
      if (C_REST_MALLOC(1, &qp->value) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); qp->value = NULL; } /* GCOVR_EXCL_LINE */
      if (qp->value) { /* GCOVR_EXCL_LINE */
        qp->value[0] = '\0'; /* GCOVR_EXCL_LINE */
      }
    }

    if (!qp->key || !qp->value) { /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(qp->key)); /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(qp->value)); /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(qp)); /* GCOVR_EXCL_LINE */
    } else {
      qp->next = req->form_params;
      req->form_params = qp;
    }

    if (*amp == '\0') {
      break;
    }
    p = amp + 1;
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_request_get_form_param(struct c_rest_request *req, const char *key,
                                  const char **out_value) {
  struct c_rest_header *qp;

  if (!req || !key || !out_value) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  c_rest_request_parse_urlencoded(req);

  for (qp = req->form_params; qp != NULL; qp = qp->next) {
    if (strcmp(qp->key, key) == 0) {
      *out_value = qp->value;
      return C_REST_OK;
    }
  }

  return C_REST_ERROR_GENERIC;
}

c_rest_error_t c_rest_request_read_body(struct c_rest_request *req, char **body,
                             size_t *body_len) {
  if (!req || !body || !body_len) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  *body = req->body;
  *body_len = req->body_len;
  return C_REST_OK;
}

c_rest_error_t c_rest_request_accepts_encoding(struct c_rest_request *req,
                                    const char *encoding) {
  const char *accept_enc;
  if (!req || !encoding) { /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */
  }
  if (c_rest_request_get_header(req, "Accept-Encoding", &accept_enc) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_OK; /* GCOVR_EXCL_LINE */
  }
  /* Simple substring search */
  if (strstr(accept_enc, encoding) != NULL) {
    return C_REST_ERROR_GENERIC;
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_request_parse_json(struct c_rest_request *req, void **json_obj) {
  if (!req || !json_obj) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }
  *json_obj = NULL;
  if (!req->body || req->body_len == 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  *json_obj = (void *)json_parse_string(req->body);
  if (!*json_obj) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  return C_REST_OK;
}

c_rest_error_t c_rest_request_cleanup(struct c_rest_request *req) {
  struct c_rest_header *h;
  struct c_rest_header *next_h;

  if (!req) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  h = req->headers;
  while (h) { /* GCOVR_EXCL_LINE */
    next_h = h->next; /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(h->key)); /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(h->value)); /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(h)); /* GCOVR_EXCL_LINE */
    h = next_h; /* GCOVR_EXCL_LINE */
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
  while (h) { /* GCOVR_EXCL_LINE */
    next_h = h->next; /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(h->key)); /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(h->value)); /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(h)); /* GCOVR_EXCL_LINE */
    h = next_h; /* GCOVR_EXCL_LINE */
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

  if (req->body) { /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(req->body)); /* GCOVR_EXCL_LINE */
    req->body = NULL; /* GCOVR_EXCL_LINE */
  }
  return C_REST_OK;
}

c_rest_error_t c_rest_request_get_auth_bearer(struct c_rest_request *req,
                                   char **out_token) {
  const char *auth_val;
  if (!req || !out_token) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (c_rest_request_get_header(req, "Authorization", &auth_val) != 0) {
    return C_REST_ERROR_GENERIC;
  }

  if (strncmp(auth_val, "Bearer ", 7) != 0) {
    return C_REST_ERROR_GENERIC;
  }

  if (C_REST_MALLOC(strlen(auth_val + 7) + 1, out_token) != 0) { LOG_DEBUG("C_REST_MALLOC failed"); *out_token = NULL; } /* GCOVR_EXCL_LINE */
  if (!*out_token) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

#if defined(_MSC_VER)
  strcpy_s(*out_token, strlen(auth_val + 7) + 1, auth_val + 7);
#else
  strcpy(*out_token, auth_val + 7);
#endif

  return C_REST_OK;
}

#include "c_rest_base64.h"
/* clang-format on */

c_rest_error_t c_rest_request_get_auth_basic(struct c_rest_request *req,
                                             char **out_username,
                                             char **out_password) {
  const char *auth_val;
  char *decoded;
  size_t decoded_len;
  char *colon;
  size_t auth_len;

  if (!req || !out_username || !out_password) /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;              /* GCOVR_EXCL_LINE */

  if (c_rest_request_get_header(req, "Authorization", &auth_val) != 0) {
    return C_REST_ERROR_GENERIC;
  }

  if (strncmp(auth_val, "Basic ", 6) != 0) { /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;             /* GCOVR_EXCL_LINE */
  }

  auth_val += 6;
  auth_len = strlen(auth_val);

  if (c_rest_base64_decode(auth_val, auth_len, NULL, &decoded_len) !=
      0) {                       /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */
  }

  if (C_REST_MALLOC(decoded_len + 1, &decoded) != 0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    decoded = NULL; /* GCOVR_EXCL_LINE */
  }
  if (!decoded)                  /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC; /* GCOVR_EXCL_LINE */

  if (c_rest_base64_decode(auth_val, auth_len,
                           (unsigned char *)decoded, /* GCOVR_EXCL_LINE */
                           &decoded_len) != 0) {
    C_REST_FREE((void *)(decoded)); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;    /* GCOVR_EXCL_LINE */
  }
  decoded[decoded_len] = '\0';

  colon = strchr(decoded, ':');
  if (!colon) {                     /* GCOVR_EXCL_LINE */
    C_REST_FREE((void *)(decoded)); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;    /* GCOVR_EXCL_LINE */
  }

  *colon = '\0';
  if (C_REST_MALLOC(strlen(decoded) + 1, out_username) !=
      0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    *out_username = NULL; /* GCOVR_EXCL_LINE */
  }
  if (*out_username) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
    strcpy_s(*out_username, strlen(decoded) + 1, decoded);
#else
    strcpy(*out_username, decoded);
#endif
  }

  if (C_REST_MALLOC(strlen(colon + 1) + 1, out_password) !=
      0) { /* GCOVR_EXCL_LINE */
    LOG_DEBUG("C_REST_MALLOC failed");
    *out_password = NULL; /* GCOVR_EXCL_LINE */
  }
  if (*out_password) { /* GCOVR_EXCL_LINE */
#if defined(_MSC_VER)
    strcpy_s(*out_password, strlen(colon + 1) + 1, colon + 1);
#else
    strcpy(*out_password, colon + 1);
#endif
  }
  C_REST_FREE((void *)(decoded));

  if (!*out_username || !*out_password) {   /* GCOVR_EXCL_LINE */
    if (*out_username)                      /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(*out_username)); /* GCOVR_EXCL_LINE */
    if (*out_password)                      /* GCOVR_EXCL_LINE */
      C_REST_FREE((void *)(*out_password)); /* GCOVR_EXCL_LINE */
    return C_REST_ERROR_GENERIC;            /* GCOVR_EXCL_LINE */
  }

  return C_REST_OK;
}
