cmake_minimum_required(VERSION 3.14)
project(oauth2_example C)

if(CMAKE_CROSSCOMPILING)
    set(CMAKE_DISABLE_FIND_PACKAGE_SQLite3 ON)
    set(CMAKE_DISABLE_FIND_PACKAGE_CURL ON)
endif()

if(NOT MSVC)
    set(CMAKE_C_STANDARD 90)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)
endif()

# Threading support
if(CMAKE_SYSTEM_NAME STREQUAL "DOS" OR EMSCRIPTEN OR (DEFINED CDD_THREADING AND NOT CDD_THREADING))
    option(OAUTH2_MULTI_THREADED "Enable multi-threading" OFF)
else()
    option(OAUTH2_MULTI_THREADED "Enable multi-threading" ON)
endif()
if(OAUTH2_MULTI_THREADED)
    add_compile_definitions(OAUTH2_MULTI_THREADED=1)
    find_package(Threads REQUIRED)
endif()

# Charset support
option(OAUTH2_UNICODE "Enable UNICODE" OFF)
if(OAUTH2_UNICODE)
    add_compile_definitions(UNICODE _UNICODE)
endif()

# LTO support
if(DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION AND CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    option(OAUTH2_LTO "Enable LTO" ON)
else()
    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_supported OUTPUT error)
    if(lto_supported)
        option(OAUTH2_LTO "Enable LTO" OFF)
    else()
        option(OAUTH2_LTO "Enable LTO" OFF)
    endif()
endif()

# Fetch c-rest-framework if not found
if(NOT TARGET c-rest-framework)
    include(FetchContent)
    FetchContent_Declare(
        c-rest-framework
        GIT_REPOSITORY https://github.com/SamuelMarks/c-rest-framework.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(c-rest-framework)
endif()

# Fetch c-orm if not found
if(NOT TARGET c-orm)
        include(FetchContent)
        FetchContent_Declare(
            c-orm
            GIT_REPOSITORY https://github.com/SamuelMarks/c-orm.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(c-orm)
endif()

add_library(oauth2_example_static STATIC src/oauth2_server.c src/oauth2_client.c)
add_library(oauth2_example_shared SHARED src/oauth2_server.c src/oauth2_client.c)

if(CRF_DEMO_HAD_COVERAGE)
    target_compile_options(oauth2_example_static PRIVATE --coverage)
    target_compile_options(oauth2_example_shared PRIVATE --coverage)
endif()

# Add "header-only" interface
add_library(oauth2_example_header_only INTERFACE)

target_include_directories(oauth2_example_static PUBLIC include)
target_include_directories(oauth2_example_shared PUBLIC include)
target_include_directories(oauth2_example_header_only INTERFACE include)

target_link_libraries(oauth2_example_static PUBLIC c-rest-framework c-orm)
target_link_libraries(oauth2_example_shared PUBLIC c-rest-framework c-orm)

# CLI Executable
add_executable(oauth2_cli src/main.c)
target_link_libraries(oauth2_cli PRIVATE oauth2_example_static)
if(OAUTH2_MULTI_THREADED)
    target_link_libraries(oauth2_cli PRIVATE Threads::Threads)
endif()

# Tests
if(BUILD_TESTING)
    set(OAUTH2_BUILD_TESTS_DEFAULT ON)
else()
    set(OAUTH2_BUILD_TESTS_DEFAULT OFF)
endif()
option(OAUTH2_BUILD_TESTS "Build tests" ${OAUTH2_BUILD_TESTS_DEFAULT})
if(OAUTH2_BUILD_TESTS AND BUILD_TESTING)
    enable_testing()
    include(FetchContent)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../greatest/greatest.h")
        message(STATUS "Using local greatest for tests")
        FetchContent_Declare(
          greatest
          SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../greatest"
        )
    else()
        FetchContent_Declare(
            greatest
            GIT_REPOSITORY https://github.com/SamuelMarks/greatest.git
            GIT_TAG master
        )
    endif()
    FetchContent_MakeAvailable(greatest)

    set(RUNNER_OAUTH2_SRC "${CMAKE_CURRENT_BINARY_DIR}/runner_oauth2.c")
    file(WRITE "${RUNNER_OAUTH2_SRC}"
"/* clang-format off */\n#include \"greatest.h\"\n#include <stdlib.h>\n/* clang-format on */\nSUITE_EXTERN(oauth2_suite);\nGREATEST_MAIN_DEFS();\n#if defined(__EMSCRIPTEN__)\nextern void emscripten_force_exit(int status);\n#endif\nint main(int argc, char **argv) {\n  int greatest_passed = 0;\n  GREATEST_MAIN_BEGIN();\n  RUN_SUITE(oauth2_suite);\n  GREATEST_PRINT_REPORT();\n  greatest_all_passed(&greatest_passed);\n#if defined(__EMSCRIPTEN__)\n  emscripten_force_exit(greatest_passed ? 0 : 1);\n#endif\n  return (greatest_passed ? EXIT_SUCCESS : EXIT_FAILURE);\n}\n")
    add_executable(oauth2_tests tests/test_oauth2.c "${RUNNER_OAUTH2_SRC}")
    target_include_directories(oauth2_tests PRIVATE ${greatest_SOURCE_DIR})
    target_link_libraries(oauth2_tests PRIVATE oauth2_example_static)
    if(CRF_DEMO_HAD_COVERAGE)
        target_compile_options(oauth2_tests PRIVATE --coverage)
        target_link_libraries(oauth2_tests PRIVATE --coverage)
    endif()
    if(EMSCRIPTEN)
        target_link_options(oauth2_tests PRIVATE "-sALLOW_MEMORY_GROWTH=1" "-sSTACK_SIZE=1048576" "-sEXIT_RUNTIME=1")
    endif()
    if(OAUTH2_MULTI_THREADED)
        target_link_libraries(oauth2_tests PRIVATE Threads::Threads)
    endif()
    add_test(NAME oauth2_test COMMAND oauth2_tests)
    set_tests_properties(oauth2_test PROPERTIES TIMEOUT 300)

    if(WIN32 AND BUILD_SHARED_LIBS)
        if(TARGET cfs)
            add_custom_command(TARGET oauth2_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:cfs>" "$<TARGET_FILE_DIR:oauth2_tests>"
            )
        endif()
        if(TARGET c89stringutils)
            add_custom_command(TARGET oauth2_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c89stringutils>" "$<TARGET_FILE_DIR:oauth2_tests>"
            )
        endif()
        if(TARGET c-abstract-http)
            add_custom_command(TARGET oauth2_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c-abstract-http>" "$<TARGET_FILE_DIR:oauth2_tests>"
            )
        endif()
        if(TARGET c_orm)
            add_custom_command(TARGET oauth2_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c_orm>" "$<TARGET_FILE_DIR:oauth2_tests>"
            )
        endif()
        if(TARGET c-rest-framework)
            add_custom_command(TARGET oauth2_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c-rest-framework>" "$<TARGET_FILE_DIR:oauth2_tests>"
            )
        endif()
        if(TARGET c_cdd)
            add_custom_command(TARGET oauth2_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c_cdd>" "$<TARGET_FILE_DIR:oauth2_tests>"
            )
        endif()
        if(TARGET c_str_span)
            add_custom_command(TARGET oauth2_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c_str_span>" "$<TARGET_FILE_DIR:oauth2_tests>"
            )
        endif()
        if(TARGET oauth2_example_shared)
            add_custom_command(TARGET oauth2_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:oauth2_example_shared>" "$<TARGET_FILE_DIR:oauth2_tests>"
            )
        endif()
    endif()
endif()

if(OAUTH2_LTO AND lto_supported)
    set_property(TARGET oauth2_example_static PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET oauth2_example_shared PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET oauth2_cli PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    if(OAUTH2_BUILD_TESTS AND TARGET oauth2_tests)
        set_property(TARGET oauth2_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()
