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

# MSVC: Treat all source files as UTF-8 to support non-ASCII literals
if(MSVC)
    add_compile_options(/utf-8)
endif()

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

# Ensure consumers of the installed package build against the static API
# by propagating UCRA_STATIC via the INTERFACE target. This avoids
# __declspec(dllimport) on Windows when linking the static library.
target_compile_definitions(ucra INTERFACE UCRA_STATIC)

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

# ===================================================================
# Core Testing & Validation Toolchain (Task 8)
# ===================================================================

# Golden Runner Test Harness
add_executable(golden_runner src/golden_runner.c)
target_link_libraries(golden_runner ucra_impl)

# Audio Comparison Module
add_executable(audio_compare src/audio_compare.c)
target_link_libraries(audio_compare ucra_impl)

# F0 RMSE Calculation Utility
add_executable(f0_rmse_calc src/f0_rmse_calc.c)
target_link_libraries(f0_rmse_calc ucra_impl)

# MCD(13) Calculation Utility
add_executable(mcd_calc src/mcd_calc.c)
target_link_libraries(mcd_calc ucra_impl)

# Validation Suite Integration Tool
add_executable(validation_suite src/validation_suite.c)
target_link_libraries(validation_suite ucra_impl)

# Test WAV file generator utility
add_executable(create_test_wav src/create_test_wav.c)

include(CTest)
enable_testing()

# Add comprehensive tests
add_test(NAME manifest_parsing_test COMMAND test_manifest)
add_test(NAME comprehensive_test_suite COMMAND test_suite)
add_test(NAME streaming_lifecycle_test COMMAND test_streaming_lifecycle)
add_test(NAME streaming_buffering_test COMMAND test_streaming_buffering)
add_test(NAME streaming_read_test COMMAND test_streaming_read)
add_test(NAME streaming_integration_test COMMAND test_streaming_integration)

# Add Core Testing & Validation Toolchain tests (Task 8)
add_test(NAME validation_suite_test COMMAND validation_suite --test-data ${CMAKE_CURRENT_SOURCE_DIR}/tests/data --output ${CMAKE_CURRENT_BINARY_DIR}/tests/output --tools ${CMAKE_CURRENT_BINARY_DIR}/Release)
add_test(NAME golden_runner_test COMMAND golden_runner ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/test_case_001 ${CMAKE_CURRENT_BINARY_DIR}/tests/output/test_output.wav)
add_test(NAME audio_compare_test COMMAND audio_compare ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/golden_ref.wav ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/test_sample.wav)
add_test(NAME f0_rmse_test COMMAND f0_rmse_calc ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/test_f0_curve.txt ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/test_f0_curve.txt)

# Set test working directory to tests/ to find data files
set_tests_properties(manifest_parsing_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set_tests_properties(comprehensive_test_suite PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set_tests_properties(validation_suite_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(golden_runner_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(audio_compare_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(f0_rmse_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(tests)

# ===================================================================
# Examples Building and Testing
# ===================================================================

# Option to build examples (enabled by default)
option(UCRA_BUILD_EXAMPLES "Build UCRA examples" ON)

if(UCRA_BUILD_EXAMPLES)
    # Create examples directory and add custom targets for examples
    # Instead of using add_subdirectory directly, we'll build examples
    # as external projects that depend on the main library

    # First, we need to define a custom target that builds examples
    # after the main library is built

    # Create a function to add example executables that depend on ucra_impl
    function(add_example_executable target_name source_file working_dir)
        add_executable(${target_name} ${working_dir}/${source_file})
        target_link_libraries(${target_name} ucra_impl)
        target_include_directories(${target_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

        # Add as a test
        add_test(NAME example_${target_name} COMMAND ${target_name})
        set_tests_properties(example_${target_name} PROPERTIES
            WORKING_DIRECTORY ${working_dir})
    endfunction()

    # Add simple-usage examples
    add_example_executable(basic_engine basic_engine.c
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple-usage)
    add_example_executable(manifest_usage manifest_usage.c
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple-usage)
    add_example_executable(simple_render simple_render.c
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple-usage)
    add_example_executable(simple_usage simple_usage.c
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple-usage)

    # Add basic-rendering examples
    add_example_executable(basic_rendering basic_rendering.c
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/basic-rendering)
    add_example_executable(multi_note_render multi_note_render.c
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/basic-rendering)
    add_example_executable(streaming_example streaming_example.c
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/basic-rendering)
    add_example_executable(wav_output wav_output.c
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/basic-rendering)

    message(STATUS "UCRA Examples will be built and tested")
else()
    message(STATUS "UCRA Examples building is disabled")
endif()

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
