add_executable(app_openapi main.c)
target_link_libraries(app_openapi PRIVATE c-rest-framework)
c_rest_configure_target(app_openapi)
c_rest_copy_runtime_dlls(app_openapi)
