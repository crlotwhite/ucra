/*
 * UCRA Manifest Parser Proof of Concept
 * Demonstrates cJSON integration for parsing resampler.json files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

// Simple manifest structure (proof of concept)
typedef struct {
    char* name;
    char* version;
    char* vendor;
    char* license;

    struct {
        char* type;
        char* path;
        char* symbol;
    } entry;

    struct {
        int* rates;
        int rates_count;
        int* channels;
        int channels_count;
        int streaming;
    } audio;
} UCRA_Manifest;

// Free manifest memory
void ucra_manifest_free(UCRA_Manifest* manifest) {
    if (!manifest) return;

    free(manifest->name);
    free(manifest->version);
    free(manifest->vendor);
    free(manifest->license);
    free(manifest->entry.type);
    free(manifest->entry.path);
    free(manifest->entry.symbol);
    free(manifest->audio.rates);
    free(manifest->audio.channels);

    memset(manifest, 0, sizeof(UCRA_Manifest));
}

// Parse manifest from JSON file
int ucra_manifest_parse_file(const char* filepath, UCRA_Manifest* manifest) {
    FILE* file = NULL;
    char* json_string = NULL;
    cJSON* json = NULL;
    cJSON* item = NULL;
    cJSON* array = NULL;
    int result = 0;

    // Initialize manifest
    memset(manifest, 0, sizeof(UCRA_Manifest));

    // Read file
    file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filepath);
        return -1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer
    json_string = malloc(file_size + 1);
    if (!json_string) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    // Read content
    size_t read_size = fread(json_string, 1, file_size, file);
    json_string[read_size] = '\0';
    fclose(file);

    // Parse JSON
    json = cJSON_Parse(json_string);
    free(json_string);

    if (!json) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "JSON Error: %s\n", error_ptr);
        }
        return -1;
    }

    // Parse basic fields
    item = cJSON_GetObjectItem(json, "name");
    if (cJSON_IsString(item)) {
        manifest->name = strdup(item->valuestring);
    }

    item = cJSON_GetObjectItem(json, "version");
    if (cJSON_IsString(item)) {
        manifest->version = strdup(item->valuestring);
    }

    item = cJSON_GetObjectItem(json, "vendor");
    if (cJSON_IsString(item)) {
        manifest->vendor = strdup(item->valuestring);
    }

    item = cJSON_GetObjectItem(json, "license");
    if (cJSON_IsString(item)) {
        manifest->license = strdup(item->valuestring);
    }

    // Parse entry object
    cJSON* entry = cJSON_GetObjectItem(json, "entry");
    if (cJSON_IsObject(entry)) {
        item = cJSON_GetObjectItem(entry, "type");
        if (cJSON_IsString(item)) {
            manifest->entry.type = strdup(item->valuestring);
        }

        item = cJSON_GetObjectItem(entry, "path");
        if (cJSON_IsString(item)) {
            manifest->entry.path = strdup(item->valuestring);
        }

        item = cJSON_GetObjectItem(entry, "symbol");
        if (cJSON_IsString(item)) {
            manifest->entry.symbol = strdup(item->valuestring);
        }
    }

    // Parse audio object
    cJSON* audio = cJSON_GetObjectItem(json, "audio");
    if (cJSON_IsObject(audio)) {
        // Parse rates array
        array = cJSON_GetObjectItem(audio, "rates");
        if (cJSON_IsArray(array)) {
            int count = cJSON_GetArraySize(array);
            manifest->audio.rates = malloc(count * sizeof(int));
            manifest->audio.rates_count = count;

            for (int i = 0; i < count; i++) {
                cJSON* rate = cJSON_GetArrayItem(array, i);
                if (cJSON_IsNumber(rate)) {
                    manifest->audio.rates[i] = rate->valueint;
                }
            }
        }

        // Parse channels array
        array = cJSON_GetObjectItem(audio, "channels");
        if (cJSON_IsArray(array)) {
            int count = cJSON_GetArraySize(array);
            manifest->audio.channels = malloc(count * sizeof(int));
            manifest->audio.channels_count = count;

            for (int i = 0; i < count; i++) {
                cJSON* channel = cJSON_GetArrayItem(array, i);
                if (cJSON_IsNumber(channel)) {
                    manifest->audio.channels[i] = channel->valueint;
                }
            }
        }

        // Parse streaming flag
        item = cJSON_GetObjectItem(audio, "streaming");
        if (cJSON_IsBool(item)) {
            manifest->audio.streaming = cJSON_IsTrue(item);
        }
    }

    result = 0; // Success

cleanup:
    cJSON_Delete(json);
    return result;
}

// Print manifest info
void ucra_manifest_print(const UCRA_Manifest* manifest) {
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
    for (int i = 0; i < manifest->audio.rates_count; i++) {
        printf("%d", manifest->audio.rates[i]);
        if (i < manifest->audio.rates_count - 1) printf(", ");
    }
    printf("\n");

    printf("  Channels: ");
    for (int i = 0; i < manifest->audio.channels_count; i++) {
        printf("%d", manifest->audio.channels[i]);
        if (i < manifest->audio.channels_count - 1) printf(", ");
    }
    printf("\n");

    printf("  Streaming: %s\n", manifest->audio.streaming ? "yes" : "no");
    printf("=====================\n");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <manifest.json>\n", argv[0]);
        return 1;
    }

    UCRA_Manifest manifest;

    printf("UCRA Manifest Parser - Proof of Concept\n");
    printf("Using cJSON library\n\n");

    if (ucra_manifest_parse_file(argv[1], &manifest) == 0) {
        printf("✓ Successfully parsed manifest\n\n");
        ucra_manifest_print(&manifest);
        ucra_manifest_free(&manifest);
        return 0;
    } else {
        printf("✗ Failed to parse manifest\n");
        return 1;
    }
}
