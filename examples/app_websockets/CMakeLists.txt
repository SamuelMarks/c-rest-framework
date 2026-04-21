cmake_minimum_required(VERSION 3.10)

project(app_websockets C)

add_executable(app_websockets main.c)

target_link_libraries(app_websockets PRIVATE c-rest-framework)

if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET app_websockets POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c-rest-framework>" "$<TARGET_FILE_DIR:app_websockets>"
    )
endif()
