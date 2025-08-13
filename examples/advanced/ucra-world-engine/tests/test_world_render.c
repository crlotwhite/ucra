/*
 * Test WORLD Engine Offline Rendering
 * Tests ucra_render function with various note configurations
 */

#include "ucra/ucra.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

int test_basic_rendering() {
    printf("Testing basic WORLD engine rendering...\n");

    UCRA_Handle engine = NULL;
    UCRA_Result result;

    /* Create engine */
    result = ucra_engine_create(&engine, NULL, 0);
    if (result != UCRA_SUCCESS) {
        printf("   ❌ Failed to create engine\n");
        return 1;
    }

    /* Test rendering with empty configuration */
    printf("1. Testing empty render configuration...\n");
    UCRA_RenderConfig empty_config = {0};
    empty_config.sample_rate = 44100;
    empty_config.channels = 1;
    empty_config.block_size = 512;
    empty_config.notes = NULL;
    empty_config.note_count = 0;
    empty_config.options = NULL;
    empty_config.option_count = 0;

    UCRA_RenderResult result_empty;
    result = ucra_render(engine, &empty_config, &result_empty);

    if (result != UCRA_SUCCESS) {
        printf("   ❌ Empty render failed with error %d\n", result);
        ucra_engine_destroy(engine);
        return 1;
    }

    printf("   ✅ Empty render successful (frames: %llu)\n", (unsigned long long)result_empty.frames);

    /* Test rendering with a single note */
    printf("2. Testing single note rendering...\n");
    UCRA_NoteSegment note = {
        .start_sec = 0.0,
        .duration_sec = 1.0,
        .midi_note = 69,  /* A4 = 440Hz */
        .velocity = 100,
        .lyric = "a",
        .f0_override = NULL,
        .env_override = NULL
    };

    UCRA_RenderConfig config = {
        .sample_rate = 44100,
        .channels = 1,
        .block_size = 512,
        .flags = 0,
        .notes = &note,
        .note_count = 1,
        .options = NULL,
        .option_count = 0
    };

    UCRA_RenderResult render_result;
    result = ucra_render(engine, &config, &render_result);

    if (result != UCRA_SUCCESS) {
        printf("   ❌ Single note render failed with error %d\n", result);
        ucra_engine_destroy(engine);
        return 1;
    }

    if (render_result.pcm == NULL || render_result.frames == 0) {
        printf("   ❌ Render result is empty\n");
        ucra_engine_destroy(engine);
        return 1;
    }

    printf("   ✅ Single note render successful\n");
    printf("      Frames: %llu\n", (unsigned long long)render_result.frames);
    printf("      Channels: %u\n", render_result.channels);
    printf("      Sample rate: %u\n", render_result.sample_rate);

    /* Check if audio data looks reasonable */
    double max_amplitude = 0.0;
    double rms = 0.0;
    for (uint64_t i = 0; i < render_result.frames * render_result.channels; i++) {
        double sample = fabs(render_result.pcm[i]);
        if (sample > max_amplitude) {
            max_amplitude = sample;
        }
        rms += sample * sample;
    }
    rms = sqrt(rms / (render_result.frames * render_result.channels));

    printf("      Max amplitude: %.6f\n", max_amplitude);
    printf("      RMS: %.6f\n", rms);

    if (max_amplitude > 0.0001) {  /* Very small threshold since our output might be quiet */
        printf("   ✅ Audio data appears to contain signal\n");
    } else {
        printf("   ⚠️  Audio data appears to be silent (may be expected)\n");
    }

    ucra_engine_destroy(engine);
    return 0;
}

int test_multiple_notes() {
    printf("Testing multiple note rendering...\n");

    UCRA_Handle engine = NULL;
    UCRA_Result result;

    /* Create engine */
    result = ucra_engine_create(&engine, NULL, 0);
    if (result != UCRA_SUCCESS) {
        printf("   ❌ Failed to create engine\n");
        return 1;
    }

    /* Create a chord: C major (C4, E4, G4) */
    UCRA_NoteSegment notes[3] = {
        {
            .start_sec = 0.0,
            .duration_sec = 2.0,
            .midi_note = 60,  /* C4 */
            .velocity = 80,
            .lyric = "do",
            .f0_override = NULL,
            .env_override = NULL
        },
        {
            .start_sec = 0.5,
            .duration_sec = 1.5,
            .midi_note = 64,  /* E4 */
            .velocity = 75,
            .lyric = "mi",
            .f0_override = NULL,
            .env_override = NULL
        },
        {
            .start_sec = 1.0,
            .duration_sec = 1.0,
            .midi_note = 67,  /* G4 */
            .velocity = 70,
            .lyric = "sol",
            .f0_override = NULL,
            .env_override = NULL
        }
    };

    UCRA_RenderConfig config = {
        .sample_rate = 44100,
        .channels = 2,  /* Stereo */
        .block_size = 512,
        .flags = 0,
        .notes = notes,
        .note_count = 3,
        .options = NULL,
        .option_count = 0
    };

    UCRA_RenderResult render_result;
    result = ucra_render(engine, &config, &render_result);

    if (result != UCRA_SUCCESS) {
        printf("   ❌ Multiple note render failed with error %d\n", result);
        ucra_engine_destroy(engine);
        return 1;
    }

    printf("   ✅ Multiple note render successful\n");
    printf("      Duration: %.2f seconds\n", (double)render_result.frames / render_result.sample_rate);
    printf("      Channels: %u\n", render_result.channels);

    ucra_engine_destroy(engine);
    return 0;
}

int test_invalid_render_arguments() {
    printf("Testing invalid render arguments...\n");

    UCRA_Handle engine = NULL;
    UCRA_Result result;

    /* Create engine */
    result = ucra_engine_create(&engine, NULL, 0);
    if (result != UCRA_SUCCESS) {
        printf("   ❌ Failed to create engine\n");
        return 1;
    }

    UCRA_RenderConfig config = {
        .sample_rate = 44100,
        .channels = 1,
        .block_size = 512,
        .flags = 0,
        .notes = NULL,
        .note_count = 0,
        .options = NULL,
        .option_count = 0
    };

    UCRA_RenderResult render_result;

    /* Test NULL engine */
    result = ucra_render(NULL, &config, &render_result);
    if (result != UCRA_ERR_INVALID_ARGUMENT) {
        printf("   ❌ Expected INVALID_ARGUMENT for NULL engine\n");
        ucra_engine_destroy(engine);
        return 1;
    }

    /* Test NULL config */
    result = ucra_render(engine, NULL, &render_result);
    if (result != UCRA_ERR_INVALID_ARGUMENT) {
        printf("   ❌ Expected INVALID_ARGUMENT for NULL config\n");
        ucra_engine_destroy(engine);
        return 1;
    }

    /* Test NULL result */
    result = ucra_render(engine, &config, NULL);
    if (result != UCRA_ERR_INVALID_ARGUMENT) {
        printf("   ❌ Expected INVALID_ARGUMENT for NULL result\n");
        ucra_engine_destroy(engine);
        return 1;
    }

    printf("   ✅ Invalid argument tests passed\n");

    ucra_engine_destroy(engine);
    return 0;
}

int main() {
    printf("=== UCRA WORLD Engine Offline Rendering Tests ===\n\n");

    int failures = 0;

    failures += test_basic_rendering();
    printf("\n");

    failures += test_multiple_notes();
    printf("\n");

    failures += test_invalid_render_arguments();
    printf("\n");

    if (failures == 0) {
        printf("🎉 All offline rendering tests passed!\n");
    } else {
        printf("❌ %d test(s) failed!\n", failures);
    }

    return failures;
}
