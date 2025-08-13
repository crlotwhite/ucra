cmake_minimum_required(VERSION 3.18)

project(ucra_basic_rendering C)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find UCRA library (installed package or local build)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/FindUCRA.cmake)
find_ucra_library()

# Create executables - super simple now! 🎉
ucra_add_executable(basic_rendering basic_rendering.c)
ucra_add_executable(multi_note_render multi_note_render.c)
ucra_add_executable(streaming_example streaming_example.c)
ucra_add_executable(wav_output wav_output.c)
