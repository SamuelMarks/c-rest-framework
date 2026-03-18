#ifndef C_REST_OPENAPI_H
#define C_REST_OPENAPI_H

/* clang-format off */
#include <stddef.h>
/* clang-format on */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct c_rest_openapi_schema_ref {
  const char *ref_name; /* Name of the C struct, e.g., "my_request_struct" */
};

struct c_rest_openapi_operation {
  const char *summary;
  const char *description;
  const char **tags;
  size_t n_tags;
  struct c_rest_openapi_schema_ref req_body_schema;
  const char *req_content_type;
  struct c_rest_openapi_schema_ref res_body_schema;
  const char *res_content_type;
};

struct c_rest_openapi_path {
  const char *route;
  struct c_rest_openapi_operation get;
  struct c_rest_openapi_operation post;
  struct c_rest_openapi_operation put;
  struct c_rest_openapi_operation del;
  struct c_rest_openapi_operation patch;
};

struct c_rest_openapi_info {
  const char *title;
  const char *version;
  const char *description;
};

struct c_rest_openapi_spec {
  struct c_rest_openapi_info info;
  struct c_rest_openapi_path *paths;
  size_t n_paths;
  size_t capacity_paths;
  char *swagger_openapi_url;
  char **component_schemas_keys;
  char **component_schemas_json;
  size_t n_components;
  size_t capacity_components;
};

int c_rest_openapi_spec_init(struct c_rest_openapi_spec **out_spec);
int c_rest_openapi_spec_destroy(struct c_rest_openapi_spec *spec);

int c_rest_openapi_spec_add_path(struct c_rest_openapi_spec *spec,
                                 const char *route, const char *method,
                                 const struct c_rest_openapi_operation *op);

/**
 * @brief Add a JSON schema to the components/schemas section.
 * @param spec The spec object.
 * @param schema_name The name of the schema (e.g., "my_request_struct").
 * @param json_schema_str The raw JSON schema string. Will be parsed and merged.
 * @return 0 on success.
 */
int c_rest_openapi_spec_add_component_schema(struct c_rest_openapi_spec *spec,
                                             const char *schema_name,
                                             const char *json_schema_str);

struct c_rest_router;

/**
 * @brief Serialize the OpenAPI spec to a JSON string.
 * @param spec The spec object.
 * @param out_json Allocated string. Must be freed via
 * json_free_serialized_string() from parson.
 * @return 0 on success.
 */
int c_rest_openapi_spec_to_json(const struct c_rest_openapi_spec *spec,
                                char **out_json);

/**
 * @brief Add a route to serve the OpenAPI specification.
 * @param router The router instance.
 * @param path The URL path (e.g., "/openapi.json").
 * @return 0 on success.
 */
int c_rest_enable_openapi(struct c_rest_router *router, const char *path);

/**
 * @brief Add a route to serve Swagger UI documentation.
 * @param router The router instance.
 * @param docs_path The URL path for the UI (e.g., "/docs").
 * @param openapi_url The URL path to the OpenAPI JSON (e.g., "/openapi.json").
 * @return 0 on success.
 */
int c_rest_enable_swagger_ui(struct c_rest_router *router,
                             const char *docs_path, const char *openapi_url);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* C_REST_OPENAPI_H */
