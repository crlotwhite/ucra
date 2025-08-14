/*
 * UCRA Reference Engine (pure C, no WORLD dependency)
 * Minimal offline renderer that produces simple sine-wave audio for pitched notes.
 * This is intended as a portable baseline so examples and bindings work without WORLD.
 */

#include "ucra/ucra.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct UCRA_Engine_ {
    double sample_rate;
    /* simple state to own last render buffers */
    float* last_pcm;
    size_t last_pcm_size; /* number of float samples in last_pcm */
    UCRA_KeyValue* last_metadata;
    uint32_t last_metadata_count;
} UCRA_Engine_;

static double midi_to_hz(int16_t midi_note) {
    if (midi_note < 0) return 0.0; /* unvoiced */
    return 440.0 * pow(2.0, ((double)midi_note - 69.0) / 12.0);
}

/* sample a step-wise curve by picking the last point <= t (seconds). */
static double sample_f0_curve(const UCRA_F0Curve* c, double t, double fallback_hz) {
    if (!c || c->length == 0 || !c->time_sec || !c->f0_hz) return fallback_hz;
    uint32_t idx = 0;
    for (uint32_t i = 0; i + 1 < c->length; ++i) {
        if ((double)c->time_sec[i] <= t) idx = i; else break;
    }
    return (double)c->f0_hz[idx];
}

/* sample an envelope curve similarly (step-wise hold). */
static double sample_env_curve(const UCRA_EnvCurve* c, double t, double fallback) {
    if (!c || c->length == 0 || !c->time_sec || !c->value) return fallback;
    uint32_t idx = 0;
    for (uint32_t i = 0; i + 1 < c->length; ++i) {
        if ((double)c->time_sec[i] <= t) idx = i; else break;
    }
    return (double)c->value[idx];
}

UCRA_Result ucra_engine_create(UCRA_Handle* outEngine,
                               const UCRA_KeyValue* options,
                               uint32_t option_count) {
    (void)options; (void)option_count;
    if (!outEngine) return UCRA_ERR_INVALID_ARGUMENT;
    UCRA_Engine_* eng = (UCRA_Engine_*)calloc(1, sizeof(UCRA_Engine_));
    if (!eng) return UCRA_ERR_OUT_OF_MEMORY;
    eng->sample_rate = 44100.0; /* default */
    *outEngine = (UCRA_Handle)eng;
    return UCRA_SUCCESS;
}

void ucra_engine_destroy(UCRA_Handle engine) {
    UCRA_Engine_* eng = (UCRA_Engine_*)engine;
    if (!eng) return;
    if (eng->last_pcm) free(eng->last_pcm);
    if (eng->last_metadata) free(eng->last_metadata);
    free(eng);
}

UCRA_Result ucra_engine_getinfo(UCRA_Handle engine,
                                char* outBuffer,
                                size_t buffer_size) {
    UCRA_Engine_* eng = (UCRA_Engine_*)engine;
    if (!eng || !outBuffer || buffer_size == 0) return UCRA_ERR_INVALID_ARGUMENT;
    const char* info = "UCRA Reference Engine (no WORLD) v1.0";
    size_t len = strlen(info);
    if (len + 1 > buffer_size) return UCRA_ERR_INVALID_ARGUMENT;
    memcpy(outBuffer, info, len + 1);
    return UCRA_SUCCESS;
}

UCRA_Result ucra_render(UCRA_Handle engine,
                        const UCRA_RenderConfig* config,
                        UCRA_RenderResult* outResult) {
    UCRA_Engine_* eng = (UCRA_Engine_*)engine;
    if (!eng || !config || !outResult) return UCRA_ERR_INVALID_ARGUMENT;

    /* Adopt sample rate from config if provided */
    if (config->sample_rate > 0) eng->sample_rate = (double)config->sample_rate;
    uint32_t channels = config->channels > 0 ? config->channels : 1;

    /* compute total duration from notes */
    double total_dur = 0.0;
    for (uint32_t i = 0; i < config->note_count; ++i) {
        double end = config->notes[i].start_sec + config->notes[i].duration_sec;
        if (end > total_dur) total_dur = end;
    }

    if (total_dur <= 0.0) {
        outResult->pcm = NULL;
        outResult->frames = 0;
        outResult->channels = channels;
        outResult->sample_rate = (uint32_t)eng->sample_rate;
        outResult->metadata = NULL;
        outResult->metadata_count = 0;
        outResult->status = UCRA_SUCCESS;
        return UCRA_SUCCESS;
    }

    uint64_t frames = (uint64_t)(total_dur * eng->sample_rate + 0.5);
    if (frames == 0) frames = 1;
    size_t total_samples = (size_t)frames * (size_t)channels;

    /* (re)allocate engine-owned buffer */
    if (eng->last_pcm) { free(eng->last_pcm); eng->last_pcm = NULL; }
    eng->last_pcm = (float*)malloc(total_samples * sizeof(float));
    if (!eng->last_pcm) {
        outResult->status = UCRA_ERR_OUT_OF_MEMORY;
        return UCRA_ERR_OUT_OF_MEMORY;
    }
    eng->last_pcm_size = total_samples;

    /* zero initialize */
    for (size_t i = 0; i < total_samples; ++i) eng->last_pcm[i] = 0.0f;

    /* naive additive synthesis: sine at per-note F0, scaled by velocity and optional env */
    for (uint64_t n = 0; n < frames; ++n) {
        double t = (double)n / eng->sample_rate;
        double mix = 0.0;
        for (uint32_t i = 0; i < config->note_count; ++i) {
            const UCRA_NoteSegment* note = &config->notes[i];
            double start = note->start_sec;
            double end = note->start_sec + note->duration_sec;
            if (t < start || t > end) continue;

            double rel_t = t - start;
            double f0 = note->f0_override ? sample_f0_curve(note->f0_override, rel_t, midi_to_hz(note->midi_note))
                                          : midi_to_hz(note->midi_note);
            if (f0 <= 0.0) continue;
            double env = note->env_override ? sample_env_curve(note->env_override, rel_t, 1.0) : 1.0;
            double vel = (double)(note->velocity) / 127.0; /* 0..1 */
            double amp = 0.2 * vel * env; /* conservative gain to avoid clipping */
            mix += amp * sin(2.0 * M_PI * f0 * t);
        }
        /* simple soft clip */
        if (mix > 1.0) mix = 1.0; else if (mix < -1.0) mix = -1.0;
        float sample = (float)mix;
        for (uint32_t ch = 0; ch < channels; ++ch) {
            eng->last_pcm[n * channels + ch] = sample;
        }
    }

    outResult->pcm = eng->last_pcm;
    outResult->frames = frames;
    outResult->channels = channels;
    outResult->sample_rate = (uint32_t)eng->sample_rate;
    outResult->metadata = NULL;
    outResult->metadata_count = 0;
    outResult->status = UCRA_SUCCESS;
    return UCRA_SUCCESS;
}
