/* clang-format off */
#include "c_rest_request.h" /* For struct c_rest_header */
#include "c_rest_response.h"
#include <parson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* clang-format on */

#include <ctype.h>

#if defined(_MSC_VER)
#define SAFE_STRCPY(dest, size, src) strcpy_s(dest, size, src)
#else
#define SAFE_STRCPY(dest, size, src) strcpy(dest, src)
#endif

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

int c_rest_response_set_header(struct c_rest_response *res, const char *key,
                               const char *value) {
  struct c_rest_header *h;
  struct c_rest_header *new_h;
  size_t val_len;

  if (!res || !key || !value) {
    return 1;
  }

  /* Check if it already exists, replace value if it does */
  if (c_rest_stricmp(key, "Set-Cookie") != 0) {
    for (h = res->headers; h != NULL; h = h->next) {
      if (c_rest_stricmp(h->key, key) == 0) {
        char *new_val;
        val_len = strlen(value) + 1;
        new_val = (char *)malloc(val_len);
        if (!new_val) {
          return 1;
        }
        SAFE_STRCPY(new_val, val_len, value);
        free(h->value);
        h->value = new_val;
        return 0;
      }
    }
  }

  /* Add new header */
  new_h = (struct c_rest_header *)malloc(sizeof(struct c_rest_header));
  if (!new_h) {
    return 1;
  }
  new_h->key = (char *)malloc(strlen(key) + 1);
  new_h->value = (char *)malloc(strlen(value) + 1);
  if (!new_h->key || !new_h->value) {
    free(new_h->key);
    free(new_h->value);
    free(new_h);
    return 1;
  }
  SAFE_STRCPY(new_h->key, strlen(key) + 1, key);
  SAFE_STRCPY(new_h->value, strlen(value) + 1, value);

  new_h->next = res->headers;
  res->headers = new_h;

  return 0;
}

int c_rest_response_set_status(struct c_rest_response *res, int status_code) {
  if (!res) {
    return 1;
  }
  res->status_code = status_code;
  return 0;
}

int c_rest_response_check_etag(struct c_rest_request *req,
                               struct c_rest_response *res, const char *etag) {
  const char *if_none_match;
  if (!req || !res || !etag) {
    return 0;
  }

  c_rest_response_set_header(res, "ETag", etag);

  if (c_rest_request_get_header(req, "If-None-Match", &if_none_match) == 0) {
    if (if_none_match && strcmp(if_none_match, etag) == 0) {
      c_rest_response_set_status(res, 304); /* Not Modified */
      return 1;                             /* Match found */
    }
  }

  return 0;
}

int c_rest_response_set_cache_control(struct c_rest_response *res,
                                      const char *policy) {
  if (!res || !policy) {
    return 1;
  }
  return c_rest_response_set_header(res, "Cache-Control", policy);
}

int c_rest_response_send(struct c_rest_response *res) {
  /* Implementation depends on the underlying transport (abstracted by
   * c-abstract-http or multiplatform). For now, just mark it as sent. */
  if (!res) {
    return 1;
  }
  if (res->headers_sent) {
    return 1; /* Prevent double-sending */
  }

  /* Auto-calculate content length if not chunked and not already set */
  if (!res->is_chunked) {
    char cl_buf[32];
#if defined(_MSC_VER)
    sprintf_s(cl_buf, sizeof(cl_buf), "%lu", (unsigned long)res->body_len);
#else
    sprintf(cl_buf, "%lu", (unsigned long)res->body_len);
#endif
    c_rest_response_set_header(res, "Content-Length", cl_buf);
  }

  res->headers_sent = 1;
  return 0;
}

int c_rest_response_json(struct c_rest_response *res, const char *json_str) {
  size_t len;
  if (!res || !json_str) {
    return 1;
  }
  len = strlen(json_str);
  c_rest_response_set_header(res, "Content-Type", "application/json");

  if (res->body) {
    free(res->body);
  }
  res->body = (char *)malloc(len + 1);
  if (!res->body) {
    return 1;
  }
  SAFE_STRCPY(res->body, len + 1, json_str);
  res->body_len = len;

  return c_rest_response_send(res);
}

int c_rest_response_json_obj(struct c_rest_response *res, void *json_obj) {
  char *json_str;
  int ret;

  if (!res || !json_obj) {
    return 1;
  }

  json_str = json_serialize_to_string((JSON_Value *)json_obj);
  if (!json_str) {
    return 1;
  }

  ret = c_rest_response_json(res, json_str);
  json_free_serialized_string(json_str);

  return ret;
}

int c_rest_response_json_dict(struct c_rest_response *res,
                              const struct c_rest_json_pair *pairs,
                              size_t count) {
  JSON_Value *root_val;
  JSON_Object *root_obj;
  size_t i;
  int ret;

  if (!res) {
    return 1;
  }

  root_val = json_value_init_object();
  if (!root_val) {
    return 1;
  }
  root_obj = json_value_get_object(root_val);

  for (i = 0; i < count; i++) {
    switch (pairs[i].type) {
    case C_REST_JSON_TYPE_STRING:
      json_object_set_string(root_obj, pairs[i].key, pairs[i].str_val);
      break;
    case C_REST_JSON_TYPE_NUMBER:
      json_object_set_number(root_obj, pairs[i].key, pairs[i].num_val);
      break;
    case C_REST_JSON_TYPE_BOOLEAN:
      json_object_set_boolean(root_obj, pairs[i].key, pairs[i].bool_val);
      break;
    case C_REST_JSON_TYPE_NULL:
      json_object_set_null(root_obj, pairs[i].key);
      break;
    }
  }

  ret = c_rest_response_json_obj(res, root_val);
  json_value_free(root_val);
  return ret;
}

int c_rest_response_html(struct c_rest_response *res, const char *html_str) {
  size_t len;
  if (!res || !html_str) {
    return 1;
  }
  len = strlen(html_str);
  c_rest_response_set_header(res, "Content-Type", "text/html");

  if (res->body) {
    free(res->body);
  }
  res->body = (char *)malloc(len + 1);
  if (!res->body) {
    return 1;
  }
  SAFE_STRCPY(res->body, len + 1, html_str);
  res->body_len = len;

  return c_rest_response_send(res);
}

int c_rest_response_write_chunk(struct c_rest_response *res, const char *chunk,
                                size_t chunk_len) {
  if (!res || !chunk) {
    return 1;
  }

  if (!res->headers_sent) {
    c_rest_response_set_header(res, "Transfer-Encoding", "chunked");
    res->is_chunked = 1;
    res->headers_sent = 1;
    /* Actually send headers down the socket here in real implementation */
  }

  /* Stub: Write chunk formatted properly "HEX\r\nData\r\n" */
  (void)chunk_len;
  return 0;
}

int c_rest_response_redirect(struct c_rest_response *res, const char *url,
                             int status_code) {
  if (!res || !url) {
    return 1;
  }
  if (status_code < 300 || status_code > 399) {
    status_code = 302; /* Default to temporary redirect */
  }
  c_rest_response_set_status(res, status_code);
  c_rest_response_set_header(res, "Location", url);
  return c_rest_response_send(res);
}

int c_rest_response_set_cookie(struct c_rest_response *res, const char *key,
                               const char *value, const char *attributes) {
  char *cookie_str;
  size_t len;
  int ret;

  if (!res || !key || !value) {
    return 1;
  }

  len = strlen(key) + strlen(value) + 2; /* key=value */
  if (attributes) {
    len += strlen(attributes) + 2; /* ; attributes */
  }

  cookie_str = (char *)malloc(len);
  if (!cookie_str) {
    return 1;
  }

#if defined(_MSC_VER)
  if (attributes) {
    sprintf_s(cookie_str, len, "%s=%s; %s", key, value, attributes);
  } else {
    sprintf_s(cookie_str, len, "%s=%s", key, value);
  }
#else
  if (attributes) {
    sprintf(cookie_str, "%s=%s; %s", key, value, attributes);
  } else {
    sprintf(cookie_str, "%s=%s", key, value);
  }
#endif

  ret = c_rest_response_set_header(res, "Set-Cookie", cookie_str);
  free(cookie_str);

  return ret;
}

int c_rest_response_send_file(struct c_rest_response *res,
                              const char *filepath) {
  if (!res || !filepath) {
    return 1;
  }
  /* Stub: read file and put into response or stream it. */
  return 0;
}

int c_rest_response_cleanup(struct c_rest_response *res) {
  struct c_rest_header *h;
  struct c_rest_header *next_h;

  if (!res) {
    return 1;
  }

  h = res->headers;
  while (h) {
    next_h = h->next;
    free(h->key);
    free(h->value);
    free(h);
    h = next_h;
  }
  res->headers = NULL;

  if (res->body) {
    free(res->body);
    res->body = NULL;
  }
  return 0;
}
