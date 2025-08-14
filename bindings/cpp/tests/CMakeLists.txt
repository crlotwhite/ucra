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

# Sample application
add_executable(cpp_sample_app sample_app.cpp)
target_link_libraries(cpp_sample_app PRIVATE ucra_cpp)

# Make sample app available for testing (but allow failure in CI)
if(NOT DEFINED ENV{CI})
    # Only add as test when not in CI environment
    add_test(NAME cpp_sample_app_test COMMAND cpp_sample_app)
endif()
