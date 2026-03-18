cmake_minimum_required(VERSION 3.10)

include(FetchContent)
FetchContent_Declare(
  greatest
  URL https://raw.githubusercontent.com/SamuelMarks/greatest/cmake-and-msvc/greatest.h
  DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_GetProperties(greatest)
if(NOT greatest_POPULATED)
  FetchContent_MakeAvailable(greatest)
endif()

add_executable(c-rest-framework-tests
    test_main.c
    test_multiplatform.c
    test_parser.c
    test_client.c
    test_multipart.c
    test_router.c
    test_request_response.c
    test_orm_integration.c
    test_crypto.c
    test_time.c
    test_tls_context.c
    test_tls_integration.c
    test_oauth2.c
    test_openapi.c
)

target_include_directories(c-rest-framework-tests PRIVATE ../include ${CMAKE_CURRENT_SOURCE_DIR} ${greatest_SOURCE_DIR})
target_link_libraries(c-rest-framework-tests PRIVATE c-rest-framework)

add_test(NAME c-rest-framework-tests COMMAND c-rest-framework-tests)

if(WIN32 AND BUILD_SHARED_LIBS)
    if(TARGET cfs)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:cfs>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
    if(TARGET c89stringutils)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c89stringutils>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
    if(TARGET c-abstract-http)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c-abstract-http>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
    if(TARGET c_orm)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c_orm>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
    if(TARGET c-rest-framework)
        add_custom_command(TARGET c-rest-framework-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c-rest-framework>" "$<TARGET_FILE_DIR:c-rest-framework-tests>"
        )
    endif()
endif()

