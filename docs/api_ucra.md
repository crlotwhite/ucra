# UCRA C API Reference

This document describes the public C99-compatible UCRA API as defined in `include/ucra/ucra.h`.

- Header: `#include "ucra/ucra.h"`
- ABI: C99, optional C++ compatibility via `extern "C"`
- Visibility macros:
  - `UCRA_API`: export/import symbol macro
  - `UCRA_CALL`: calling convention macro (e.g., `__cdecl` on Windows)

## Error Codes (`UCRA_Result`)

```c
typedef enum UCRA_Result {
    UCRA_SUCCESS = 0,
    UCRA_ERR_INVALID_ARGUMENT = 1,
    UCRA_ERR_OUT_OF_MEMORY = 2,
    UCRA_ERR_NOT_SUPPORTED = 3,
    UCRA_ERR_INTERNAL = 4,
    UCRA_ERR_FILE_NOT_FOUND = 5,
    UCRA_ERR_INVALID_JSON = 6,
    UCRA_ERR_INVALID_MANIFEST = 7
} UCRA_Result;
```

## Handles

```c
/* Engine and streaming opaque handles */
typedef struct UCRA_Engine_* UCRA_Handle;
typedef struct UCRA_StreamState_* UCRA_StreamHandle;
```

## Utility Types

```c
typedef struct UCRA_KeyValue {
    const char* key;   /* UTF-8 key */
    const char* value; /* UTF-8 value */
} UCRA_KeyValue;
```

## Manifest Types

```c
typedef struct UCRA_ManifestFlag {
    const char* key;         // name
    const char* type;        // "float"|"int"|"bool"|"string"|"enum"
    const char* desc;        // description
    const char* default_val; // default value as string
    const float* range;      // [min, max] for numeric types
    const char** values;     // enum values
    uint32_t values_count;   // enum values count
} UCRA_ManifestFlag;

typedef struct UCRA_ManifestEntry {
    const char* type;   // "dll"|"cli"|"ipc"
    const char* path;   // engine path
    const char* symbol; // entry symbol for dll
} UCRA_ManifestEntry;

typedef struct UCRA_ManifestAudio {
    const uint32_t* rates;    // supported sample rates
    uint32_t rates_count;
    const uint32_t* channels; // supported channel counts
    uint32_t channels_count;
    int streaming;            // streaming supported flag
} UCRA_ManifestAudio;

typedef struct UCRA_Manifest {
    const char* name;
    const char* version;
    const char* vendor;
    const char* license;
    UCRA_ManifestEntry entry;
    UCRA_ManifestAudio audio;
    const UCRA_ManifestFlag* flags;
    uint32_t flags_count;
} UCRA_Manifest;
```

## Curves and Note Input

```c
typedef struct UCRA_F0Curve {
    const float* time_sec;
    const float* f0_hz;
    uint32_t length;
} UCRA_F0Curve;

typedef struct UCRA_EnvCurve {
    const float* time_sec;
    const float* value; // normalized [0..1] or dB
    uint32_t length;
} UCRA_EnvCurve;

typedef struct UCRA_NoteSegment {
    double   start_sec;
    double   duration_sec;
    int16_t  midi_note;   // 0..127, -1 if N/A
    uint8_t  velocity;    // 0..127
    const char* lyric;    // UTF-8, optional
    const UCRA_F0Curve*  f0_override;  // optional
    const UCRA_EnvCurve* env_override; // optional
} UCRA_NoteSegment;
```

## Render Configuration and Result

```c
typedef struct UCRA_RenderConfig {
    uint32_t sample_rate;
    uint32_t channels;   // 1=mono, 2=stereo
    uint32_t block_size; // frames per streaming block
    uint32_t flags;      // reserved
    const UCRA_NoteSegment* notes;
    uint32_t note_count;
    const UCRA_KeyValue* options;
    uint32_t option_count;
} UCRA_RenderConfig;

typedef struct UCRA_RenderResult {
    const float* pcm;     // interleaved float32, frames*channels
    uint64_t frames;
    uint32_t channels;
    uint32_t sample_rate;
    const UCRA_KeyValue* metadata; // optional
    uint32_t metadata_count;
    UCRA_Result status;
} UCRA_RenderResult;
```

## Core API

```c
UCRA_API UCRA_Result UCRA_CALL
ucra_engine_create(UCRA_Handle* outEngine,
                   const UCRA_KeyValue* options,
                   uint32_t option_count);

UCRA_API void UCRA_CALL
ucra_engine_destroy(UCRA_Handle engine);

UCRA_API UCRA_Result UCRA_CALL
ucra_engine_getinfo(UCRA_Handle engine,
                    char* outBuffer,
                    size_t buffer_size);

UCRA_API UCRA_Result UCRA_CALL
ucra_render(UCRA_Handle engine,
            const UCRA_RenderConfig* config,
            UCRA_RenderResult* outResult);
```

### Manifest API

```c
UCRA_API UCRA_Result UCRA_CALL
ucra_manifest_load(const char* manifest_path,
                   UCRA_Manifest** outManifest);

UCRA_API void UCRA_CALL
ucra_manifest_free(UCRA_Manifest* manifest);
```

## Streaming API

```c
typedef UCRA_Result (UCRA_CALL *UCRA_PullPCM)(void* user_data,
                                              UCRA_RenderConfig* out_config);

UCRA_API UCRA_Result UCRA_CALL
ucra_stream_open(UCRA_StreamHandle* out_stream,
                 const UCRA_RenderConfig* config,
                 UCRA_PullPCM callback,
                 void* user_data);

UCRA_API UCRA_Result UCRA_CALL
ucra_stream_read(UCRA_StreamHandle stream,
                 float* out_buffer,
                 uint32_t frame_count,
                 uint32_t* out_frames_read);

UCRA_API void UCRA_CALL
ucra_stream_close(UCRA_StreamHandle stream);
```

## Notes on Ownership and Threading

- Memory returned via `UCRA_RenderResult` is owned by the engine.
- Validity: until the next `ucra_render()` on the same engine or `ucra_engine_destroy()`.
- Thread safety: engine handles are not guaranteed to be thread-safe unless stated by the implementation.

## Minimal Example

See `README.md` quick-start examples and `examples/` sources.
