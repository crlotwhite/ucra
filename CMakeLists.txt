cmake_minimum_required(VERSION 3.18)

project(ucra C CXX)

# Use C99 for compatibility as requested
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set C++ standard for C++ sources
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Define UCRA_STATIC globally for static library builds
# This prevents Windows DLL import/export issues (LNK2019 errors)
# by ensuring UCRA_API resolves to empty instead of __declspec(dllimport)
add_compile_definitions(UCRA_STATIC)

# Add cJSON library from third-party sources
add_library(cjson STATIC third-party/cJSON.c)
target_include_directories(cjson PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third-party>
    $<INSTALL_INTERFACE:include/third-party>
)

# Public header-only interface for now
add_library(ucra INTERFACE)

target_include_directories(ucra INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Main UCRA library implementation
set(UCRA_SOURCES src/ucra_manifest.c src/ucra_streaming.c src/ucra_world_engine.cpp)

add_library(ucra_impl STATIC ${UCRA_SOURCES})
target_include_directories(ucra_impl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(ucra_impl cjson)

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
target_link_libraries(poc_parser cjson)

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

# Set test working directory to tests/ to find data files
set_tests_properties(manifest_parsing_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set_tests_properties(comprehensive_test_suite PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_subdirectory(tests)

# ===================================================================
# Installation and Packaging
# ===================================================================

# Install headers
install(DIRECTORY include/ucra
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

# Install third-party headers (cJSON)
install(FILES third-party/cJSON.h
        DESTINATION include/third-party)

# Install libraries
install(TARGETS ucra ucra_impl cjson
        EXPORT UCRATargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

# Install executables
install(TARGETS resampler
        DESTINATION bin)

# Create and install CMake config files
include(CMakePackageConfigHelpers)

# Generate the config file that includes the targets
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/UCRAConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/UCRAConfig.cmake"
    INSTALL_DESTINATION lib/cmake/UCRA
)

# Generate the version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/UCRAConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

# Install the config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/UCRAConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/UCRAConfigVersion.cmake"
    DESTINATION lib/cmake/UCRA
)

# Install the targets
install(EXPORT UCRATargets
        FILE UCRATargets.cmake
        NAMESPACE UCRA::
        DESTINATION lib/cmake/UCRA)
