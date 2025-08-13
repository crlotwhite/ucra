/*
 * Test for UCRA Streaming API - Internal Buffering and Callback Logic
 * Tests the refill_stream_buffer and UCRA_PullPCM callback integration
 */

#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

/* Test data for callback */
typedef struct {
    int call_count;
    UCRA_NoteSegment* notes;
    uint32_t note_count;
    int should_fail;
} TestCallbackData;

/* Mock callback that provides note data */
static UCRA_Result test_pull_pcm_with_notes(void* user_data, UCRA_RenderConfig* out_config) {
    TestCallbackData* test_data = (TestCallbackData*)user_data;
    test_data->call_count++;

    if (test_data->should_fail) {
        return UCRA_ERR_INTERNAL;
    }

    /* Provide the notes to render */
    out_config->notes = test_data->notes;
    out_config->note_count = test_data->note_count;

    return UCRA_SUCCESS;
}

/* Mock callback that provides no notes (silence) */
static UCRA_Result test_pull_pcm_silence(void* user_data, UCRA_RenderConfig* out_config) {
    TestCallbackData* test_data = (TestCallbackData*)user_data;
    test_data->call_count++;

    /* Provide no notes */
    out_config->notes = NULL;
    out_config->note_count = 0;

    return UCRA_SUCCESS;
}

/* Test basic buffering with silence */
static void test_buffering_silence() {
    printf("Testing basic buffering with silence...\n");

    UCRA_RenderConfig config = {
        .sample_rate = 44100,
        .channels = 2,
        .block_size = 512,
        .flags = 0,
        .notes = NULL,
        .note_count = 0,
        .options = NULL,
        .option_count = 0
    };

    TestCallbackData test_data = {
        .call_count = 0,
        .notes = NULL,
        .note_count = 0,
        .should_fail = 0
    };

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result = ucra_stream_open(&stream, &config, test_pull_pcm_silence, &test_data);
    assert(result == UCRA_SUCCESS);
    assert(stream != NULL);

    /* Read some data to trigger buffering */
    float buffer[1024 * 2]; /* 1024 frames, 2 channels */
    uint32_t frames_read = 0;

    result = ucra_stream_read(stream, buffer, 1024, &frames_read);
    assert(result == UCRA_SUCCESS);
    assert(frames_read == 1024);
    assert(test_data.call_count > 0); /* Callback should have been called */

    /* Verify that the data is silence */
    for (uint32_t i = 0; i < frames_read * config.channels; i++) {
        assert(buffer[i] == 0.0f);
    }

    ucra_stream_close(stream);
    printf("✓ Basic buffering with silence test passed\n");
}

/* Test buffering with actual note data */
static void test_buffering_with_notes() {
    printf("Testing buffering with note data...\n");

    UCRA_RenderConfig config = {
        .sample_rate = 44100,
        .channels = 1,
        .block_size = 256,
        .flags = 0,
        .notes = NULL,
        .note_count = 0,
        .options = NULL,
        .option_count = 0
    };

    /* Create a test note (A4 = 440Hz) */
    UCRA_NoteSegment test_note = {
        .start_sec = 0.0,
        .duration_sec = 1.0,
        .midi_note = 69, /* A4 */
        .velocity = 100,
        .lyric = "test",
        .f0_override = NULL,
        .env_override = NULL
    };

    TestCallbackData test_data = {
        .call_count = 0,
        .notes = &test_note,
        .note_count = 1,
        .should_fail = 0
    };

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result = ucra_stream_open(&stream, &config, test_pull_pcm_with_notes, &test_data);
    assert(result == UCRA_SUCCESS);
    assert(stream != NULL);

    /* Read some data to trigger buffering */
    float buffer[512]; /* 512 frames, 1 channel */
    uint32_t frames_read = 0;

    result = ucra_stream_read(stream, buffer, 512, &frames_read);
    assert(result == UCRA_SUCCESS);
    assert(frames_read == 512);
    assert(test_data.call_count > 0); /* Callback should have been called */

    /* Verify that the data is not silence (should contain sine wave) */
    int has_non_zero = 0;
    for (uint32_t i = 0; i < frames_read; i++) {
        if (fabs(buffer[i]) > 0.001f) {
            has_non_zero = 1;
            break;
        }
    }
    assert(has_non_zero); /* Should have generated audio */

    ucra_stream_close(stream);
    printf("✓ Buffering with note data test passed\n");
}

/* Test callback error handling */
static void test_callback_error_handling() {
    printf("Testing callback error handling...\n");

    UCRA_RenderConfig config = {
        .sample_rate = 44100,
        .channels = 2,
        .block_size = 512,
        .flags = 0,
        .notes = NULL,
        .note_count = 0,
        .options = NULL,
        .option_count = 0
    };

    TestCallbackData test_data = {
        .call_count = 0,
        .notes = NULL,
        .note_count = 0,
        .should_fail = 1  /* Force callback to return error */
    };

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result = ucra_stream_open(&stream, &config, test_pull_pcm_with_notes, &test_data);
    assert(result == UCRA_SUCCESS);
    assert(stream != NULL);

    /* Try to read data - should fail due to callback error */
    float buffer[1024 * 2];
    uint32_t frames_read = 0;

    result = ucra_stream_read(stream, buffer, 1024, &frames_read);
    assert(result != UCRA_SUCCESS); /* Should fail */
    assert(test_data.call_count > 0); /* Callback should have been called */

    ucra_stream_close(stream);
    printf("✓ Callback error handling test passed\n");
}

/* Test multiple read operations */
static void test_multiple_reads() {
    printf("Testing multiple read operations...\n");

    UCRA_RenderConfig config = {
        .sample_rate = 44100,
        .channels = 2,
        .block_size = 256,
        .flags = 0,
        .notes = NULL,
        .note_count = 0,
        .options = NULL,
        .option_count = 0
    };

    TestCallbackData test_data = {
        .call_count = 0,
        .notes = NULL,
        .note_count = 0,
        .should_fail = 0
    };

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result = ucra_stream_open(&stream, &config, test_pull_pcm_silence, &test_data);
    assert(result == UCRA_SUCCESS);

    /* Perform multiple reads */
    for (int i = 0; i < 5; i++) {
        float buffer[128 * 2]; /* 128 frames, 2 channels */
        uint32_t frames_read = 0;

        result = ucra_stream_read(stream, buffer, 128, &frames_read);
        assert(result == UCRA_SUCCESS);
        assert(frames_read == 128);
    }

    assert(test_data.call_count > 0); /* Should have called callback multiple times */

    ucra_stream_close(stream);
    printf("✓ Multiple read operations test passed\n");
}

int main() {
    printf("=== UCRA Streaming API Buffering Tests ===\n\n");

    test_buffering_silence();
    test_buffering_with_notes();
    test_callback_error_handling();
    test_multiple_reads();

    printf("\n=== All buffering tests passed! ===\n");
    return 0;
}
