cmake_minimum_required(VERSION 3.10)

add_executable(app_server_sent_events_sse main.c)

target_link_libraries(app_server_sent_events_sse PRIVATE c-rest-framework)
