cmake_minimum_required(VERSION 3.18)

project(ucra_simple_usage C)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find UCRA library (installed package or local build)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/FindUCRA.cmake)
find_ucra_library()

# Create executables - super simple now! 🎉
ucra_add_executable(basic_engine basic_engine.c)
ucra_add_executable(manifest_usage manifest_usage.c)
ucra_add_executable(simple_render simple_render.c)
ucra_add_executable(simple_usage simple_usage.c)
