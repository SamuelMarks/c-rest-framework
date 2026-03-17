cmake_minimum_required(VERSION 3.15)
project(oauth2_example C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Threading support
option(OAUTH2_MULTI_THREADED "Enable multi-threading" ON)
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
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)
if(lto_supported)
    option(OAUTH2_LTO "Enable LTO" ON)
else()
    option(OAUTH2_LTO "Enable LTO" OFF)
endif()

# Fetch dependencies if not found
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." c-rest-framework-build)
else()
    include(FetchContent)
    FetchContent_Declare(
        c-rest-framework
        GIT_REPOSITORY https://github.com/SamuelMarks/c-abstract-http.git
        GIT_TAG master
        SOURCE_SUBDIR c-rest-framework
    )
    FetchContent_MakeAvailable(c-rest-framework)
endif()

# Fetch c-orm if not found
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../c-orm/CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../../c-orm" c-orm-build)
else()
    include(FetchContent)
    FetchContent_Declare(
        c-orm
        GIT_REPOSITORY https://github.com/SamuelMarks/c-orm.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(c-orm)
endif()

# Library versions
add_library(oauth2_example_static STATIC src/oauth2_server.c src/oauth2_client.c)
add_library(oauth2_example_shared SHARED src/oauth2_server.c src/oauth2_client.c)

target_include_directories(oauth2_example_static PUBLIC include)
target_include_directories(oauth2_example_shared PUBLIC include)

target_link_libraries(oauth2_example_static PUBLIC c-rest-framework_static c-orm_static)
target_link_libraries(oauth2_example_shared PUBLIC c-rest-framework_shared c-orm_shared)

if(OAUTH2_LTO AND lto_supported)
    set_property(TARGET oauth2_example_static PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET oauth2_example_shared PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# CLI Executable
add_executable(oauth2_cli src/main.c)
target_link_libraries(oauth2_cli PRIVATE oauth2_example_static)

# Tests
option(OAUTH2_BUILD_TESTS "Build tests" ON)
if(OAUTH2_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        greatest
        URL https://raw.githubusercontent.com/SamuelMarks/greatest/cmake-and-msvc/greatest.h
        DOWNLOAD_NO_EXTRACT TRUE
    )
    FetchContent_Populate(greatest)
    
    add_executable(oauth2_tests tests/test_oauth2.c)
    target_include_directories(oauth2_tests PRIVATE ${greatest_SOURCE_DIR})
    target_link_libraries(oauth2_tests PRIVATE oauth2_example_static)
    add_test(NAME oauth2_test COMMAND oauth2_tests)
endif()
