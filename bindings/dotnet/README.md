# UCRA .NET Adapter

This folder contains a C# wrapper for the UCRA C API and a lightweight streaming adapter suitable for OpenUtau integration.

## Contents

- UCRA.NET.csproj: Class library exposing friendly wrappers.
- Interop/NativeMethods.cs: P/Invoke signatures for the native C API.
- Engine.cs: High-level render API.
- Streaming.cs: High-level streaming session wrapper built on UCRA streaming API.
- tests/: NUnit test project with unit and smoke tests (including streaming).

## Quick start

1) Build native UCRA first to produce libucra_impl.so:

   - cmake -S . -B build -DUCRA_BUILD_TOOLS=ON && cmake --build build -j
   - The shared library will be at build/libucra_impl.so

2) Install .NET SDK (Ubuntu example):

   - See <https://learn.microsoft.com/dotnet/core/install/linux-ubuntu> for the official steps
   - For Ubuntu 22.04 (Jammy), typically:
     - sudo apt-get update
     - sudo apt-get install -y dotnet-sdk-8.0

3) Build and run tests:

   - cd bindings/dotnet
   - dotnet build -c Release
   - export LD_LIBRARY_PATH="$(pwd)/../../build:${LD_LIBRARY_PATH}"
   - dotnet test tests -c Release --no-build

Note: If the native engine features are not available in your environment, tests may expect a NotSupported error. The streaming tests will still exercise the managed adapter path and verify graceful handling.

## Using the streaming adapter

```csharp
using var session = new UCRA.StreamSession(
    new UCRA.RenderConfig { SampleRate = 44100, Channels = 1, BlockSize = 512 },
    provider: () => {
        var cfg = new UCRA.RenderConfig { SampleRate = 44100, Channels = 1, BlockSize = 512 };
        cfg.Notes.Add(new UCRA.NoteSegment(0.0, 0.1, 69, 100, "a"));
        return cfg;
    }
);

var buffer = new float[512];
session.Read(buffer, 512, out var frames);
```

Ensure libucra_impl.so is discoverable (LD_LIBRARY_PATH) when running .NET.
