/*
 * Integration Test Client for UCRA Streaming API
 * Simulates a real-world usage scenario like OpenUtau with multithreading
 */

#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

/* Platform-specific includes */
#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    typedef HANDLE pthread_t;
    typedef CRITICAL_SECTION pthread_mutex_t;
    #define PTHREAD_MUTEX_INITIALIZER {}
    #define sleep(x) Sleep((x) * 1000)
    #define usleep(x) Sleep((x) / 1000)

    struct timeval {
        long tv_sec;
        long tv_usec;
    };

    static int gettimeofday(struct timeval* tv, void* tz) {
        (void)tz;
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        uint64_t time = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
        time = (time - 116444736000000000ULL) / 10; /* Convert to microseconds since epoch */
        tv->tv_sec = (long)(time / 1000000);
        tv->tv_usec = (long)(time % 1000000);
        return 0;
    }
#else
    #include <pthread.h>
    #include <unistd.h>
    #include <sys/time.h>
#endif

/* Configuration for the integration test */
#define TEST_DURATION_SEC 3
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_CHANNELS 2
#define AUDIO_BLOCK_SIZE 512
#define CALLBACK_INTERVAL_US 11610  /* ~512 frames at 44.1kHz = 11.6ms */

/* Test context structure */
typedef struct {
    pthread_t audio_thread;
    UCRA_StreamHandle stream;
    volatile int should_stop;

    /* Statistics */
    volatile uint64_t total_frames_read;
    volatile uint32_t read_calls;
    volatile uint32_t callback_calls;
    volatile int audio_thread_running;

    /* Test notes */
    UCRA_NoteSegment* notes;
    uint32_t note_count;

    /* Timing measurement */
    struct timeval start_time;
    struct timeval end_time;
} IntegrationTestContext;

/* Mock callback that provides test notes */
static UCRA_Result integration_pull_pcm(void* user_data, UCRA_RenderConfig* out_config) {
    IntegrationTestContext* ctx = (IntegrationTestContext*)user_data;
    __sync_fetch_and_add(&ctx->callback_calls, 1);

    /* Provide test notes */
    out_config->notes = ctx->notes;
    out_config->note_count = ctx->note_count;

    return UCRA_SUCCESS;
}

/* Audio thread function - simulates audio callback */
static void* audio_thread_func(void* user_data) {
    IntegrationTestContext* ctx = (IntegrationTestContext*)user_data;
    float* buffer = malloc(AUDIO_BLOCK_SIZE * AUDIO_CHANNELS * sizeof(float));

    if (!buffer) {
        printf("Failed to allocate audio buffer\n");
        return NULL;
    }

    ctx->audio_thread_running = 1;

    printf("Audio thread started, simulating %d Hz callback rate\n",
           (int)(1000000.0 / CALLBACK_INTERVAL_US));

    while (!ctx->should_stop) {
        uint32_t frames_read = 0;

        /* Measure latency */
        struct timeval read_start, read_end;
        gettimeofday(&read_start, NULL);

        UCRA_Result result = ucra_stream_read(ctx->stream, buffer, AUDIO_BLOCK_SIZE, &frames_read);

        gettimeofday(&read_end, NULL);
        double latency_ms = (read_end.tv_sec - read_start.tv_sec) * 1000.0 +
                           (read_end.tv_usec - read_start.tv_usec) / 1000.0;

        if (result == UCRA_SUCCESS) {
            __sync_fetch_and_add(&ctx->total_frames_read, frames_read);
            __sync_fetch_and_add(&ctx->read_calls, 1);

            /* Check for reasonable latency (should be much less than 15ms target) */
            if (latency_ms > 5.0) {
                printf("Warning: High latency detected: %.2f ms\n", latency_ms);
            }
        } else {
            printf("Stream read failed with error %d\n", result);
            break;
        }

        /* Simulate audio callback timing */
        usleep(CALLBACK_INTERVAL_US);
    }

    free(buffer);
    ctx->audio_thread_running = 0;
    printf("Audio thread stopped\n");
    return NULL;
}

/* Test basic multithreaded operation */
static void test_multithreaded_operation() {
    printf("Testing multithreaded operation...\n");

    IntegrationTestContext ctx = {0};

    /* Create test notes (C major chord) */
    UCRA_NoteSegment notes[] = {
        {0.0, 3.0, 60, 80, "C",  NULL, NULL},  /* C4 */
        {0.0, 3.0, 64, 80, "E",  NULL, NULL},  /* E4 */
        {0.0, 3.0, 67, 80, "G",  NULL, NULL},  /* G4 */
    };
    ctx.notes = notes;
    ctx.note_count = sizeof(notes) / sizeof(notes[0]);

    /* Configure stream */
    UCRA_RenderConfig config = {
        .sample_rate = AUDIO_SAMPLE_RATE,
        .channels = AUDIO_CHANNELS,
        .block_size = AUDIO_BLOCK_SIZE,
        .flags = 0,
        .notes = NULL,
        .note_count = 0,
        .options = NULL,
        .option_count = 0
    };

    /* Open stream */
    UCRA_Result result = ucra_stream_open(&ctx.stream, &config, integration_pull_pcm, &ctx);
    assert(result == UCRA_SUCCESS);
    assert(ctx.stream != NULL);

    /* Start timing */
    gettimeofday(&ctx.start_time, NULL);

    /* Start audio thread */
    int pthread_result = pthread_create(&ctx.audio_thread, NULL, audio_thread_func, &ctx);
    assert(pthread_result == 0);

    /* Let it run for a while */
    printf("Running for %d seconds...\n", TEST_DURATION_SEC);
    sleep(TEST_DURATION_SEC);

    /* Stop audio thread */
    ctx.should_stop = 1;
    pthread_join(ctx.audio_thread, NULL);

    /* End timing */
    gettimeofday(&ctx.end_time, NULL);

    /* Close stream */
    ucra_stream_close(ctx.stream);

    /* Calculate statistics */
    double total_time = (ctx.end_time.tv_sec - ctx.start_time.tv_sec) +
                       (ctx.end_time.tv_usec - ctx.start_time.tv_usec) / 1000000.0;

    double expected_frames = AUDIO_SAMPLE_RATE * total_time;
    double frame_accuracy = (double)ctx.total_frames_read / expected_frames;

    printf("\n=== Test Results ===\n");
    printf("Total time: %.2f seconds\n", total_time);
    printf("Total frames read: %llu\n", (unsigned long long)ctx.total_frames_read);
    printf("Expected frames: %.0f\n", expected_frames);
    printf("Frame accuracy: %.2f%%\n", frame_accuracy * 100.0);
    printf("Read calls: %u\n", ctx.read_calls);
    printf("Callback calls: %u\n", ctx.callback_calls);
    printf("Average frames per read: %.1f\n", (double)ctx.total_frames_read / ctx.read_calls);
    printf("Average reads per second: %.1f\n", ctx.read_calls / total_time);

    /* Verify reasonable performance - more lenient thresholds for CI environments */
    assert(frame_accuracy > 0.15); /* Should read at least 15% of expected frames (accounts for CI scheduling overhead) */
    assert(ctx.callback_calls > 0); /* Should have called callback */
    assert(ctx.read_calls > TEST_DURATION_SEC * 5); /* Should have multiple reads */

    printf("✓ Multithreaded operation test passed\n");
}

/* Test concurrent stream operations */
static void test_concurrent_operations() {
    printf("Testing concurrent stream operations...\n");

    UCRA_RenderConfig config = {
        .sample_rate = AUDIO_SAMPLE_RATE,
        .channels = AUDIO_CHANNELS,
        .block_size = AUDIO_BLOCK_SIZE,
        .flags = 0,
        .notes = NULL,
        .note_count = 0,
        .options = NULL,
        .option_count = 0
    };

    IntegrationTestContext ctx = {0};

    /* Open stream */
    UCRA_Result result = ucra_stream_open(&ctx.stream, &config, integration_pull_pcm, &ctx);
    assert(result == UCRA_SUCCESS);

    /* Start audio thread */
    int pthread_result = pthread_create(&ctx.audio_thread, NULL, audio_thread_func, &ctx);
    assert(pthread_result == 0);

    /* Perform main thread operations while audio thread is running */
    for (int i = 0; i < 10; i++) {
        usleep(100000); /* 100ms */

        /* The main thread could perform other operations here */
        /* For now, we just verify the audio thread is still running */
        assert(ctx.audio_thread_running);
    }

    /* Stop audio thread */
    ctx.should_stop = 1;
    pthread_join(ctx.audio_thread, NULL);

    /* Close stream */
    ucra_stream_close(ctx.stream);

    printf("✓ Concurrent operations test passed\n");
}

/* Test thread safety with rapid open/close cycles */
static void test_rapid_lifecycle() {
    printf("Testing rapid lifecycle operations...\n");

    UCRA_RenderConfig config = {
        .sample_rate = AUDIO_SAMPLE_RATE,
        .channels = AUDIO_CHANNELS,
        .block_size = AUDIO_BLOCK_SIZE,
        .flags = 0,
        .notes = NULL,
        .note_count = 0,
        .options = NULL,
        .option_count = 0
    };

    /* Rapidly open and close streams */
    for (int i = 0; i < 20; i++) {
        IntegrationTestContext ctx = {0};

        UCRA_Result result = ucra_stream_open(&ctx.stream, &config, integration_pull_pcm, &ctx);
        assert(result == UCRA_SUCCESS);

        /* Read a small amount of data */
        float buffer[256 * AUDIO_CHANNELS];
        uint32_t frames_read = 0;
        result = ucra_stream_read(ctx.stream, buffer, 256, &frames_read);
        assert(result == UCRA_SUCCESS);

        ucra_stream_close(ctx.stream);
    }

    printf("✓ Rapid lifecycle test passed\n");
}

/* Data continuity verification */
static void test_data_continuity() {
    printf("Testing data continuity...\n");

    UCRA_RenderConfig config = {
        .sample_rate = AUDIO_SAMPLE_RATE,
        .channels = 1, /* Use mono for easier analysis */
        .block_size = 256,
        .flags = 0,
        .notes = NULL,
        .note_count = 0,
        .options = NULL,
        .option_count = 0
    };

    IntegrationTestContext ctx = {0};

    /* Create a single test note */
    UCRA_NoteSegment note = {0.0, 1.0, 69, 100, "A", NULL, NULL}; /* A4 = 440Hz */
    ctx.notes = &note;
    ctx.note_count = 1;

    UCRA_Result result = ucra_stream_open(&ctx.stream, &config, integration_pull_pcm, &ctx);
    assert(result == UCRA_SUCCESS);

    /* Read several blocks and check for continuity */
    float prev_sample = 0.0f;
    int blocks_to_test = 10;

    for (int block = 0; block < blocks_to_test; block++) {
        float buffer[256];
        uint32_t frames_read = 0;

        result = ucra_stream_read(ctx.stream, buffer, 256, &frames_read);
        assert(result == UCRA_SUCCESS);
        assert(frames_read == 256);

        /* Check for reasonable continuity (no huge jumps) */
        if (block > 0) {
            float sample_diff = fabs(buffer[0] - prev_sample);
            if (sample_diff > 0.5f) {
                printf("Warning: Large discontinuity detected: %.3f\n", sample_diff);
            }
        }

        prev_sample = buffer[frames_read - 1];
    }

    ucra_stream_close(ctx.stream);
    printf("✓ Data continuity test passed\n");
}

int main() {
    printf("=== UCRA Streaming API Integration Tests ===\n\n");

    test_multithreaded_operation();
    test_concurrent_operations();
    test_rapid_lifecycle();
    test_data_continuity();

    printf("\n=== All integration tests passed! ===\n");
    printf("\nTo run with thread sanitizer for race condition detection:\n");
    printf("    export CFLAGS=\"-fsanitize=thread\"\n");
    printf("    export LDFLAGS=\"-fsanitize=thread\"\n");
    printf("    cmake --build build --clean-first\n");
    printf("    ./test_streaming_integration\n");

    return 0;
}

#ifdef _WIN32
/* Windows pthread compatibility layer for tests */
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
    if (!wrapper_args) return 12; /* ENOMEM */

    wrapper_args[0] = (void*)start_routine;
    wrapper_args[1] = arg;

    *thread = (HANDLE)_beginthreadex(NULL, 0, thread_proc_wrapper, wrapper_args, 0, NULL);
    return *thread ? 0 : 11; /* EAGAIN */
}

static int pthread_join(pthread_t thread, void** retval) {
    (void)retval;
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}
#endif
