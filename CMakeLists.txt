cmake_minimum_required(VERSION 3.18)

project(ucra C CXX)

# Use C99 for compatibility as requested
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set C++ standard for C++ sources
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
# Replace WORLD-dependent engine with pure C engine implementation
set(UCRA_SOURCES src/ucra_manifest.c src/ucra_streaming.c src/ucra_engine.c)

# Static library (existing)
add_library(ucra_impl STATIC ${UCRA_SOURCES})
target_include_directories(ucra_impl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(ucra_impl cjson)
# For static library, consumers should define UCRA_STATIC
target_compile_definitions(ucra_impl PUBLIC UCRA_STATIC)

# Shared library for language bindings
add_library(ucra_impl_shared SHARED ${UCRA_SOURCES})
target_include_directories(ucra_impl_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(ucra_impl_shared cjson)
# For shared library, define UCRA_BUILD when building
target_compile_definitions(ucra_impl_shared PRIVATE UCRA_BUILD)

# Set output name to match what the bindings expect
set_target_properties(ucra_impl_shared PROPERTIES
    OUTPUT_NAME "ucra_impl"
    VERSION 1.0.0
    SOVERSION 1
)

# Link pthread for streaming functionality (Unix only)
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(ucra_impl Threads::Threads)
    target_link_libraries(ucra_impl_shared Threads::Threads)
endif()

# Link math library on Unix systems (required for sin, cos, pow, etc.)
if(UNIX)
    target_link_libraries(ucra_impl m)
    target_link_libraries(ucra_impl_shared m)
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
# Validation and Testing Tools (Task 8)
# ===================================================================

# Option to build tools (disabled by default for production builds)
option(UCRA_BUILD_TOOLS "Build UCRA validation and testing tools" OFF)

# Option to build language bindings
option(UCRA_BUILD_BINDINGS "Build all language bindings" OFF)
option(UCRA_BUILD_CPP_BINDINGS "Build C++ language bindings" ON)
option(UCRA_BUILD_PYTHON_BINDINGS "Build Python language bindings" OFF)
option(UCRA_BUILD_DOTNET_BINDINGS "Build .NET language bindings" OFF)
option(UCRA_BUILD_RUST_BINDINGS "Build Rust language bindings" OFF)

if(UCRA_BUILD_TOOLS)
    # F0 RMSE Calculation Utility
    add_executable(f0_rmse_calc tools/f0_rmse_calc.c)
    target_link_libraries(f0_rmse_calc m)

    # MCD(13) Calculation Utility
    add_executable(mcd_calc tools/mcd_calc.c)
    target_link_libraries(mcd_calc m)

    # Audio Comparison Module
    add_executable(audio_compare tools/audio_compare.c)
    target_link_libraries(audio_compare m)

    # Golden Runner Test Harness
    add_executable(golden_runner tools/golden_runner.c)
    target_link_libraries(golden_runner cjson)

    # Golden WAV generator (direct C API)
    add_executable(create_golden_wav tools/create_golden_wav.c)
    target_link_libraries(create_golden_wav ucra_impl)

    # Validation Suite (Main orchestration tool)
    add_executable(validation_suite tools/validation_suite.c)
    target_link_libraries(validation_suite cjson)

    # OpenUtau manifest generator
    add_executable(ucra_manifest_gen tools/ucra_manifest_gen.c)
    target_link_libraries(ucra_manifest_gen cjson)

    message(STATUS "UCRA Tools will be built")

    # Simple test for manifest generator using sample voicebank if present
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/sample-voicebank/resampler.json)
    add_test(NAME tools_ucra_manifest_gen_test
         COMMAND ucra_manifest_gen --input ${CMAKE_CURRENT_SOURCE_DIR}/examples/sample-voicebank/resampler.json --output ${CMAKE_BINARY_DIR}/openutau_resampler.yaml --exe resampler)
    set_tests_properties(tools_ucra_manifest_gen_test PROPERTIES DEPENDS wrappers_integration_generate_golden)
    endif()
else()
    message(STATUS "UCRA Tools building is disabled")
endif()

include(CTest)
enable_testing()

# Add comprehensive tests
add_test(NAME manifest_parsing_test COMMAND test_manifest)
add_test(NAME comprehensive_test_suite COMMAND test_suite)
add_test(NAME streaming_lifecycle_test COMMAND test_streaming_lifecycle)
add_test(NAME streaming_buffering_test COMMAND test_streaming_buffering)
add_test(NAME streaming_read_test COMMAND test_streaming_read)
add_test(NAME streaming_integration_test COMMAND test_streaming_integration)

# ---------------------------------------------------------------
# Cross-language wrapper integration test (Task 6.5)
# Generates golden WAV via C, then runs C++/.NET/Python/Rust samples
# to produce WAVs and compares them using audio_compare.
# ---------------------------------------------------------------
if(UCRA_BUILD_TOOLS)
    # Generate golden WAV in build directory
    add_test(NAME wrappers_integration_generate_golden
             COMMAND create_golden_wav
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    # Run C++ minimal emitter app (emits cpp_sample_output.wav)
    add_test(NAME wrappers_integration_run_cpp
             COMMAND cpp_emit_wav
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    # Run Python sample (emits python_sample_output.wav)
    find_package(Python COMPONENTS Interpreter QUIET)
    if(Python_Interpreter_FOUND)
        add_test(NAME wrappers_integration_run_python
                 COMMAND ${CMAKE_COMMAND} -E env DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}:$ENV{DYLD_LIBRARY_PATH}
                         ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/bindings/python/examples/sample_emit_wav.py
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()

    # Run .NET sample (emits dotnet_sample_output.wav)
    find_program(DOTNET_EXECUTABLE dotnet)
    if(DOTNET_EXECUTABLE)
        add_test(NAME wrappers_integration_run_dotnet
                 COMMAND ${CMAKE_COMMAND} -E env DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}:$ENV{DYLD_LIBRARY_PATH}
                         ${DOTNET_EXECUTABLE} run --project ${CMAKE_SOURCE_DIR}/bindings/dotnet/samples/UCRA.NET.Sample.csproj -c Release
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()

    # Run Rust sample (emits rust_sample_output.wav)
    find_program(CARGO_EXECUTABLE cargo)
    if(CARGO_EXECUTABLE)
        add_test(NAME wrappers_integration_run_rust
                 COMMAND ${CMAKE_COMMAND} -E env UCRA_LIB_DIR=${CMAKE_BINARY_DIR} DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}:$ENV{DYLD_LIBRARY_PATH}
                         ${CARGO_EXECUTABLE} run --manifest-path ${CMAKE_SOURCE_DIR}/bindings/rust/ucra/Cargo.toml --quiet --example emit_wav
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()

    # Compare outputs against golden
    add_test(NAME compare_cpp_vs_golden
             COMMAND audio_compare golden_output.wav cpp_sample_output.wav
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    if(Python_Interpreter_FOUND)
        add_test(NAME compare_python_vs_golden
                 COMMAND audio_compare golden_output.wav python_sample_output.wav
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
    if(DOTNET_EXECUTABLE)
        add_test(NAME compare_dotnet_vs_golden
                 COMMAND audio_compare golden_output.wav dotnet_sample_output.wav
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
    if(CARGO_EXECUTABLE)
        add_test(NAME compare_rust_vs_golden
                 COMMAND audio_compare golden_output.wav rust_sample_output.wav
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
endif()

# Add validation tool tests (only if tools are enabled)
if(UCRA_BUILD_TOOLS)
    add_test(NAME validation_suite_test COMMAND validation_suite --help)
    # Note: Other tools require specific input files, so we only test --help or basic execution
    # For production use, these would be tested with proper test data
    set_tests_properties(validation_suite_test PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage:|Golden Runner Test Harness")
endif()

# Set test working directory to tests/ to find data files
set_tests_properties(manifest_parsing_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set_tests_properties(comprehensive_test_suite PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

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
    # If sample voicebank is not present, disable the manifest_usage test to avoid false failures
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/sample-voicebank/resampler.json)
        set_tests_properties(example_manifest_usage PROPERTIES DISABLED TRUE)
    endif()
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

# ===================================================================
# Language Bindings (Task 6)
# ===================================================================

# Add language bindings if requested
if(UCRA_BUILD_BINDINGS)
    # If general bindings flag is set, enable all specific bindings
    set(UCRA_BUILD_CPP_BINDINGS ON)
    set(UCRA_BUILD_PYTHON_BINDINGS ON)
    set(UCRA_BUILD_DOTNET_BINDINGS ON)
    set(UCRA_BUILD_RUST_BINDINGS ON)
endif()

if(UCRA_BUILD_CPP_BINDINGS)
    add_subdirectory(bindings/cpp)
    message(STATUS "C++ bindings will be built")
endif()

if(UCRA_BUILD_PYTHON_BINDINGS)
    add_subdirectory(bindings/python)
    message(STATUS "Python bindings will be built")
endif()

if(UCRA_BUILD_DOTNET_BINDINGS)
    add_subdirectory(bindings/dotnet)
    message(STATUS ".NET bindings will be built")
endif()

if(UCRA_BUILD_RUST_BINDINGS)
    add_subdirectory(bindings/rust)
    message(STATUS "Rust bindings will be built")
endif()
