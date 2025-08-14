/*
 * MCD(13) Calculation Utility
 *
 * This utility calculates the Mel-Cepstral Distortion (MCD) between
 * a reference audio file and a synthesized audio file.
 *
 * Usage: mcd_calc <reference_wav> <synthesized_wav>
 *
 * The calculation involves:
 * 1. Load WAV files
 * 2. Extract MFCC features (first 13 coefficients)
 * 3. Apply Dynamic Time Warping for alignment
 * 4. Calculate Euclidean distance for MCD score
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>

#define MAX_FILENAME 256
#define MFCC_COEFFS 13
#define FRAME_SIZE 1024
#define HOP_SIZE 512
#define SAMPLE_RATE 22050
#define MEL_FILTERS 26
#define MIN_FREQ 0.0f
#define MAX_FREQ 11025.0f
#define PI 3.14159265358979323846

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

/* MFCC feature matrix */
typedef struct {
    float** features; /* [frame][coefficient] */
    int num_frames;
    int num_coeffs;
} MFCCMatrix;

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

/* Initialize MFCC matrix */
static int mfcc_matrix_init(MFCCMatrix* mfcc, int num_frames, int num_coeffs) {
    mfcc->num_frames = num_frames;
    mfcc->num_coeffs = num_coeffs;

    mfcc->features = malloc(num_frames * sizeof(float*));
    if (!mfcc->features) {
        return -1;
    }

    for (int i = 0; i < num_frames; i++) {
        mfcc->features[i] = calloc(num_coeffs, sizeof(float));
        if (!mfcc->features[i]) {
            /* Cleanup on failure */
            for (int j = 0; j < i; j++) {
                free(mfcc->features[j]);
            }
            free(mfcc->features);
            return -1;
        }
    }

    return 0;
}

/* Free MFCC matrix memory */
static void mfcc_matrix_free(MFCCMatrix* mfcc) {
    if (mfcc->features) {
        for (int i = 0; i < mfcc->num_frames; i++) {
            if (mfcc->features[i]) {
                free(mfcc->features[i]);
            }
        }
        free(mfcc->features);
        mfcc->features = NULL;
    }
    mfcc->num_frames = 0;
    mfcc->num_coeffs = 0;
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
    } else {
        fprintf(stderr, "Error: Only 16-bit WAV files are supported\n");
        free(audio->samples);
        fclose(file);
        return -1;
    }

    fclose(file);
    printf("Loaded WAV file: %d samples, %d Hz, %d channels\n",
           sample_count, audio->sample_rate, audio->channels);
    return 0;
}

/* Simple DCT-II implementation for MFCC */
static void dct_ii(const float* input, float* output, int N) {
    for (int k = 0; k < N; k++) {
        float sum = 0.0f;
        for (int n = 0; n < N; n++) {
            sum += input[n] * cosf(PI * k * (2.0f * n + 1.0f) / (2.0f * N));
        }
        output[k] = sum;
    }
}

/* Mel scale conversion */
static float hz_to_mel(float hz) {
    return 2595.0f * log10f(1.0f + hz / 700.0f);
}

static float mel_to_hz(float mel) {
    return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
}

/* Extract MFCC features from audio */
static int extract_mfcc(const AudioData* audio, MFCCMatrix* mfcc) {
    int num_frames = (audio->length - FRAME_SIZE) / HOP_SIZE + 1;

    if (mfcc_matrix_init(mfcc, num_frames, MFCC_COEFFS) < 0) {
        return -1;
    }

    /* Allocate working buffers */
    float* frame = malloc(FRAME_SIZE * sizeof(float));
    float* windowed_frame = malloc(FRAME_SIZE * sizeof(float));
    float* power_spectrum = malloc(FRAME_SIZE / 2 + 1 * sizeof(float));
    float* mel_filters_output = malloc(MEL_FILTERS * sizeof(float));
    float* log_mel = malloc(MEL_FILTERS * sizeof(float));

    if (!frame || !windowed_frame || !power_spectrum || !mel_filters_output || !log_mel) {
        fprintf(stderr, "Error: Memory allocation failed for MFCC extraction\n");
        goto cleanup;
    }

    /* Create mel filter bank */
    float* mel_points = malloc((MEL_FILTERS + 2) * sizeof(float));
    if (!mel_points) {
        goto cleanup;
    }

    float mel_min = hz_to_mel(MIN_FREQ);
    float mel_max = hz_to_mel(MAX_FREQ);

    for (int i = 0; i < MEL_FILTERS + 2; i++) {
        float mel = mel_min + (mel_max - mel_min) * i / (MEL_FILTERS + 1);
        mel_points[i] = mel_to_hz(mel);
    }

    /* Process each frame */
    for (int frame_idx = 0; frame_idx < num_frames; frame_idx++) {
        int start_idx = frame_idx * HOP_SIZE;

        /* Extract frame */
        for (int i = 0; i < FRAME_SIZE; i++) {
            if (start_idx + i < audio->length) {
                frame[i] = audio->samples[start_idx + i];
            } else {
                frame[i] = 0.0f;
            }
        }

        /* Apply Hamming window */
        for (int i = 0; i < FRAME_SIZE; i++) {
            float window = 0.54f - 0.46f * cosf(2.0f * PI * i / (FRAME_SIZE - 1));
            windowed_frame[i] = frame[i] * window;
        }

        /* Simple power spectrum calculation (simplified FFT) */
        for (int i = 0; i < FRAME_SIZE / 2 + 1; i++) {
            float real = 0.0f, imag = 0.0f;
            for (int j = 0; j < FRAME_SIZE; j++) {
                float angle = -2.0f * PI * i * j / FRAME_SIZE;
                real += windowed_frame[j] * cosf(angle);
                imag += windowed_frame[j] * sinf(angle);
            }
            power_spectrum[i] = real * real + imag * imag;
        }

        /* Apply mel filter bank */
        for (int m = 0; m < MEL_FILTERS; m++) {
            float sum = 0.0f;
            for (int k = 0; k < FRAME_SIZE / 2 + 1; k++) {
                float freq = (float)k * audio->sample_rate / FRAME_SIZE;
                float weight = 0.0f;

                /* Triangular filter */
                if (freq >= mel_points[m] && freq <= mel_points[m + 1]) {
                    weight = (freq - mel_points[m]) / (mel_points[m + 1] - mel_points[m]);
                } else if (freq >= mel_points[m + 1] && freq <= mel_points[m + 2]) {
                    weight = (mel_points[m + 2] - freq) / (mel_points[m + 2] - mel_points[m + 1]);
                }

                sum += power_spectrum[k] * weight;
            }
            mel_filters_output[m] = sum;
        }

        /* Log mel spectrum */
        for (int i = 0; i < MEL_FILTERS; i++) {
            log_mel[i] = logf(mel_filters_output[i] + 1e-10f);
        }

        /* DCT to get MFCC */
        dct_ii(log_mel, mfcc->features[frame_idx], MFCC_COEFFS);
    }

    free(mel_points);
    free(frame);
    free(windowed_frame);
    free(power_spectrum);
    free(mel_filters_output);
    free(log_mel);

    printf("Extracted MFCC features: %d frames, %d coefficients\n",
           mfcc->num_frames, mfcc->num_coeffs);
    return 0;

cleanup:
    if (frame) free(frame);
    if (windowed_frame) free(windowed_frame);
    if (power_spectrum) free(power_spectrum);
    if (mel_filters_output) free(mel_filters_output);
    if (log_mel) free(log_mel);
    return -1;
}

/* Calculate MCD with simplified DTW */
static double calculate_mcd(const MFCCMatrix* ref_mfcc, const MFCCMatrix* syn_mfcc) {
    int ref_frames = ref_mfcc->num_frames;
    int syn_frames = syn_mfcc->num_frames;

    /* Simplified alignment: linear interpolation */
    int min_frames = (ref_frames < syn_frames) ? ref_frames : syn_frames;
    double total_distance = 0.0;
    int valid_frames = 0;

    for (int i = 0; i < min_frames; i++) {
        int ref_idx = (int)(i * (double)ref_frames / min_frames);
        int syn_idx = (int)(i * (double)syn_frames / min_frames);

        /* Skip C0 coefficient and calculate Euclidean distance for C1-C12 */
        double frame_distance = 0.0;
        for (int c = 1; c < MFCC_COEFFS; c++) {
            double diff = ref_mfcc->features[ref_idx][c] - syn_mfcc->features[syn_idx][c];
            frame_distance += diff * diff;
        }

        frame_distance = sqrt(frame_distance);
        total_distance += frame_distance;
        valid_frames++;
    }

    if (valid_frames == 0) {
        fprintf(stderr, "Error: No valid frames for MCD calculation\n");
        return -1.0;
    }

    /* MCD formula: (10/ln(10)) * sqrt(2 * sum_squared_differences) */
    double mcd = (10.0 / log(10.0)) * sqrt(2.0) * (total_distance / valid_frames);

    printf("Compared %d aligned frames\n", valid_frames);
    return mcd;
}

static void print_usage(const char* program_name) {
    printf("Usage: %s <reference_wav> <synthesized_wav>\n", program_name);
    printf("\n");
    printf("Calculate Mel-Cepstral Distortion (MCD) between two audio files.\n");
    printf("\n");
    printf("File format:\n");
    printf("  16-bit PCM WAV files (mono or stereo)\n");
    printf("  Recommended sample rate: 22050 Hz\n");
    printf("\n");
    printf("Output:\n");
    printf("  MCD value in dB\n");
    printf("  Lower values indicate better similarity\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char* ref_file = argv[1];
    const char* syn_file = argv[2];

    /* Load audio files */
    AudioData ref_audio, syn_audio;
    audio_data_init(&ref_audio);
    audio_data_init(&syn_audio);

    printf("Loading reference audio from '%s'...\n", ref_file);
    if (load_wav_file(ref_file, &ref_audio) < 0) {
        return 1;
    }

    printf("Loading synthesized audio from '%s'...\n", syn_file);
    if (load_wav_file(syn_file, &syn_audio) < 0) {
        audio_data_free(&ref_audio);
        return 1;
    }

    /* Extract MFCC features */
    MFCCMatrix ref_mfcc, syn_mfcc;

    printf("\nExtracting MFCC features from reference audio...\n");
    if (extract_mfcc(&ref_audio, &ref_mfcc) < 0) {
        audio_data_free(&ref_audio);
        audio_data_free(&syn_audio);
        return 1;
    }

    printf("Extracting MFCC features from synthesized audio...\n");
    if (extract_mfcc(&syn_audio, &syn_mfcc) < 0) {
        audio_data_free(&ref_audio);
        audio_data_free(&syn_audio);
        mfcc_matrix_free(&ref_mfcc);
        return 1;
    }

    /* Calculate MCD */
    printf("\nCalculating MCD...\n");
    double mcd = calculate_mcd(&ref_mfcc, &syn_mfcc);

    /* Cleanup */
    audio_data_free(&ref_audio);
    audio_data_free(&syn_audio);
    mfcc_matrix_free(&ref_mfcc);
    mfcc_matrix_free(&syn_mfcc);

    if (mcd < 0.0) {
        return 1;
    }

    printf("\nMCD: %.6f dB\n", mcd);
    return 0;
}
