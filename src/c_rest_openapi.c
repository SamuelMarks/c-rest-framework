/* clang-format off */
#include "c_rest_openapi.h"

#include <stdlib.h>
#include <string.h>
/* clang-format on */

int c_rest_openapi_spec_init(struct c_rest_openapi_spec **out_spec) {
  struct c_rest_openapi_spec *spec;
  if (!out_spec)
    return 1;

  spec =
      (struct c_rest_openapi_spec *)malloc(sizeof(struct c_rest_openapi_spec));
  if (!spec)
    return 1;

  memset(spec, 0, sizeof(struct c_rest_openapi_spec));
  spec->paths = NULL;
  spec->n_paths = 0;
  spec->capacity_paths = 0;
  spec->component_schemas_keys = NULL;
  spec->component_schemas_json = NULL;
  spec->n_components = 0;
  spec->capacity_components = 0;

  *out_spec = spec;
  return 0;
}

static void free_operation(struct c_rest_openapi_operation *op) {
  size_t i;
  if (!op)
    return;

  if (op->summary)
    free((void *)op->summary);
  if (op->description)
    free((void *)op->description);

  if (op->tags) {
    for (i = 0; i < op->n_tags; i++) {
      if (op->tags[i])
        free((void *)op->tags[i]);
    }
    free((void *)op->tags);
  }

  if (op->req_body_schema.ref_name)
    free((void *)op->req_body_schema.ref_name);
  if (op->res_body_schema.ref_name)
    free((void *)op->res_body_schema.ref_name);

  if (op->req_content_type)
    free((void *)op->req_content_type);
  if (op->res_content_type)
    free((void *)op->res_content_type);
}

int c_rest_openapi_spec_destroy(struct c_rest_openapi_spec *spec) {
  size_t i;
  if (!spec)
    return 1;

  if (spec->info.title)
    free((void *)spec->info.title);
  if (spec->info.version)
    free((void *)spec->info.version);
  if (spec->info.description)
    free((void *)spec->info.description);

  if (spec->paths) {
    for (i = 0; i < spec->n_paths; i++) {
      struct c_rest_openapi_path *p = &spec->paths[i];
      if (p->route)
        free((void *)p->route);
      free_operation(&p->get);
      free_operation(&p->post);
      free_operation(&p->put);
      free_operation(&p->del);
      free_operation(&p->patch);
    }
    free(spec->paths);
  }

  if (spec->component_schemas_keys) {
    for (i = 0; i < spec->n_components; i++) {
      if (spec->component_schemas_keys[i])
        free(spec->component_schemas_keys[i]);
    }
    free(spec->component_schemas_keys);
  }

  if (spec->component_schemas_json) {
    for (i = 0; i < spec->n_components; i++) {
      if (spec->component_schemas_json[i])
        free(spec->component_schemas_json[i]);
    }
    free(spec->component_schemas_json);
  }

  if (spec->swagger_openapi_url)
    free(spec->swagger_openapi_url);

  free(spec);
  return 0;
}

static int copy_operation(struct c_rest_openapi_operation *dst,
                          const struct c_rest_openapi_operation *src) {
  size_t i;

  if (src->summary) {
    dst->summary = (char *)malloc(strlen(src->summary) + 1);
    if (dst->summary) {
#if defined(_MSC_VER)
      strcpy_s((char *)dst->summary, strlen(src->summary) + 1, src->summary);
#else
      strcpy((char *)dst->summary, src->summary);
#endif
    }
  }

  if (src->description) {
    dst->description = (char *)malloc(strlen(src->description) + 1);
    if (dst->description) {
#if defined(_MSC_VER)
      strcpy_s((char *)dst->description, strlen(src->description) + 1,
               src->description);
#else
      strcpy((char *)dst->description, src->description);
#endif
    }
  }

  if (src->n_tags > 0 && src->tags) {
    dst->tags = (const char **)malloc(sizeof(char *) * src->n_tags);
    if (dst->tags) {
      dst->n_tags = src->n_tags;
      for (i = 0; i < src->n_tags; i++) {
        if (src->tags[i]) {
          char *t = (char *)malloc(strlen(src->tags[i]) + 1);
          if (t) {
#if defined(_MSC_VER)
            strcpy_s(t, strlen(src->tags[i]) + 1, src->tags[i]);
#else
            strcpy(t, src->tags[i]);
#endif
            dst->tags[i] = t;
          }
        } else {
          dst->tags[i] = NULL;
        }
      }
    }
  }

  if (src->req_body_schema.ref_name) {
    char *r = (char *)malloc(strlen(src->req_body_schema.ref_name) + 1);
    if (r) {
#if defined(_MSC_VER)
      strcpy_s(r, strlen(src->req_body_schema.ref_name) + 1,
               src->req_body_schema.ref_name);
#else
      strcpy(r, src->req_body_schema.ref_name);
#endif
      dst->req_body_schema.ref_name = r;
    }
  }

  if (src->res_body_schema.ref_name) {
    char *r = (char *)malloc(strlen(src->res_body_schema.ref_name) + 1);
    if (r) {
#if defined(_MSC_VER)
      strcpy_s(r, strlen(src->res_body_schema.ref_name) + 1,
               src->res_body_schema.ref_name);
#else
      strcpy(r, src->res_body_schema.ref_name);
#endif
      dst->res_body_schema.ref_name = r;
    }
  }

  if (src->req_content_type) {
    dst->req_content_type = (char *)malloc(strlen(src->req_content_type) + 1);
    if (dst->req_content_type) {
#if defined(_MSC_VER)
      strcpy_s((char *)dst->req_content_type, strlen(src->req_content_type) + 1,
               src->req_content_type);
#else
      strcpy((char *)dst->req_content_type, src->req_content_type);
#endif
    }
  }

  if (src->res_content_type) {
    dst->res_content_type = (char *)malloc(strlen(src->res_content_type) + 1);
    if (dst->res_content_type) {
#if defined(_MSC_VER)
      strcpy_s((char *)dst->res_content_type, strlen(src->res_content_type) + 1,
               src->res_content_type);
#else
      strcpy((char *)dst->res_content_type, src->res_content_type);
#endif
    }
  }

  return 0;
}

int c_rest_openapi_spec_add_component_schema(struct c_rest_openapi_spec *spec,
                                             const char *schema_name,
                                             const char *json_schema_str) {
  if (!spec || !schema_name || !json_schema_str)
    return 1;

  if (spec->n_components >= spec->capacity_components) {
    size_t new_cap =
        spec->capacity_components == 0 ? 8 : spec->capacity_components * 2;
    char **new_keys = (char **)malloc(sizeof(char *) * new_cap);
    char **new_json = (char **)malloc(sizeof(char *) * new_cap);
    if (!new_keys || !new_json) {
      if (new_keys)
        free(new_keys);
      if (new_json)
        free(new_json);
      return 1;
    }
    if (spec->component_schemas_keys) {
      memcpy(new_keys, spec->component_schemas_keys,
             sizeof(char *) * spec->n_components);
      free(spec->component_schemas_keys);
    }
    if (spec->component_schemas_json) {
      memcpy(new_json, spec->component_schemas_json,
             sizeof(char *) * spec->n_components);
      free(spec->component_schemas_json);
    }
    spec->component_schemas_keys = new_keys;
    spec->component_schemas_json = new_json;
    spec->capacity_components = new_cap;
  }

  spec->component_schemas_keys[spec->n_components] =
      (char *)malloc(strlen(schema_name) + 1);
  if (spec->component_schemas_keys[spec->n_components]) {
#if defined(_MSC_VER)
    strcpy_s(spec->component_schemas_keys[spec->n_components],
             strlen(schema_name) + 1, schema_name);
#else
    strcpy(spec->component_schemas_keys[spec->n_components], schema_name);
#endif
  }

  spec->component_schemas_json[spec->n_components] =
      (char *)malloc(strlen(json_schema_str) + 1);
  if (spec->component_schemas_json[spec->n_components]) {
#if defined(_MSC_VER)
    strcpy_s(spec->component_schemas_json[spec->n_components],
             strlen(json_schema_str) + 1, json_schema_str);
#else
    strcpy(spec->component_schemas_json[spec->n_components], json_schema_str);
#endif
  }

  spec->n_components++;
  return 0;
}

int c_rest_openapi_spec_add_path(struct c_rest_openapi_spec *spec,
                                 const char *route, const char *method,
                                 const struct c_rest_openapi_operation *op) {
  size_t i;
  struct c_rest_openapi_path *path = NULL;

  if (!spec || !route || !method || !op)
    return 1;

  for (i = 0; i < spec->n_paths; i++) {
    if (strcmp(spec->paths[i].route, route) == 0) {
      path = &spec->paths[i];
      break;
    }
  }

  if (!path) {
    if (spec->n_paths >= spec->capacity_paths) {
      size_t new_cap = spec->capacity_paths == 0 ? 4 : spec->capacity_paths * 2;
      struct c_rest_openapi_path *new_paths =
          (struct c_rest_openapi_path *)malloc(
              sizeof(struct c_rest_openapi_path) * new_cap);
      if (!new_paths)
        return 1;

      if (spec->paths) {
        memcpy(new_paths, spec->paths,
               sizeof(struct c_rest_openapi_path) * spec->n_paths);
        free(spec->paths);
      }
      spec->paths = new_paths;
      spec->capacity_paths = new_cap;
    }

    path = &spec->paths[spec->n_paths++];
    memset(path, 0, sizeof(struct c_rest_openapi_path));
    path->route = (char *)malloc(strlen(route) + 1);
    if (path->route) {
#if defined(_MSC_VER)
      strcpy_s((char *)path->route, strlen(route) + 1, route);
#else
      strcpy((char *)path->route, route);
#endif
    }
  }

  if (strcmp(method, "GET") == 0) {
    copy_operation(&path->get, op);
  } else if (strcmp(method, "POST") == 0) {
    copy_operation(&path->post, op);
  } else if (strcmp(method, "PUT") == 0) {
    copy_operation(&path->put, op);
  } else if (strcmp(method, "DELETE") == 0) {
    copy_operation(&path->del, op);
  } else if (strcmp(method, "PATCH") == 0) {
    copy_operation(&path->patch, op);
  }

  return 0;
}

/* clang-format off */
#include "parson.h"
#include "c_rest_router.h"
#include "c_rest_response.h"
#include "c_rest_request.h"

#include <stdio.h>
/* clang-format on */

static int serialize_operation(JSON_Object *methods_obj,
                               const char *method_name,
                               const struct c_rest_openapi_operation *op) {
  JSON_Value *op_val;
  JSON_Object *op_obj;
  JSON_Value *tags_val;
  JSON_Array *tags_arr;
  JSON_Value *req_body_val;
  JSON_Object *req_body_obj;
  JSON_Value *content_val;
  JSON_Object *content_obj;
  JSON_Value *app_json_val;
  JSON_Object *app_json_obj;
  JSON_Value *schema_val;
  JSON_Object *schema_obj;
  JSON_Value *responses_val;
  JSON_Object *responses_obj;
  JSON_Value *res_200_val;
  JSON_Object *res_200_obj;
  size_t i;
  char ref_buf[256];

  if (!op || (!op->summary && !op->description && op->n_tags == 0 &&
              !op->req_body_schema.ref_name && !op->res_body_schema.ref_name))
    return 0;

  op_val = json_value_init_object();
  op_obj = json_value_get_object(op_val);

  if (op->summary)
    json_object_set_string(op_obj, "summary", op->summary);
  if (op->description)
    json_object_set_string(op_obj, "description", op->description);

  if (op->n_tags > 0 && op->tags) {
    tags_val = json_value_init_array();
    tags_arr = json_value_get_array(tags_val);
    for (i = 0; i < op->n_tags; i++) {
      if (op->tags[i]) {
        json_array_append_string(tags_arr, op->tags[i]);
      }
    }
    json_object_set_value(op_obj, "tags", tags_val);
  }

  if (op->req_body_schema.ref_name) {
    req_body_val = json_value_init_object();
    req_body_obj = json_value_get_object(req_body_val);
    content_val = json_value_init_object();
    content_obj = json_value_get_object(content_val);
    app_json_val = json_value_init_object();
    app_json_obj = json_value_get_object(app_json_val);
    schema_val = json_value_init_object();
    schema_obj = json_value_get_object(schema_val);

#if defined(_MSC_VER)
    sprintf_s(ref_buf, sizeof(ref_buf), "#/components/schemas/%s",
              op->req_body_schema.ref_name);
#else
    sprintf(ref_buf, "#/components/schemas/%s", op->req_body_schema.ref_name);
#endif

    json_object_set_string(schema_obj, "$ref", ref_buf);
    json_object_set_value(app_json_obj, "schema", schema_val);
    json_object_set_value(content_obj,
                          op->req_content_type ? op->req_content_type
                                               : "application/json",
                          app_json_val);
    json_object_set_value(req_body_obj, "content", content_val);
    json_object_set_value(op_obj, "requestBody", req_body_val);
  }

  responses_val = json_value_init_object();
  responses_obj = json_value_get_object(responses_val);
  res_200_val = json_value_init_object();
  res_200_obj = json_value_get_object(res_200_val);
  json_object_set_string(res_200_obj, "description", "Successful response");

  if (op->res_body_schema.ref_name) {
    content_val = json_value_init_object();
    content_obj = json_value_get_object(content_val);
    app_json_val = json_value_init_object();
    app_json_obj = json_value_get_object(app_json_val);
    schema_val = json_value_init_object();
    schema_obj = json_value_get_object(schema_val);

#if defined(_MSC_VER)
    sprintf_s(ref_buf, sizeof(ref_buf), "#/components/schemas/%s",
              op->res_body_schema.ref_name);
#else
    sprintf(ref_buf, "#/components/schemas/%s", op->res_body_schema.ref_name);
#endif

    json_object_set_string(schema_obj, "$ref", ref_buf);
    json_object_set_value(app_json_obj, "schema", schema_val);
    json_object_set_value(content_obj, "application/json", app_json_val);
    json_object_set_value(res_200_obj, "content", content_val);
  }
  json_object_set_value(responses_obj, "200", res_200_val);
  json_object_set_value(op_obj, "responses", responses_val);

  json_object_set_value(methods_obj, method_name, op_val);
  return 0;
}

int c_rest_openapi_spec_to_json(const struct c_rest_openapi_spec *spec,
                                char **out_json) {
  JSON_Value *root_val;
  JSON_Object *root_obj;
  JSON_Value *info_val;
  JSON_Object *info_obj;
  JSON_Value *paths_val;
  JSON_Object *paths_obj;
  JSON_Value *components_val;
  size_t i;

  if (!spec || !out_json)
    return 1;

  root_val = json_value_init_object();
  root_obj = json_value_get_object(root_val);

  json_object_set_string(root_obj, "openapi", "3.0.0");

  info_val = json_value_init_object();
  info_obj = json_value_get_object(info_val);
  json_object_set_string(info_obj, "title",
                         spec->info.title ? spec->info.title : "API");
  json_object_set_string(info_obj, "version",
                         spec->info.version ? spec->info.version : "1.0.0");
  if (spec->info.description) {
    json_object_set_string(info_obj, "description", spec->info.description);
  }
  json_object_set_value(root_obj, "info", info_val);

  paths_val = json_value_init_object();
  paths_obj = json_value_get_object(paths_val);

  for (i = 0; i < spec->n_paths; i++) {
    struct c_rest_openapi_path *p = &spec->paths[i];
    JSON_Value *path_item_val = json_value_init_object();
    JSON_Object *path_item_obj = json_value_get_object(path_item_val);

    serialize_operation(path_item_obj, "get", &p->get);
    serialize_operation(path_item_obj, "post", &p->post);
    serialize_operation(path_item_obj, "put", &p->put);
    serialize_operation(path_item_obj, "delete", &p->del);
    serialize_operation(path_item_obj, "patch", &p->patch);

    json_object_set_value(paths_obj, p->route, path_item_val);
  }
  json_object_set_value(root_obj, "paths", paths_val);

  components_val = json_value_init_object();
  if (spec->n_components > 0) {
    JSON_Object *components_obj = json_value_get_object(components_val);
    JSON_Value *schemas_val = json_value_init_object();
    JSON_Object *schemas_obj = json_value_get_object(schemas_val);

    for (i = 0; i < spec->n_components; i++) {
      JSON_Value *parsed = json_parse_string(spec->component_schemas_json[i]);
      if (parsed) {
        json_object_set_value(schemas_obj, spec->component_schemas_keys[i],
                              parsed);
      }
    }
    json_object_set_value(components_obj, "schemas", schemas_val);
  }
  json_object_set_value(root_obj, "components", components_val);

  *out_json = json_serialize_to_string_pretty(root_val);
  json_value_free(root_val);

  if (!*out_json)
    return 1;
  return 0;
}

static int openapi_handler(struct c_rest_request *req,
                           struct c_rest_response *res, void *user_data) {
  struct c_rest_router *router = (struct c_rest_router *)user_data;
  struct c_rest_openapi_spec *spec = c_rest_router_get_openapi_spec(router);
  char *json_str = NULL;

  /* unused args */
  (void)req;

  if (c_rest_openapi_spec_to_json(spec, &json_str) == 0 && json_str) {
    res->status_code = 200;
    c_rest_response_json(res, json_str);
    json_free_serialized_string(json_str);
  } else {
    res->status_code = 500;
    c_rest_response_json(res,
                         "{\"error\": \"Failed to serialize OpenAPI spec\"}");
  }
  return 0;
}

int c_rest_enable_openapi(struct c_rest_router *router, const char *path) {
  if (!router || !path)
    return 1;
  return c_rest_router_add(router, "GET", path, openapi_handler, router);
}

static const char *swagger_html_template_1 =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <title>Swagger UI</title>\n"
    "  <link rel=\"stylesheet\" type=\"text/css\" "
    "href=\"https://unpkg.com/swagger-ui-dist@5.9.0/swagger-ui.css\">\n"
    "</head>\n"
    "<body>\n"
    "  <div id=\"swagger-ui\"></div>\n"
    "  <script "
    "src=\"https://unpkg.com/swagger-ui-dist@5.9.0/swagger-ui-bundle.js\"></"
    "script>\n";

static const char *swagger_html_template_2 =
    "  <script>\n"
    "    window.onload = function() {\n"
    "      SwaggerUIBundle({\n"
    "        url: \"%s\",\n"
    "        dom_id: '#swagger-ui',\n"
    "        presets: [SwaggerUIBundle.presets.apis, "
    "SwaggerUIBundle.SwaggerUIStandalonePreset],\n"
    "        layout: \"BaseLayout\"\n"
    "      });\n"
    "    };\n"
    "  </script>\n"
    "</body>\n"
    "</html>";

static int swagger_ui_handler(struct c_rest_request *req,
                              struct c_rest_response *res, void *user_data) {
  struct c_rest_router *router = (struct c_rest_router *)user_data;
  struct c_rest_openapi_spec *spec = c_rest_router_get_openapi_spec(router);
  char *html_buf;
  size_t html_len;
  const char *openapi_url = "/openapi.json";

  /* unused args */
  (void)req;

  if (spec && spec->swagger_openapi_url) {
    openapi_url = spec->swagger_openapi_url;
  }

  html_len = strlen(swagger_html_template_1) + strlen(swagger_html_template_2) +
             strlen(openapi_url) + 1;
  html_buf = (char *)malloc(html_len);
  if (!html_buf) {
    res->status_code = 500;
    return c_rest_response_html(res, "Internal Server Error");
  }

#if defined(_MSC_VER)
  strcpy_s(html_buf, html_len, swagger_html_template_1);
  sprintf_s(html_buf + strlen(swagger_html_template_1),
            html_len - strlen(swagger_html_template_1), swagger_html_template_2,
            openapi_url);
#else
  strcpy(html_buf, swagger_html_template_1);
  sprintf(html_buf + strlen(swagger_html_template_1), swagger_html_template_2,
          openapi_url);
#endif

  res->status_code = 200;
  c_rest_response_html(res, html_buf);
  free(html_buf);
  return 0;
}

int c_rest_enable_swagger_ui(struct c_rest_router *router,
                             const char *docs_path, const char *openapi_url) {
  struct c_rest_openapi_spec *spec;
  if (!router || !docs_path || !openapi_url)
    return 1;

  spec = c_rest_router_get_openapi_spec(router);
  if (spec) {
    if (spec->swagger_openapi_url)
      free(spec->swagger_openapi_url);
    spec->swagger_openapi_url = (char *)malloc(strlen(openapi_url) + 1);
    if (spec->swagger_openapi_url) {
#if defined(_MSC_VER)
      strcpy_s(spec->swagger_openapi_url, strlen(openapi_url) + 1, openapi_url);
#else
      strcpy(spec->swagger_openapi_url, openapi_url);
#endif
    }
  }

  return c_rest_router_add(router, "GET", docs_path, swagger_ui_handler,
                           router);
}
