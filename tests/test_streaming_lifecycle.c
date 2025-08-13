/*
 * Test for UCRA Streaming API - Stream Lifecycle Functions
 * Tests ucra_stream_open and ucra_stream_close functions
 */

#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Mock callback function for testing */
static UCRA_Result mock_pull_pcm(void* user_data, UCRA_RenderConfig* out_config) {
    (void)user_data;  /* Unused */
    (void)out_config; /* Unused for this test */
    return UCRA_SUCCESS;
}

/* Test basic stream open and close */
static void test_stream_open_close_basic() {
    printf("Testing basic stream open/close...\n");

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

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result;

    /* Test successful open */
    result = ucra_stream_open(&stream, &config, mock_pull_pcm, NULL);
    assert(result == UCRA_SUCCESS);
    assert(stream != NULL);

    /* Test close */
    ucra_stream_close(stream);

    printf("✓ Basic stream open/close test passed\n");
}

/* Test error cases */
static void test_stream_open_error_cases() {
    printf("Testing stream open error cases...\n");

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

    UCRA_StreamHandle stream = NULL;
    UCRA_Result result;

    /* Test NULL out_stream */
    result = ucra_stream_open(NULL, &config, mock_pull_pcm, NULL);
    assert(result == UCRA_ERR_INVALID_ARGUMENT);

    /* Test NULL config */
    result = ucra_stream_open(&stream, NULL, mock_pull_pcm, NULL);
    assert(result == UCRA_ERR_INVALID_ARGUMENT);

    /* Test NULL callback */
    result = ucra_stream_open(&stream, &config, NULL, NULL);
    assert(result == UCRA_ERR_INVALID_ARGUMENT);

    /* Test invalid sample rate */
    UCRA_RenderConfig invalid_config = config;
    invalid_config.sample_rate = 0;
    result = ucra_stream_open(&stream, &invalid_config, mock_pull_pcm, NULL);
    assert(result == UCRA_ERR_INVALID_ARGUMENT);

    /* Test invalid channels */
    invalid_config = config;
    invalid_config.channels = 0;
    result = ucra_stream_open(&stream, &invalid_config, mock_pull_pcm, NULL);
    assert(result == UCRA_ERR_INVALID_ARGUMENT);

    /* Test invalid block size */
    invalid_config = config;
    invalid_config.block_size = 0;
    result = ucra_stream_open(&stream, &invalid_config, mock_pull_pcm, NULL);
    assert(result == UCRA_ERR_INVALID_ARGUMENT);

    printf("✓ Stream open error cases test passed\n");
}

/* Test multiple open/close cycles */
static void test_multiple_stream_cycles() {
    printf("Testing multiple stream open/close cycles...\n");

    UCRA_RenderConfig config = {
        .sample_rate = 48000,
        .channels = 1,
        .block_size = 256,
        .flags = 0,
        .notes = NULL,
        .note_count = 0,
        .options = NULL,
        .option_count = 0
    };

    for (int i = 0; i < 10; i++) {
        UCRA_StreamHandle stream = NULL;
        UCRA_Result result = ucra_stream_open(&stream, &config, mock_pull_pcm, NULL);
        assert(result == UCRA_SUCCESS);
        assert(stream != NULL);

        ucra_stream_close(stream);
    }

    printf("✓ Multiple stream cycles test passed\n");
}

/* Test close with NULL handle */
static void test_stream_close_null() {
    printf("Testing stream close with NULL handle...\n");

    /* Should not crash */
    ucra_stream_close(NULL);

    printf("✓ Stream close NULL test passed\n");
}

int main() {
    printf("=== UCRA Streaming API Lifecycle Tests ===\n\n");

    test_stream_open_close_basic();
    test_stream_open_error_cases();
    test_multiple_stream_cycles();
    test_stream_close_null();

    printf("\n=== All lifecycle tests passed! ===\n");
    return 0;
}
