cmake_minimum_required(VERSION 3.14)
project(oauth2_example C)

if(CMAKE_CROSSCOMPILING)
    set(CMAKE_DISABLE_FIND_PACKAGE_SQLite3 ON)
    set(CMAKE_DISABLE_FIND_PACKAGE_CURL ON)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "DOS" OR EMSCRIPTEN OR (DEFINED C_REST_FRAMEWORK_THREADING AND NOT C_REST_FRAMEWORK_THREADING) OR (DEFINED CDD_THREADING AND NOT CDD_THREADING))
    option(OAUTH2_MULTI_THREADED "Enable multi-threading" OFF)
else()
    option(OAUTH2_MULTI_THREADED "Enable multi-threading" ON)
endif()

if(OAUTH2_MULTI_THREADED)
    find_package(Threads REQUIRED)
endif()

option(OAUTH2_UNICODE "Enable UNICODE" OFF)

if(MSVC AND (CMAKE_C_FLAGS MATCHES "[/-]RTC" OR (DEFINED C_REST_FRAMEWORK_MSVC_RTC AND NOT "${C_REST_FRAMEWORK_MSVC_RTC}" STREQUAL "OFF" AND NOT "${C_REST_FRAMEWORK_MSVC_RTC}" STREQUAL "") OR (DEFINED CDD_MSVC_RTC AND NOT "${CDD_MSVC_RTC}" STREQUAL "OFF" AND NOT "${CDD_MSVC_RTC}" STREQUAL "")))
    option(OAUTH2_LTO "Enable LTO" OFF)
    set(OAUTH2_LTO OFF CACHE BOOL "Enable LTO" FORCE)
    set(lto_supported FALSE)
elseif(DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION AND CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    if(MSVC)
        foreach(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            string(REGEX REPLACE "[/-]RTC[1suc]+" "" ${flag_var} "${${flag_var}}")
        endforeach()
    endif()
    option(OAUTH2_LTO "Enable LTO" ON)
    set(lto_supported TRUE)
else()
    option(OAUTH2_LTO "Enable LTO" OFF)
    if(OAUTH2_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT lto_supported OUTPUT error)
    else()
        set(lto_supported FALSE)
    endif()
endif()

if(NOT TARGET c-rest-framework)
    include(FetchContent)
    FetchContent_Declare(
        c-rest-framework
        GIT_REPOSITORY https://github.com/SamuelMarks/c-rest-framework.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(c-rest-framework)
endif()

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
add_library(oauth2_example_header_only INTERFACE)

set_target_properties(oauth2_example_shared PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
set_target_properties(oauth2_example_static PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(oauth2_example_static PUBLIC include)
target_include_directories(oauth2_example_shared PUBLIC include)
target_include_directories(oauth2_example_header_only INTERFACE include)

target_link_libraries(oauth2_example_static PUBLIC c-rest-framework c-orm)
target_link_libraries(oauth2_example_shared PUBLIC c-rest-framework c-orm)
target_link_libraries(oauth2_example_header_only INTERFACE c-rest-framework c-orm)

if(OAUTH2_MULTI_THREADED)
    target_compile_definitions(oauth2_example_static PUBLIC OAUTH2_MULTI_THREADED=1)
    target_compile_definitions(oauth2_example_shared PUBLIC OAUTH2_MULTI_THREADED=1)
    target_compile_definitions(oauth2_example_header_only INTERFACE OAUTH2_MULTI_THREADED=1)
    target_link_libraries(oauth2_example_static PUBLIC Threads::Threads)
    target_link_libraries(oauth2_example_shared PUBLIC Threads::Threads)
    target_link_libraries(oauth2_example_header_only INTERFACE Threads::Threads)
endif()

if(OAUTH2_UNICODE)
    target_compile_definitions(oauth2_example_static PUBLIC UNICODE _UNICODE)
    target_compile_definitions(oauth2_example_shared PUBLIC UNICODE _UNICODE)
    target_compile_definitions(oauth2_example_header_only INTERFACE UNICODE _UNICODE)
endif()

if(COMMAND c_rest_configure_target)
    c_rest_configure_target(oauth2_example_static)
    c_rest_configure_target(oauth2_example_shared)
endif()
if(CRF_COVERAGE_ENABLED)
    target_compile_options(oauth2_example_static PRIVATE --coverage)
endif()

add_executable(oauth2_cli src/main.c)
target_link_libraries(oauth2_cli PRIVATE oauth2_example_static)
if(COMMAND c_rest_configure_target)
    c_rest_configure_target(oauth2_cli)
endif()
if(COMMAND c_rest_copy_runtime_dlls)
    c_rest_copy_runtime_dlls(oauth2_cli)
endif()

if(BUILD_TESTING)
    set(OAUTH2_BUILD_TESTS_DEFAULT ON)
else()
    set(OAUTH2_BUILD_TESTS_DEFAULT OFF)
endif()
option(OAUTH2_BUILD_TESTS "Build tests" ${OAUTH2_BUILD_TESTS_DEFAULT})

if(OAUTH2_BUILD_TESTS AND BUILD_TESTING)
    enable_testing()
    if(NOT TARGET c_rest_greatest)
        include(FetchContent)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../greatest/greatest.h")
            FetchContent_Declare(
                greatest
                SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../greatest"
            )
        else()
            FetchContent_Declare(
                greatest
                GIT_REPOSITORY https://github.com/SamuelMarks/greatest.git
                GIT_TAG master
            )
        endif()
        FetchContent_MakeAvailable(greatest)
        add_library(c_rest_greatest INTERFACE)
        target_include_directories(c_rest_greatest SYSTEM INTERFACE "${greatest_SOURCE_DIR}")
    endif()

    set(RUNNER_OAUTH2_SRC "${CMAKE_CURRENT_BINARY_DIR}/runner_oauth2.c")
    if(NOT EXISTS "${RUNNER_OAUTH2_SRC}")
        file(WRITE "${RUNNER_OAUTH2_SRC}" [[/* clang-format off */
#include "greatest.h"
#include <stdlib.h>
/* clang-format on */
SUITE_EXTERN(oauth2_suite);
GREATEST_MAIN_DEFS();
#if defined(__EMSCRIPTEN__)
extern void emscripten_force_exit(int status);
#endif
int main(int argc, char **argv) {
  int greatest_passed = 0;
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(oauth2_suite);
  GREATEST_PRINT_REPORT();
  greatest_all_passed(&greatest_passed);
#if defined(__EMSCRIPTEN__)
  emscripten_force_exit(greatest_passed ? 0 : 1);
#endif
  return (greatest_passed ? EXIT_SUCCESS : EXIT_FAILURE);
}
]])
    endif()

    add_executable(oauth2_tests tests/test_oauth2.c "${RUNNER_OAUTH2_SRC}")
    target_link_libraries(oauth2_tests PRIVATE oauth2_example_static c_rest_greatest)
    if(COMMAND c_rest_configure_target)
        c_rest_configure_target(oauth2_tests)
    endif()
    if(CRF_COVERAGE_ENABLED)
        target_compile_options(oauth2_tests PRIVATE --coverage)
    endif()
    if(EMSCRIPTEN)
        target_link_options(oauth2_tests PRIVATE
            "-sALLOW_MEMORY_GROWTH=1"
            "-sSTACK_SIZE=1048576"
            "-sEXIT_RUNTIME=1"
        )
    endif()
    if(COMMAND c_rest_copy_runtime_dlls)
        c_rest_copy_runtime_dlls(oauth2_tests)
    endif()
    if(WIN32 AND BUILD_SHARED_LIBS AND TARGET oauth2_example_shared)
        add_custom_command(TARGET oauth2_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:oauth2_example_shared>"
                "$<TARGET_FILE_DIR:oauth2_tests>"
        )
    endif()
    add_test(NAME oauth2_test COMMAND oauth2_tests)
    set_tests_properties(oauth2_test PROPERTIES TIMEOUT 300)
endif()

if(OAUTH2_LTO AND lto_supported)
    set_property(TARGET oauth2_example_static PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET oauth2_example_shared PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET oauth2_cli PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    if(OAUTH2_BUILD_TESTS AND TARGET oauth2_tests)
        set_property(TARGET oauth2_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()
