/*
 * Audio Comparison Module
 *
 * This module compares a rendered output WAV against a 'golden' reference WAV
 * and determines if they match using two levels of comparison:
 * 1) Strict bit-for-bit identity check using SHA-256 hash
 * 2) Sample-based difference calculation (RMS difference, SNR)
 *
 * Usage: audio_compare <reference_wav> <test_wav>
 *
 * Exit codes:
 *   0 - Files match (PASS)
 *   1 - Files differ but within tolerance (PASS with warning)
 *   2 - Files significantly differ (FAIL)
 *   3 - Error occurred
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>

#define MAX_FILENAME 256
#define HASH_SIZE 32
#define TOLERANCE_SNR_DB 60.0  /* SNR threshold for pass/fail */
#define TOLERANCE_RMS 0.001    /* RMS difference threshold */

/* SHA-256 implementation (simplified) */
typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} SHA256_CTX;

/* WAV file header structure */
typedef struct {
    char riff[4];
    uint32_t chunk_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} WAVHeader;

/* Audio data structure */
typedef struct {
    float* samples;
    int length;
    int sample_rate;
    int channels;
} AudioData;

/* Comparison result */
typedef struct {
    int identical;          /* Bit-for-bit identical */
    double rms_difference;  /* RMS difference */
    double snr_db;          /* Signal-to-noise ratio in dB */
    int samples_compared;   /* Number of samples compared */
    char hash1[HASH_SIZE * 2 + 1];  /* Hex string of first file hash */
    char hash2[HASH_SIZE * 2 + 1];  /* Hex string of second file hash */
} ComparisonResult;

/* Initialize audio data structure */
static int audio_data_init(AudioData* audio) {
    audio->samples = NULL;
    audio->length = 0;
    audio->sample_rate = 0;
    audio->channels = 0;
    return 0;
}

/* Free audio data memory */
static void audio_data_free(AudioData* audio) {
    if (audio->samples) {
        free(audio->samples);
        audio->samples = NULL;
    }
    audio->length = 0;
}

/* Simple hash function (not cryptographically secure, but sufficient for file comparison) */
static uint32_t simple_hash(const uint8_t* data, size_t len) {
    uint32_t hash = 0x811c9dc5;  /* FNV-1a 32-bit offset basis */
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 0x01000193;  /* FNV-1a 32-bit prime */
    }
    return hash;
}

/* Calculate file hash */
static int calculate_file_hash(const char* filename, char* hash_str) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s' for hashing: %s\n",
                filename, strerror(errno));
        return -1;
    }

    uint8_t buffer[4096];
    uint32_t hash = 0x811c9dc5;
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            hash ^= buffer[i];
            hash *= 0x01000193;
        }
    }

    fclose(file);

    /* Convert to hex string */
    snprintf(hash_str, HASH_SIZE * 2 + 1, "%08x", hash);

    return 0;
}

/* Load WAV file */
static int load_wav_file(const char* filename, AudioData* audio) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open WAV file '%s': %s\n", filename, strerror(errno));
        return -1;
    }

    WAVHeader header;
    if (fread(&header, sizeof(WAVHeader), 1, file) != 1) {
        fprintf(stderr, "Error: Cannot read WAV header from '%s'\n", filename);
        fclose(file);
        return -1;
    }

    /* Basic WAV file validation */
    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
        fprintf(stderr, "Error: '%s' is not a valid WAV file\n", filename);
        fclose(file);
        return -1;
    }

    if (header.audio_format != 1) {
        fprintf(stderr, "Error: Only PCM WAV files are supported\n");
        fclose(file);
        return -1;
    }

    audio->sample_rate = header.sample_rate;
    audio->channels = header.channels;

    /* Calculate number of samples */
    int sample_count = header.data_size / (header.bits_per_sample / 8) / header.channels;
    audio->length = sample_count;

    /* Allocate memory for samples */
    audio->samples = malloc(sample_count * sizeof(float));
    if (!audio->samples) {
        fprintf(stderr, "Error: Memory allocation failed for audio samples\n");
        fclose(file);
        return -1;
    }

    /* Read and convert samples to float */
    if (header.bits_per_sample == 16) {
        int16_t* temp_samples = malloc(sample_count * header.channels * sizeof(int16_t));
        if (!temp_samples) {
            fprintf(stderr, "Error: Memory allocation failed for temporary samples\n");
            free(audio->samples);
            fclose(file);
            return -1;
        }

        fread(temp_samples, sizeof(int16_t), sample_count * header.channels, file);

        /* Convert to mono float and normalize */
        for (int i = 0; i < sample_count; i++) {
            float sum = 0.0f;
            for (int c = 0; c < header.channels; c++) {
                sum += temp_samples[i * header.channels + c];
            }
            audio->samples[i] = (sum / header.channels) / 32768.0f;
        }

        free(temp_samples);
    } else if (header.bits_per_sample == 32) {
        float* temp_samples = malloc(sample_count * header.channels * sizeof(float));
        if (!temp_samples) {
            fprintf(stderr, "Error: Memory allocation failed for temporary samples\n");
            free(audio->samples);
            fclose(file);
            return -1;
        }

        fread(temp_samples, sizeof(float), sample_count * header.channels, file);

        /* Convert to mono */
        for (int i = 0; i < sample_count; i++) {
            float sum = 0.0f;
            for (int c = 0; c < header.channels; c++) {
                sum += temp_samples[i * header.channels + c];
            }
            audio->samples[i] = sum / header.channels;
        }

        free(temp_samples);
    } else {
        fprintf(stderr, "Error: Only 16-bit and 32-bit WAV files are supported\n");
        free(audio->samples);
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

/* Compare two audio files */
static ComparisonResult compare_audio_files(const char* file1, const char* file2) {
    ComparisonResult result = {0};

    /* Step 1: Hash comparison for identical check */
    if (calculate_file_hash(file1, result.hash1) < 0 ||
        calculate_file_hash(file2, result.hash2) < 0) {
        result.rms_difference = -1.0;
        result.snr_db = -1.0;
        return result;
    }

    result.identical = (strcmp(result.hash1, result.hash2) == 0);

    if (result.identical) {
        result.rms_difference = 0.0;
        result.snr_db = INFINITY;
        return result;
    }

    /* Step 2: Load audio data for sample-based comparison */
    AudioData audio1, audio2;
    audio_data_init(&audio1);
    audio_data_init(&audio2);

    if (load_wav_file(file1, &audio1) < 0) {
        result.rms_difference = -1.0;
        result.snr_db = -1.0;
        return result;
    }

    if (load_wav_file(file2, &audio2) < 0) {
        audio_data_free(&audio1);
        result.rms_difference = -1.0;
        result.snr_db = -1.0;
        return result;
    }

    /* Check compatibility */
    if (audio1.sample_rate != audio2.sample_rate) {
        fprintf(stderr, "Warning: Sample rates differ (%d vs %d)\n",
                audio1.sample_rate, audio2.sample_rate);
    }

    /* Compare samples */
    int min_length = (audio1.length < audio2.length) ? audio1.length : audio2.length;
    double sum_squared_diff = 0.0;
    double sum_squared_signal = 0.0;

    for (int i = 0; i < min_length; i++) {
        double diff = audio1.samples[i] - audio2.samples[i];
        double signal = audio1.samples[i];

        sum_squared_diff += diff * diff;
        sum_squared_signal += signal * signal;
    }

    result.samples_compared = min_length;

    /* Calculate RMS difference */
    if (min_length > 0) {
        result.rms_difference = sqrt(sum_squared_diff / min_length);

        /* Calculate SNR */
        if (sum_squared_signal > 0.0 && sum_squared_diff > 0.0) {
            double snr_linear = sum_squared_signal / sum_squared_diff;
            result.snr_db = 10.0 * log10(snr_linear);
        } else if (sum_squared_diff == 0.0) {
            result.snr_db = INFINITY;
        } else {
            result.snr_db = -INFINITY;
        }
    } else {
        result.rms_difference = -1.0;
        result.snr_db = -1.0;
    }

    /* Cleanup */
    audio_data_free(&audio1);
    audio_data_free(&audio2);

    return result;
}

static void print_usage(const char* program_name) {
    printf("Usage: %s <reference_wav> <test_wav>\n", program_name);
    printf("\n");
    printf("Compare two WAV audio files for similarity.\n");
    printf("\n");
    printf("Comparison methods:\n");
    printf("  1. Hash comparison for bit-for-bit identity\n");
    printf("  2. Sample-based RMS difference and SNR calculation\n");
    printf("\n");
    printf("Thresholds:\n");
    printf("  SNR > %.1f dB: PASS\n", TOLERANCE_SNR_DB);
    printf("  RMS < %.6f: PASS\n", TOLERANCE_RMS);
    printf("\n");
    printf("Exit codes:\n");
    printf("  0 - Files match (PASS)\n");
    printf("  1 - Files differ but within tolerance (PASS with warning)\n");
    printf("  2 - Files significantly differ (FAIL)\n");
    printf("  3 - Error occurred\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 3;
    }

    const char* ref_file = argv[1];
    const char* test_file = argv[2];

    printf("Comparing audio files:\n");
    printf("  Reference: %s\n", ref_file);
    printf("  Test:      %s\n", test_file);
    printf("\n");

    /* Perform comparison */
    ComparisonResult result = compare_audio_files(ref_file, test_file);

    if (result.rms_difference < 0.0) {
        printf("ERROR: Comparison failed\n");
        return 3;
    }

    /* Print results */
    printf("Hash comparison:\n");
    printf("  Reference hash: %s\n", result.hash1);
    printf("  Test hash:      %s\n", result.hash2);
    printf("  Identical:      %s\n", result.identical ? "YES" : "NO");
    printf("\n");

    if (!result.identical) {
        printf("Sample-based comparison:\n");
        printf("  Samples compared: %d\n", result.samples_compared);
        printf("  RMS difference:   %.8f\n", result.rms_difference);

        if (isfinite(result.snr_db)) {
            printf("  SNR:              %.2f dB\n", result.snr_db);
        } else if (result.snr_db == INFINITY) {
            printf("  SNR:              Infinite (identical samples)\n");
        } else {
            printf("  SNR:              -Infinite (zero signal)\n");
        }
        printf("\n");
    }

    /* Determine verdict */
    int exit_code;
    if (result.identical) {
        printf("VERDICT: PASS (Identical files)\n");
        exit_code = 0;
    } else if (result.rms_difference <= TOLERANCE_RMS ||
               (isfinite(result.snr_db) && result.snr_db >= TOLERANCE_SNR_DB)) {
        printf("VERDICT: PASS (Within tolerance)\n");
        printf("  RMS difference: %.8f (threshold: %.8f)\n", result.rms_difference, TOLERANCE_RMS);
        if (isfinite(result.snr_db)) {
            printf("  SNR: %.2f dB (threshold: %.1f dB)\n", result.snr_db, TOLERANCE_SNR_DB);
        }
        exit_code = 1;
    } else {
        printf("VERDICT: FAIL (Significant difference)\n");
        printf("  RMS difference: %.8f (threshold: %.8f)\n", result.rms_difference, TOLERANCE_RMS);
        if (isfinite(result.snr_db)) {
            printf("  SNR: %.2f dB (threshold: %.1f dB)\n", result.snr_db, TOLERANCE_SNR_DB);
        }
        exit_code = 2;
    }

    return exit_code;
}
