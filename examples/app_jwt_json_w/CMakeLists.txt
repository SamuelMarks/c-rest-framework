cmake_minimum_required(VERSION 3.10)
project(app_jwt_json_w C)

add_executable(app_jwt_json_w main.c)
target_link_libraries(app_jwt_json_w PRIVATE c-rest-framework)
