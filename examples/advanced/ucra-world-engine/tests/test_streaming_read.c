/*
 * Test for UCRA Streaming API - ucra_stream_read Function
 * Tests various edge cases and scenarios for the stream read function
 */

#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

/* Platform-specific includes */
#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
#endif

/* Test data for callback */
typedef struct {
    int call_count;
    UCRA_NoteSegment* notes;
    uint32_t note_count;
    int should_fail_after;
} TestReadCallbackData;

/* Mock callback for testing reads */
static UCRA_Result test_read_pull_pcm(void* user_data, UCRA_RenderConfig* out_config) {
    TestReadCallbackData* test_data = (TestReadCallbackData*)user_data;
    test_data->call_count++;

    if (test_data->should_fail_after > 0 &&
        test_data->call_count > test_data->should_fail_after) {
        return UCRA_ERR_INTERNAL;
    }

    /* Provide notes if available */
    out_config->notes = test_data->notes;
    out_config->note_count = test_data->note_count;

    return UCRA_SUCCESS;
}

/* Test reading various block sizes */
static void test_read_various_block_sizes() {
    printf("Testing read with various block sizes...\n");

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

    TestReadCallbackData test_data = {
        .call_count = 0,
        .notes = NULL,
        .note_count = 0,
        .should_fail_after = 0
    };

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result = ucra_stream_open(&stream, &config, test_read_pull_pcm, &test_data);
    assert(result == UCRA_SUCCESS);

    /* Test different read sizes */
    uint32_t test_sizes[] = {1, 32, 256, 512, 1024, 2048, 4096};
    size_t num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);

    for (size_t i = 0; i < num_tests; i++) {
        uint32_t frames_to_read = test_sizes[i];
        float* buffer = malloc(frames_to_read * config.channels * sizeof(float));
        assert(buffer != NULL);

        uint32_t frames_read = 0;
        result = ucra_stream_read(stream, buffer, frames_to_read, &frames_read);
        assert(result == UCRA_SUCCESS);
        assert(frames_read == frames_to_read);

        free(buffer);
    }

    ucra_stream_close(stream);
    printf("✓ Various block sizes test passed\n");
}

/* Test reading larger than buffer size */
static void test_read_larger_than_buffer() {
    printf("Testing read larger than internal buffer...\n");

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

    TestReadCallbackData test_data = {
        .call_count = 0,
        .notes = NULL,
        .note_count = 0,
        .should_fail_after = 0
    };

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result = ucra_stream_open(&stream, &config, test_read_pull_pcm, &test_data);
    assert(result == UCRA_SUCCESS);

    /* Try to read 8192 frames (larger than typical buffer size) */
    uint32_t large_size = 8192;
    float* buffer = malloc(large_size * config.channels * sizeof(float));
    assert(buffer != NULL);

    uint32_t frames_read = 0;
    result = ucra_stream_read(stream, buffer, large_size, &frames_read);
    assert(result == UCRA_SUCCESS);
    assert(frames_read == large_size);
    assert(test_data.call_count > 1); /* Should have called callback multiple times */

    free(buffer);
    ucra_stream_close(stream);
    printf("✓ Large read test passed\n");
}

/* Test reading with NULL parameters */
static void test_read_error_cases() {
    printf("Testing read error cases...\n");

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

    TestReadCallbackData test_data = {
        .call_count = 0,
        .notes = NULL,
        .note_count = 0,
        .should_fail_after = 0
    };

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result = ucra_stream_open(&stream, &config, test_read_pull_pcm, &test_data);
    assert(result == UCRA_SUCCESS);

    float buffer[1024];
    uint32_t frames_read = 0;

    /* Test NULL stream */
    result = ucra_stream_read(NULL, buffer, 512, &frames_read);
    assert(result == UCRA_ERR_INVALID_ARGUMENT);

    /* Test NULL buffer */
    result = ucra_stream_read(stream, NULL, 512, &frames_read);
    assert(result == UCRA_ERR_INVALID_ARGUMENT);

    /* Test NULL frames_read */
    result = ucra_stream_read(stream, buffer, 512, NULL);
    assert(result == UCRA_ERR_INVALID_ARGUMENT);

    ucra_stream_close(stream);
    printf("✓ Read error cases test passed\n");
}

/* Test reading from closed stream */
static void test_read_from_closed_stream() {
    printf("Testing read from closed stream...\n");

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

    TestReadCallbackData test_data = {
        .call_count = 0,
        .notes = NULL,
        .note_count = 0,
        .should_fail_after = 0
    };

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result = ucra_stream_open(&stream, &config, test_read_pull_pcm, &test_data);
    assert(result == UCRA_SUCCESS);

    /* Close the stream */
    ucra_stream_close(stream);

    /* Try to read from closed stream - this should not crash but may return error */
    float buffer[1024];
    uint32_t frames_read = 0;

    /* Note: Reading from closed stream behavior is implementation-defined */
    /* We mainly want to ensure it doesn't crash */
    result = ucra_stream_read(stream, buffer, 512, &frames_read);
    /* Result may be error or success with 0 frames - both are acceptable */

    printf("✓ Read from closed stream test passed (no crash)\n");
}

/* Test continuous reading */
static void test_continuous_reading() {
    printf("Testing continuous reading...\n");

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

    TestReadCallbackData test_data = {
        .call_count = 0,
        .notes = NULL,
        .note_count = 0,
        .should_fail_after = 0
    };

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result = ucra_stream_open(&stream, &config, test_read_pull_pcm, &test_data);
    assert(result == UCRA_SUCCESS);

    /* Read continuously for a while */
    uint32_t total_frames_read = 0;
    for (int i = 0; i < 50; i++) {
        float buffer[128 * 2];
        uint32_t frames_read = 0;

        result = ucra_stream_read(stream, buffer, 128, &frames_read);
        assert(result == UCRA_SUCCESS);
        assert(frames_read == 128);

        total_frames_read += frames_read;
    }

    assert(total_frames_read == 50 * 128);
    assert(test_data.call_count > 0);

    ucra_stream_close(stream);
    printf("✓ Continuous reading test passed\n");
}

/* Test zero-frame read */
static void test_zero_frame_read() {
    printf("Testing zero-frame read...\n");

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

    TestReadCallbackData test_data = {
        .call_count = 0,
        .notes = NULL,
        .note_count = 0,
        .should_fail_after = 0
    };

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result = ucra_stream_open(&stream, &config, test_read_pull_pcm, &test_data);
    assert(result == UCRA_SUCCESS);

    float buffer[1024];
    uint32_t frames_read = 0;

    /* Try to read 0 frames */
    result = ucra_stream_read(stream, buffer, 0, &frames_read);
    assert(result == UCRA_SUCCESS);
    assert(frames_read == 0);

    ucra_stream_close(stream);
    printf("✓ Zero-frame read test passed\n");
}

int main() {
    printf("=== UCRA Streaming API Read Function Tests ===\n\n");

    test_read_various_block_sizes();
    test_read_larger_than_buffer();
    test_read_error_cases();
    test_read_from_closed_stream();
    test_continuous_reading();
    test_zero_frame_read();

    printf("\n=== All read function tests passed! ===\n");
    return 0;
}
