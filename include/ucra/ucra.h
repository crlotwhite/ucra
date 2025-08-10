/*
 * UCRA Public C API (C99 compatible)
 * This header defines the core types, error codes, and function signatures
 * for the Universal Choir Rendering API (UCRA).
 */
#ifndef UCRA_UCRA_H
#define UCRA_UCRA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*
 * Visibility & Calling Convention
 * - Define UCRA_BUILD when building the library (shared/dll) to export symbols
 * - Define UCRA_STATIC for static library consumers to omit visibility attrs
 */
#if defined(_WIN32) || defined(_WIN64)
  #if defined(UCRA_BUILD)
    #define UCRA_API __declspec(dllexport)
  #elif defined(UCRA_STATIC)
    #define UCRA_API
  #else
    #define UCRA_API __declspec(dllimport)
  #endif
  #ifndef UCRA_CALL
    #define UCRA_CALL __cdecl
  #endif
#else
  #if defined(UCRA_STATIC)
    #define UCRA_API
  #else
    #if defined(__GNUC__) && (__GNUC__ >= 4)
      #define UCRA_API __attribute__((visibility("default")))
    #else
      #define UCRA_API
    #endif
  #endif
  #ifndef UCRA_CALL
    #define UCRA_CALL
  #endif
#endif

/* Opaque engine handle */
typedef struct UCRA_Engine_* UCRA_Handle;

/* Result / Error codes (0 == success) */
typedef enum UCRA_Result {
    UCRA_SUCCESS = 0,
    UCRA_ERR_INVALID_ARGUMENT = 1,
    UCRA_ERR_OUT_OF_MEMORY = 2,
    UCRA_ERR_NOT_SUPPORTED = 3,
    UCRA_ERR_INTERNAL = 4
} UCRA_Result;

/* Generic key/value pair for options and metadata */
typedef struct UCRA_KeyValue {
    const char* key;   /* UTF-8 key (null-terminated) */
    const char* value; /* UTF-8 value (null-terminated) */
} UCRA_KeyValue;

/* Fundamental curves */
typedef struct UCRA_F0Curve {
    const float* time_sec; /* length-sized, seconds */
    const float* f0_hz;    /* length-sized, Hz */
    uint32_t length;       /* number of points */
} UCRA_F0Curve;

typedef struct UCRA_EnvCurve {
    const float* time_sec; /* length-sized, seconds */
    const float* value;    /* length-sized, normalized [0..1] or dB depending on engine */
    uint32_t length;       /* number of points */
} UCRA_EnvCurve;

/* Note segment input */
typedef struct UCRA_NoteSegment {
    double start_sec;          /* note start time in seconds */
    double duration_sec;       /* note duration in seconds */
    int16_t midi_note;         /* 0..127, -1 if N/A */
    uint8_t velocity;          /* 0..127 */
    const char* lyric;         /* optional UTF-8 lyric, may be NULL */
    const UCRA_F0Curve* f0_override;  /* optional */
    const UCRA_EnvCurve* env_override;/* optional */
} UCRA_NoteSegment;

/* Render configuration */
typedef struct UCRA_RenderConfig {
    uint32_t sample_rate;      /* e.g., 44100/48000 */
    uint32_t channels;         /* 1=mono, 2=stereo, ... */
    uint32_t flags;            /* reserved for future use */

    const UCRA_NoteSegment* notes; /* pointer to array of notes */
    uint32_t note_count;           /* number of notes */

    const UCRA_KeyValue* options;  /* optional engine options */
    uint32_t option_count;         /* number of options */
} UCRA_RenderConfig;

/* Render output */
typedef struct UCRA_RenderResult {
    const float* pcm;          /* interleaved PCM32F, frames*channels length; valid until next render or engine destroy */
    uint64_t frames;           /* number of frames in pcm */
    uint32_t channels;         /* channel count for pcm */
    uint32_t sample_rate;      /* sample rate for pcm */

    const UCRA_KeyValue* metadata; /* optional metadata array (engine-owned) */
    uint32_t metadata_count;       /* number of metadata entries */

    UCRA_Result status;        /* status of the render call */
} UCRA_RenderResult;

/*
 * Core API
 * Notes:
 * - Memory Ownership: any pointers returned via UCRA_RenderResult are owned by
 *   the engine and remain valid until the next ucra_render() call on the same
 *   engine or until ucra_engine_destroy(engine) is called.
 * - Threading: unless stated otherwise by a specific engine, handles are not
 *   guaranteed to be thread-safe across concurrent calls.
 */
UCRA_API UCRA_Result UCRA_CALL
ucra_engine_create(UCRA_Handle* outEngine,
                   const UCRA_KeyValue* options,
                   uint32_t option_count);

UCRA_API void UCRA_CALL
ucra_engine_destroy(UCRA_Handle engine);

/* Obtain implementation info string (e.g., name/version). Returns UCRA_SUCCESS
 * and writes a null-terminated string into outBuffer if size permits. */
UCRA_API UCRA_Result UCRA_CALL
ucra_engine_getinfo(UCRA_Handle engine,
                    char* outBuffer,
                    size_t buffer_size);

/* Render audio for the provided configuration. On success, outResult is filled. */
UCRA_API UCRA_Result UCRA_CALL
ucra_render(UCRA_Handle engine,
            const UCRA_RenderConfig* config,
            UCRA_RenderResult* outResult);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UCRA_UCRA_H */
