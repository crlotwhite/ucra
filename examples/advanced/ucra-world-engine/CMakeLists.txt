cmake_minimum_required(VERSION 3.18)

project(ucra C CXX)

# Use C99 for compatibility as requested
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Use C++11 for WORLD compatibility
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include ExternalProject module
include(ExternalProject)

# Option to enable WORLD engine support
option(UCRA_ENABLE_WORLD "Enable WORLD vocoder engine" ON)

if(UCRA_ENABLE_WORLD)
    # Download and build WORLD vocoder library
    ExternalProject_Add(world
        GIT_REPOSITORY https://github.com/mmorise/World.git
        GIT_TAG master
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/world_install
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/world_install
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_STANDARD=11
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        BUILD_BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/world_install/lib/libworld.a
            ${CMAKE_CURRENT_BINARY_DIR}/world_install/include/world/d4c.h
            ${CMAKE_CURRENT_BINARY_DIR}/world_install/include/world/dio.h
            ${CMAKE_CURRENT_BINARY_DIR}/world_install/include/world/harvest.h
            ${CMAKE_CURRENT_BINARY_DIR}/world_install/include/world/cheaptrick.h
            ${CMAKE_CURRENT_BINARY_DIR}/world_install/include/world/stonemask.h
            ${CMAKE_CURRENT_BINARY_DIR}/world_install/include/world/synthesis.h
            ${CMAKE_CURRENT_BINARY_DIR}/world_install/include/world/synthesisrealtime.h
            ${CMAKE_CURRENT_BINARY_DIR}/world_install/include/world/constantnumbers.h
            ${CMAKE_CURRENT_BINARY_DIR}/world_install/include/world/common.h
    )

    # Create an interface library for WORLD
    add_library(world_lib INTERFACE)
    add_dependencies(world_lib world)
    target_include_directories(world_lib INTERFACE
        ${CMAKE_CURRENT_BINARY_DIR}/world_install/include)
    target_link_libraries(world_lib INTERFACE
        ${CMAKE_CURRENT_BINARY_DIR}/world_install/lib/libworld.a)
endif()

# Define UCRA_STATIC globally for static library builds
# This prevents Windows DLL import/export issues (LNK2019 errors)
# by ensuring UCRA_API resolves to empty instead of __declspec(dllimport)
add_compile_definitions(UCRA_STATIC)
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Use parent project's cJSON library (search downloaded build artifacts)
# On Windows (multi-config), libraries live in subfolders like build/Debug or build/Release.
set(_CJSON_SEARCH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../build")
set(_CJSON_HINTS "${_CJSON_SEARCH_ROOT}" "${_CJSON_SEARCH_ROOT}/build")
if(WIN32)
    # Try common multi-config subfolders as suffixes
    set(_CJSON_SUFFIXES "${CMAKE_BUILD_TYPE}" Debug Release x64/Debug x64/Release)
    list(REMOVE_DUPLICATES _CJSON_SUFFIXES)
    find_library(CJSON_LIB cjson
        HINTS ${_CJSON_HINTS}
        PATH_SUFFIXES ${_CJSON_SUFFIXES}
        NO_DEFAULT_PATH)
else()
    find_library(CJSON_LIB cjson
        HINTS ${_CJSON_HINTS}
        NO_DEFAULT_PATH)
endif()
if(NOT CJSON_LIB)
    message(FATAL_ERROR "cJSON library not found under ${_CJSON_SEARCH_ROOT}.\n"
                        "Make sure the main build artifacts were downloaded to 'build/' ")
endif()

# Ensure cJSON headers are visible for ALL targets (tests, helpers) built in this
# standalone advanced example directory during the separate CI job. Without this
# the test sources like tests/poc_parser.c fail to locate "cJSON.h".
include_directories(../../../third-party)

# Public header-only interface for now
add_library(ucra INTERFACE)

target_include_directories(ucra INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
    $<INSTALL_INTERFACE:include>
)

# Main UCRA library implementation
set(UCRA_SOURCES src/ucra_manifest.c src/ucra_streaming.c)

# Add WORLD engine wrapper if enabled
if(UCRA_ENABLE_WORLD)
    list(APPEND UCRA_SOURCES src/ucra_world_engine.cpp)
endif()

add_library(ucra_impl STATIC ${UCRA_SOURCES})
target_include_directories(ucra_impl PUBLIC ../../../include)
# Ensure third-party (cJSON) headers are visible when building this advanced example
# In the separated CI job we build this directory standalone, so the top-level
# third-party include path must be added explicitly for "cJSON.h" includes.
target_include_directories(ucra_impl PRIVATE ../../../third-party)
target_link_libraries(ucra_impl ${CJSON_LIB})

# Link WORLD library if enabled
if(UCRA_ENABLE_WORLD)
    target_link_libraries(ucra_impl world_lib)
    target_compile_definitions(ucra_impl PUBLIC UCRA_HAS_WORLD)
endif()

# Link pthread for streaming functionality (Unix only)
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(ucra_impl Threads::Threads)
endif()

# Link math library on Unix systems (required for sin, cos, pow, etc.)
if(UNIX)
    target_link_libraries(ucra_impl m)
endif()

# Update interface library to include implementation
target_link_libraries(ucra INTERFACE ucra_impl)

# Proof of concept parser for cJSON evaluation
add_executable(poc_parser tests/poc_parser.c)
target_link_libraries(poc_parser ${CJSON_LIB})

# Official manifest parser test
add_executable(test_manifest tests/test_manifest.c)
target_link_libraries(test_manifest ucra_impl)

# Comprehensive test suite
add_executable(test_suite tests/test_suite.c)
target_link_libraries(test_suite ucra_impl)

# Streaming API lifecycle test
add_executable(test_streaming_lifecycle tests/test_streaming_lifecycle.c)
target_link_libraries(test_streaming_lifecycle ucra_impl)

# Streaming API buffering test
add_executable(test_streaming_buffering tests/test_streaming_buffering.c)
target_link_libraries(test_streaming_buffering ucra_impl)

# Streaming API read function test
add_executable(test_streaming_read tests/test_streaming_read.c)
target_link_libraries(test_streaming_read ucra_impl)

# Streaming API integration test
add_executable(test_streaming_integration tests/test_streaming_integration.c)
target_link_libraries(test_streaming_integration ucra_impl)

# WORLD Engine lifecycle test (only if WORLD is enabled)
if(UCRA_ENABLE_WORLD)
    add_executable(test_world_engine tests/test_world_engine.c)
    target_link_libraries(test_world_engine ucra_impl)

    add_executable(test_world_render tests/test_world_render.c)
    target_link_libraries(test_world_render ucra_impl)

    add_executable(test_world_comprehensive tests/test_world_comprehensive.c)
    target_link_libraries(test_world_comprehensive ucra_impl)
endif()

# UCRA Legacy CLI Bridge (resampler.exe replacement)
add_executable(resampler src/resampler_cli.c)
target_link_libraries(resampler ucra_impl)

include(CTest)
enable_testing()

# Add comprehensive tests
add_test(NAME manifest_parsing_test COMMAND test_manifest)
add_test(NAME comprehensive_test_suite COMMAND test_suite)
add_test(NAME streaming_lifecycle_test COMMAND test_streaming_lifecycle)
add_test(NAME streaming_buffering_test COMMAND test_streaming_buffering)
add_test(NAME streaming_read_test COMMAND test_streaming_read)
add_test(NAME streaming_integration_test COMMAND test_streaming_integration)

# Add WORLD engine test if enabled
if(UCRA_ENABLE_WORLD)
    add_test(NAME world_engine_test COMMAND test_world_engine)
    add_test(NAME world_render_test COMMAND test_world_render)
    add_test(NAME world_comprehensive_test COMMAND test_world_comprehensive)
endif()

# Set test working directory to tests/ to find data files
set_tests_properties(manifest_parsing_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set_tests_properties(comprehensive_test_suite PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_subdirectory(tests)
