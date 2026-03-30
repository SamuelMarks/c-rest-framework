cmake_minimum_required(VERSION 3.10)
project(app_full_multi C)

add_executable(app_full_multi main.c)

target_include_directories(app_full_multi PRIVATE
    /include
)

target_link_libraries(app_full_multi PRIVATE c-rest-framework)

if(C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING)
    target_compile_definitions(app_full_multi PRIVATE C_REST_ENABLE_FULL_MULTIPART_FORM_STREAMING)
endif()
