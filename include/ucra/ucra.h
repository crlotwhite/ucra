/**
 * @file ucra.h
 * @brief UCRA Public C API (C99 compatible)
 * @author UCRA Development Team
 * @date 2025
 *
 * This header defines the core types, error codes, and function signatures
 * for the Universal Choir Rendering API (UCRA).
 *
 * UCRA is a universal audio synthesis and voicebank rendering library designed
 * for applications such as singing voice synthesis, virtual instruments,
 * and voice conversion tools.
 */
#ifndef UCRA_UCRA_H
#define UCRA_UCRA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Visibility & Calling Convention
 *
 * Define UCRA_BUILD when building the library (shared/dll) to export symbols.
 * Define UCRA_STATIC for static library consumers to omit visibility attributes.
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

/** @brief Opaque engine handle */
typedef struct UCRA_Engine_* UCRA_Handle;

/** @brief Opaque stream handle for streaming API */
typedef struct UCRA_StreamState_* UCRA_StreamHandle;

/**
 * @brief Result / Error codes (0 == success)
 *
 * All UCRA functions return a result code indicating success or failure.
 * UCRA_SUCCESS (0) indicates successful operation.
 */
typedef enum UCRA_Result {
    UCRA_SUCCESS = 0,                /**< Operation completed successfully */
    UCRA_ERR_INVALID_ARGUMENT = 1,   /**< Invalid function argument provided */
    UCRA_ERR_OUT_OF_MEMORY = 2,      /**< Memory allocation failed */
    UCRA_ERR_NOT_SUPPORTED = 3,      /**< Operation not supported by engine */
    UCRA_ERR_INTERNAL = 4,           /**< Internal engine error */
    UCRA_ERR_FILE_NOT_FOUND = 5,     /**< Requested file not found */
    UCRA_ERR_INVALID_JSON = 6,       /**< JSON parsing error */
    UCRA_ERR_INVALID_MANIFEST = 7    /**< Manifest validation error */
} UCRA_Result;

/**
 * @brief Generic key/value pair for options and metadata
 *
 * Used throughout the API to pass configuration options, metadata,
 * and engine parameters in a flexible format.
 */
typedef struct UCRA_KeyValue {
    const char* key;   /**< UTF-8 key (null-terminated) */
    const char* value; /**< UTF-8 value (null-terminated) */
} UCRA_KeyValue;

/**
 * @brief Manifest flag definition
 *
 * Describes a configurable parameter supported by an engine,
 * including its type, default value, and constraints.
 */
typedef struct UCRA_ManifestFlag {
    const char* key;         /**< Flag key/name */
    const char* type;        /**< Flag type: "float", "int", "bool", "string", "enum" */
    const char* desc;        /**< Human-readable description */
    const char* default_val; /**< Default value as string */
    const float* range;      /**< Range for numeric types [min, max], NULL if not applicable */
    const char** values;     /**< Valid values for enum type, NULL if not applicable */
    uint32_t values_count;   /**< Number of values in the values array */
} UCRA_ManifestFlag;

/**
 * @brief Manifest entry point configuration
 *
 * Specifies how the engine should be loaded and invoked.
 */
typedef struct UCRA_ManifestEntry {
    const char* type;   /**< Entry type: "dll", "cli", "ipc" */
    const char* path;   /**< Path to engine binary/library */
    const char* symbol; /**< Entry symbol name (for dll type) */
} UCRA_ManifestEntry;

/**
 * @brief Manifest audio capabilities
 *
 * Describes the audio formats and features supported by an engine.
 */
typedef struct UCRA_ManifestAudio {
    const uint32_t* rates;     /**< Supported sample rates */
    uint32_t rates_count;      /**< Number of supported rates */
    const uint32_t* channels;  /**< Supported channel counts */
    uint32_t channels_count;   /**< Number of supported channel counts */
    int streaming;             /**< Whether streaming is supported */
} UCRA_ManifestAudio;

/**
 * @brief Engine manifest data
 *
 * Complete manifest information for a UCRA engine, including
 * metadata, capabilities, and configuration options.
 */
typedef struct UCRA_Manifest {
    const char* name;               /**< Engine name */
    const char* version;            /**< Engine version */
    const char* vendor;             /**< Engine vendor/author */
    const char* license;            /**< License identifier */

    UCRA_ManifestEntry entry;       /**< Entry point configuration */
    UCRA_ManifestAudio audio;       /**< Audio capabilities */

    const UCRA_ManifestFlag* flags; /**< Engine flags array */
    uint32_t flags_count;           /**< Number of flags */
} UCRA_Manifest;

/**
 * @brief Fundamental frequency curve
 *
 * Represents a time-varying F0 (fundamental frequency) curve
 * for pitch control in synthesis.
 */
typedef struct UCRA_F0Curve {
    const float* time_sec; /**< length-sized array, time points in seconds */
    const float* f0_hz;    /**< length-sized array, frequency values in Hz */
    uint32_t length;       /**< number of points in the curve */
} UCRA_F0Curve;

/**
 * @brief Envelope curve
 *
 * Represents a time-varying envelope for amplitude or other
 * parameter modulation.
 */
typedef struct UCRA_EnvCurve {
    const float* time_sec; /**< length-sized array, time points in seconds */
    const float* value;    /**< length-sized array, normalized [0..1] or dB depending on engine */
    uint32_t length;       /**< number of points in the curve */
} UCRA_EnvCurve;

/**
 * @brief Note segment input
 *
 * Defines a single note or phoneme to be synthesized,
 * including timing, pitch, and optional modulation curves.
 */
typedef struct UCRA_NoteSegment {
    double start_sec;          /**< note start time in seconds */
    double duration_sec;       /**< note duration in seconds */
    int16_t midi_note;         /**< MIDI note number 0..127, -1 if N/A */
    uint8_t velocity;          /**< MIDI velocity 0..127 */
    const char* lyric;         /**< optional UTF-8 lyric, may be NULL */
    const UCRA_F0Curve* f0_override;  /**< optional F0 curve override */
    const UCRA_EnvCurve* env_override;/**< optional envelope curve override */
} UCRA_NoteSegment;

/**
 * @brief Render configuration
 *
 * Configuration parameters for audio rendering, including sample rate,
 * notes to synthesize, and engine-specific options.
 */
typedef struct UCRA_RenderConfig {
    uint32_t sample_rate;      /**< Sample rate (e.g., 44100, 48000) */
    uint32_t channels;         /**< Channel count (1=mono, 2=stereo, ...) */
    uint32_t block_size;       /**< Frames per block for streaming (e.g., 256, 512) */
    uint32_t flags;            /**< Reserved for future use */

    const UCRA_NoteSegment* notes; /**< Pointer to array of notes */
    uint32_t note_count;           /**< Number of notes in the array */

    const UCRA_KeyValue* options;  /**< Optional engine options */
    uint32_t option_count;         /**< Number of options */
} UCRA_RenderConfig;

/**
 * @brief Render output
 *
 * Result of audio rendering, containing synthesized PCM data and metadata.
 * The PCM data remains valid until the next render call or engine destruction.
 */
typedef struct UCRA_RenderResult {
    const float* pcm;          /**< Interleaved PCM32F data (frames*channels length) */
    uint64_t frames;           /**< Number of frames in PCM data */
    uint32_t channels;         /**< Channel count for PCM data */
    uint32_t sample_rate;      /**< Sample rate for PCM data */

    const UCRA_KeyValue* metadata; /**< Optional metadata array (engine-owned) */
    uint32_t metadata_count;       /**< Number of metadata entries */

    UCRA_Result status;        /**< Status of the render call */
} UCRA_RenderResult;

/*
 * Streaming API Types
 */

/**
 * @brief Callback function for pulling PCM data during streaming
 *
 * The host application provides this callback to supply note segments
 * and render parameters for the next audio block.
 *
 * @param user_data User-provided context data
 * @param out_config Output render configuration for the next block
 * @return UCRA_SUCCESS if the configuration was provided successfully,
 *         or an error code if no more data is available or an error occurred.
 */
typedef UCRA_Result (UCRA_CALL *UCRA_PullPCM)(void* user_data,
                                               UCRA_RenderConfig* out_config);

/**
 * @brief Core API
 * @defgroup CoreAPI Core UCRA API Functions
 * @{
 *
 * @note Memory Ownership: any pointers returned via UCRA_RenderResult are owned by
 *   the engine and remain valid until the next ucra_render() call on the same
 *   engine or until ucra_engine_destroy(engine) is called.
 * @note Threading: unless stated otherwise by a specific engine, handles are not
 *   guaranteed to be thread-safe across concurrent calls.
 */

/**
 * @brief Create a new UCRA engine instance
 *
 * Initializes a new engine with the specified options. The engine
 * must be destroyed with ucra_engine_destroy() when no longer needed.
 *
 * @param outEngine Pointer to store the created engine handle
 * @param options Array of configuration options (may be NULL)
 * @param option_count Number of options in the array
 * @return UCRA_SUCCESS on success, error code on failure
 */
UCRA_API UCRA_Result UCRA_CALL
ucra_engine_create(UCRA_Handle* outEngine,
                   const UCRA_KeyValue* options,
                   uint32_t option_count);

/**
 * @brief Destroy a UCRA engine instance
 *
 * Releases all resources associated with the engine. The handle
 * becomes invalid after this call.
 *
 * @param engine Engine handle to destroy
 */
UCRA_API void UCRA_CALL
ucra_engine_destroy(UCRA_Handle engine);

/**
 * @brief Get engine implementation information
 *
 * Returns a human-readable string describing the engine implementation,
 * including name and version information.
 *
 * @param engine Engine handle
 * @param outBuffer Buffer to write the information string
 * @param buffer_size Size of the output buffer in bytes
 * @return UCRA_SUCCESS if the string was written successfully
 */
UCRA_API UCRA_Result UCRA_CALL
ucra_engine_getinfo(UCRA_Handle engine,
                    char* outBuffer,
                    size_t buffer_size);

/**
 * @brief Render audio for the provided configuration
 *
 * Synthesizes audio based on the input note segments and configuration.
 * The result contains PCM data that remains valid until the next render
 * call or engine destruction.
 *
 * @param engine Engine handle
 * @param config Render configuration including notes and options
 * @param outResult Pointer to store the render result
 * @return UCRA_SUCCESS on successful rendering
 */
UCRA_API UCRA_Result UCRA_CALL
ucra_render(UCRA_Handle engine,
            const UCRA_RenderConfig* config,
            UCRA_RenderResult* outResult);

/** @} */

/**
 * @brief Manifest Parsing API
 * @defgroup ManifestAPI Manifest Parsing Functions
 * @{
 */

/**
 * @brief Load and parse a manifest file
 *
 * Reads a JSON manifest file and parses it into a UCRA_Manifest structure.
 * The manifest must be freed with ucra_manifest_free() when no longer needed.
 *
 * @param manifest_path Path to the manifest JSON file
 * @param outManifest Pointer to store the parsed manifest
 * @return UCRA_SUCCESS on successful parsing
 */
UCRA_API UCRA_Result UCRA_CALL
ucra_manifest_load(const char* manifest_path,
                   UCRA_Manifest** outManifest);

/**
 * @brief Free a manifest and all associated memory
 *
 * Releases all memory allocated for the manifest structure and its contents.
 *
 * @param manifest Manifest to free (may be NULL)
 */
UCRA_API void UCRA_CALL
ucra_manifest_free(UCRA_Manifest* manifest);

/** @} */

/**
 * @brief Streaming API
 * @defgroup StreamingAPI Real-time Streaming Functions
 * @{
 */

/**
 * @brief Initialize a new streaming session
 *
 * Creates a streaming session with the provided configuration and callback.
 * The callback will be invoked when more PCM data is needed.
 *
 * @param out_stream Output stream handle
 * @param config Base render configuration (sample_rate, channels, block_size)
 * @param callback Function to call when more data is needed
 * @param user_data User context data passed to the callback
 * @return UCRA_SUCCESS on success, or an error code on failure
 */
UCRA_API UCRA_Result UCRA_CALL
ucra_stream_open(UCRA_StreamHandle* out_stream,
                 const UCRA_RenderConfig* config,
                 UCRA_PullPCM callback,
                 void* user_data);

/**
 * @brief Read a block of PCM data from the stream
 *
 * This function may block until sufficient data is available.
 *
 * @param stream Stream handle
 * @param out_buffer Buffer to write PCM data (interleaved float32)
 * @param frame_count Number of frames to read
 * @param out_frames_read Actual number of frames read (may be less than requested)
 * @return UCRA_SUCCESS on success, or an error code on failure
 */
UCRA_API UCRA_Result UCRA_CALL
ucra_stream_read(UCRA_StreamHandle stream,
                 float* out_buffer,
                 uint32_t frame_count,
                 uint32_t* out_frames_read);

/**
 * @brief Close and cleanup a streaming session
 *
 * All resources associated with the stream will be freed.
 *
 * @param stream Stream handle to close
 */
UCRA_API void UCRA_CALL
ucra_stream_close(UCRA_StreamHandle stream);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* UCRA_UCRA_H */
