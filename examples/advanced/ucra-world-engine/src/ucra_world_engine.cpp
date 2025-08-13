/*
 * UCRA WORLD Vocoder Engine Implementation
 * This file provides the WORLD vocoder engine implementation for UCRA.
 */

#include "ucra/ucra.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <memory>
#include <vector>

#ifdef UCRA_HAS_WORLD
#include "world/d4c.h"
#include "world/dio.h"
#include "world/harvest.h"
#include "world/cheaptrick.h"
#include "world/stonemask.h"
#include "world/synthesis.h"
#include "world/synthesisrealtime.h"
#include "world/constantnumbers.h"
#include "world/common.h"
#endif

extern "C" {

#ifdef UCRA_HAS_WORLD

/* Internal WORLD engine state */
typedef struct UCRA_WorldEngine {
    double sample_rate;
    int fft_size;
    double frame_period; /* Frame period in milliseconds */

    /* Analysis parameters */
    DioOption dio_option;
    HarvestOption harvest_option;
    CheapTrickOption cheaptrick_option;
    D4COption d4c_option;

    /* Synthesis parameters */
    int synthesis_frame_period;

    /* Engine info */
    char engine_info[256];

    /* Last render results (memory owned by engine) */
    float* last_pcm;
    size_t last_pcm_size;
    UCRA_KeyValue* last_metadata;
    size_t last_metadata_count;
} UCRA_WorldEngine;

/* Global engine instance (for simplicity - in production might want multiple) */
static UCRA_WorldEngine* g_world_engine = nullptr;

/* Helper function to convert MIDI note to frequency */
static double midi_note_to_frequency(int16_t midi_note) {
    if (midi_note < 0) {
        return 440.0; /* Default to A4 */
    }
    return 440.0 * std::pow(2.0, (midi_note - 69) / 12.0);
}

/* Convert UCRA note segments to WORLD-compatible F0 and time arrays */
static void prepare_world_f0_data(const UCRA_NoteSegment* notes, uint32_t note_count,
                                  double sample_rate, double frame_period_ms,
                                  double total_duration_sec,
                                  std::vector<double>& f0_array,
                                  std::vector<double>& time_array) {
    /* Calculate number of frames */
    int frame_count = static_cast<int>(1000.0 * total_duration_sec / frame_period_ms) + 1;

    f0_array.resize(frame_count, 0.0);
    time_array.resize(frame_count);

    /* Fill time array */
    for (int i = 0; i < frame_count; i++) {
        time_array[i] = i * frame_period_ms / 1000.0;
    }

    /* Fill F0 array based on note segments */
    for (uint32_t note_idx = 0; note_idx < note_count; note_idx++) {
        const UCRA_NoteSegment* note = &notes[note_idx];

        if (note->midi_note < 0) continue; /* Skip non-pitched notes */

        double note_f0 = midi_note_to_frequency(note->midi_note);

        /* Find frames that correspond to this note */
        int start_frame = static_cast<int>(note->start_sec * 1000.0 / frame_period_ms);
        int end_frame = static_cast<int>((note->start_sec + note->duration_sec) * 1000.0 / frame_period_ms);

        /* Clamp to valid range */
        start_frame = std::max(0, std::min(start_frame, frame_count - 1));
        end_frame = std::max(0, std::min(end_frame, frame_count - 1));

        /* Fill F0 for this note */
        for (int frame = start_frame; frame <= end_frame; frame++) {
            if (note->f0_override && note->f0_override->length > 0) {
                /* Use F0 override if available */
                double relative_time = time_array[frame] - note->start_sec;
                /* Simple linear interpolation for F0 override */
                if (relative_time >= 0 && relative_time <= note->duration_sec) {
                    /* Find closest point in F0 curve */
                    uint32_t curve_idx = 0;
                    for (uint32_t i = 0; i < note->f0_override->length - 1; i++) {
                        if (note->f0_override->time_sec[i] <= relative_time) {
                            curve_idx = i;
                        }
                    }
                    f0_array[frame] = note->f0_override->f0_hz[curve_idx];
                }
            } else {
                /* Use MIDI note frequency */
                f0_array[frame] = note_f0;
            }
        }
    }
}

UCRA_Result ucra_engine_create(UCRA_Handle* outEngine,
                               const UCRA_KeyValue* options,
                               uint32_t option_count) {
    if (!outEngine) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    /* For simplicity, create a single global engine instance */
    if (g_world_engine != nullptr) {
        *outEngine = reinterpret_cast<UCRA_Handle>(g_world_engine);
        return UCRA_SUCCESS;
    }

    /* Allocate engine state */
    g_world_engine = static_cast<UCRA_WorldEngine*>(malloc(sizeof(UCRA_WorldEngine)));
    if (!g_world_engine) {
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    /* Initialize with default parameters */
    g_world_engine->sample_rate = 44100.0;
    g_world_engine->frame_period = 5.0; /* 5ms frame period */

    /* Initialize WORLD analysis options */
    InitializeDioOption(&g_world_engine->dio_option);
    InitializeHarvestOption(&g_world_engine->harvest_option);
    InitializeCheapTrickOption(static_cast<int>(g_world_engine->sample_rate), &g_world_engine->cheaptrick_option);
    InitializeD4COption(&g_world_engine->d4c_option);

    /* Get FFT size from CheapTrick option */
    g_world_engine->fft_size = GetFFTSizeForCheapTrick(static_cast<int>(g_world_engine->sample_rate), &g_world_engine->cheaptrick_option);
    g_world_engine->synthesis_frame_period = static_cast<int>(g_world_engine->frame_period);

    /* Set frame period for DIO and Harvest options */
    g_world_engine->dio_option.frame_period = g_world_engine->frame_period;
    g_world_engine->harvest_option.frame_period = g_world_engine->frame_period;

    /* Initialize memory for results */
    g_world_engine->last_pcm = nullptr;
    g_world_engine->last_pcm_size = 0;
    g_world_engine->last_metadata = nullptr;
    g_world_engine->last_metadata_count = 0;

    /* Process options if provided */
    for (uint32_t i = 0; i < option_count; i++) {
        const char* key = options[i].key;
        const char* value = options[i].value;

        if (key && value) {
            if (strcmp(key, "sample_rate") == 0) {
                double sr = atof(value);
                if (sr > 0) {
                    g_world_engine->sample_rate = sr;
                    InitializeCheapTrickOption(static_cast<int>(sr), &g_world_engine->cheaptrick_option);
                    g_world_engine->fft_size = GetFFTSizeForCheapTrick(static_cast<int>(sr), &g_world_engine->cheaptrick_option);
                }
            } else if (strcmp(key, "frame_period") == 0) {
                double fp = atof(value);
                if (fp > 0) {
                    g_world_engine->frame_period = fp;
                    g_world_engine->synthesis_frame_period = static_cast<int>(fp);
                    g_world_engine->dio_option.frame_period = fp;
                    g_world_engine->harvest_option.frame_period = fp;
                }
            }
        }
    }

    /* Prepare engine info string */
    snprintf(g_world_engine->engine_info, sizeof(g_world_engine->engine_info),
             "WORLD Vocoder Engine v1.0 (sample_rate=%.1f, frame_period=%.1f)",
             g_world_engine->sample_rate, g_world_engine->frame_period);

    *outEngine = reinterpret_cast<UCRA_Handle>(g_world_engine);
    return UCRA_SUCCESS;
}

void ucra_engine_destroy(UCRA_Handle engine) {
    if (!engine || engine != reinterpret_cast<UCRA_Handle>(g_world_engine)) {
        return;
    }

    UCRA_WorldEngine* world_engine = reinterpret_cast<UCRA_WorldEngine*>(engine);

    /* Free last render results */
    if (world_engine->last_pcm) {
        free(world_engine->last_pcm);
        world_engine->last_pcm = nullptr;
    }

    if (world_engine->last_metadata) {
        free(world_engine->last_metadata);
        world_engine->last_metadata = nullptr;
    }

    /* Free engine state */
    free(world_engine);
    g_world_engine = nullptr;
}

UCRA_Result ucra_engine_getinfo(UCRA_Handle engine,
                                char* outBuffer,
                                size_t buffer_size) {
    if (!engine || !outBuffer || buffer_size == 0) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    UCRA_WorldEngine* world_engine = reinterpret_cast<UCRA_WorldEngine*>(engine);

    size_t info_len = strlen(world_engine->engine_info);
    if (info_len >= buffer_size) {
        return UCRA_ERR_INVALID_ARGUMENT; /* Buffer too small */
    }

    strcpy(outBuffer, world_engine->engine_info);
    return UCRA_SUCCESS;
}

UCRA_Result ucra_render(UCRA_Handle engine,
                        const UCRA_RenderConfig* config,
                        UCRA_RenderResult* outResult) {
    if (!engine || !config || !outResult) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    UCRA_WorldEngine* world_engine = reinterpret_cast<UCRA_WorldEngine*>(engine);

    /* Update engine parameters from config */
    if (config->sample_rate > 0 && config->sample_rate != world_engine->sample_rate) {
        world_engine->sample_rate = config->sample_rate;
        InitializeCheapTrickOption(static_cast<int>(config->sample_rate), &world_engine->cheaptrick_option);
        world_engine->fft_size = GetFFTSizeForCheapTrick(static_cast<int>(config->sample_rate), &world_engine->cheaptrick_option);
    }

    /* Calculate total duration from notes */
    double total_duration = 0.0;
    for (uint32_t i = 0; i < config->note_count; i++) {
        double note_end = config->notes[i].start_sec + config->notes[i].duration_sec;
        if (note_end > total_duration) {
            total_duration = note_end;
        }
    }

    if (total_duration <= 0.0) {
        /* No notes to render - return silence */
        outResult->pcm = nullptr;
        outResult->frames = 0;
        outResult->channels = config->channels;
        outResult->sample_rate = static_cast<uint32_t>(world_engine->sample_rate);
        outResult->metadata = nullptr;
        outResult->metadata_count = 0;
        outResult->status = UCRA_SUCCESS;
        return UCRA_SUCCESS;
    }

    /* Prepare F0 data for WORLD */
    std::vector<double> f0_array, time_array;
    prepare_world_f0_data(config->notes, config->note_count,
                          world_engine->sample_rate, world_engine->frame_period,
                          total_duration, f0_array, time_array);

    int frame_count = static_cast<int>(f0_array.size());
    if (frame_count <= 0) {
        outResult->status = UCRA_ERR_INTERNAL;
        return UCRA_ERR_INTERNAL;
    }

    /* Allocate arrays for spectral envelope and aperiodicity */
    double** spectrogram = nullptr;
    double** aperiodicity = nullptr;

    try {
        /* Allocate 2D arrays for WORLD */
        spectrogram = new double*[frame_count];
        aperiodicity = new double*[frame_count];

        for (int i = 0; i < frame_count; i++) {
            spectrogram[i] = new double[world_engine->fft_size / 2 + 1];
            aperiodicity[i] = new double[world_engine->fft_size / 2 + 1];

            /* Initialize with default values */
            for (int j = 0; j < world_engine->fft_size / 2 + 1; j++) {
                spectrogram[i][j] = -60.0; /* -60dB default */
                aperiodicity[i][j] = 0.0;  /* Voiced by default */
            }
        }

        /* Perform CheapTrick analysis (spectral envelope) */
        /* For synthesis, we'll use a simple spectral model */
        for (int i = 0; i < frame_count; i++) {
            if (f0_array[i] > 0.0) {
                /* Generate harmonic spectrum for voiced frames */
                double fundamental = f0_array[i];
                for (int j = 0; j < world_engine->fft_size / 2 + 1; j++) {
                    double freq = static_cast<double>(j) * world_engine->sample_rate / world_engine->fft_size;

                    /* Simple harmonic model */
                    if (freq > 0) {
                        double harmonic_num = freq / fundamental;
                        if (harmonic_num >= 1.0 && harmonic_num <= 20.0) {
                            /* Calculate harmonic amplitude (decreasing with harmonic number) */
                            double amplitude = -20.0 * log10(harmonic_num + 1.0);
                            spectrogram[i][j] = amplitude;
                        }
                    }
                }

                /* Set aperiodicity for voiced frames */
                for (int j = 0; j < world_engine->fft_size / 2 + 1; j++) {
                    aperiodicity[i][j] = 0.1; /* Slightly aperiodic */
                }
            } else {
                /* Unvoiced frame - set high aperiodicity */
                for (int j = 0; j < world_engine->fft_size / 2 + 1; j++) {
                    spectrogram[i][j] = -40.0; /* Noise spectrum */
                    aperiodicity[i][j] = 0.9;  /* Highly aperiodic */
                }
            }
        }

        /* Synthesize audio using WORLD */
        int output_length = static_cast<int>(total_duration * world_engine->sample_rate);
        std::vector<double> synthesized_audio(output_length);

        Synthesis(f0_array.data(), frame_count, spectrogram, aperiodicity,
                  world_engine->fft_size, world_engine->frame_period,
                  static_cast<int>(world_engine->sample_rate), output_length,
                  synthesized_audio.data());

        /* Convert to float and store in engine */
        size_t total_samples = output_length * config->channels;

        /* Free previous results */
        if (world_engine->last_pcm) {
            free(world_engine->last_pcm);
        }

        world_engine->last_pcm = static_cast<float*>(malloc(total_samples * sizeof(float)));
        if (!world_engine->last_pcm) {
            /* Cleanup and return error */
            for (int i = 0; i < frame_count; i++) {
                delete[] spectrogram[i];
                delete[] aperiodicity[i];
            }
            delete[] spectrogram;
            delete[] aperiodicity;

            outResult->status = UCRA_ERR_OUT_OF_MEMORY;
            return UCRA_ERR_OUT_OF_MEMORY;
        }

        world_engine->last_pcm_size = total_samples;

        /* Convert to interleaved float and duplicate for multiple channels */
        for (int sample = 0; sample < output_length; sample++) {
            float sample_value = static_cast<float>(synthesized_audio[sample]);
            for (uint32_t ch = 0; ch < config->channels; ch++) {
                world_engine->last_pcm[sample * config->channels + ch] = sample_value;
            }
        }

        /* Fill output result */
        outResult->pcm = world_engine->last_pcm;
        outResult->frames = output_length;
        outResult->channels = config->channels;
        outResult->sample_rate = static_cast<uint32_t>(world_engine->sample_rate);
        outResult->metadata = nullptr;
        outResult->metadata_count = 0;
        outResult->status = UCRA_SUCCESS;

        /* Cleanup WORLD arrays */
        for (int i = 0; i < frame_count; i++) {
            delete[] spectrogram[i];
            delete[] aperiodicity[i];
        }
        delete[] spectrogram;
        delete[] aperiodicity;

        return UCRA_SUCCESS;

    } catch (...) {
        /* Exception occurred - cleanup and return error */
        if (spectrogram) {
            for (int i = 0; i < frame_count; i++) {
                delete[] spectrogram[i];
            }
            delete[] spectrogram;
        }

        if (aperiodicity) {
            for (int i = 0; i < frame_count; i++) {
                delete[] aperiodicity[i];
            }
            delete[] aperiodicity;
        }

        outResult->status = UCRA_ERR_INTERNAL;
        return UCRA_ERR_INTERNAL;
    }
}

#else /* !UCRA_HAS_WORLD */

/* Stub implementations when WORLD is not available */
UCRA_Result ucra_engine_create(UCRA_Handle* outEngine,
                               const UCRA_KeyValue* options,
                               uint32_t option_count) {
    (void)options;
    (void)option_count;

    if (!outEngine) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    *outEngine = nullptr;
    return UCRA_ERR_NOT_SUPPORTED;
}

void ucra_engine_destroy(UCRA_Handle engine) {
    (void)engine;
}

UCRA_Result ucra_engine_getinfo(UCRA_Handle engine,
                                char* outBuffer,
                                size_t buffer_size) {
    (void)engine;

    if (!outBuffer || buffer_size == 0) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    const char* info = "No engine available (WORLD not compiled)";
    if (strlen(info) >= buffer_size) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    strcpy(outBuffer, info);
    return UCRA_SUCCESS;
}

UCRA_Result ucra_render(UCRA_Handle engine,
                        const UCRA_RenderConfig* config,
                        UCRA_RenderResult* outResult) {
    (void)engine;
    (void)config;

    if (!outResult) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    outResult->status = UCRA_ERR_NOT_SUPPORTED;
    return UCRA_ERR_NOT_SUPPORTED;
}

#endif /* UCRA_HAS_WORLD */

} /* extern "C" */
