# Rust bindings build integration
# Requires Cargo to be installed

find_program(CARGO_EXECUTABLE cargo)

if(NOT CARGO_EXECUTABLE)
    message(WARNING "Cargo not found - Rust bindings will not be built")
    return()
endif()

# Build ucra-sys (generates raw bindings)
add_custom_target(ucra_rust_sys ALL
    COMMAND ${CMAKE_COMMAND} -E env UCRA_LIB_DIR=${CMAKE_BINARY_DIR} ${CARGO_EXECUTABLE} build --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/ucra-sys/Cargo.toml --release
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building ucra-sys (raw FFI bindings)"
    VERBATIM
)

# Ensure C library is built before Rust bindings
add_dependencies(ucra_rust_sys ucra_impl ucra_impl_shared)

# Build high-level ucra crate
add_custom_target(ucra_rust ALL
    COMMAND ${CMAKE_COMMAND} -E env UCRA_LIB_DIR=${CMAKE_BINARY_DIR} ${CARGO_EXECUTABLE} build --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/ucra/Cargo.toml --release
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building ucra (safe Rust wrapper)"
    DEPENDS ucra_rust_sys
    VERBATIM
)

# Add tests via CTest (runs cargo tests)
if(BUILD_TESTING)
    add_test(NAME rust_sys_tests
             COMMAND ${CMAKE_COMMAND} -E env UCRA_LIB_DIR=${CMAKE_BINARY_DIR} LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH} DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}:$ENV{DYLD_LIBRARY_PATH} ${CARGO_EXECUTABLE} test --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/ucra-sys/Cargo.toml --quiet)
    add_test(NAME rust_wrapper_tests
             COMMAND ${CMAKE_COMMAND} -E env UCRA_LIB_DIR=${CMAKE_BINARY_DIR} LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}:$ENV{LD_LIBRARY_PATH} DYLD_LIBRARY_PATH=${CMAKE_BINARY_DIR}:$ENV{DYLD_LIBRARY_PATH} ${CARGO_EXECUTABLE} test --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/ucra/Cargo.toml --quiet)
endif()
