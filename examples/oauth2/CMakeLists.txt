cmake_minimum_required(VERSION 3.10)
project(oauth2_example C)

if(CMAKE_CROSSCOMPILING)
    set(CMAKE_DISABLE_FIND_PACKAGE_SQLite3 ON)
    set(CMAKE_DISABLE_FIND_PACKAGE_CURL ON)
endif()

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Threading support
if(CMAKE_SYSTEM_NAME STREQUAL "DOS")
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
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../..")
        # Ensure we are actually adding the framework, we could check project name but this is good enough
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." c-rest-framework-build)
    else()
        include(FetchContent)
        FetchContent_Declare(
            c-rest-framework
            GIT_REPOSITORY https://github.com/SamuelMarks/c-rest-framework.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(c-rest-framework)
    endif()
endif()

# Fetch c-orm if not found
if(NOT TARGET c-orm)
    if(FALSE)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../c-orm" c-orm-build)
    else()
        include(FetchContent)
        FetchContent_Declare(
            c-orm
            GIT_REPOSITORY https://github.com/SamuelMarks/c-orm.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(c-orm)
    endif()
endif()

add_library(oauth2_example_static STATIC src/oauth2_server.c src/oauth2_client.c)
add_library(oauth2_example_shared SHARED src/oauth2_server.c src/oauth2_client.c)

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
option(OAUTH2_BUILD_TESTS "Build tests" ON)
if(OAUTH2_BUILD_TESTS)
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
            URL https://raw.githubusercontent.com/SamuelMarks/greatest/master/greatest.h
            DOWNLOAD_NO_EXTRACT TRUE
        )
    endif()
    FetchContent_MakeAvailable(greatest)

    add_executable(oauth2_tests tests/test_oauth2.c)
    target_include_directories(oauth2_tests PRIVATE ${greatest_SOURCE_DIR})
    target_link_libraries(oauth2_tests PRIVATE oauth2_example_static)
    if(EMSCRIPTEN)
        target_link_options(oauth2_tests PRIVATE "-sALLOW_MEMORY_GROWTH=1")
    endif()
    if(OAUTH2_MULTI_THREADED)
        target_link_libraries(oauth2_tests PRIVATE Threads::Threads)
    endif()
    add_test(NAME oauth2_test COMMAND oauth2_tests)

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
    if(OAUTH2_BUILD_TESTS)
        set_property(TARGET oauth2_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()
