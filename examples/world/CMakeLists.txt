# Example: WORLD-based engine integration using UCRA high-level API
# This is an optional demonstration and does not affect the core build.

cmake_minimum_required(VERSION 3.18)
project(ucra_world_example C CXX)

# Assume this example lives inside the main repository (so UCRA targets exist)
# If copied out-of-tree, user should FetchContent the main repo then include(UCRA.cmake).

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# World source (minimal stub reusing previous implementation idea but simplified)
add_library(ucra_world_engine STATIC world_engine_stub.cpp)

target_include_directories(ucra_world_engine PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../../include
)

target_compile_definitions(ucra_world_engine PUBLIC UCRA_ENGINE_ID="world")

# Example executable that would use world engine once integrated
add_executable(world_demo main_world_demo.c)

target_link_libraries(world_demo PRIVATE ucra_impl ucra_world_engine)

