# FindUCRA.cmake
# Helper script for examples to find UCRA library
#
# This script provides two methods:
# 1. Find installed UCRA package (preferred)
# 2. Fallback to local build directory
#
# Usage in example CMakeLists.txt:
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/FindUCRA.cmake)
#   find_ucra_library()
#   target_link_libraries(my_example ${UCRA_LIBRARIES})

function(find_ucra_library)
    # Try to find installed UCRA first
    find_package(UCRA QUIET)

    if(UCRA_FOUND)
        # Use installed UCRA package
        message(STATUS "✓ Using installed UCRA package")
        set(UCRA_LIBRARIES UCRA::ucra PARENT_SCOPE)
    else()
        # Fallback to local build
        message(STATUS "⚠ UCRA package not found, using local build")
        message(STATUS "  To install UCRA: cd ../../build && sudo make install")

        # Find the main UCRA library from local build
        set(UCRA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
        set(UCRA_BUILD_DIR "${UCRA_ROOT}/build")

        # Check if build directory exists
        if(NOT EXISTS "${UCRA_BUILD_DIR}")
            message(FATAL_ERROR "UCRA build directory not found: ${UCRA_BUILD_DIR}")
        endif()

        # Include UCRA headers
        include_directories("${UCRA_ROOT}/include")

        # Link directories first
        link_directories("${UCRA_BUILD_DIR}")

        # Use direct file paths for absolute control
        set(UCRA_IMPL_LIB "${UCRA_BUILD_DIR}/libucra_impl.a")
        set(CJSON_LIB "${UCRA_BUILD_DIR}/libcjson.a")

        # Verify files exist
        if(NOT EXISTS "${UCRA_IMPL_LIB}")
            message(FATAL_ERROR "UCRA implementation library not found: ${UCRA_IMPL_LIB}")
        endif()

        if(NOT EXISTS "${CJSON_LIB}")
            message(FATAL_ERROR "cJSON library not found: ${CJSON_LIB}")
        endif()

        # Create a convenience variable
        set(UCRA_LIBRARIES_LIST ${UCRA_IMPL_LIB} ${CJSON_LIB})

        # Add platform-specific dependencies
        if(NOT WIN32)
            find_package(Threads REQUIRED)
            list(APPEND UCRA_LIBRARIES_LIST Threads::Threads)
        endif()

        if(UNIX)
            find_library(MATH_LIB m REQUIRED)
            list(APPEND UCRA_LIBRARIES_LIST ${MATH_LIB})
        endif()

        set(UCRA_LIBRARIES ${UCRA_LIBRARIES_LIST} PARENT_SCOPE)

        message(STATUS "✓ Found UCRA libraries: ${UCRA_LIBRARIES_LIST}")
    endif()
endfunction()

# Convenience macro for simple usage
macro(ucra_add_executable TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} ${UCRA_LIBRARIES})
endmacro()
