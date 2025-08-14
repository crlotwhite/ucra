/*
 * UCRA Validation Suite
 *
 * This is the main orchestration script that combines the Golden Runner harness,
 * audio comparison, F0 RMSE, and MCD calculation utilities into a single,
 * automated command-line tool that produces a consolidated test report.
 *
 * Usage: validation_suite [options] [test_directory]
 *
 * Options:
 *   --config FILE    Use configuration file
 *   --output FILE    Output report to file (default: console)
 *   --format FORMAT  Report format: console, json, markdown (default: console)
 *   --parallel N     Run N test cases in parallel (default: 1)
 *   --verbose        Enable verbose output
 *   --help           Show this help message
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "../third-party/cJSON.h"

#define MAX_PATH 512
#define MAX_COMMAND_LENGTH 2048
#define VERSION "1.0.0"

/* Configuration structure */
typedef struct {
    char test_directory[MAX_PATH];
    char output_file[MAX_PATH];
    char format[32];
    int parallel_jobs;
    int verbose;
    char config_file[MAX_PATH];
} Config;

/* Test suite statistics */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
    double total_runtime;
    char start_time[64];
    char end_time[64];
} SuiteStats;

/* Validation result for a single test */
typedef struct {
    char test_name[MAX_PATH];
    int passed;
    double runtime;
    char error_message[512];

    /* Metrics */
    int audio_comparison_passed;
    double audio_diff_score;
    double f0_rmse;
    double mcd_score;

    /* File paths */
    char input_config[MAX_PATH];
    char expected_output[MAX_PATH];
    char actual_output[MAX_PATH];
} ValidationResult;

/* Initialize configuration with defaults */
static void config_init(Config* config) {
    strcpy(config->test_directory, "./tests/data");
    strcpy(config->output_file, "");  /* Empty means console output */
    strcpy(config->format, "console");
    config->parallel_jobs = 1;
    config->verbose = 0;
    strcpy(config->config_file, "");
}

/* Load configuration from JSON file */
static int load_config_file(Config* config, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open config file '%s': %s\n",
                filename, strerror(errno));
        return -1;
    }

    /* Read file content */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return -1;
    }

    fread(content, 1, file_size, file);
    content[file_size] = '\0';
    fclose(file);

    /* Parse JSON */
    cJSON* json = cJSON_Parse(content);
    free(content);

    if (!json) {
        fprintf(stderr, "Error: Invalid JSON in config file '%s'\n", filename);
        return -1;
    }

    /* Extract configuration values */
    cJSON* item;

    if ((item = cJSON_GetObjectItem(json, "test_directory")) && cJSON_IsString(item)) {
        strncpy(config->test_directory, item->valuestring, MAX_PATH - 1);
    }

    if ((item = cJSON_GetObjectItem(json, "output_file")) && cJSON_IsString(item)) {
        strncpy(config->output_file, item->valuestring, MAX_PATH - 1);
    }

    if ((item = cJSON_GetObjectItem(json, "format")) && cJSON_IsString(item)) {
        strncpy(config->format, item->valuestring, 31);
    }

    if ((item = cJSON_GetObjectItem(json, "parallel_jobs")) && cJSON_IsNumber(item)) {
        config->parallel_jobs = item->valueint;
    }

    if ((item = cJSON_GetObjectItem(json, "verbose")) && cJSON_IsBool(item)) {
        config->verbose = cJSON_IsTrue(item);
    }

    cJSON_Delete(json);
    return 0;
}

/* Get current timestamp */
static void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* Check if required executables exist */
static int check_prerequisites(const Config* config) {
    const char* required_tools[] = {
        "./golden_runner",
        "./audio_compare",
        "./f0_rmse_calc",
        "./mcd_calc",
        "./resampler"
    };

    int missing_tools = 0;

    for (int i = 0; i < 5; i++) {
        if (access(required_tools[i], X_OK) != 0) {
            fprintf(stderr, "Error: Required executable '%s' not found or not executable\n",
                    required_tools[i]);
            missing_tools++;
        }
    }

    if (missing_tools > 0) {
        fprintf(stderr, "\nPlease ensure all validation tools are built and available.\n");
        fprintf(stderr, "Run 'make' or 'cmake --build .' to build the tools.\n");
        return -1;
    }

    return 0;
}

/* Run the golden runner and capture results */
static int run_validation_suite(const Config* config, SuiteStats* stats) {
    char command[MAX_COMMAND_LENGTH];
    char temp_output[MAX_PATH];

    /* Create temporary output file */
    snprintf(temp_output, MAX_PATH, "/tmp/ucra_validation_%d.log", getpid());

    /* Build command */
    snprintf(command, MAX_COMMAND_LENGTH,
             "./golden_runner '%s' > '%s' 2>&1",
             config->test_directory, temp_output);

    if (config->verbose) {
        printf("Executing: %s\n", command);
    }

    /* Record start time */
    get_timestamp(stats->start_time, sizeof(stats->start_time));
    clock_t start_clock = clock();

    /* Execute command */
    int exit_code = system(command);

    /* Record end time */
    clock_t end_clock = clock();
    get_timestamp(stats->end_time, sizeof(stats->end_time));
    stats->total_runtime = (double)(end_clock - start_clock) / CLOCKS_PER_SEC;

    /* Parse output for statistics */
    FILE* output_file = fopen(temp_output, "r");
    if (output_file) {
        char line[512];
        while (fgets(line, sizeof(line), output_file)) {
            if (config->verbose) {
                printf("%s", line);
            }

            /* Parse summary line */
            if (strstr(line, "Summary:")) {
                sscanf(line, "Summary: %d passed, %d failed, %d total",
                       &stats->passed_tests, &stats->failed_tests, &stats->total_tests);
            }
        }
        fclose(output_file);
    }

    /* Cleanup temporary file */
    unlink(temp_output);

    return (exit_code == 0) ? 0 : -1;
}

/* Generate console report */
static void generate_console_report(const Config* config, const SuiteStats* stats) {
    printf("\n");
    printf("============================================================\n");
    printf("UCRA Validation Suite Report\n");
    printf("============================================================\n");
    printf("Version:         %s\n", VERSION);
    printf("Test Directory:  %s\n", config->test_directory);
    printf("Start Time:      %s\n", stats->start_time);
    printf("End Time:        %s\n", stats->end_time);
    printf("Total Runtime:   %.2f seconds\n", stats->total_runtime);
    printf("------------------------------------------------------------\n");
    printf("Test Results:\n");
    printf("  Total Tests:   %d\n", stats->total_tests);
    printf("  Passed:        %d\n", stats->passed_tests);
    printf("  Failed:        %d\n", stats->failed_tests);
    printf("  Skipped:       %d\n", stats->skipped_tests);

    if (stats->total_tests > 0) {
        double success_rate = (100.0 * stats->passed_tests) / stats->total_tests;
        printf("  Success Rate:  %.1f%%\n", success_rate);
    }

    printf("------------------------------------------------------------\n");

    if (stats->failed_tests == 0) {
        printf("Status: ALL TESTS PASSED ✓\n");
    } else {
        printf("Status: %d TEST(S) FAILED ✗\n", stats->failed_tests);
    }

    printf("============================================================\n");
}

/* Generate JSON report */
static void generate_json_report(const Config* config, const SuiteStats* stats) {
    cJSON* report = cJSON_CreateObject();
    cJSON* metadata = cJSON_CreateObject();
    cJSON* results = cJSON_CreateObject();

    /* Metadata */
    cJSON_AddStringToObject(metadata, "version", VERSION);
    cJSON_AddStringToObject(metadata, "test_directory", config->test_directory);
    cJSON_AddStringToObject(metadata, "start_time", stats->start_time);
    cJSON_AddStringToObject(metadata, "end_time", stats->end_time);
    cJSON_AddNumberToObject(metadata, "total_runtime", stats->total_runtime);

    /* Results */
    cJSON_AddNumberToObject(results, "total_tests", stats->total_tests);
    cJSON_AddNumberToObject(results, "passed_tests", stats->passed_tests);
    cJSON_AddNumberToObject(results, "failed_tests", stats->failed_tests);
    cJSON_AddNumberToObject(results, "skipped_tests", stats->skipped_tests);

    if (stats->total_tests > 0) {
        double success_rate = (100.0 * stats->passed_tests) / stats->total_tests;
        cJSON_AddNumberToObject(results, "success_rate", success_rate);
    }

    cJSON_AddItemToObject(report, "metadata", metadata);
    cJSON_AddItemToObject(report, "results", results);

    char* json_string = cJSON_Print(report);
    printf("%s\n", json_string);

    free(json_string);
    cJSON_Delete(report);
}

/* Generate Markdown report */
static void generate_markdown_report(const Config* config, const SuiteStats* stats) {
    printf("# UCRA Validation Suite Report\n\n");
    printf("## Test Configuration\n\n");
    printf("- **Version**: %s\n", VERSION);
    printf("- **Test Directory**: `%s`\n", config->test_directory);
    printf("- **Start Time**: %s\n", stats->start_time);
    printf("- **End Time**: %s\n", stats->end_time);
    printf("- **Total Runtime**: %.2f seconds\n\n", stats->total_runtime);

    printf("## Test Results\n\n");
    printf("| Metric | Value |\n");
    printf("|--------|-------|\n");
    printf("| Total Tests | %d |\n", stats->total_tests);
    printf("| Passed | %d |\n", stats->passed_tests);
    printf("| Failed | %d |\n", stats->failed_tests);
    printf("| Skipped | %d |\n", stats->skipped_tests);

    if (stats->total_tests > 0) {
        double success_rate = (100.0 * stats->passed_tests) / stats->total_tests;
        printf("| Success Rate | %.1f%% |\n", success_rate);
    }

    printf("\n## Status\n\n");
    if (stats->failed_tests == 0) {
        printf("✅ **ALL TESTS PASSED**\n");
    } else {
        printf("❌ **%d TEST(S) FAILED**\n", stats->failed_tests);
    }
}

/* Save report to file */
static int save_report_to_file(const Config* config, const SuiteStats* stats) {
    FILE* file = fopen(config->output_file, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot open output file '%s': %s\n",
                config->output_file, strerror(errno));
        return -1;
    }

    /* Redirect stdout to file temporarily */
    FILE* old_stdout = stdout;
    stdout = file;

    if (strcmp(config->format, "json") == 0) {
        generate_json_report(config, stats);
    } else if (strcmp(config->format, "markdown") == 0) {
        generate_markdown_report(config, stats);
    } else {
        generate_console_report(config, stats);
    }

    /* Restore stdout */
    stdout = old_stdout;
    fclose(file);

    printf("Report saved to: %s\n", config->output_file);
    return 0;
}

static void print_usage(const char* program_name) {
    printf("Usage: %s [options] [test_directory]\n", program_name);
    printf("\n");
    printf("UCRA Validation Suite - Comprehensive testing framework\n");
    printf("\n");
    printf("Options:\n");
    printf("  --config FILE    Use configuration file\n");
    printf("  --output FILE    Output report to file (default: console)\n");
    printf("  --format FORMAT  Report format: console, json, markdown (default: console)\n");
    printf("  --parallel N     Run N test cases in parallel (default: 1)\n");
    printf("  --verbose        Enable verbose output\n");
    printf("  --help           Show this help message\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  test_directory   Directory containing test cases (default: ./tests/data)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                           # Run with defaults\n", program_name);
    printf("  %s --verbose ./my_tests      # Verbose mode with custom directory\n", program_name);
    printf("  %s --output report.json --format json # JSON report to file\n", program_name);
    printf("  %s --config validation.json  # Use configuration file\n", program_name);
}

int main(int argc, char* argv[]) {
    Config config;
    config_init(&config);

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            strncpy(config.config_file, argv[++i], MAX_PATH - 1);
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            strncpy(config.output_file, argv[++i], MAX_PATH - 1);
        } else if (strcmp(argv[i], "--format") == 0 && i + 1 < argc) {
            strncpy(config.format, argv[++i], 31);
        } else if (strcmp(argv[i], "--parallel") == 0 && i + 1 < argc) {
            config.parallel_jobs = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config.verbose = 1;
        } else if (argv[i][0] != '-') {
            /* Test directory argument */
            strncpy(config.test_directory, argv[i], MAX_PATH - 1);
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Load configuration file if specified */
    if (strlen(config.config_file) > 0) {
        if (load_config_file(&config, config.config_file) < 0) {
            return 1;
        }
    }

    /* Validate format */
    if (strcmp(config.format, "console") != 0 &&
        strcmp(config.format, "json") != 0 &&
        strcmp(config.format, "markdown") != 0) {
        fprintf(stderr, "Error: Invalid format '%s'. Must be: console, json, or markdown\n",
                config.format);
        return 1;
    }

    printf("UCRA Validation Suite v%s\n", VERSION);
    if (config.verbose) {
        printf("Configuration:\n");
        printf("  Test Directory: %s\n", config.test_directory);
        printf("  Output Format:  %s\n", config.format);
        printf("  Parallel Jobs:  %d\n", config.parallel_jobs);
        printf("  Verbose Mode:   %s\n", config.verbose ? "enabled" : "disabled");
        if (strlen(config.output_file) > 0) {
            printf("  Output File:    %s\n", config.output_file);
        }
        printf("\n");
    }

    /* Check prerequisites */
    if (check_prerequisites(&config) < 0) {
        return 1;
    }

    /* Run validation suite */
    SuiteStats stats = {0};
    if (run_validation_suite(&config, &stats) < 0) {
        fprintf(stderr, "Error: Validation suite execution failed\n");
        return 1;
    }

    /* Generate report */
    if (strlen(config.output_file) > 0) {
        if (save_report_to_file(&config, &stats) < 0) {
            return 1;
        }
    } else {
        if (strcmp(config.format, "json") == 0) {
            generate_json_report(&config, &stats);
        } else if (strcmp(config.format, "markdown") == 0) {
            generate_markdown_report(&config, &stats);
        } else {
            generate_console_report(&config, &stats);
        }
    }

    /* Return appropriate exit code */
    return (stats.failed_tests == 0) ? 0 : 1;
}
