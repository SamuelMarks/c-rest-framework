cmake_minimum_required(VERSION 3.10)

add_executable(c-rest-framework-tests
    # we will add test files here
    test_main.c
    test_multiplatform.c
    test_parser.c
    test_client.c
    test_multipart.c
    test_router.c
    test_request_response.c
    test_orm_integration.c
    test_crypto.c
    test_tls_context.c
    test_tls_integration.c
)

target_include_directories(c-rest-framework-tests PRIVATE ../include)
target_link_libraries(c-rest-framework-tests PRIVATE c-rest-framework)

add_test(NAME c-rest-framework-tests COMMAND c-rest-framework-tests)
