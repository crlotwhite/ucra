---
description: Guidelines for ensuring MSVC compatibility across the vv-dsp codebase
applyTo: "**/*.{c,cpp,h,hpp}"
---

# MSVC Compatibility Guidelines

This document outlines the coding standards and practices necessary to ensure compatibility with Microsoft Visual C++ (MSVC) compiler while maintaining C99/C++11 compliance.

## C99 Compatibility Issues

### Variable Length Arrays (VLA)
**Problem**: MSVC does not fully support C99 Variable Length Arrays.

**❌ Avoid:**
```c
const int N = 10;
float array[N];  // VLA - will fail on MSVC
```

**✅ Use instead:**
```c
#define N 10
float array[N];  // Fixed-size array

// Or for dynamic sizes:
float* array = malloc(N * sizeof(float));
// ... use array ...
free(array);
```

### Const Variables as Array Sizes
**Problem**: MSVC requires compile-time constants for array declarations.

**❌ Avoid:**
```c
const int size = calculate_size();
float buffer[size];  // Will fail on MSVC
```

**✅ Use instead:**
```c
#define MAX_SIZE 1024
float buffer[MAX_SIZE];

// Or dynamic allocation:
int size = calculate_size();
float* buffer = malloc(size * sizeof(float));
```

## Warning Management

### Unused Variables
**Problem**: MSVC generates C4101, C4189 warnings for unused variables.

**✅ Solutions:**
```c
// Option 1: Remove unused variables
// int unused_var;  // Just delete it

// Option 2: Explicit void casting for intentionally unused
int result = some_function();
(void)result;  // Suppress warning

// Option 3: Use the variable meaningfully
int result = some_function();
assert(result == EXPECTED_VALUE);
```

### Sign Conversion Warnings
**Problem**: MSVC warns about implicit int ↔ size_t conversions.

**✅ Solutions:**
```c
// Cast explicitly when needed
int i;
size_t array_size = 100;
for (i = 0; i < (int)array_size; ++i) {
    array[(size_t)i] = value;
}

// Or use consistent types
size_t i;
size_t array_size = 100;
for (i = 0; i < array_size; ++i) {
    array[i] = value;
}
```

## Header Inclusion

### Standard Library Headers
**Problem**: MSVC may require explicit inclusion of standard headers.

**✅ Always include:**
```c
#include <stdlib.h>  // For malloc, free
#include <string.h>  // For memset, memcpy
#include <math.h>    // For math functions
#include <stdio.h>   // For printf, fprintf
```

## Test Code Patterns

### Array Declarations in Tests
**✅ Use these patterns:**
```c
// Fixed-size arrays
#define TEST_SIZE 10
float test_array[TEST_SIZE];

// Or dynamic allocation
int size = get_test_size();
float* test_array = malloc(size * sizeof(float));
// ... test code ...
free(test_array);
```

### Loop Variables
**✅ Declare loop variables consistently:**
```c
// C99 style (if C99 mode is enabled)
for (int i = 0; i < N; ++i) {
    // loop body
}

// C89 compatible (more portable)
int i;
for (i = 0; i < N; ++i) {
    // loop body
}
```

## CMake Configuration

### Compiler-Specific Settings
```cmake
# In CMakeLists.txt, add MSVC-specific flags if needed
if(MSVC)
    # Disable specific warnings that are too strict
    add_compile_options(/wd4101)  # Unreferenced local variable
    add_compile_options(/wd4189)  # Local variable initialized but not referenced

    # Enable C99 mode
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED ON)
endif()
```

## Code Review Checklist

When reviewing code for MSVC compatibility:

- [ ] No VLA usage (Variable Length Arrays)
- [ ] All array sizes are compile-time constants or use dynamic allocation
- [ ] All declared variables are actually used
- [ ] Standard library headers are properly included
- [ ] No implicit sign conversions without explicit casts
- [ ] Test arrays use `#define` for sizes or dynamic allocation

## Common Patterns

### Safe Array Pattern
```c
#define BUFFER_SIZE 256
vv_dsp_real buffer[BUFFER_SIZE];
```

### Safe Dynamic Array Pattern
```c
size_t buffer_size = calculate_needed_size();
vv_dsp_real* buffer = malloc(buffer_size * sizeof(vv_dsp_real));
if (!buffer) return VV_DSP_ERROR_INTERNAL;
// ... use buffer ...
free(buffer);
```

### Safe Loop Pattern
```c
size_t i;
for (i = 0; i < array_length; ++i) {
    process_element(array[i]);
}
```

## Migration Strategy

When updating existing code:

1. **Identify VLA usage**: Search for patterns like `type array[variable]`
2. **Replace with fixed arrays**: Use `#define` for compile-time constants
3. **Use dynamic allocation**: For runtime-determined sizes
4. **Clean up warnings**: Remove unused variables or add `(void)` casts
5. **Test on multiple compilers**: Verify both GCC/Clang and MSVC compatibility

## References

- [MSVC C99 Support](https://docs.microsoft.com/en-us/cpp/c-runtime-library/c99-library-support)
- [MSVC Compiler Warnings](https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/)
