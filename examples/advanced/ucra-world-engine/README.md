# UCRA with WORLD Vocoder Engine Example

This example demonstrates the integration of the WORLD vocoder engine with the UCRA (Universal Choir Rendering API).

## Overview

This example project shows how to integrate a high-quality vocoder engine (WORLD) into the UCRA framework. WORLD is a high-quality speech analysis, manipulation and synthesis system developed by Masanori Morise.

## Features

- **WORLD Vocoder Integration**: Complete C++ wrapper for the WORLD vocoder library
- **Engine Lifecycle Management**: Create, configure, and destroy WORLD engine instances
- **Offline Rendering**: Render notes to audio using WORLD's advanced synthesis
- **Streaming API Support**: Real-time audio streaming with WORLD engine
- **Comprehensive Testing**: Full test suite covering all aspects of WORLD integration

## WORLD Vocoder

WORLD is a vocoder for high-quality speech analysis, manipulation and synthesis. It can estimate F0, aperiodicity and spectral envelope and also generate the speech that has the same quality as the original speech.

- **Repository**: [mmorise/World](https://github.com/mmorise/World)
- **License**: Modified-BSD License
- **Language**: C++
- **Key Features**:
  - F0 estimation (fundamental frequency)
  - Aperiodicity analysis
  - Spectral envelope analysis
  - High-quality speech synthesis

## Build Requirements

- CMake 3.10+
- C++11 compatible compiler
- Git (for downloading WORLD library)

## Building

```bash
mkdir build
cd build
cmake ..
make
```

The build system will automatically download and compile the WORLD library.

## Running Tests

```bash
# Run all tests
ctest --verbose

# Run specific WORLD engine tests
./test_world_engine
./test_world_render
./test_world_comprehensive
```

## Test Results

All tests should pass with output similar to:

```text
100% tests passed, 0 tests failed out of 10
Total Test time (real) = 8.61 sec
```

## Implementation Details

### Engine Wrapper (`src/ucra_world_engine.cpp`)

The WORLD engine is wrapped to provide a C-compatible interface that matches the UCRA engine API:

- `ucra_engine_create()` - Initialize WORLD engine
- `ucra_engine_destroy()` - Clean up resources
- `ucra_engine_getinfo()` - Get engine information
- `ucra_render()` - Render notes to audio using WORLD

### Streaming Integration (`src/ucra_streaming.c`)

The streaming API has been enhanced to support WORLD engine rendering:

- Conditional compilation with `#ifdef UCRA_ENABLE_WORLD`
- Forward declarations for C++ functions
- Fallback to sine wave generation when WORLD is not available

### Test Coverage

- **Lifecycle Tests**: Engine creation/destruction
- **Rendering Tests**: Single and multiple note rendering
- **Comprehensive Tests**: Frequency accuracy, duration accuracy, multichannel support
- **Integration Tests**: Streaming API with WORLD engine

## API Usage Example

```c
#include "ucra/ucra.h"

// Create WORLD engine
UCRA_Handle engine = ucra_engine_create(NULL);

// Configure rendering
UCRA_RenderConfig config = {
    .sample_rate = 44100,
    .channels = 1,
    .note_count = 1
};

UCRA_NoteSegment note = {
    .frequency = 440.0f,    // A4
    .duration = 1.0f,       // 1 second
    .amplitude = 0.8f
};

config.notes = &note;

// Render audio
UCRA_RenderResult result;
int ret = ucra_render(engine, &config, &result);

if (ret == 0) {
    printf("Rendered %zu frames\n", result.frame_count);
    // result.audio_data contains the synthesized audio
}

// Cleanup
ucra_engine_destroy(engine);
```

## Performance Notes

WORLD vocoder provides high-quality synthesis but requires more computational resources than simple synthesis methods. For real-time applications, consider:

- Appropriate buffer sizes for streaming
- CPU capabilities for the target platform
- Memory usage for longer audio segments

## Integration with Main UCRA Project

This example is separated from the main UCRA project to keep the core library lightweight. To integrate WORLD support into your own project:

1. Copy the relevant source files (`ucra_world_engine.cpp`)
2. Add WORLD library dependency to your CMakeLists.txt
3. Enable C++ compilation in your build system
4. Include the WORLD engine in your streaming configuration

## License

This example follows the same license as the main UCRA project. Note that WORLD vocoder itself uses the Modified-BSD License.
