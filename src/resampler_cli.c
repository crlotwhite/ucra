/*
 * UCRA Legacy CLI Bridge (resampler.exe)
 * Drop-in replacement for UTAU resamplers with CLI compatibility
 */

#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Cross-platform argument parsing (Windows compatible) */
static int ucra_find_arg(int argc, char* argv[], const char* short_opt, const char* long_opt, char** value) {
    *value = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], short_opt) == 0 || strcmp(argv[i], long_opt) == 0) {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                *value = argv[i + 1];
                return i;
            } else {
                return i; /* Found flag but no value */
            }
        }
    }
    return 0; /* Not found */
}

static int ucra_has_flag(int argc, char* argv[], const char* short_opt, const char* long_opt) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], short_opt) == 0 || strcmp(argv[i], long_opt) == 0) {
            return 1;
        }
    }
    return 0;
}

/* CLI argument structure */
typedef struct UCRA_CLIArgs {
    char* input_wav;        /* --input */
    char* output_wav;       /* --output */
    char* note_info;        /* --note */
    double tempo;           /* --tempo */
    char* flags_str;        /* --flags */
    char* f0_curve_file;    /* --f0-curve */
    char* vb_root;          /* --vb-root */
    char* oto_file;         /* --oto */
    uint32_t sample_rate;   /* --rate */

    /* Parsed note information */
    char* lyric;
    int16_t midi_note;
    double start_sec;
    double duration_sec;
    uint8_t velocity;
} UCRA_CLIArgs;

/* Initialize CLI args structure */
static void ucra_cli_args_init(UCRA_CLIArgs* args) {
    memset(args, 0, sizeof(UCRA_CLIArgs));
    args->tempo = 120.0;        /* default tempo */
    args->sample_rate = 44100;  /* default sample rate */
    args->midi_note = 60;       /* default to C4 */
    args->velocity = 100;       /* default velocity */
    args->duration_sec = 1.0;   /* default duration */
}

/* Free CLI args structure */
static void ucra_cli_args_free(UCRA_CLIArgs* args) {
    if (!args) return;

    free(args->input_wav);
    free(args->output_wav);
    free(args->note_info);
    free(args->flags_str);
    free(args->f0_curve_file);
    free(args->vb_root);
    free(args->oto_file);
    free(args->lyric);

    memset(args, 0, sizeof(UCRA_CLIArgs));
}

/* Parse note information string */
static UCRA_Result ucra_parse_note_info(const char* note_str, UCRA_CLIArgs* args) {
    if (!note_str || !args) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    /* UTAU note format: "lyric note velocity" or more complex formats */
    char* note_copy = strdup(note_str);
    if (!note_copy) {
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    char* token;
    int token_count = 0;

    /* Simple parsing: space-separated values */
    token = strtok(note_copy, " \t");
    while (token && token_count < 3) {
        switch (token_count) {
            case 0: /* lyric */
                args->lyric = strdup(token);
                break;
            case 1: /* MIDI note */
                args->midi_note = (int16_t)atoi(token);
                if (args->midi_note < 0 || args->midi_note > 127) {
                    args->midi_note = 60; /* default to C4 */
                }
                break;
            case 2: /* velocity */
                args->velocity = (uint8_t)atoi(token);
                if (args->velocity > 127) {
                    args->velocity = 100; /* default velocity */
                }
                break;
        }
        token = strtok(NULL, " \t");
        token_count++;
    }

    free(note_copy);

    /* If no lyric was provided, use default */
    if (!args->lyric) {
        args->lyric = strdup("a");
    }

    return UCRA_SUCCESS;
}

/* Print usage information */
static void ucra_print_usage(const char* program_name) {
    printf("UCRA Legacy CLI Bridge v1.0\n");
    printf("Usage: %s [options]\n\n", program_name);
    printf("Required options:\n");
    printf("  -i, --input PATH        Input WAV file path\n");
    printf("  -o, --output PATH       Output WAV file path\n");
    printf("  -n, --note INFO         Note information (lyric midi_note velocity)\n");
    printf("  -v, --vb-root PATH      Voicebank root directory\n\n");
    printf("Optional options:\n");
    printf("  -t, --tempo BPM         Tempo in BPM (default: 120)\n");
    printf("  -f, --flags FLAGS       Engine-specific flags\n");
    printf("  -c, --f0-curve PATH     F0 curve file path\n");
    printf("  -O, --oto PATH          OTO configuration file\n");
    printf("  -r, --rate RATE         Sample rate (default: 44100)\n");
    printf("  -h, --help              Show this help message\n\n");
    printf("Example:\n");
    printf("  %s -i input.wav -o output.wav -n \"a 60 100\" -v /path/to/voicebank\n", program_name);
}

/* Parse command line arguments using cross-platform approach */
static UCRA_Result ucra_parse_cli_args(int argc, char* argv[], UCRA_CLIArgs* args) {
    if (!args) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    /* Check for help first */
    if (ucra_has_flag(argc, argv, "-h", "--help")) {
        ucra_print_usage(argv[0]);
        return UCRA_ERR_INVALID_ARGUMENT; /* Use as "help requested" signal */
    }

    char* value;

    /* Parse required arguments */
    if (ucra_find_arg(argc, argv, "-i", "--input", &value) && value) {
        args->input_wav = strdup(value);
    }

    if (ucra_find_arg(argc, argv, "-o", "--output", &value) && value) {
        args->output_wav = strdup(value);
    }

    if (ucra_find_arg(argc, argv, "-n", "--note", &value) && value) {
        args->note_info = strdup(value);
    }

    if (ucra_find_arg(argc, argv, "-v", "--vb-root", &value) && value) {
        args->vb_root = strdup(value);
    }

    /* Parse optional arguments */
    if (ucra_find_arg(argc, argv, "-t", "--tempo", &value) && value) {
        args->tempo = atof(value);
        if (args->tempo <= 0.0) {
            args->tempo = 120.0;
        }
    }

    if (ucra_find_arg(argc, argv, "-f", "--flags", &value) && value) {
        args->flags_str = strdup(value);
    }

    if (ucra_find_arg(argc, argv, "-c", "--f0-curve", &value) && value) {
        args->f0_curve_file = strdup(value);
    }

    if (ucra_find_arg(argc, argv, "-O", "--oto", &value) && value) {
        args->oto_file = strdup(value);
    }

    if (ucra_find_arg(argc, argv, "-r", "--rate", &value) && value) {
        args->sample_rate = (uint32_t)atoi(value);
        if (args->sample_rate < 8000 || args->sample_rate > 192000) {
            args->sample_rate = 44100;
        }
    }

    /* Validate required arguments */
    if (!args->input_wav) {
        fprintf(stderr, "Error: Input WAV file is required (--input)\n");
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    if (!args->output_wav) {
        fprintf(stderr, "Error: Output WAV file is required (--output)\n");
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    if (!args->note_info) {
        fprintf(stderr, "Error: Note information is required (--note)\n");
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    if (!args->vb_root) {
        fprintf(stderr, "Error: Voicebank root directory is required (--vb-root)\n");
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    /* Parse note information */
    UCRA_Result result = ucra_parse_note_info(args->note_info, args);
    if (result != UCRA_SUCCESS) {
        fprintf(stderr, "Error: Failed to parse note information\n");
        return result;
    }

    return UCRA_SUCCESS;
}

/* Load manifest from voicebank directory */
static UCRA_Result ucra_load_manifest_from_vb(const char* vb_root, UCRA_Manifest** manifest) {
    if (!vb_root || !manifest) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    /* Construct manifest path: vb_root/resampler.json */
    size_t path_len = strlen(vb_root) + strlen("/resampler.json") + 1;
    char* manifest_path = malloc(path_len);
    if (!manifest_path) {
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    snprintf(manifest_path, path_len, "%s/resampler.json", vb_root);

    UCRA_Result result = ucra_manifest_load(manifest_path, manifest);
    free(manifest_path);

    if (result != UCRA_SUCCESS) {
        fprintf(stderr, "Error: Failed to load manifest from %s (error: %d)\n", vb_root, result);
    }

    return result;
}

/* Load F0 curve from file */
static UCRA_Result ucra_load_f0_curve(const char* f0_file, UCRA_F0Curve* curve) {
    if (!f0_file || !curve) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    FILE* file = fopen(f0_file, "r");
    if (!file) {
        return UCRA_ERR_FILE_NOT_FOUND;
    }

    /* Count lines first */
    uint32_t line_count = 0;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) {
        line_count++;
    }

    if (line_count == 0) {
        fclose(file);
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    /* Allocate arrays */
    float* time_sec = malloc(line_count * sizeof(float));
    float* f0_hz = malloc(line_count * sizeof(float));
    if (!time_sec || !f0_hz) {
        free(time_sec);
        free(f0_hz);
        fclose(file);
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    /* Read data */
    rewind(file);
    uint32_t valid_count = 0;
    while (fgets(buffer, sizeof(buffer), file) && valid_count < line_count) {
        float time, f0;
        if (sscanf(buffer, "%f %f", &time, &f0) == 2) {
            time_sec[valid_count] = time;
            f0_hz[valid_count] = f0;
            valid_count++;
        }
    }

    fclose(file);

    if (valid_count == 0) {
        free(time_sec);
        free(f0_hz);
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    curve->time_sec = time_sec;
    curve->f0_hz = f0_hz;
    curve->length = valid_count;

    return UCRA_SUCCESS;
}

/* Convert CLI args to UCRA_NoteSegment */
static UCRA_Result ucra_cli_to_note_segment(const UCRA_CLIArgs* args, UCRA_NoteSegment* note, UCRA_F0Curve* f0_curve) {
    if (!args || !note) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    memset(note, 0, sizeof(UCRA_NoteSegment));

    note->start_sec = args->start_sec;
    note->duration_sec = args->duration_sec;
    note->midi_note = args->midi_note;
    note->velocity = args->velocity;
    note->lyric = args->lyric;

    /* Load F0 curve if provided */
    if (args->f0_curve_file && f0_curve) {
        UCRA_Result result = ucra_load_f0_curve(args->f0_curve_file, f0_curve);
        if (result == UCRA_SUCCESS) {
            note->f0_override = f0_curve;
        } else {
            fprintf(stderr, "Warning: Failed to load F0 curve from %s\n", args->f0_curve_file);
        }
    }

    return UCRA_SUCCESS;
}

/* Convert CLI args to UCRA_RenderConfig */
static UCRA_Result ucra_cli_to_render_config(const UCRA_CLIArgs* args, const UCRA_NoteSegment* note,
                                             UCRA_RenderConfig* config, UCRA_KeyValue* options) {
    if (!args || !note || !config) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    memset(config, 0, sizeof(UCRA_RenderConfig));

    config->sample_rate = args->sample_rate;
    config->channels = 1; /* Mono output for compatibility */
    config->block_size = 512; /* Default block size */
    config->flags = 0;

    config->notes = note;
    config->note_count = 1;

    /* Parse engine flags if provided */
    if (args->flags_str && options) {
        /* Simple key=value parsing for demonstration */
        /* TODO: Implement more sophisticated flag parsing */
        config->options = options;
        config->option_count = 1;
        options[0].key = "flags";
        options[0].value = args->flags_str;
    }

    return UCRA_SUCCESS;
}

/* Write PCM data to WAV file (simplified implementation) */
static UCRA_Result ucra_write_wav_file(const char* filename, const float* pcm, uint64_t frames,
                                       uint32_t channels, uint32_t sample_rate) {
    if (!filename || !pcm || frames == 0) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    FILE* file = fopen(filename, "wb");
    if (!file) {
        return UCRA_ERR_FILE_NOT_FOUND;
    }

    /* Write basic WAV header (simplified) */
    uint32_t data_size = (uint32_t)(frames * channels * sizeof(float));
    uint32_t file_size = data_size + 36;

    /* RIFF header */
    fwrite("RIFF", 1, 4, file);
    fwrite(&file_size, 4, 1, file);
    fwrite("WAVE", 1, 4, file);

    /* fmt chunk */
    fwrite("fmt ", 1, 4, file);
    uint32_t fmt_size = 16;
    uint16_t audio_format = 3; /* IEEE float */
    uint16_t num_channels = (uint16_t)channels;
    uint32_t byte_rate = sample_rate * channels * sizeof(float);
    uint16_t block_align = (uint16_t)(channels * sizeof(float));
    uint16_t bits_per_sample = 32;

    fwrite(&fmt_size, 4, 1, file);
    fwrite(&audio_format, 2, 1, file);
    fwrite(&num_channels, 2, 1, file);
    fwrite(&sample_rate, 4, 1, file);
    fwrite(&byte_rate, 4, 1, file);
    fwrite(&block_align, 2, 1, file);
    fwrite(&bits_per_sample, 2, 1, file);

    /* data chunk */
    fwrite("data", 1, 4, file);
    fwrite(&data_size, 4, 1, file);
    fwrite(pcm, sizeof(float), frames * channels, file);

    fclose(file);
    return UCRA_SUCCESS;
}

/* Main CLI bridge function */
int main(int argc, char* argv[]) {
    UCRA_CLIArgs args;
    UCRA_Manifest* manifest = NULL;
    UCRA_Handle engine = NULL;
    UCRA_F0Curve f0_curve = {0};
    UCRA_KeyValue options[8] = {0}; /* Space for engine options */

    ucra_cli_args_init(&args);

    /* Parse command line arguments */
    UCRA_Result result = ucra_parse_cli_args(argc, argv, &args);
    if (result != UCRA_SUCCESS) {
        ucra_cli_args_free(&args);
        return (result == UCRA_ERR_INVALID_ARGUMENT) ? 1 : 2; /* 1 for help, 2 for error */
    }

    printf("UCRA Legacy CLI Bridge\n");
    printf("Input: %s\n", args.input_wav);
    printf("Output: %s\n", args.output_wav);
    printf("Note: %s (MIDI %d, Vel %d)\n", args.lyric, args.midi_note, args.velocity);
    printf("Voicebank: %s\n", args.vb_root);

    /* Load manifest from voicebank */
    result = ucra_load_manifest_from_vb(args.vb_root, &manifest);
    if (result != UCRA_SUCCESS) {
        fprintf(stderr, "Error: Failed to load manifest (error %d)\n", result);
        ucra_cli_args_free(&args);
        return 3;
    }

    printf("Loaded engine: %s v%s by %s\n",
           manifest->name ? manifest->name : "Unknown",
           manifest->version ? manifest->version : "Unknown",
           manifest->vendor ? manifest->vendor : "Unknown");

    /* Create engine (placeholder - would need actual engine implementation) */
    printf("Note: Engine creation and rendering not yet implemented\n");
    printf("      This is a placeholder CLI bridge demonstration\n");

    /* Convert CLI arguments to UCRA structures */
    UCRA_NoteSegment note;
    result = ucra_cli_to_note_segment(&args, &note, &f0_curve);
    if (result != UCRA_SUCCESS) {
        fprintf(stderr, "Error: Failed to convert note data (error %d)\n", result);
        ucra_manifest_free(manifest);
        ucra_cli_args_free(&args);
        return 4;
    }

    UCRA_RenderConfig config;
    result = ucra_cli_to_render_config(&args, &note, &config, options);
    if (result != UCRA_SUCCESS) {
        fprintf(stderr, "Error: Failed to create render config (error %d)\n", result);
        ucra_manifest_free(manifest);
        ucra_cli_args_free(&args);
        return 5;
    }

    printf("✓ Successfully parsed CLI arguments\n");
    printf("✓ Successfully loaded manifest\n");
    printf("✓ Successfully converted to UCRA structures\n");
    printf("✓ CLI bridge framework is working\n");

    /* TODO: Implement actual engine creation and rendering */
    /* For now, create a simple test tone as placeholder output */
    const uint64_t test_frames = (uint64_t)(args.sample_rate * args.duration_sec);
    float* test_pcm = malloc(test_frames * sizeof(float));
    if (test_pcm) {
        /* Generate simple sine wave at the specified MIDI note frequency */
        double frequency = 440.0 * pow(2.0, (args.midi_note - 69) / 12.0);
        for (uint64_t i = 0; i < test_frames; i++) {
            double t = (double)i / args.sample_rate;
            test_pcm[i] = 0.5f * sin(2.0 * M_PI * frequency * t);
        }

        result = ucra_write_wav_file(args.output_wav, test_pcm, test_frames, 1, args.sample_rate);
        if (result == UCRA_SUCCESS) {
            printf("✓ Test WAV file written to %s\n", args.output_wav);
        } else {
            fprintf(stderr, "Error: Failed to write WAV file (error %d)\n", result);
        }

        free(test_pcm);
    }

    /* Cleanup */
    if (f0_curve.time_sec) {
        free((void*)f0_curve.time_sec);
        free((void*)f0_curve.f0_hz);
    }

    ucra_manifest_free(manifest);
    ucra_cli_args_free(&args);

    printf("UCRA CLI Bridge completed successfully\n");
    return 0;
}
