# Tests for UCRA header/API

add_executable(test_headers test_headers.c)

target_link_libraries(test_headers PRIVATE ucra)

add_test(NAME headers COMMAND test_headers)
