/*
 * UCRA Streaming API Implementation
 * Real-time audio streaming functionality
 */

#include "ucra/ucra.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

/* Include WORLD engine functions if available */
#ifdef UCRA_HAS_WORLD
/* Forward declarations for WORLD engine functions */
extern UCRA_Result ucra_engine_create(UCRA_Handle* outEngine,
                                      const UCRA_KeyValue* options,
                                      uint32_t option_count);
extern void ucra_engine_destroy(UCRA_Handle engine);
extern UCRA_Result ucra_render(UCRA_Handle engine,
                               const UCRA_RenderConfig* config,
                               UCRA_RenderResult* outResult);
#endif

/* Platform-specific threading includes */
#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    typedef HANDLE pthread_t;
    typedef CRITICAL_SECTION pthread_mutex_t;
    typedef CONDITION_VARIABLE pthread_cond_t;
    #define PTHREAD_MUTEX_INITIALIZER {}
    #define PTHREAD_COND_INITIALIZER CONDITION_VARIABLE_INIT

    /* Define M_PI if not available on Windows */
    #ifndef M_PI
        #define M_PI 3.14159265358979323846
    #endif
#else
    #include <pthread.h>
#endif

/* Internal stream state structure */
typedef struct UCRA_StreamState {
    /* Configuration */
    UCRA_RenderConfig config;
    UCRA_PullPCM callback;
    void* user_data;

    /* Ring buffer for PCM data */
    float* buffer;
    uint32_t buffer_size_frames;  /* Total buffer size in frames */
    uint32_t write_pos;           /* Current write position (frames) */
    uint32_t read_pos;            /* Current read position (frames) */
    uint32_t available_frames;    /* Number of frames available to read */

    /* Thread synchronization */
    pthread_mutex_t mutex;
    pthread_cond_t data_available; /* Signaled when data is available */

    /* State flags */
    int is_initialized;
    int is_closed;

    /* Audio generation state (for mock rendering) */
    double phase;                 /* Current phase for oscillator */
    uint64_t total_frames_generated; /* Total frames generated */
} UCRA_StreamState;

/* Default buffer size: 4096 frames (about 93ms at 44.1kHz) */
#define DEFAULT_BUFFER_SIZE_FRAMES 4096

/* Simple sine wave generator for testing/demo purposes */
static void generate_sine_wave(float* buffer, uint32_t frames, uint32_t channels,
                                uint32_t sample_rate, double* phase, float frequency) {
    const double phase_increment = 2.0 * M_PI * frequency / sample_rate;

    for (uint32_t frame = 0; frame < frames; frame++) {
        float sample = (float)(sin(*phase) * 0.1); /* Low volume to prevent ear damage */

        for (uint32_t ch = 0; ch < channels; ch++) {
            buffer[frame * channels + ch] = sample;
        }

        *phase += phase_increment;
        if (*phase >= 2.0 * M_PI) {
            *phase -= 2.0 * M_PI;
        }
    }
}

/* Render audio based on note segments from the callback */
static UCRA_Result render_audio_from_notes(UCRA_StreamState* state,
                                           const UCRA_RenderConfig* render_config,
                                           float* output_buffer,
                                           uint32_t frames_to_render) {
    /* Clear output buffer first */
    memset(output_buffer, 0, frames_to_render * state->config.channels * sizeof(float));

    /* If no notes provided, generate silence */
    if (!render_config->notes || render_config->note_count == 0) {
        return UCRA_SUCCESS;
    }

#ifdef UCRA_HAS_WORLD
    /* Use WORLD engine for rendering if available */
    UCRA_Handle world_engine = NULL;
    UCRA_Result engine_result = ucra_engine_create(&world_engine, NULL, 0);

    if (engine_result == UCRA_SUCCESS && world_engine != NULL) {
        /* Create a temporary render config for the current block */
        UCRA_RenderConfig block_config = *render_config;
        block_config.sample_rate = state->config.sample_rate;
        block_config.channels = state->config.channels;

        /* Calculate the time range for this block */
        double current_time = (double)state->total_frames_generated / state->config.sample_rate;
        double block_duration = (double)frames_to_render / state->config.sample_rate;

        /* Filter notes that are active in this time range */
        UCRA_NoteSegment* active_notes = NULL;
        uint32_t active_note_count = 0;

        /* Count active notes */
        for (uint32_t i = 0; i < render_config->note_count; i++) {
            const UCRA_NoteSegment* note = &render_config->notes[i];
            if (current_time < note->start_sec + note->duration_sec &&
                current_time + block_duration > note->start_sec) {
                active_note_count++;
            }
        }

        if (active_note_count > 0) {
            /* Allocate memory for active notes */
            active_notes = malloc(active_note_count * sizeof(UCRA_NoteSegment));
            if (active_notes) {
                uint32_t active_idx = 0;

                /* Copy active notes and adjust timing for this block */
                for (uint32_t i = 0; i < render_config->note_count; i++) {
                    const UCRA_NoteSegment* note = &render_config->notes[i];
                    if (current_time < note->start_sec + note->duration_sec &&
                        current_time + block_duration > note->start_sec) {

                        active_notes[active_idx] = *note;
                        /* Adjust start time to be relative to current block */
                        active_notes[active_idx].start_sec = fmax(0.0, note->start_sec - current_time);
                        /* Adjust duration to fit within block */
                        double note_end = note->start_sec + note->duration_sec;
                        double block_end = current_time + block_duration;
                        active_notes[active_idx].duration_sec = fmin(note_end, block_end) -
                                                               fmax(note->start_sec, current_time);
                        active_idx++;
                    }
                }

                block_config.notes = active_notes;
                block_config.note_count = active_note_count;

                /* Render using WORLD engine */
                UCRA_RenderResult world_result;
                UCRA_Result render_result = ucra_render(world_engine, &block_config, &world_result);

                if (render_result == UCRA_SUCCESS && world_result.pcm && world_result.frames > 0) {
                    /* Copy rendered audio to output buffer */
                    uint32_t frames_to_copy = (uint32_t)fmin(frames_to_render, world_result.frames);
                    uint32_t samples_to_copy = frames_to_copy * state->config.channels;

                    for (uint32_t i = 0; i < samples_to_copy; i++) {
                        output_buffer[i] = world_result.pcm[i];
                    }
                }

                free(active_notes);
            }
        }

        ucra_engine_destroy(world_engine);
        return UCRA_SUCCESS;
    }
#endif

    /* Fallback to simple sine wave generator if WORLD is not available */
    /* Simple implementation: render each note as a sine wave and mix them */
    for (uint32_t note_idx = 0; note_idx < render_config->note_count; note_idx++) {
        const UCRA_NoteSegment* note = &render_config->notes[note_idx];

        /* Skip notes that are not active in the current time range */
        double current_time = (double)state->total_frames_generated / state->config.sample_rate;
        double block_duration = (double)frames_to_render / state->config.sample_rate;

        if (current_time >= note->start_sec + note->duration_sec ||
            current_time + block_duration <= note->start_sec) {
            continue; /* Note is not active in this time range */
        }

        /* Calculate frequency from MIDI note */
        float frequency = 440.0f; /* Default A4 */
        if (note->midi_note >= 0) {
            frequency = 440.0f * powf(2.0f, (note->midi_note - 69) / 12.0f);
        }

        /* Generate sine wave for this note */
        float* temp_buffer = malloc(frames_to_render * state->config.channels * sizeof(float));
        if (temp_buffer) {
            double temp_phase = state->phase; /* Use a temporary phase for mixing */
            generate_sine_wave(temp_buffer, frames_to_render, state->config.channels,
                              state->config.sample_rate, &temp_phase, frequency);

            /* Mix into output buffer with velocity scaling */
            float volume = note->velocity / 127.0f * 0.3f; /* Scale velocity to reasonable volume */
            for (uint32_t i = 0; i < frames_to_render * state->config.channels; i++) {
                output_buffer[i] += temp_buffer[i] * volume;
            }

            free(temp_buffer);
        }
    }

    return UCRA_SUCCESS;
}

/* Helper function to calculate available space in ring buffer */
static inline uint32_t get_available_space_frames(UCRA_StreamState* state) {
    return state->buffer_size_frames - state->available_frames;
}

/* Helper function to advance ring buffer position */
static inline uint32_t advance_position(uint32_t pos, uint32_t frames, uint32_t buffer_size) {
    return (pos + frames) % buffer_size;
}

UCRA_Result ucra_stream_open(UCRA_StreamHandle* out_stream,
                             const UCRA_RenderConfig* config,
                             UCRA_PullPCM callback,
                             void* user_data) {
    if (!out_stream || !config || !callback) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    /* Validate configuration */
    if (config->sample_rate == 0 || config->channels == 0 || config->block_size == 0) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    /* Allocate stream state */
    UCRA_StreamState* state = calloc(1, sizeof(UCRA_StreamState));
    if (!state) {
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    /* Copy configuration */
    memcpy(&state->config, config, sizeof(UCRA_RenderConfig));
    state->callback = callback;
    state->user_data = user_data;

    /* Allocate ring buffer - ensure it's at least 4x the block size for good buffering */
    uint32_t min_buffer_size = config->block_size * 4;
    state->buffer_size_frames = (min_buffer_size > DEFAULT_BUFFER_SIZE_FRAMES) ?
                                min_buffer_size : DEFAULT_BUFFER_SIZE_FRAMES;

    size_t buffer_size_bytes = state->buffer_size_frames * config->channels * sizeof(float);
    state->buffer = malloc(buffer_size_bytes);
    if (!state->buffer) {
        free(state);
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    /* Initialize buffer to silence */
    memset(state->buffer, 0, buffer_size_bytes);

    /* Initialize ring buffer state */
    state->write_pos = 0;
    state->read_pos = 0;
    state->available_frames = 0;

    /* Initialize audio generation state */
    state->phase = 0.0;
    state->total_frames_generated = 0;

    /* Initialize mutex and condition variable */
    int result = pthread_mutex_init(&state->mutex, NULL);
    if (result != 0) {
        free(state->buffer);
        free(state);
        return UCRA_ERR_INTERNAL;
    }

    result = pthread_cond_init(&state->data_available, NULL);
    if (result != 0) {
        pthread_mutex_destroy(&state->mutex);
        free(state->buffer);
        free(state);
        return UCRA_ERR_INTERNAL;
    }

    state->is_initialized = 1;
    state->is_closed = 0;

    *out_stream = (UCRA_StreamHandle)state;
    return UCRA_SUCCESS;
}

void ucra_stream_close(UCRA_StreamHandle stream) {
    if (!stream) return;

    UCRA_StreamState* state = (UCRA_StreamState*)stream;

    /* Lock mutex to ensure thread safety */
    pthread_mutex_lock(&state->mutex);

    /* Mark as closed */
    state->is_closed = 1;

    /* Wake up any waiting readers */
    pthread_cond_broadcast(&state->data_available);

    pthread_mutex_unlock(&state->mutex);

    /* Cleanup resources */
    pthread_cond_destroy(&state->data_available);
    pthread_mutex_destroy(&state->mutex);

    free(state->buffer);
    free(state);
}

/* Private function to refill the stream buffer by calling the user callback */
static UCRA_Result refill_stream_buffer(UCRA_StreamState* state) {
    /* This function should be called with the mutex locked */

    if (state->is_closed) {
        return UCRA_ERR_INTERNAL;
    }

    /* Check if we have space for at least one block */
    uint32_t available_space = get_available_space_frames(state);
    if (available_space < state->config.block_size) {
        /* Buffer is full, nothing to do */
        return UCRA_SUCCESS;
    }

    /* Prepare render config for callback */
    UCRA_RenderConfig render_config = state->config;

    /* Call user callback to get next render configuration */
    UCRA_Result callback_result = state->callback(state->user_data, &render_config);
    if (callback_result != UCRA_SUCCESS) {
        return callback_result;
    }

    /* Determine how many frames to render */
    uint32_t frames_to_write = state->config.block_size;
    uint32_t channels = state->config.channels;

    /* Ensure we don't overflow the buffer */
    if (frames_to_write > available_space) {
        frames_to_write = available_space;
    }

    /* Allocate temporary buffer for rendering */
    float* temp_buffer = malloc(frames_to_write * channels * sizeof(float));
    if (!temp_buffer) {
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    /* Render audio using the provided configuration */
    UCRA_Result render_result = render_audio_from_notes(state, &render_config,
                                                        temp_buffer, frames_to_write);
    if (render_result != UCRA_SUCCESS) {
        free(temp_buffer);
        return render_result;
    }

    /* Copy rendered data to ring buffer, handling wraparound */
    uint32_t frames_copied = 0;
    while (frames_copied < frames_to_write) {
        uint32_t frames_until_wrap = state->buffer_size_frames - state->write_pos;
        uint32_t frames_to_copy = frames_to_write - frames_copied;

        if (frames_to_copy > frames_until_wrap) {
            frames_to_copy = frames_until_wrap;
        }

        /* Copy data */
        size_t src_offset = frames_copied * channels;
        size_t dst_offset = state->write_pos * channels;
        size_t samples_to_copy = frames_to_copy * channels;

        memcpy(state->buffer + dst_offset, temp_buffer + src_offset,
               samples_to_copy * sizeof(float));

        /* Update positions */
        state->write_pos = advance_position(state->write_pos, frames_to_copy, state->buffer_size_frames);
        frames_copied += frames_to_copy;
    }

    /* Update buffer state */
    state->available_frames += frames_to_write;
    state->total_frames_generated += frames_to_write;

    /* Update phase for continuous audio generation */
    double phase_increment = 2.0 * M_PI * 440.0 / state->config.sample_rate; /* A4 frequency for now */
    state->phase += phase_increment * frames_to_write;
    if (state->phase >= 2.0 * M_PI) {
        state->phase = fmod(state->phase, 2.0 * M_PI);
    }

    free(temp_buffer);

    /* Signal that data is available */
    pthread_cond_signal(&state->data_available);

    return UCRA_SUCCESS;
}

UCRA_Result ucra_stream_read(UCRA_StreamHandle stream,
                             float* out_buffer,
                             uint32_t frame_count,
                             uint32_t* out_frames_read) {
    if (!stream || !out_buffer || !out_frames_read) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    UCRA_StreamState* state = (UCRA_StreamState*)stream;
    *out_frames_read = 0;

    if (!state->is_initialized || state->is_closed) {
        return UCRA_ERR_INTERNAL;
    }

    pthread_mutex_lock(&state->mutex);

    uint32_t frames_copied = 0;
    uint32_t channels = state->config.channels;

    while (frames_copied < frame_count && !state->is_closed) {
        /* If no data available, try to refill the buffer */
        if (state->available_frames == 0) {
            UCRA_Result refill_result = refill_stream_buffer(state);
            if (refill_result != UCRA_SUCCESS) {
                pthread_mutex_unlock(&state->mutex);
                return refill_result;
            }
        }

        /* If still no data available after refill, wait for it */
        if (state->available_frames == 0 && !state->is_closed) {
            pthread_cond_wait(&state->data_available, &state->mutex);
            continue;
        }

        /* Copy available data */
        uint32_t frames_to_copy = frame_count - frames_copied;
        if (frames_to_copy > state->available_frames) {
            frames_to_copy = state->available_frames;
        }

        /* Handle ring buffer wraparound */
        uint32_t frames_until_wrap = state->buffer_size_frames - state->read_pos;
        if (frames_to_copy > frames_until_wrap) {
            frames_to_copy = frames_until_wrap;
        }

        /* Copy the data */
        size_t samples_to_copy = frames_to_copy * channels;
        size_t src_offset = state->read_pos * channels;
        size_t dst_offset = frames_copied * channels;

        memcpy(out_buffer + dst_offset, state->buffer + src_offset,
               samples_to_copy * sizeof(float));

        /* Update buffer state */
        state->read_pos = advance_position(state->read_pos, frames_to_copy, state->buffer_size_frames);
        state->available_frames -= frames_to_copy;
        frames_copied += frames_to_copy;
    }

    pthread_mutex_unlock(&state->mutex);

    *out_frames_read = frames_copied;
    return UCRA_SUCCESS;
}

#ifdef _WIN32
/* Windows pthread compatibility layer */
static int pthread_mutex_init(pthread_mutex_t* mutex, void* attr) {
    (void)attr;
    InitializeCriticalSection(mutex);
    return 0;
}

static int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    DeleteCriticalSection(mutex);
    return 0;
}

static int pthread_mutex_lock(pthread_mutex_t* mutex) {
    EnterCriticalSection(mutex);
    return 0;
}

static int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    LeaveCriticalSection(mutex);
    return 0;
}

static int pthread_cond_init(pthread_cond_t* cond, void* attr) {
    (void)attr;
    InitializeConditionVariable(cond);
    return 0;
}

static int pthread_cond_destroy(pthread_cond_t* cond) {
    (void)cond; /* No cleanup needed for Windows condition variables */
    return 0;
}

static int pthread_cond_signal(pthread_cond_t* cond) {
    WakeConditionVariable(cond);
    return 0;
}

static int pthread_cond_broadcast(pthread_cond_t* cond) {
    WakeAllConditionVariable(cond);
    return 0;
}

static int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    SleepConditionVariableCS(cond, mutex, INFINITE);
    return 0;
}

static unsigned int __stdcall thread_proc_wrapper(void* arg) {
    void* (*start_routine)(void*) = ((void**)arg)[0];
    void* thread_arg = ((void**)arg)[1];
    free(arg);
    start_routine(thread_arg);
    return 0;
}

static int pthread_create(pthread_t* thread, void* attr, void* (*start_routine)(void*), void* arg) {
    (void)attr;
    void** wrapper_args = malloc(2 * sizeof(void*));
    if (!wrapper_args) return ENOMEM;

    wrapper_args[0] = (void*)start_routine;
    wrapper_args[1] = arg;

    *thread = (HANDLE)_beginthreadex(NULL, 0, thread_proc_wrapper, wrapper_args, 0, NULL);
    return *thread ? 0 : EAGAIN;
}

static int pthread_join(pthread_t thread, void** retval) {
    (void)retval;
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}
#endif
