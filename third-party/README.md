# Third-Party Dependencies

This directory contains external libraries used by the UCRA SDK.

## cJSON

**Version**: v1.7.16
**License**: MIT
**Repository**: <https://github.com/DaveGamble/cJSON>
**Usage**: JSON manifest parsing for UCRA engine discovery

### Files

- `cJSON.c` - cJSON library source code
- `cJSON.h` - cJSON library header file
- `cJSON/` - Full cJSON repository (for reference, excluded from builds)

### Why this approach?

We use source-level integration of cJSON rather than CMake FetchContent or submodules because:

1. **Simplicity**: Single source file integration without complex build dependencies
2. **Version Control**: Explicit control over the exact version used
3. **Build Reliability**: No network dependencies during build
4. **CMake Compatibility**: Avoids CMake version conflicts in cJSON's build system

### Updating cJSON

To update to a newer version:

1. Download the latest release from <https://github.com/DaveGamble/cJSON/releases>
2. Replace `cJSON.c` and `cJSON.h` with the new versions
3. Test the build and functionality
4. Update this README with the new version information

### License

cJSON is licensed under the MIT License. See the original repository for full license terms.
