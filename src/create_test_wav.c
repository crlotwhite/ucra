/**
 * @file create_test_wav.c
 * @brief Simple WAV file generator for testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    char riff_header[4];
    uint32_t wav_size;
    char wave_header[4];
    char fmt_header[4];
    uint32_t fmt_chunk_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_header[4];
    uint32_t data_bytes;
} WAVHeader;

void create_test_wav(const char* filename, double frequency, double duration, int sample_rate) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return;
    }

    int num_samples = (int)(duration * sample_rate);
    int data_size = num_samples * sizeof(int16_t);

    WAVHeader header = {0};
    memcpy(header.riff_header, "RIFF", 4);
    header.wav_size = 36 + data_size;
    memcpy(header.wave_header, "WAVE", 4);
    memcpy(header.fmt_header, "fmt ", 4);
    header.fmt_chunk_size = 16;
    header.audio_format = 1; // PCM
    header.num_channels = 1;
    header.sample_rate = sample_rate;
    header.bits_per_sample = 16;
    header.byte_rate = sample_rate * header.num_channels * header.bits_per_sample / 8;
    header.block_align = header.num_channels * header.bits_per_sample / 8;
    memcpy(header.data_header, "data", 4);
    header.data_bytes = data_size;

    fwrite(&header, sizeof(WAVHeader), 1, file);

    // Generate sine wave
    for (int i = 0; i < num_samples; i++) {
        double t = (double)i / sample_rate;
        double sample = sin(2.0 * M_PI * frequency * t);
        int16_t value = (int16_t)(sample * 32767 * 0.8); // 80% amplitude
        fwrite(&value, sizeof(int16_t), 1, file);
    }

    fclose(file);
    printf("Created %s: %.1f Hz, %.1f sec, %d Hz\n", filename, frequency, duration, sample_rate);
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        printf("Usage: %s <output.wav> <frequency> <duration> <sample_rate>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    double frequency = atof(argv[2]);
    double duration = atof(argv[3]);
    int sample_rate = atoi(argv[4]);

    create_test_wav(filename, frequency, duration, sample_rate);
    return 0;
}
