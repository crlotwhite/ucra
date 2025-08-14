cmake_minimum_required(VERSION 3.15)

# This CMakeLists.txt is included from the main UCRA project
# so we don't need to find UCRA package - it's already available

# C++17 requirement for optional, unordered_map, etc.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only library target
add_library(ucra_cpp INTERFACE)

target_include_directories(ucra_cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Link to the main UCRA library
target_link_libraries(ucra_cpp INTERFACE ucra_impl)

# Set C++17 requirement for consumers
target_compile_features(ucra_cpp INTERFACE cxx_std_17)

# Enable testing if requested
option(UCRA_CPP_BUILD_TESTS "Build C++ binding tests" ON)

if(UCRA_CPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Install the header file
install(FILES ucra_cpp.hpp
        DESTINATION include/ucra
        COMPONENT Development)

# Install the target
install(TARGETS ucra_cpp
        EXPORT UCRATargets
        COMPONENT Development)
