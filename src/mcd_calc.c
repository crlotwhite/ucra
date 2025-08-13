/**
 * @file mcd_calc.c
 * @brief Mel-Cepstral Distortion (MCD) Calculation Utility
 *
 * This utility computes the MCD between a golden reference audio file and
 * a synthesized audio file using MFCC extraction and Dynamic Time Warping (DTW).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <float.h>
#include <stdint.h>

#ifdef _WIN32
#include <io.h>
#define access(path, mode) _access(path, mode)
#define F_OK 0
#else
#include <unistd.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief WAV file header structure (simplified)
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
 * @brief MFCC feature extraction parameters
 */
typedef struct {
    int sample_rate;            // Sample rate
    int frame_size;             // Frame size in samples
    int hop_size;               // Hop size in samples
    int num_mel_filters;        // Number of mel filter banks
    int num_mfcc;               // Number of MFCC coefficients (13)
    double pre_emphasis;        // Pre-emphasis coefficient
    double* window;             // Window function
} MFCCConfig;

/**
 * @brief MFCC feature matrix
 */
typedef struct {
    double** features;          // MFCC matrix [frames][coefficients]
    int num_frames;             // Number of frames
    int num_coeffs;             // Number of coefficients
} MFCCFeatures;

/**
 * @brief DTW result structure
 */
typedef struct {
    double total_distance;      // Total DTW distance
    double normalized_distance; // Distance normalized by path length
    int* path_x;               // DTW path for reference sequence
    int* path_y;               // DTW path for test sequence
    int path_length;           // Length of DTW path
} DTWResult;

/**
 * @brief MCD calculation result
 */
typedef struct {
    double mcd_score;           // MCD score in dB
    double mean_distance;       // Mean frame distance
    double std_distance;        // Standard deviation of frame distances
    DTWResult dtw_result;       // DTW alignment result
    char* error_message;        // Error message if calculation failed
} MCDResult;

/* Forward declarations */
static int file_exists(const char* path);
static int load_wav_audio(const char* filename, float** samples, size_t* num_samples, int* sample_rate);
static void init_mfcc_config(MFCCConfig* config, int sample_rate);
static void cleanup_mfcc_config(MFCCConfig* config);
static int extract_mfcc_features(const float* audio, size_t num_samples,
                                const MFCCConfig* config, MFCCFeatures* features);
static void cleanup_mfcc_features(MFCCFeatures* features);
static int compute_dtw(const MFCCFeatures* ref_features, const MFCCFeatures* test_features,
                      DTWResult* result);
static void cleanup_dtw_result(DTWResult* result);
static double calculate_mcd(const MFCCFeatures* ref_features, const MFCCFeatures* test_features,
                           const DTWResult* dtw_result);
static void print_usage(const char* program_name);
static double euclidean_distance(const double* vec1, const double* vec2, int dim);
static void apply_hamming_window(float* frame, int frame_size, const double* window);
static void compute_mel_filterbank(const double* magnitude_spectrum, int spectrum_size,
                                  double* mel_features, const MFCCConfig* config, int sample_rate);
static void compute_dct(const double* mel_features, double* mfcc_coeffs, int num_mel, int num_mfcc);

/**
 * @brief Print usage information
 */
static void print_usage(const char* program_name) {
    printf("UCRA MCD(13) Calculation Utility\n");
    printf("Usage: %s <reference_wav> <test_wav> [options]\n", program_name);
    printf("\nArguments:\n");
    printf("  reference_wav          Path to reference/golden WAV file\n");
    printf("  test_wav              Path to test/synthesized WAV file\n");
    printf("\nOptions:\n");
    printf("  --frame-size SIZE     Frame size in samples (default: 512)\n");
    printf("  --hop-size SIZE       Hop size in samples (default: 256)\n");
    printf("  --mel-filters NUM     Number of mel filter banks (default: 40)\n");
    printf("  --verbose             Enable verbose output\n");
    printf("  -h, --help            Show this help message\n");
    printf("\nDescription:\n");
    printf("  Calculates Mel-Cepstral Distortion (MCD) using the first 13 MFCC\n");
    printf("  coefficients and Dynamic Time Warping for sequence alignment.\n");
    printf("\nReturn codes:\n");
    printf("  0: Calculation successful\n");
    printf("  1: Error occurred during calculation\n");
}

/**
 * @brief Check if a file exists
 */
static int file_exists(const char* path) {
    return access(path, F_OK) == 0;
}

/**
 * @brief Load WAV audio file
 */
static int load_wav_audio(const char* filename, float** samples, size_t* num_samples, int* sample_rate) {
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

    // Basic WAV header validation
    if (strncmp(header.riff_header, "RIFF", 4) != 0 ||
        strncmp(header.wave_header, "WAVE", 4) != 0) {
        fprintf(stderr, "Error: Invalid WAV file format in '%s'\n", filename);
        fclose(file);
        return -1;
    }

    *sample_rate = header.sample_rate;
    *num_samples = header.data_bytes / (header.bits_per_sample / 8);

    *samples = malloc(*num_samples * sizeof(float));
    if (!*samples) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    // Read and convert samples to float
    if (header.bits_per_sample == 16) {
        int16_t* raw_samples = malloc(header.data_bytes);
        if (fread(raw_samples, 1, header.data_bytes, file) != header.data_bytes) {
            free(raw_samples);
            free(*samples);
            fclose(file);
            return -1;
        }

        for (size_t i = 0; i < *num_samples; i++) {
            (*samples)[i] = raw_samples[i] / 32768.0f;
        }
        free(raw_samples);
    } else if (header.bits_per_sample == 32 && header.audio_format == 3) {
        // IEEE float format
        if (fread(*samples, sizeof(float), *num_samples, file) != *num_samples) {
            free(*samples);
            fclose(file);
            return -1;
        }
    } else {
        fprintf(stderr, "Error: Unsupported audio format in '%s'\n", filename);
        free(*samples);
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

/**
 * @brief Initialize MFCC configuration
 */
static void init_mfcc_config(MFCCConfig* config, int sample_rate) {
    config->sample_rate = sample_rate;
    config->frame_size = 512;
    config->hop_size = 256;
    config->num_mel_filters = 40;
    config->num_mfcc = 13;
    config->pre_emphasis = 0.97;

    // Create Hamming window
    config->window = malloc(config->frame_size * sizeof(double));
    for (int i = 0; i < config->frame_size; i++) {
        config->window[i] = 0.54 - 0.46 * cos(2.0 * M_PI * i / (config->frame_size - 1));
    }
}

/**
 * @brief Cleanup MFCC configuration
 */
static void cleanup_mfcc_config(MFCCConfig* config) {
    if (config) {
        free(config->window);
        memset(config, 0, sizeof(MFCCConfig));
    }
}

/**
 * @brief Calculate Euclidean distance between two vectors
 */
static double euclidean_distance(const double* vec1, const double* vec2, int dim) {
    double sum = 0.0;
    for (int i = 0; i < dim; i++) {
        double diff = vec1[i] - vec2[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}

/**
 * @brief Apply Hamming window to audio frame
 */
static void apply_hamming_window(float* frame, int frame_size, const double* window) {
    for (int i = 0; i < frame_size; i++) {
        frame[i] *= window[i];
    }
}

/**
 * @brief Simple FFT for magnitude spectrum (simplified implementation)
 * Note: This is a basic implementation. For production use, consider using FFTW or similar.
 */
static void compute_magnitude_spectrum(const float* frame, int frame_size, double* magnitude_spectrum) {
    // Zero-pad to next power of 2 for simplicity
    int fft_size = 1;
    while (fft_size < frame_size) {
        fft_size <<= 1;
    }

    // Simplified magnitude spectrum calculation
    // In a real implementation, this would use proper FFT
    for (int k = 0; k < fft_size / 2 + 1; k++) {
        double real_part = 0.0;
        double imag_part = 0.0;

        for (int n = 0; n < frame_size; n++) {
            double angle = -2.0 * M_PI * k * n / fft_size;
            real_part += frame[n] * cos(angle);
            imag_part += frame[n] * sin(angle);
        }

        magnitude_spectrum[k] = sqrt(real_part * real_part + imag_part * imag_part);
    }
}

/**
 * @brief Convert Hz to Mel scale
 */
static double hz_to_mel(double hz) {
    return 2595.0 * log10(1.0 + hz / 700.0);
}

/**
 * @brief Convert Mel to Hz scale
 */
static double mel_to_hz(double mel) {
    return 700.0 * (pow(10.0, mel / 2595.0) - 1.0);
}

/**
 * @brief Compute mel filter bank features
 */
static void compute_mel_filterbank(const double* magnitude_spectrum, int spectrum_size,
                                  double* mel_features, const MFCCConfig* config, int sample_rate) {
    double nyquist = sample_rate / 2.0;
    double mel_low = hz_to_mel(0);
    double mel_high = hz_to_mel(nyquist);

    // Create mel filter bank
    double* mel_points = malloc((config->num_mel_filters + 2) * sizeof(double));
    for (int i = 0; i < config->num_mel_filters + 2; i++) {
        mel_points[i] = mel_to_hz(mel_low + i * (mel_high - mel_low) / (config->num_mel_filters + 1));
    }

    // Convert mel points to bin indices
    int* bin_indices = malloc((config->num_mel_filters + 2) * sizeof(int));
    for (int i = 0; i < config->num_mel_filters + 2; i++) {
        bin_indices[i] = (int)(mel_points[i] * spectrum_size / nyquist);
    }

    // Apply mel filters
    for (int i = 0; i < config->num_mel_filters; i++) {
        double filter_sum = 0.0;

        // Lower slope
        for (int k = bin_indices[i]; k < bin_indices[i + 1]; k++) {
            if (k < spectrum_size) {
                double weight = (double)(k - bin_indices[i]) / (bin_indices[i + 1] - bin_indices[i]);
                filter_sum += magnitude_spectrum[k] * weight;
            }
        }

        // Upper slope
        for (int k = bin_indices[i + 1]; k < bin_indices[i + 2]; k++) {
            if (k < spectrum_size) {
                double weight = (double)(bin_indices[i + 2] - k) / (bin_indices[i + 2] - bin_indices[i + 1]);
                filter_sum += magnitude_spectrum[k] * weight;
            }
        }

        mel_features[i] = log(filter_sum + 1e-10); // Add small epsilon to avoid log(0)
    }

    free(mel_points);
    free(bin_indices);
}

/**
 * @brief Compute Discrete Cosine Transform for MFCC
 */
static void compute_dct(const double* mel_features, double* mfcc_coeffs, int num_mel, int num_mfcc) {
    for (int i = 0; i < num_mfcc; i++) {
        mfcc_coeffs[i] = 0.0;
        for (int j = 0; j < num_mel; j++) {
            mfcc_coeffs[i] += mel_features[j] * cos(M_PI * i * (j + 0.5) / num_mel);
        }
        mfcc_coeffs[i] *= sqrt(2.0 / num_mel);
    }
}

/**
 * @brief Extract MFCC features from audio
 */
static int extract_mfcc_features(const float* audio, size_t num_samples,
                                const MFCCConfig* config, MFCCFeatures* features) {
    if (!audio || !config || !features) {
        return -1;
    }

    // Calculate number of frames
    int num_frames = (num_samples - config->frame_size) / config->hop_size + 1;
    if (num_frames <= 0) {
        return -1;
    }

    // Allocate feature matrix
    features->num_frames = num_frames;
    features->num_coeffs = config->num_mfcc;
    features->features = malloc(num_frames * sizeof(double*));

    for (int i = 0; i < num_frames; i++) {
        features->features[i] = malloc(config->num_mfcc * sizeof(double));
    }

    // Process each frame
    int spectrum_size = config->frame_size / 2 + 1;
    double* magnitude_spectrum = malloc(spectrum_size * sizeof(double));
    double* mel_features = malloc(config->num_mel_filters * sizeof(double));
    float* frame_buffer = malloc(config->frame_size * sizeof(float));

    for (int frame_idx = 0; frame_idx < num_frames; frame_idx++) {
        int start_sample = frame_idx * config->hop_size;

        // Copy frame
        for (int i = 0; i < config->frame_size; i++) {
            if (start_sample + i < num_samples) {
                frame_buffer[i] = audio[start_sample + i];
            } else {
                frame_buffer[i] = 0.0f; // Zero padding
            }
        }

        // Apply pre-emphasis
        for (int i = config->frame_size - 1; i > 0; i--) {
            frame_buffer[i] -= config->pre_emphasis * frame_buffer[i - 1];
        }

        // Apply window
        apply_hamming_window(frame_buffer, config->frame_size, config->window);

        // Compute magnitude spectrum
        compute_magnitude_spectrum(frame_buffer, config->frame_size, magnitude_spectrum);

        // Compute mel filter bank features
        compute_mel_filterbank(magnitude_spectrum, spectrum_size, mel_features, config, config->sample_rate);

        // Compute MFCC via DCT
        compute_dct(mel_features, features->features[frame_idx], config->num_mel_filters, config->num_mfcc);
    }

    free(magnitude_spectrum);
    free(mel_features);
    free(frame_buffer);

    return 0;
}

/**
 * @brief Cleanup MFCC features
 */
static void cleanup_mfcc_features(MFCCFeatures* features) {
    if (!features) return;

    for (int i = 0; i < features->num_frames; i++) {
        free(features->features[i]);
    }
    free(features->features);
    memset(features, 0, sizeof(MFCCFeatures));
}

/**
 * @brief Compute Dynamic Time Warping alignment
 */
static int compute_dtw(const MFCCFeatures* ref_features, const MFCCFeatures* test_features,
                      DTWResult* result) {
    if (!ref_features || !test_features || !result) {
        return -1;
    }

    int ref_frames = ref_features->num_frames;
    int test_frames = test_features->num_frames;
    int num_coeffs = ref_features->num_coeffs;

    // Allocate DTW distance matrix
    double** dtw_matrix = malloc(ref_frames * sizeof(double*));
    for (int i = 0; i < ref_frames; i++) {
        dtw_matrix[i] = malloc(test_frames * sizeof(double));
    }

    // Initialize first row and column
    dtw_matrix[0][0] = euclidean_distance(ref_features->features[0],
                                         test_features->features[0], num_coeffs);

    for (int i = 1; i < ref_frames; i++) {
        double dist = euclidean_distance(ref_features->features[i],
                                        test_features->features[0], num_coeffs);
        dtw_matrix[i][0] = dtw_matrix[i-1][0] + dist;
    }

    for (int j = 1; j < test_frames; j++) {
        double dist = euclidean_distance(ref_features->features[0],
                                        test_features->features[j], num_coeffs);
        dtw_matrix[0][j] = dtw_matrix[0][j-1] + dist;
    }

    // Fill DTW matrix
    for (int i = 1; i < ref_frames; i++) {
        for (int j = 1; j < test_frames; j++) {
            double dist = euclidean_distance(ref_features->features[i],
                                           test_features->features[j], num_coeffs);

            double min_prev = dtw_matrix[i-1][j-1]; // Diagonal
            if (dtw_matrix[i-1][j] < min_prev) min_prev = dtw_matrix[i-1][j]; // Up
            if (dtw_matrix[i][j-1] < min_prev) min_prev = dtw_matrix[i][j-1]; // Left

            dtw_matrix[i][j] = dist + min_prev;
        }
    }

    // Backtrack to find optimal path
    int max_path_length = ref_frames + test_frames;
    result->path_x = malloc(max_path_length * sizeof(int));
    result->path_y = malloc(max_path_length * sizeof(int));

    int i = ref_frames - 1;
    int j = test_frames - 1;
    int path_idx = 0;

    while (i > 0 || j > 0) {
        result->path_x[path_idx] = i;
        result->path_y[path_idx] = j;
        path_idx++;

        if (i == 0) {
            j--;
        } else if (j == 0) {
            i--;
        } else {
            double diag = dtw_matrix[i-1][j-1];
            double up = dtw_matrix[i-1][j];
            double left = dtw_matrix[i][j-1];

            if (diag <= up && diag <= left) {
                i--; j--;
            } else if (up <= left) {
                i--;
            } else {
                j--;
            }
        }
    }

    // Add final point
    result->path_x[path_idx] = 0;
    result->path_y[path_idx] = 0;
    path_idx++;

    result->path_length = path_idx;
    result->total_distance = dtw_matrix[ref_frames-1][test_frames-1];
    result->normalized_distance = result->total_distance / path_idx;

    // Cleanup DTW matrix
    for (int i = 0; i < ref_frames; i++) {
        free(dtw_matrix[i]);
    }
    free(dtw_matrix);

    return 0;
}

/**
 * @brief Cleanup DTW result
 */
static void cleanup_dtw_result(DTWResult* result) {
    if (!result) return;

    free(result->path_x);
    free(result->path_y);
    memset(result, 0, sizeof(DTWResult));
}

/**
 * @brief Calculate MCD score from aligned features
 */
static double calculate_mcd(const MFCCFeatures* ref_features, const MFCCFeatures* test_features,
                           const DTWResult* dtw_result) {
    if (!ref_features || !test_features || !dtw_result) {
        return -1.0;
    }

    double total_distance = 0.0;
    int num_coeffs = ref_features->num_coeffs;

    // Calculate distance along DTW path (excluding c0)
    for (int path_idx = 0; path_idx < dtw_result->path_length; path_idx++) {
        int ref_idx = dtw_result->path_x[path_idx];
        int test_idx = dtw_result->path_y[path_idx];

        double frame_distance = 0.0;

        // Sum squared differences for coefficients 1-12 (skip c0)
        for (int c = 1; c < num_coeffs; c++) {
            double diff = ref_features->features[ref_idx][c] - test_features->features[test_idx][c];
            frame_distance += diff * diff;
        }

        total_distance += sqrt(frame_distance);
    }

    // MCD formula: (10/ln(10)) * (2 / path_length) * sum_of_distances
    double mcd = (10.0 / log(10.0)) * (2.0 / dtw_result->path_length) * total_distance;

    return mcd;
}

/**
 * @brief Print calculation results
 */
static void print_results(const MCDResult* result, int verbose) {
    if (!result) return;

    if (result->error_message) {
        printf("Error: %s\n", result->error_message);
        return;
    }

    printf("MCD(13) Calculation Results:\n");
    printf("  MCD Score:        %.4f dB\n", result->mcd_score);
    printf("  Mean Distance:    %.4f\n", result->mean_distance);
    printf("  Std Distance:     %.4f\n", result->std_distance);

    if (verbose) {
        printf("  DTW Path Length:  %d\n", result->dtw_result.path_length);
        printf("  DTW Total Dist:   %.4f\n", result->dtw_result.total_distance);
        printf("  DTW Norm Dist:    %.4f\n", result->dtw_result.normalized_distance);
    }
}

/**
 * @brief Main function for MCD calculation
 */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char* reference_file = argv[1];
    const char* test_file = argv[2];
    int verbose = 0;

    // Parse options
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (verbose) {
        printf("MCD(13) Calculation:\n");
        printf("  Reference: %s\n", reference_file);
        printf("  Test:      %s\n\n", test_file);
    }

    // Check if files exist
    if (!file_exists(reference_file)) {
        fprintf(stderr, "Error: Reference file '%s' not found\n", reference_file);
        return 1;
    }

    if (!file_exists(test_file)) {
        fprintf(stderr, "Error: Test file '%s' not found\n", test_file);
        return 1;
    }

    // Load audio files
    float* ref_audio, *test_audio;
    size_t ref_samples, test_samples;
    int ref_sample_rate, test_sample_rate;

    if (load_wav_audio(reference_file, &ref_audio, &ref_samples, &ref_sample_rate) != 0) {
        return 1;
    }

    if (load_wav_audio(test_file, &test_audio, &test_samples, &test_sample_rate) != 0) {
        free(ref_audio);
        return 1;
    }

    // Check sample rates match
    if (ref_sample_rate != test_sample_rate) {
        fprintf(stderr, "Error: Sample rates do not match (%d vs %d)\n",
                ref_sample_rate, test_sample_rate);
        free(ref_audio);
        free(test_audio);
        return 1;
    }

    if (verbose) {
        printf("Loaded audio files:\n");
        printf("  Reference: %zu samples, %d Hz\n", ref_samples, ref_sample_rate);
        printf("  Test:      %zu samples, %d Hz\n\n", test_samples, test_sample_rate);
    }

    // Initialize MFCC configuration
    MFCCConfig config;
    init_mfcc_config(&config, ref_sample_rate);

    // Extract MFCC features
    MFCCFeatures ref_features, test_features;

    if (extract_mfcc_features(ref_audio, ref_samples, &config, &ref_features) != 0) {
        fprintf(stderr, "Error: Failed to extract MFCC features from reference file\n");
        cleanup_mfcc_config(&config);
        free(ref_audio);
        free(test_audio);
        return 1;
    }

    if (extract_mfcc_features(test_audio, test_samples, &config, &test_features) != 0) {
        fprintf(stderr, "Error: Failed to extract MFCC features from test file\n");
        cleanup_mfcc_features(&ref_features);
        cleanup_mfcc_config(&config);
        free(ref_audio);
        free(test_audio);
        return 1;
    }

    if (verbose) {
        printf("Extracted MFCC features:\n");
        printf("  Reference: %d frames, %d coefficients\n", ref_features.num_frames, ref_features.num_coeffs);
        printf("  Test:      %d frames, %d coefficients\n\n", test_features.num_frames, test_features.num_coeffs);
    }

    // Compute DTW alignment
    MCDResult result;
    memset(&result, 0, sizeof(MCDResult));

    if (compute_dtw(&ref_features, &test_features, &result.dtw_result) != 0) {
        result.error_message = strdup("Failed to compute DTW alignment");
        print_results(&result, verbose);
        cleanup_mfcc_features(&ref_features);
        cleanup_mfcc_features(&test_features);
        cleanup_mfcc_config(&config);
        free(ref_audio);
        free(test_audio);
        return 1;
    }

    // Calculate MCD score
    result.mcd_score = calculate_mcd(&ref_features, &test_features, &result.dtw_result);
    result.mean_distance = result.dtw_result.normalized_distance;
    result.std_distance = 0.0; // Simplified - would need another pass to calculate

    // Print results
    print_results(&result, verbose);

    // Cleanup
    cleanup_dtw_result(&result.dtw_result);
    cleanup_mfcc_features(&ref_features);
    cleanup_mfcc_features(&test_features);
    cleanup_mfcc_config(&config);
    free(ref_audio);
    free(test_audio);

    return 0;
}
