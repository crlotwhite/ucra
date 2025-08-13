# UCRA High-level CMake API (Phase 1)
# Inspired by JUCE's CMake helper approach.
# Provides functions to configure and consume the UCRA SDK with minimal boilerplate.

include(CMakePackageConfigHelpers)
include(FetchContent OPTIONAL)
include(CMakeDependentOption)

# Guard to avoid multiple inclusions
if(DEFINED _UCRA_CMAKE_INCLUDED)
  return()
endif()
set(_UCRA_CMAKE_INCLUDED TRUE)

# =============================
# User-facing Options (Cache)
# =============================
option(UCRA_BUILD_STREAMING "Build Streaming API support" ON)
option(UCRA_WITH_RESAMPLER "Enable built-in resampler CLI build" ON)
option(UCRA_BUILD_EXAMPLES "Build UCRA examples" ON)
option(UCRA_BUILD_TESTS "Build UCRA tests" ON)
option(UCRA_INSTALL "Enable install/export logic" ON)
option(UCRA_VERBOSE_CONFIG "Print UCRA configuration summary" ON)

# Allow user override of C/C++ standard
set(UCRA_C_STANDARD 99 CACHE STRING "C standard for UCRA core")
set(UCRA_CXX_STANDARD 11 CACHE STRING "C++ standard for UCRA world engine")

# =============================
# Internal State Variables
# =============================
set(_UCRA_CORE_CREATED FALSE)
set(_UCRA_REGISTERED_ENGINES "")

# =============================
# Utility
# =============================
function(_ucra_log msg)
  if(UCRA_VERBOSE_CONFIG)
    message(STATUS "[UCRA] ${msg}")
  endif()
endfunction()

# =============================
# Setup
# =============================
function(ucra_setup)
  # Basic project-wide policies for consumers who just fetched UCRA
  set(CMAKE_C_STANDARD ${UCRA_C_STANDARD} PARENT_SCOPE)
  set(CMAKE_C_STANDARD_REQUIRED ON PARENT_SCOPE)
  set(CMAKE_CXX_STANDARD ${UCRA_CXX_STANDARD} PARENT_SCOPE)
  set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  add_compile_definitions(UCRA_STATIC)
  if(UCRA_BUILD_STREAMING)
    add_compile_definitions(UCRA_HAS_STREAMING)
  else()
    add_compile_definitions(UCRA_NO_STREAMING)
  endif()
  if(UCRA_WITH_RESAMPLER)
    add_compile_definitions(UCRA_ENABLE_RESAMPLER)
  endif()
  _ucra_log("Setup complete (C=${UCRA_C_STANDARD} CXX=${UCRA_CXX_STANDARD})")
endfunction()

# =============================
# Core Library Creation
# =============================
function(ucra_add_core)
  if(_UCRA_CORE_CREATED)
    _ucra_log("Core already created")
    return()
  endif()
  set(options)
  set(oneValueArgs TARGET)
  set(multiValueArgs)
  cmake_parse_arguments(C "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT C_TARGET)
    set(C_TARGET ucra)
  endif()

  # Third-party cJSON (kept simple; could be object library later)
  if(NOT TARGET ucra_cjson)
    add_library(ucra_cjson STATIC ${CMAKE_CURRENT_LIST_DIR}/../third-party/cJSON.c)
    target_include_directories(ucra_cjson PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../third-party>
      $<INSTALL_INTERFACE:include/third-party>)
  endif()

  set(_UCRA_CORE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../src/ucra_manifest.c
    ${CMAKE_CURRENT_LIST_DIR}/../src/ucra_streaming.c
  )

  add_library(${C_TARGET}_impl STATIC ${_UCRA_CORE_SOURCES})
  target_include_directories(${C_TARGET}_impl PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
      $<INSTALL_INTERFACE:include>)
  target_link_libraries(${C_TARGET}_impl PUBLIC ucra_cjson)

  if(UCRA_BUILD_STREAMING AND NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(${C_TARGET}_impl PUBLIC Threads::Threads)
  endif()

  if(UNIX)
    find_library(M_LIB m)
    if(M_LIB)
      target_link_libraries(${C_TARGET}_impl PUBLIC ${M_LIB})
    endif()
  endif()

  # WORLD integration removed: world engine kept separate for examples only if needed.

  # Public interface target (header-only façade + impl linkage)
  add_library(${C_TARGET} INTERFACE)
  target_include_directories(${C_TARGET} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
    $<INSTALL_INTERFACE:include>)
  target_link_libraries(${C_TARGET} INTERFACE ${C_TARGET}_impl)
  add_library(UCRA::${C_TARGET} ALIAS ${C_TARGET})

  set(_UCRA_CORE_CREATED TRUE PARENT_SCOPE)
  set(UCRA_MAIN_TARGET ${C_TARGET} PARENT_SCOPE)
  _ucra_log("Core target '${C_TARGET}' created")
endfunction()

# =============================
# Engine Registration (extensible modules)
# =============================
function(ucra_register_engine)
  set(options)
  set(oneValueArgs NAME TYPE)
  set(multiValueArgs SOURCES DEFINES)
  cmake_parse_arguments(E "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT E_NAME)
    message(FATAL_ERROR "ucra_register_engine(NAME <id> SOURCES <files> [DEFINES ...])")
  endif()
  if(NOT E_SOURCES)
    message(FATAL_ERROR "ucra_register_engine requires SOURCES")
  endif()
  if(NOT TARGET ucra)
    message(FATAL_ERROR "Call ucra_add_core() before registering engines")
  endif()
  if(NOT E_TYPE)
    set(E_TYPE STATIC)
  endif()
  string(TOLOWER "${E_TYPE}" _etype)
  if(_etype STREQUAL "static")
    add_library(${E_NAME} STATIC ${E_SOURCES})
  elseif(_etype STREQUAL "object")
    add_library(${E_NAME} OBJECT ${E_SOURCES})
  else()
    message(FATAL_ERROR "Unsupported engine TYPE='${E_TYPE}'")
  endif()
  target_include_directories(${E_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../include)
  if(E_DEFINES)
    target_compile_definitions(${E_NAME} PUBLIC ${E_DEFINES})
  endif()
  target_compile_definitions(${E_NAME} PUBLIC UCRA_ENGINE_ID=\"${E_NAME}\")
  list(APPEND _UCRA_REGISTERED_ENGINES ${E_NAME})
  set(_UCRA_REGISTERED_ENGINES "${_UCRA_REGISTERED_ENGINES}" PARENT_SCOPE)
  _ucra_log("Registered engine '${E_NAME}' (${E_TYPE})")
endfunction()

# =============================
# Export / Install
# =============================
function(ucra_export_package)
  if(NOT UCRA_INSTALL)
    _ucra_log("Install/export disabled (UCRA_INSTALL=OFF)")
    return()
  endif()
  if(NOT TARGET ucra)
    message(FATAL_ERROR "ucra_export_package requires core target (call ucra_add_core)")
  endif()
  set(options)
  set(oneValueArgs VERSION)
  set(multiValueArgs)
  cmake_parse_arguments(P "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT P_VERSION)
    set(P_VERSION 1.0.0)
  endif()
  install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../include/ucra DESTINATION include)
  install(FILES ${CMAKE_CURRENT_LIST_DIR}/../third-party/cJSON.h DESTINATION include/third-party)
  # Implementation + deps
  install(TARGETS ucra_cjson ucra_impl ucra
          EXPORT UCRATargets
          ARCHIVE DESTINATION lib
          LIBRARY DESTINATION lib
          RUNTIME DESTINATION bin
          INCLUDES DESTINATION include)
  # Resampler (optional)
  if(UCRA_WITH_RESAMPLER AND TARGET resampler)
    install(TARGETS resampler DESTINATION bin)
  endif()
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/UCRAConfigVersion.cmake
    VERSION ${P_VERSION}
    COMPATIBILITY SameMajorVersion)
  # Reuse existing template if present
  if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/UCRAConfig.cmake.in)
    configure_package_config_file(
      ${CMAKE_CURRENT_LIST_DIR}/UCRAConfig.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/UCRAConfig.cmake
      INSTALL_DESTINATION lib/cmake/UCRA)
    install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/UCRAConfig.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/UCRAConfigVersion.cmake
      DESTINATION lib/cmake/UCRA)
  endif()
  install(EXPORT UCRATargets
          FILE UCRATargets.cmake
          NAMESPACE UCRA::
          DESTINATION lib/cmake/UCRA)
  _ucra_log("Export package configured (version=${P_VERSION})")
endfunction()

# =============================
# Examples Wrapper (Phase 1 simple)
# =============================
function(ucra_add_examples)
  if(NOT UCRA_BUILD_EXAMPLES)
    _ucra_log("Examples disabled")
    return()
  endif()
  if(NOT TARGET ucra_impl)
    message(FATAL_ERROR "Examples require core (ucra_impl)")
  endif()
  function(_ucra_example target file dir)
    add_executable(${target} ${dir}/${file})
    target_link_libraries(${target} ucra_impl)
    add_test(NAME example_${target} COMMAND ${target})
    set_tests_properties(example_${target} PROPERTIES WORKING_DIRECTORY ${dir})
  endfunction()
  _ucra_example(basic_engine basic_engine.c ${CMAKE_CURRENT_LIST_DIR}/../examples/simple-usage)
  _ucra_example(manifest_usage manifest_usage.c ${CMAKE_CURRENT_LIST_DIR}/../examples/simple-usage)
  _ucra_example(simple_render simple_render.c ${CMAKE_CURRENT_LIST_DIR}/../examples/simple-usage)
  _ucra_example(simple_usage simple_usage.c ${CMAKE_CURRENT_LIST_DIR}/../examples/simple-usage)
  _ucra_example(basic_rendering basic_rendering.c ${CMAKE_CURRENT_LIST_DIR}/../examples/basic-rendering)
  _ucra_example(multi_note_render multi_note_render.c ${CMAKE_CURRENT_LIST_DIR}/../examples/basic-rendering)
  _ucra_example(streaming_example streaming_example.c ${CMAKE_CURRENT_LIST_DIR}/../examples/basic-rendering)
  _ucra_example(wav_output wav_output.c ${CMAKE_CURRENT_LIST_DIR}/../examples/basic-rendering)
  _ucra_log("Examples added")
endfunction()

# =============================
# Tests Wrapper
# =============================
function(ucra_add_tests)
  if(NOT UCRA_BUILD_TESTS)
    _ucra_log("Tests disabled")
    return()
  endif()
  if(NOT TARGET ucra_impl)
    message(FATAL_ERROR "Tests require core (ucra_impl)")
  endif()
  include(CTest)
  enable_testing()
  add_executable(test_manifest ${CMAKE_CURRENT_LIST_DIR}/../tests/test_manifest.c)
  target_link_libraries(test_manifest ucra_impl)
  add_test(NAME manifest_parsing_test COMMAND test_manifest)
  set_tests_properties(manifest_parsing_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../tests)
  add_executable(test_suite ${CMAKE_CURRENT_LIST_DIR}/../tests/test_suite.c)
  target_link_libraries(test_suite ucra_impl)
  add_test(NAME comprehensive_test_suite COMMAND test_suite)
  set_tests_properties(comprehensive_test_suite PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../tests)
  # Streaming related
  if(UCRA_BUILD_STREAMING)
    foreach(name lifecycle buffering read integration)
      set(src ${CMAKE_CURRENT_LIST_DIR}/../tests/test_streaming_${name}.c)
      if(EXISTS ${src})
        add_executable(test_streaming_${name} ${src})
        target_link_libraries(test_streaming_${name} ucra_impl)
        add_test(NAME streaming_${name}_test COMMAND test_streaming_${name})
      endif()
    endforeach()
  endif()
  # Resampler CLI
  if(UCRA_WITH_RESAMPLER AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/../src/resampler_cli.c)
    add_executable(resampler ${CMAKE_CURRENT_LIST_DIR}/../src/resampler_cli.c)
    target_link_libraries(resampler ucra_impl)
  endif()
  _ucra_log("Tests added")
endfunction()

# =============================
# Config Summary
# =============================
function(ucra_print_config)
  _ucra_log("Summary:")
  _ucra_log("  Streaming:      ${UCRA_BUILD_STREAMING}")
  _ucra_log("  Resampler CLI:  ${UCRA_WITH_RESAMPLER}")
  _ucra_log("  Examples:       ${UCRA_BUILD_EXAMPLES}")
  _ucra_log("  Tests:          ${UCRA_BUILD_TESTS}")
  _ucra_log("  Install:        ${UCRA_INSTALL}")
endfunction()
