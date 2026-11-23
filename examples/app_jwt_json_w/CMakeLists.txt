add_executable(app_jwt_json_w main.c)
target_link_libraries(app_jwt_json_w PRIVATE c-rest-framework)
c_rest_configure_target(app_jwt_json_w)
c_rest_copy_runtime_dlls(app_jwt_json_w)
