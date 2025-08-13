cmake_minimum_required(VERSION 3.18)

project(ucra C)

# Use C99 for compatibility as requested
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add cJSON library from third-party sources
add_library(cjson STATIC third-party/cJSON.c)
target_include_directories(cjson PUBLIC third-party)

# Public header-only interface for now
add_library(ucra INTERFACE)

target_include_directories(ucra INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Main UCRA library implementation
add_library(ucra_impl STATIC src/ucra_manifest.c)
target_include_directories(ucra_impl PUBLIC include)
target_link_libraries(ucra_impl cjson)

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

include(CTest)
enable_testing()

# Add comprehensive tests
add_test(NAME manifest_parsing_test COMMAND test_manifest)
add_test(NAME comprehensive_test_suite COMMAND test_suite)

# Set test working directory to tests/ to find data files
set_tests_properties(manifest_parsing_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set_tests_properties(comprehensive_test_suite PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_subdirectory(tests)
