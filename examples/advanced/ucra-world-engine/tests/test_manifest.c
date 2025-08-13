/*
 * UCRA Manifest Parser Test
 * Tests the official manifest loading API
 */

#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>

void print_manifest(const UCRA_Manifest* manifest) {
    printf("=== UCRA Manifest ===\n");
    printf("Name: %s\n", manifest->name ? manifest->name : "N/A");
    printf("Version: %s\n", manifest->version ? manifest->version : "N/A");
    printf("Vendor: %s\n", manifest->vendor ? manifest->vendor : "N/A");
    printf("License: %s\n", manifest->license ? manifest->license : "N/A");

    printf("\nEntry:\n");
    printf("  Type: %s\n", manifest->entry.type ? manifest->entry.type : "N/A");
    printf("  Path: %s\n", manifest->entry.path ? manifest->entry.path : "N/A");
    printf("  Symbol: %s\n", manifest->entry.symbol ? manifest->entry.symbol : "N/A");

    printf("\nAudio:\n");
    printf("  Rates: ");
    for (uint32_t i = 0; i < manifest->audio.rates_count; i++) {
        printf("%u", manifest->audio.rates[i]);
        if (i < manifest->audio.rates_count - 1) printf(", ");
    }
    printf("\n");

    printf("  Channels: ");
    for (uint32_t i = 0; i < manifest->audio.channels_count; i++) {
        printf("%u", manifest->audio.channels[i]);
        if (i < manifest->audio.channels_count - 1) printf(", ");
    }
    printf("\n");

    printf("  Streaming: %s\n", manifest->audio.streaming ? "yes" : "no");

    printf("\nFlags (%u):\n", manifest->flags_count);
    for (uint32_t i = 0; i < manifest->flags_count; i++) {
        const UCRA_ManifestFlag* flag = &manifest->flags[i];
        printf("  [%u] %s (%s): %s\n", i, flag->key, flag->type, flag->desc);

        if (flag->default_val) {
            printf("      Default: %s\n", flag->default_val);
        }

        if (flag->range) {
            printf("      Range: [%.2f, %.2f]\n", flag->range[0], flag->range[1]);
        }

        if (flag->values && flag->values_count > 0) {
            printf("      Values: ");
            for (uint32_t j = 0; j < flag->values_count; j++) {
                printf("%s", flag->values[j]);
                if (j < flag->values_count - 1) printf(", ");
            }
            printf("\n");
        }
    }

    printf("=====================\n");
}

const char* ucra_result_string(UCRA_Result result) {
    switch (result) {
        case UCRA_SUCCESS: return "Success";
        case UCRA_ERR_INVALID_ARGUMENT: return "Invalid argument";
        case UCRA_ERR_OUT_OF_MEMORY: return "Out of memory";
        case UCRA_ERR_NOT_SUPPORTED: return "Not supported";
        case UCRA_ERR_INTERNAL: return "Internal error";
        case UCRA_ERR_FILE_NOT_FOUND: return "File not found";
        case UCRA_ERR_INVALID_JSON: return "Invalid JSON";
        case UCRA_ERR_INVALID_MANIFEST: return "Invalid manifest";
        default: return "Unknown error";
    }
}

int main(int argc, char* argv[]) {
    const char* test_file = (argc > 1) ? argv[1] : "data/example_manifest.json";

    printf("UCRA Manifest Parser Test\n");
    printf("Loading: %s\n\n", test_file);    UCRA_Manifest* manifest = NULL;
    UCRA_Result result = ucra_manifest_load(test_file, &manifest);

    if (result == UCRA_SUCCESS && manifest) {
        printf("✓ Successfully loaded manifest\n\n");
        print_manifest(manifest);
        ucra_manifest_free(manifest);
        return 0;
    } else {
        printf("✗ Failed to load manifest: %s (%d)\n", ucra_result_string(result), result);
        return 1;
    }
}
