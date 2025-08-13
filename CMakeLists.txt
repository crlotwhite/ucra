cmake_minimum_required(VERSION 3.18)

project(ucra C CXX)

# Phase 1: Introduce high-level JUCE-style CMake API (non-intrusive)
# Users can optionally replace legacy logic with helper calls in future phases.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/UCRA.cmake OPTIONAL RESULT_VARIABLE _UCRA_HELPER)
if(_UCRA_HELPER)
    message(STATUS "UCRA high-level API loaded (Phase 1). Legacy build still active.")
    cmake_minimum_required(VERSION 3.18)
    project(ucra C CXX)

    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/UCRA.cmake)

    ucra_setup()
    ucra_add_core(TARGET ucra)
    ucra_add_tests()
    ucra_add_examples()
    ucra_export_package(VERSION 1.0.0)
    ucra_print_config()
install(EXPORT UCRATargets
        FILE UCRATargets.cmake
        NAMESPACE UCRA::
        DESTINATION lib/cmake/UCRA)
