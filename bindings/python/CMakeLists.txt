cmake_minimum_required(VERSION 3.18)

project(ucra_python_bindings)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Python first
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Try to find pybind11 using Python
execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import pybind11; print(pybind11.get_cmake_dir())"
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(pybind11_DIR)
    message(STATUS "Found pybind11: ${pybind11_DIR}")
    set(pybind11_DIR ${pybind11_DIR})
    find_package(pybind11 REQUIRED PATHS ${pybind11_DIR})
else()
    message(FATAL_ERROR "pybind11 not found. Please install with: pip install pybind11")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)

# Source files
set(PYBIND_SOURCES
    src/main.cpp
    src/engine_wrapper.cpp
    src/curves_wrapper.cpp
    src/manifest_wrapper.cpp
    src/types_wrapper.cpp
)

# Create the pybind11 module
pybind11_add_module(ucra_py ${PYBIND_SOURCES})

# Link against UCRA shared library target defined by the top-level project
# On all platforms the shared target name is 'ucra_impl' (OUTPUT_NAME is 'ucra_impl').
target_link_libraries(ucra_py PRIVATE ucra_impl)

# Set module properties
set_target_properties(ucra_py PROPERTIES
    OUTPUT_NAME "ucra"
    PREFIX ""
    SUFFIX "${Python_SOABI}${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

# Ensure we find the UCRA shared library if built in top-level 'build'
target_link_directories(ucra_py PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build
    ${CMAKE_BINARY_DIR}
)

# Add include directories
target_include_directories(ucra_py PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${pybind11_INCLUDE_DIRS}
)

# Compiler definitions
target_compile_definitions(ucra_py PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
