add_executable(app_http23 main.c)
target_link_libraries(app_http23 PRIVATE c-rest-framework)
c_rest_configure_target(app_http23)
c_rest_copy_runtime_dlls(app_http23)
