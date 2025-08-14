# Tests for UCRA header/API

add_executable(test_headers test_headers.c)

target_link_libraries(test_headers PRIVATE ucra)

add_test(NAME headers COMMAND test_headers)

# Flag mapper test
add_executable(test_flag_mapper test_flag_mapper.c)
target_link_libraries(test_flag_mapper ucra_impl)
add_test(NAME flag_mapper_test COMMAND test_flag_mapper)
