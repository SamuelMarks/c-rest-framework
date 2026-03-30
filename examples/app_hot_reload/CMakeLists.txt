cmake_minimum_required(VERSION 3.10)
project(app_hot_reload C)

add_executable(app_hot_reload main.c)
target_link_libraries(app_hot_reload PRIVATE c-rest-framework)
if(WIN32)
  target_link_libraries(app_hot_reload PRIVATE ws2_32)
endif()

