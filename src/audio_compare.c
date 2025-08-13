/**
 * @file audio_compare.c
 * @brief Audio Comparison Module implementation
 * 
 * This module compares a rendered output WAV against a 'golden' reference WAV
 * and determines if they match using both strict bit-for-bit comparison and
 * lenient sample-based difference calculation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>

#ifdef _WIN32
#include <io.h>
#define access(path, mode) _access(path, mode)
#define F_OK 0
#else
#include <unistd.h>
#endif

/**
 * @brief WAV file header structure
 */
typedef struct {
    char riff_header[4];        // "RIFF"
    uint32_t wav_size;          // File size - 8
    char wave_header[4];        // "WAVE"
    char fmt_header[4];         // "fmt "
    uint32_t fmt_chunk_size;    // Format chunk size
    uint16_t audio_format;      // Audio format (1 = PCM)
    uint16_t num_channels;      // Number of channels
    uint32_t sample_rate;       // Sample rate
    uint32_t byte_rate;         // Byte rate
    uint16_t block_align;       // Block align
    uint16_t bits_per_sample;   // Bits per sample
    char data_header[4];        // "data"
    uint32_t data_bytes;        // Number of data bytes
} WAVHeader;

/**
 * @brief Audio comparison result structure
 */
typedef struct {
    int strict_match;           // 1 if files are bit-for-bit identical
    double rms_difference;      // RMS difference between samples
    double snr_db;              // Signal-to-noise ratio in dB
    double max_difference;      // Maximum absolute difference
    char* error_message;        // Error message if comparison failed
} AudioCompareResult;

/* Forward declarations */
static int file_exists(const char* path);
static int read_wav_header(FILE* file, WAVHeader* header);
static int validate_wav_header(const WAVHeader* header);
static int compare_files_bitwise(const char* file1, const char* file2);
static int compare_wav_samples(const char* file1, const char* file2, AudioCompareResult* result);
static void print_usage(const char* program_name);
static double calculate_rms_difference(const float* samples1, const float* samples2, size_t count);
static double calculate_snr_db(const float* signal, const float* difference, size_t count);

/**
 * @brief Print usage information
 */
static void print_usage(const char* program_name) {
    printf("UCRA Audio Comparison Module\n");
    printf("Usage: %s <reference_wav> <test_wav> [options]\n", program_name);
    printf("\nArguments:\n");
    printf("  reference_wav           Path to the golden/reference WAV file\n");
    printf("  test_wav               Path to the test/generated WAV file\n");
    printf("\nOptions:\n");
    printf("  --strict-only          Only perform bit-for-bit comparison\n");
    printf("  --tolerance THRESHOLD  RMS difference tolerance (default: 0.001)\n");
    printf("  --verbose              Enable verbose output\n");
    printf("  -h, --help             Show this help message\n");
    printf("\nReturn codes:\n");
    printf("  0: Files match (within tolerance)\n");
    printf("  1: Files do not match\n");
    printf("  2: Error occurred during comparison\n");
}

/**
 * @brief Check if a file exists
 */
static int file_exists(const char* path) {
    return access(path, F_OK) == 0;
}

/**
 * @brief Read WAV file header
 */
static int read_wav_header(FILE* file, WAVHeader* header) {
    if (!file || !header) {
        return -1;
    }

    if (fread(header, sizeof(WAVHeader), 1, file) != 1) {
        return -1;
    }

    return 0;
}

/**
 * @brief Validate WAV file header
 */
static int validate_wav_header(const WAVHeader* header) {
    if (!header) {
        return -1;
    }

    // Check RIFF header
    if (strncmp(header->riff_header, "RIFF", 4) != 0) {
        return -1;
    }

    // Check WAVE header
    if (strncmp(header->wave_header, "WAVE", 4) != 0) {
        return -1;
    }

    // Check fmt header
    if (strncmp(header->fmt_header, "fmt ", 4) != 0) {
        return -1;
    }

    // Check data header
    if (strncmp(header->data_header, "data", 4) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief Compare two files bit-for-bit
 */
static int compare_files_bitwise(const char* file1, const char* file2) {
    FILE* f1 = fopen(file1, "rb");
    FILE* f2 = fopen(file2, "rb");
    
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return -1;
    }

    int result = 1; // Assume files are identical
    
    // Compare file sizes first
    fseek(f1, 0, SEEK_END);
    fseek(f2, 0, SEEK_END);
    long size1 = ftell(f1);
    long size2 = ftell(f2);
    
    if (size1 != size2) {
        result = 0;
        goto cleanup;
    }

    // Compare contents
    fseek(f1, 0, SEEK_SET);
    fseek(f2, 0, SEEK_SET);

    const size_t buffer_size = 8192;
    unsigned char buffer1[8192];
    unsigned char buffer2[8192];

    while (!feof(f1) && !feof(f2)) {
        size_t read1 = fread(buffer1, 1, buffer_size, f1);
        size_t read2 = fread(buffer2, 1, buffer_size, f2);

        if (read1 != read2 || memcmp(buffer1, buffer2, read1) != 0) {
            result = 0;
            break;
        }
    }

cleanup:
    fclose(f1);
    fclose(f2);
    return result;
}

/**
 * @brief Calculate RMS difference between two sample arrays
 */
static double calculate_rms_difference(const float* samples1, const float* samples2, size_t count) {
    if (!samples1 || !samples2 || count == 0) {
        return 0.0;
    }

    double sum_squared_diff = 0.0;
    
    for (size_t i = 0; i < count; i++) {
        double diff = samples1[i] - samples2[i];
        sum_squared_diff += diff * diff;
    }

    return sqrt(sum_squared_diff / count);
}

/**
 * @brief Calculate signal-to-noise ratio in dB
 */
static double calculate_snr_db(const float* signal, const float* difference, size_t count) {
    if (!signal || !difference || count == 0) {
        return 0.0;
    }

    double signal_power = 0.0;
    double noise_power = 0.0;

    for (size_t i = 0; i < count; i++) {
        signal_power += signal[i] * signal[i];
        noise_power += difference[i] * difference[i];
    }

    signal_power /= count;
    noise_power /= count;

    if (noise_power < 1e-12) {
        return 120.0; // Very high SNR
    }

    return 10.0 * log10(signal_power / noise_power);
}

/**
 * @brief Read WAV samples as float array
 */
static float* read_wav_samples(const char* filename, size_t* sample_count, WAVHeader* header) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    if (read_wav_header(file, header) != 0 || validate_wav_header(header) != 0) {
        fclose(file);
        return NULL;
    }

    *sample_count = header->data_bytes / (header->bits_per_sample / 8);
    float* samples = malloc(*sample_count * sizeof(float));
    
    if (!samples) {
        fclose(file);
        return NULL;
    }

    // Read samples based on bit depth
    if (header->bits_per_sample == 16) {
        int16_t* raw_samples = malloc(header->data_bytes);
        if (fread(raw_samples, 1, header->data_bytes, file) != header->data_bytes) {
            free(raw_samples);
            free(samples);
            fclose(file);
            return NULL;
        }

        // Convert 16-bit samples to float
        for (size_t i = 0; i < *sample_count; i++) {
            samples[i] = raw_samples[i] / 32768.0f;
        }
        
        free(raw_samples);
    } else if (header->bits_per_sample == 32 && header->audio_format == 3) {
        // IEEE float format
        if (fread(samples, sizeof(float), *sample_count, file) != *sample_count) {
            free(samples);
            fclose(file);
            return NULL;
        }
    } else {
        // Unsupported format
        free(samples);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return samples;
}

/**
 * @brief Compare WAV files at sample level
 */
static int compare_wav_samples(const char* file1, const char* file2, AudioCompareResult* result) {
    WAVHeader header1, header2;
    size_t sample_count1, sample_count2;

    float* samples1 = read_wav_samples(file1, &sample_count1, &header1);
    float* samples2 = read_wav_samples(file2, &sample_count2, &header2);

    if (!samples1 || !samples2) {
        free(samples1);
        free(samples2);
        result->error_message = strdup("Failed to read WAV samples");
        return -1;
    }

    // Check if headers match
    if (header1.sample_rate != header2.sample_rate ||
        header1.num_channels != header2.num_channels ||
        sample_count1 != sample_count2) {
        
        free(samples1);
        free(samples2);
        result->error_message = strdup("WAV file formats do not match");
        return -1;
    }

    // Calculate differences
    float* difference = malloc(sample_count1 * sizeof(float));
    if (!difference) {
        free(samples1);
        free(samples2);
        result->error_message = strdup("Memory allocation failed");
        return -1;
    }

    result->max_difference = 0.0;
    for (size_t i = 0; i < sample_count1; i++) {
        difference[i] = samples1[i] - samples2[i];
        double abs_diff = fabs(difference[i]);
        if (abs_diff > result->max_difference) {
            result->max_difference = abs_diff;
        }
    }

    result->rms_difference = calculate_rms_difference(samples1, samples2, sample_count1);
    result->snr_db = calculate_snr_db(samples1, difference, sample_count1);

    free(samples1);
    free(samples2);
    free(difference);

    return 0;
}

/**
 * @brief Perform audio comparison
 */
static int perform_audio_comparison(const char* reference_file, const char* test_file, 
                                  AudioCompareResult* result, int strict_only, double tolerance) {
    if (!reference_file || !test_file || !result) {
        return -1;
    }

    memset(result, 0, sizeof(AudioCompareResult));

    // Check if files exist
    if (!file_exists(reference_file)) {
        result->error_message = strdup("Reference file not found");
        return -1;
    }

    if (!file_exists(test_file)) {
        result->error_message = strdup("Test file not found");
        return -1;
    }

    // Perform bit-for-bit comparison
    result->strict_match = compare_files_bitwise(reference_file, test_file);

    if (strict_only) {
        return result->strict_match ? 0 : 1;
    }

    // If files are identical, no need for sample-level comparison
    if (result->strict_match) {
        result->rms_difference = 0.0;
        result->snr_db = 120.0; // Very high SNR
        result->max_difference = 0.0;
        return 0;
    }

    // Perform sample-level comparison
    if (compare_wav_samples(reference_file, test_file, result) != 0) {
        return -1;
    }

    // Check if difference is within tolerance
    return (result->rms_difference <= tolerance) ? 0 : 1;
}

/**
 * @brief Print comparison results
 */
static void print_results(const AudioCompareResult* result, int verbose) {
    if (!result) return;

    if (result->error_message) {
        printf("Error: %s\n", result->error_message);
        return;
    }

    printf("Audio Comparison Results:\n");
    printf("  Bit-for-bit identical: %s\n", result->strict_match ? "Yes" : "No");
    
    if (verbose || !result->strict_match) {
        printf("  RMS difference: %.6f\n", result->rms_difference);
        printf("  Maximum difference: %.6f\n", result->max_difference);
        printf("  Signal-to-noise ratio: %.2f dB\n", result->snr_db);
    }
}

/**
 * @brief Main function for audio comparison
 */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 2;
    }

    const char* reference_file = argv[1];
    const char* test_file = argv[2];
    int strict_only = 0;
    int verbose = 0;
    double tolerance = 0.001; // Default tolerance

    // Parse options
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--strict-only") == 0) {
            strict_only = 1;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "--tolerance") == 0) {
            if (i + 1 < argc) {
                tolerance = atof(argv[++i]);
            } else {
                fprintf(stderr, "Error: --tolerance requires a numeric argument\n");
                return 2;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (verbose) {
        printf("Comparing WAV files:\n");
        printf("  Reference: %s\n", reference_file);
        printf("  Test:      %s\n", test_file);
        printf("  Tolerance: %.6f\n", tolerance);
        printf("  Strict only: %s\n\n", strict_only ? "Yes" : "No");
    }

    AudioCompareResult result;
    int comparison_result = perform_audio_comparison(reference_file, test_file, &result, strict_only, tolerance);

    if (comparison_result < 0) {
        print_results(&result, verbose);
        free(result.error_message);
        return 2; // Error
    }

    print_results(&result, verbose);

    if (comparison_result == 0) {
        printf("Result: PASS - Files match within tolerance\n");
    } else {
        printf("Result: FAIL - Files do not match\n");
    }

    free(result.error_message);
    return comparison_result;
}
