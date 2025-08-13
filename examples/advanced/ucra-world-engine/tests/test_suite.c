/*
 * UCRA Manifest Parser Comprehensive Test Suite
 */

#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int test_count = 0;
static int passed_count = 0;

#define TEST(name) \
    do { \
        test_count++; \
        printf("Running test: " #name " ... "); \
        if (test_##name()) { \
            printf("PASSED\n"); \
            passed_count++; \
        } else { \
            printf("FAILED\n"); \
        } \
    } while(0)

/* Test valid manifest parsing */
static int test_valid_manifest() {
    UCRA_Manifest* manifest = NULL;
    UCRA_Result result = ucra_manifest_load("data/example_manifest.json", &manifest);

    if (result != UCRA_SUCCESS || !manifest) {
        return 0;
    }

    /* Verify basic fields */
    if (!manifest->name || strcmp(manifest->name, "Example UCRA Engine") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (!manifest->version || strcmp(manifest->version, "1.0.0") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (!manifest->vendor || strcmp(manifest->vendor, "UCRA Project") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (!manifest->license || strcmp(manifest->license, "MIT") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    /* Verify entry */
    if (!manifest->entry.type || strcmp(manifest->entry.type, "dll") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (!manifest->entry.path || strcmp(manifest->entry.path, "./libexample.so") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (!manifest->entry.symbol || strcmp(manifest->entry.symbol, "ucra_entry") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    /* Verify audio */
    if (manifest->audio.rates_count != 2) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (manifest->audio.rates[0] != 44100 || manifest->audio.rates[1] != 48000) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (manifest->audio.channels_count != 2) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (manifest->audio.channels[0] != 1 || manifest->audio.channels[1] != 2) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (!manifest->audio.streaming) {
        ucra_manifest_free(manifest);
        return 0;
    }

    /* Verify flags */
    if (manifest->flags_count != 4) {
        ucra_manifest_free(manifest);
        return 0;
    }

    /* Check first flag (g) */
    const UCRA_ManifestFlag* flag_g = &manifest->flags[0];
    if (!flag_g->key || strcmp(flag_g->key, "g") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (!flag_g->type || strcmp(flag_g->type, "float") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (!flag_g->range || flag_g->range[0] != -12.0f || flag_g->range[1] != 12.0f) {
        ucra_manifest_free(manifest);
        return 0;
    }

    /* Check enum flag (algo) */
    const UCRA_ManifestFlag* flag_algo = &manifest->flags[2];
    if (!flag_algo->key || strcmp(flag_algo->key, "algo") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (!flag_algo->type || strcmp(flag_algo->type, "enum") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (flag_algo->values_count != 3) {
        ucra_manifest_free(manifest);
        return 0;
    }

    if (!flag_algo->values[0] || strcmp(flag_algo->values[0], "WORLD") != 0) {
        ucra_manifest_free(manifest);
        return 0;
    }

    ucra_manifest_free(manifest);
    return 1;
}

/* Test file not found error */
static int test_file_not_found() {
    UCRA_Manifest* manifest = NULL;
    UCRA_Result result = ucra_manifest_load("non_existent.json", &manifest);

    return (result == UCRA_ERR_FILE_NOT_FOUND && manifest == NULL);
}

/* Test invalid JSON error */
static int test_invalid_json() {
    UCRA_Manifest* manifest = NULL;
    UCRA_Result result = ucra_manifest_load("data/broken_manifest.json", &manifest);

    return (result == UCRA_ERR_INVALID_JSON && manifest == NULL);
}

/* Test missing required field */
static int test_missing_required_field() {
    UCRA_Manifest* manifest = NULL;
    UCRA_Result result = ucra_manifest_load("data/invalid_missing_name.json", &manifest);

    return (result == UCRA_ERR_INVALID_MANIFEST && manifest == NULL);
}

/* Test invalid entry type */
static int test_invalid_entry_type() {
    UCRA_Manifest* manifest = NULL;
    UCRA_Result result = ucra_manifest_load("data/invalid_entry_type.json", &manifest);

    return (result == UCRA_ERR_INVALID_MANIFEST && manifest == NULL);
}

/* Test invalid sample rate */
static int test_invalid_sample_rate() {
    UCRA_Manifest* manifest = NULL;
    UCRA_Result result = ucra_manifest_load("data/invalid_negative_rate.json", &manifest);

    return (result == UCRA_ERR_INVALID_MANIFEST && manifest == NULL);
}

/* Test enum without values */
static int test_enum_no_values() {
    UCRA_Manifest* manifest = NULL;
    UCRA_Result result = ucra_manifest_load("data/invalid_enum_no_values.json", &manifest);

    return (result == UCRA_ERR_INVALID_MANIFEST && manifest == NULL);
}/* Test null argument handling */
static int test_null_arguments() {
    UCRA_Manifest* manifest = NULL;

    /* Test null path */
    UCRA_Result result1 = ucra_manifest_load(NULL, &manifest);
    if (result1 != UCRA_ERR_INVALID_ARGUMENT) {
        return 0;
    }

    /* Test null output pointer */
    UCRA_Result result2 = ucra_manifest_load("data/example_manifest.json", NULL);
    if (result2 != UCRA_ERR_INVALID_ARGUMENT) {
        return 0;
    }

    /* Test free with null */
    ucra_manifest_free(NULL); /* Should not crash */

    return 1;
}

int main(int argc, char* argv[]) {
    printf("UCRA Manifest Parser Test Suite\n");
    printf("===============================\n\n");

    /* Run all tests */
    TEST(valid_manifest);
    TEST(file_not_found);
    TEST(invalid_json);
    TEST(missing_required_field);
    TEST(invalid_entry_type);
    TEST(invalid_sample_rate);
    TEST(enum_no_values);
    TEST(null_arguments);

    printf("\n===============================\n");
    printf("Test Results: %d/%d passed\n", passed_count, test_count);

    if (passed_count == test_count) {
        printf("All tests PASSED! ✓\n");
        return 0;
    } else {
        printf("Some tests FAILED! ✗\n");
        return 1;
    }
}
