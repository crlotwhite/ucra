/*
 * Create a golden WAV using the direct C API
 * Config: 44100 Hz, mono, 2.0s, MIDI 67 (G4), velocity 120
 * Output: golden_output.wav (32-bit float WAV, PCM IEEE float format)
 */

#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int write_wav_float32(const char* filename, const float* pcm, uint64_t frames,
                              uint32_t channels, uint32_t sample_rate) {
    if (!filename || !pcm || frames == 0 || channels == 0) return -1;
    FILE* f = fopen(filename, "wb");
    if (!f) return -2;

    uint32_t data_size = (uint32_t)(frames * channels * sizeof(float));
    uint32_t file_size = data_size + 36;

    /* RIFF */
    fwrite("RIFF", 1, 4, f);
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);

    /* fmt */
    fwrite("fmt ", 1, 4, f);
    uint32_t fmt_size = 16;
    uint16_t audio_format = 3; /* IEEE float */
    uint16_t num_channels = (uint16_t)channels;
    uint32_t byte_rate = sample_rate * channels * (uint32_t)sizeof(float);
    uint16_t block_align = (uint16_t)(channels * sizeof(float));
    uint16_t bits_per_sample = 32;
    fwrite(&fmt_size, 4, 1, f);
    fwrite(&audio_format, 2, 1, f);
    fwrite(&num_channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);

    /* data */
    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);
    fwrite(pcm, sizeof(float), frames * channels, f);

    fclose(f);
    return 0;
}

int main(void) {
    const uint32_t sample_rate = 44100;
    const uint32_t channels = 1;
    const double duration_sec = 2.0;

    UCRA_Handle eng = NULL;
    if (ucra_engine_create(&eng, NULL, 0) != UCRA_SUCCESS) {
        fprintf(stderr, "Failed to create engine\n");
        return 1;
    }

    UCRA_NoteSegment note = {0};
    note.start_sec = 0.0;
    note.duration_sec = duration_sec;
    note.midi_note = 67;
    note.velocity = 120;
    note.lyric = "sol";

    UCRA_RenderConfig cfg = {0};
    cfg.sample_rate = sample_rate;
    cfg.channels = channels;
    cfg.block_size = 512;
    cfg.notes = &note;
    cfg.note_count = 1;

    UCRA_RenderResult out = {0};
    UCRA_Result r = ucra_render(eng, &cfg, &out);
    if (r != UCRA_SUCCESS) {
        fprintf(stderr, "Render failed: %d\n", r);
        ucra_engine_destroy(eng);
        return 2;
    }

    if (!out.pcm || out.frames == 0) {
        fprintf(stderr, "No PCM output\n");
        ucra_engine_destroy(eng);
        return 3;
    }

    if (write_wav_float32("golden_output.wav", out.pcm, out.frames, out.channels, out.sample_rate) != 0) {
        fprintf(stderr, "Failed to write golden_output.wav\n");
        ucra_engine_destroy(eng);
        return 4;
    }

    printf("Golden WAV written: golden_output.wav\n");
    ucra_engine_destroy(eng);
    return 0;
}
