# UCRA SDK Tests

This directory contains all test files and test data for the UCRA SDK.

## Structure

```text
tests/
├── CMakeLists.txt           # Test build configuration
├── README.md               # This file
├── data/                   # Test data files
│   ├── example_manifest.json      # Valid manifest for testing
│   ├── broken_manifest.json       # Invalid JSON syntax
│   ├── invalid_*.json             # Various invalid manifests
├── poc_parser.c            # Proof of concept JSON parser
├── test_headers.c          # Header structure size tests
├── test_manifest.c         # Basic manifest parsing test
└── test_suite.c           # Comprehensive test suite
```

## Test Data

The `data/` directory contains various manifest files for testing:

- **`example_manifest.json`** - A complete, valid UCRA manifest
- **`broken_manifest.json`** - Malformed JSON for syntax error testing
- **`invalid_missing_name.json`** - Missing required 'name' field
- **`invalid_entry_type.json`** - Invalid entry type value
- **`invalid_negative_rate.json`** - Negative sample rate (invalid)
- **`invalid_enum_no_values.json`** - Enum flag without values array

## Running Tests

From the build directory:

```bash
# Run all tests
ctest

# Run specific test
ctest -R manifest_parsing_test

# Run with verbose output
ctest -V
```

## Test Programs

### test_manifest

Basic manifest parser test that loads and displays a manifest file.

```bash
./test_manifest                    # Uses default example_manifest.json
./test_manifest data/broken_manifest.json  # Test specific file
```

### test_suite

Comprehensive test suite covering all error conditions and edge cases.

```bash
./test_suite  # Runs 8 different test scenarios
```

### poc_parser

Proof of concept parser for evaluating JSON libraries.

### test_headers

Validates C struct sizes and memory layout.
