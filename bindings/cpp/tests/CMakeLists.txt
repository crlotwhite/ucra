# Find a testing framework (using simple assert-based tests if GTest not available)
find_package(GTest QUIET)

if(GTest_FOUND)
    # Use Google Test if available
    add_executable(test_ucra_cpp test_ucra_cpp_gtest.cpp)
    target_link_libraries(test_ucra_cpp
        PRIVATE ucra_cpp GTest::gtest GTest::gtest_main)

    add_test(NAME cpp_wrapper_test COMMAND test_ucra_cpp)
else()
    # Use simple assert-based tests
    add_executable(test_ucra_cpp test_ucra_cpp_simple.cpp)
    target_link_libraries(test_ucra_cpp PRIVATE ucra_cpp)

    add_test(NAME cpp_wrapper_test COMMAND test_ucra_cpp)
endif()

# WORLD 엔진 사용 여부 (기본: OFF). WORLD를 통합하면 상위 CMake에서 ON으로 설정하세요.
option(UCRA_WITH_WORLD "Build/Run tests that require WORLD engine" OFF)

# Sample application
add_executable(cpp_sample_app sample_app.cpp)
target_link_libraries(cpp_sample_app PRIVATE ucra_cpp)

# Make sample app available for testing only when WORLD is enabled
if(UCRA_WITH_WORLD)
    add_test(NAME cpp_sample_app_test COMMAND cpp_sample_app)
    set_tests_properties(cpp_sample_app_test PROPERTIES
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
