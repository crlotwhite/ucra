/*
 * Golden Runner Test Harness
 *
 * This harness discovers, manages, and executes a suite of test cases
 * for the UCRA rendering engine. It compares rendered outputs against
 * pre-recorded 'golden' reference files.
 *
 * Usage: golden_runner [test_directory]
 *
 * Test directory structure:
 *   test_case_001/
 *     input.json         - Render configuration
 *     expected_output.wav - Golden reference WAV
 *     f0_curve.txt       - Optional F0 curve for F0 RMSE test
 *   test_case_002/
 *     ...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "../third-party/cJSON.h"

#define MAX_PATH 512
#define MAX_TEST_CASES 1000
#define MAX_COMMAND_LENGTH 2048

/* Test case information */
typedef struct {
    char name[MAX_PATH];
    char directory[MAX_PATH];
    char input_config[MAX_PATH];
    char expected_wav[MAX_PATH];
    char f0_curve[MAX_PATH];
    char actual_output[MAX_PATH];
} TestCase;

/* Test suite */
typedef struct {
    TestCase* cases;
    int count;
    int capacity;
    char base_directory[MAX_PATH];
} TestSuite;

/* Test result */
typedef struct {
    char test_name[MAX_PATH];
    int passed;
    char error_message[512];
    double audio_diff_score;
    double f0_rmse;
    double mcd_score;
} TestResult;

/* Initialize test suite */
static int test_suite_init(TestSuite* suite, const char* base_dir) {
    suite->capacity = 100;
    suite->cases = malloc(suite->capacity * sizeof(TestCase));
    suite->count = 0;

    if (!suite->cases) {
        return -1;
    }

    strncpy(suite->base_directory, base_dir, MAX_PATH - 1);
    suite->base_directory[MAX_PATH - 1] = '\0';

    return 0;
}

/* Free test suite memory */
static void test_suite_free(TestSuite* suite) {
    if (suite->cases) {
        free(suite->cases);
        suite->cases = NULL;
    }
    suite->count = 0;
}

/* Check if file exists */
static int file_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

/* Check if directory exists */
static int dir_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

/* Add test case to suite */
static int add_test_case(TestSuite* suite, const char* test_name, const char* test_dir) {
    if (suite->count >= suite->capacity) {
        suite->capacity *= 2;
        TestCase* new_cases = realloc(suite->cases, suite->capacity * sizeof(TestCase));
        if (!new_cases) {
            return -1;
        }
        suite->cases = new_cases;
    }

    TestCase* test = &suite->cases[suite->count];

    /* Set basic info */
    strncpy(test->name, test_name, MAX_PATH - 1);
    test->name[MAX_PATH - 1] = '\0';

    strncpy(test->directory, test_dir, MAX_PATH - 1);
    test->directory[MAX_PATH - 1] = '\0';

    /* Build file paths */
    snprintf(test->input_config, MAX_PATH, "%s/input.json", test_dir);
    snprintf(test->expected_wav, MAX_PATH, "%s/expected_output.wav", test_dir);
    snprintf(test->f0_curve, MAX_PATH, "%s/f0_curve.txt", test_dir);
    snprintf(test->actual_output, MAX_PATH, "%s/actual_output.wav", test_dir);

    /* Validate required files */
    if (!file_exists(test->input_config)) {
        fprintf(stderr, "Warning: Missing input.json in test case '%s'\n", test_name);
        return 0; /* Skip this test case */
    }

    if (!file_exists(test->expected_wav)) {
        fprintf(stderr, "Warning: Missing expected_output.wav in test case '%s'\n", test_name);
        return 0; /* Skip this test case */
    }

    suite->count++;
    printf("Added test case: %s\n", test_name);
    return 1;
}

/* Discover test cases in directory */
static int discover_test_cases(TestSuite* suite) {
    DIR* dir = opendir(suite->base_directory);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open test directory '%s': %s\n",
                suite->base_directory, strerror(errno));
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Check if it's a directory */
        char test_dir[MAX_PATH];
        snprintf(test_dir, MAX_PATH, "%s/%s", suite->base_directory, entry->d_name);

        if (dir_exists(test_dir)) {
            add_test_case(suite, entry->d_name, test_dir);
        }
    }

    closedir(dir);

    printf("Discovered %d test cases\n", suite->count);
    return suite->count;
}

/* Execute rendering engine for a test case */
static int execute_render(const TestCase* test) {
    char command[MAX_COMMAND_LENGTH];

    /* Use the resampler CLI to render the test case */
    /* This assumes resampler is built and available */
    snprintf(command, MAX_COMMAND_LENGTH,
             "./resampler --config '%s' --output '%s'",
             test->input_config, test->actual_output);

    printf("Executing: %s\n", command);

    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Error: Rendering command failed with exit code %d\n", result);
        return -1;
    }

    /* Check if output file was created */
    if (!file_exists(test->actual_output)) {
        fprintf(stderr, "Error: Expected output file '%s' was not created\n", test->actual_output);
        return -1;
    }

    return 0;
}

/* Run audio comparison */
static int run_audio_comparison(const TestCase* test, TestResult* result) {
    char command[MAX_COMMAND_LENGTH];
    char temp_output[MAX_PATH];

    /* Create temporary file for audio comparison output */
    snprintf(temp_output, MAX_PATH, "%s/audio_diff.txt", test->directory);

    snprintf(command, MAX_COMMAND_LENGTH,
             "./audio_compare '%s' '%s' > '%s'",
             test->expected_wav, test->actual_output, temp_output);

    int exit_code = system(command);

    /* Read the comparison result */
    FILE* file = fopen(temp_output, "r");
    if (file) {
        char line[256];
        if (fgets(line, sizeof(line), file)) {
            if (strstr(line, "PASS")) {
                result->audio_diff_score = 0.0;
                return 0;
            } else if (strstr(line, "FAIL")) {
                sscanf(line, "FAIL: %lf", &result->audio_diff_score);
                return 1;
            }
        }
        fclose(file);
    }

    /* Remove temporary file */
    unlink(temp_output);

    return (exit_code == 0) ? 0 : 1;
}

/* Run F0 RMSE calculation */
static int run_f0_rmse(const TestCase* test, TestResult* result) {
    if (!file_exists(test->f0_curve)) {
        result->f0_rmse = -1.0; /* Not available */
        return 0;
    }

    char command[MAX_COMMAND_LENGTH];
    char temp_output[MAX_PATH];

    /* Create temporary file for F0 RMSE output */
    snprintf(temp_output, MAX_PATH, "%s/f0_rmse.txt", test->directory);

    /* We need another F0 curve from the actual output for comparison */
    /* For now, skip F0 RMSE if no reference curve */
    char actual_f0_curve[MAX_PATH];
    snprintf(actual_f0_curve, MAX_PATH, "%s/actual_f0_curve.txt", test->directory);

    if (!file_exists(actual_f0_curve)) {
        result->f0_rmse = -1.0; /* Not available */
        return 0;
    }

    snprintf(command, MAX_COMMAND_LENGTH,
             "./f0_rmse_calc '%s' '%s' > '%s'",
             test->f0_curve, actual_f0_curve, temp_output);

    int exit_code = system(command);

    /* Read the F0 RMSE result */
    FILE* file = fopen(temp_output, "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            if (strstr(line, "F0 RMSE:")) {
                sscanf(line, "F0 RMSE: %lf", &result->f0_rmse);
                break;
            }
        }
        fclose(file);
    }

    /* Remove temporary file */
    unlink(temp_output);

    return (exit_code == 0) ? 0 : 1;
}

/* Run MCD calculation */
static int run_mcd(const TestCase* test, TestResult* result) {
    char command[MAX_COMMAND_LENGTH];
    char temp_output[MAX_PATH];

    /* Create temporary file for MCD output */
    snprintf(temp_output, MAX_PATH, "%s/mcd.txt", test->directory);

    snprintf(command, MAX_COMMAND_LENGTH,
             "./mcd_calc '%s' '%s' > '%s'",
             test->expected_wav, test->actual_output, temp_output);

    int exit_code = system(command);

    /* Read the MCD result */
    FILE* file = fopen(temp_output, "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            if (strstr(line, "MCD:")) {
                sscanf(line, "MCD: %lf", &result->mcd_score);
                break;
            }
        }
        fclose(file);
    }

    /* Remove temporary file */
    unlink(temp_output);

    return (exit_code == 0) ? 0 : 1;
}

/* Run a single test case */
static TestResult run_test_case(const TestCase* test) {
    TestResult result = {0};
    strncpy(result.test_name, test->name, MAX_PATH - 1);
    result.audio_diff_score = -1.0;
    result.f0_rmse = -1.0;
    result.mcd_score = -1.0;

    printf("\n=== Running test case: %s ===\n", test->name);

    /* Step 1: Execute rendering */
    if (execute_render(test) < 0) {
        strcpy(result.error_message, "Rendering failed");
        result.passed = 0;
        return result;
    }

    /* Step 2: Audio comparison */
    int audio_pass = run_audio_comparison(test, &result);

    /* Step 3: F0 RMSE (optional) */
    run_f0_rmse(test, &result);

    /* Step 4: MCD calculation */
    run_mcd(test, &result);

    /* Determine overall pass/fail */
    result.passed = (audio_pass == 0);

    if (result.passed) {
        printf("✓ PASS: %s\n", test->name);
    } else {
        printf("✗ FAIL: %s\n", test->name);
        if (result.audio_diff_score >= 0) {
            printf("  Audio difference: %.6f\n", result.audio_diff_score);
        }
    }

    if (result.f0_rmse >= 0) {
        printf("  F0 RMSE: %.6f Hz\n", result.f0_rmse);
    }

    if (result.mcd_score >= 0) {
        printf("  MCD: %.6f dB\n", result.mcd_score);
    }

    return result;
}

/* Generate test report */
static void generate_report(const TestResult* results, int num_results) {
    int passed = 0;
    int failed = 0;

    printf("\n");
    printf("============================================================\n");
    printf("TEST REPORT\n");
    printf("============================================================\n");

    for (int i = 0; i < num_results; i++) {
        const TestResult* result = &results[i];

        printf("%-30s: %s\n", result->test_name, result->passed ? "PASS" : "FAIL");

        if (result->audio_diff_score >= 0) {
            printf("  Audio Diff: %.6f\n", result->audio_diff_score);
        }

        if (result->f0_rmse >= 0) {
            printf("  F0 RMSE: %.6f Hz\n", result->f0_rmse);
        }

        if (result->mcd_score >= 0) {
            printf("  MCD: %.6f dB\n", result->mcd_score);
        }

        if (!result->passed && strlen(result->error_message) > 0) {
            printf("  Error: %s\n", result->error_message);
        }

        printf("\n");

        if (result->passed) {
            passed++;
        } else {
            failed++;
        }
    }

    printf("============================================================\n");
    printf("Summary: %d passed, %d failed, %d total\n", passed, failed, num_results);
    printf("Success rate: %.1f%%\n", (100.0 * passed) / num_results);
    printf("============================================================\n");
}

static void print_usage(const char* program_name) {
    printf("Usage: %s [test_directory]\n", program_name);
    printf("\n");
    printf("Golden Runner Test Harness for UCRA\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  test_directory  Directory containing test cases (default: ./tests/data)\n");
    printf("\n");
    printf("Test case structure:\n");
    printf("  test_case_XXX/\n");
    printf("    input.json          - Render configuration (required)\n");
    printf("    expected_output.wav - Golden reference WAV (required)\n");
    printf("    f0_curve.txt        - F0 curve for RMSE test (optional)\n");
    printf("\n");
    printf("Prerequisites:\n");
    printf("  - resampler executable in current directory\n");
    printf("  - audio_compare executable in current directory\n");
    printf("  - f0_rmse_calc executable in current directory\n");
    printf("  - mcd_calc executable in current directory\n");
}

int main(int argc, char* argv[]) {
    const char* test_directory = (argc > 1) ? argv[1] : "./tests/data";

    if (argc > 2) {
        print_usage(argv[0]);
        return 1;
    }

    printf("Golden Runner Test Harness\n");
    printf("Test directory: %s\n\n", test_directory);

    /* Initialize test suite */
    TestSuite suite;
    if (test_suite_init(&suite, test_directory) < 0) {
        fprintf(stderr, "Error: Failed to initialize test suite\n");
        return 1;
    }

    /* Discover test cases */
    if (discover_test_cases(&suite) < 0) {
        test_suite_free(&suite);
        return 1;
    }

    if (suite.count == 0) {
        printf("No test cases found in directory '%s'\n", test_directory);
        test_suite_free(&suite);
        return 0;
    }

    /* Run test cases */
    TestResult* results = malloc(suite.count * sizeof(TestResult));
    if (!results) {
        fprintf(stderr, "Error: Memory allocation failed for test results\n");
        test_suite_free(&suite);
        return 1;
    }

    for (int i = 0; i < suite.count; i++) {
        results[i] = run_test_case(&suite.cases[i]);
    }

    /* Generate report */
    generate_report(results, suite.count);

    /* Cleanup */
    free(results);
    test_suite_free(&suite);

    return 0;
}
