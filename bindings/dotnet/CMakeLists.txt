# Check if .NET SDK is available
find_program(DOTNET_EXECUTABLE dotnet)

if(DOTNET_EXECUTABLE)
    message(STATUS ".NET SDK found: ${DOTNET_EXECUTABLE}")

    # Build the .NET library
    add_custom_target(ucra_dotnet_bindings ALL
        COMMAND ${DOTNET_EXECUTABLE} build UCRA.NET.csproj -c Release
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building UCRA .NET bindings"
        VERBATIM
    )

    # Build and run tests
    add_custom_target(ucra_dotnet_tests
        COMMAND ${DOTNET_EXECUTABLE} build tests/UCRA.NET.Tests.csproj -c Release
        COMMAND ${DOTNET_EXECUTABLE} test tests/UCRA.NET.Tests.csproj -c Release --no-build
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building and running UCRA .NET tests"
        DEPENDS ucra_dotnet_bindings
        VERBATIM
    )

    # Build sample application
    add_custom_target(ucra_dotnet_sample
        COMMAND ${DOTNET_EXECUTABLE} run --project tests/UCRA.NET.Tests.csproj -c Release --no-build
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running UCRA .NET sample application"
        DEPENDS ucra_dotnet_bindings
        VERBATIM
    )

    # Add to CTest if enabled
    if(BUILD_TESTING)
        add_test(NAME dotnet_wrapper_test
                 COMMAND ${DOTNET_EXECUTABLE} test tests/UCRA.NET.Tests.csproj -c Release --logger "console;verbosity=normal"
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

        add_test(NAME dotnet_sample_app_test
                 COMMAND ${DOTNET_EXECUTABLE} run --project tests/UCRA.NET.Tests.csproj -c Release
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

else()
    message(WARNING ".NET SDK not found - .NET bindings will not be built")
    message(STATUS "To install .NET SDK, visit: https://dotnet.microsoft.com/download")
endif()
