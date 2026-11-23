add_executable(app_https_server main.c)
target_link_libraries(app_https_server PRIVATE c-rest-framework)
c_rest_configure_target(app_https_server)
c_rest_copy_runtime_dlls(app_https_server)
