/*
 * UCRA Manifest Parser Implementation
 * Core parser and data loading functionality
 */

#include "ucra/ucra.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Internal manifest structure with memory management info */
typedef struct UCRA_ManifestInternal {
    UCRA_Manifest public_manifest;

    /* Memory management */
    char* allocated_strings;
    size_t allocated_strings_size;
    uint32_t* allocated_rates;
    uint32_t* allocated_channels;
    UCRA_ManifestFlag* allocated_flags;
    float* allocated_ranges;
    char** allocated_enum_values;
    size_t allocated_enum_values_count;
} UCRA_ManifestInternal;

/* Helper function to duplicate a string */
static char* ucra_strdup(const char* src) {
    if (!src) return NULL;

    size_t len = strlen(src);
    char* dst = malloc(len + 1);
    if (dst) {
        memcpy(dst, src, len + 1);
    }
    return dst;
}

/* Schema validation functions */
static int ucra_validate_entry_type(const char* type) {
    if (!type) return 0;
    return (strcmp(type, "dll") == 0 ||
            strcmp(type, "cli") == 0 ||
            strcmp(type, "ipc") == 0);
}

static int ucra_validate_flag_type(const char* type) {
    if (!type) return 0;
    return (strcmp(type, "float") == 0 ||
            strcmp(type, "int") == 0 ||
            strcmp(type, "bool") == 0 ||
            strcmp(type, "string") == 0 ||
            strcmp(type, "enum") == 0);
}

static UCRA_Result ucra_validate_required_fields(cJSON* json) {
    /* Check required root fields */
    if (!cJSON_GetObjectItem(json, "name") || !cJSON_IsString(cJSON_GetObjectItem(json, "name"))) {
        return UCRA_ERR_INVALID_MANIFEST;
    }

    if (!cJSON_GetObjectItem(json, "version") || !cJSON_IsString(cJSON_GetObjectItem(json, "version"))) {
        return UCRA_ERR_INVALID_MANIFEST;
    }

    /* Check required entry object and its fields */
    cJSON* entry = cJSON_GetObjectItem(json, "entry");
    if (!entry || !cJSON_IsObject(entry)) {
        return UCRA_ERR_INVALID_MANIFEST;
    }

    cJSON* entry_type = cJSON_GetObjectItem(entry, "type");
    if (!entry_type || !cJSON_IsString(entry_type) || !ucra_validate_entry_type(entry_type->valuestring)) {
        return UCRA_ERR_INVALID_MANIFEST;
    }

    if (!cJSON_GetObjectItem(entry, "path") || !cJSON_IsString(cJSON_GetObjectItem(entry, "path"))) {
        return UCRA_ERR_INVALID_MANIFEST;
    }

    /* Check required audio object and its fields */
    cJSON* audio = cJSON_GetObjectItem(json, "audio");
    if (!audio || !cJSON_IsObject(audio)) {
        return UCRA_ERR_INVALID_MANIFEST;
    }

    cJSON* rates = cJSON_GetObjectItem(audio, "rates");
    if (!rates || !cJSON_IsArray(rates) || cJSON_GetArraySize(rates) == 0) {
        return UCRA_ERR_INVALID_MANIFEST;
    }

    /* Validate rates array elements */
    int rates_count = cJSON_GetArraySize(rates);
    for (int i = 0; i < rates_count; i++) {
        cJSON* rate = cJSON_GetArrayItem(rates, i);
        if (!cJSON_IsNumber(rate) || rate->valueint <= 0 || rate->valueint > 192000) {
            return UCRA_ERR_INVALID_MANIFEST;
        }
    }

    cJSON* channels = cJSON_GetObjectItem(audio, "channels");
    if (!channels || !cJSON_IsArray(channels) || cJSON_GetArraySize(channels) == 0) {
        return UCRA_ERR_INVALID_MANIFEST;
    }

    /* Validate channels array elements */
    int channels_count = cJSON_GetArraySize(channels);
    for (int i = 0; i < channels_count; i++) {
        cJSON* channel = cJSON_GetArrayItem(channels, i);
        if (!cJSON_IsNumber(channel) || channel->valueint <= 0 || channel->valueint > 8) {
            return UCRA_ERR_INVALID_MANIFEST;
        }
    }

    return UCRA_SUCCESS;
}

static UCRA_Result ucra_validate_flags_array(cJSON* flags_array) {
    if (!flags_array) {
        return UCRA_SUCCESS; /* flags array is optional */
    }

    if (!cJSON_IsArray(flags_array)) {
        return UCRA_ERR_INVALID_MANIFEST;
    }

    int count = cJSON_GetArraySize(flags_array);
    for (int i = 0; i < count; i++) {
        cJSON* flag = cJSON_GetArrayItem(flags_array, i);
        if (!cJSON_IsObject(flag)) {
            return UCRA_ERR_INVALID_MANIFEST;
        }

        /* Check required flag fields */
        cJSON* key = cJSON_GetObjectItem(flag, "key");
        if (!key || !cJSON_IsString(key) || strlen(key->valuestring) == 0) {
            return UCRA_ERR_INVALID_MANIFEST;
        }

        cJSON* type = cJSON_GetObjectItem(flag, "type");
        if (!type || !cJSON_IsString(type) || !ucra_validate_flag_type(type->valuestring)) {
            return UCRA_ERR_INVALID_MANIFEST;
        }

        cJSON* desc = cJSON_GetObjectItem(flag, "desc");
        if (!desc || !cJSON_IsString(desc) || strlen(desc->valuestring) == 0) {
            return UCRA_ERR_INVALID_MANIFEST;
        }

        /* Validate type-specific constraints */
        const char* type_str = type->valuestring;

        if (strcmp(type_str, "float") == 0 || strcmp(type_str, "int") == 0) {
            /* Numeric types should have range */
            cJSON* range = cJSON_GetObjectItem(flag, "range");
            if (range) {
                if (!cJSON_IsArray(range) || cJSON_GetArraySize(range) != 2) {
                    return UCRA_ERR_INVALID_MANIFEST;
                }

                cJSON* min_val = cJSON_GetArrayItem(range, 0);
                cJSON* max_val = cJSON_GetArrayItem(range, 1);
                if (!cJSON_IsNumber(min_val) || !cJSON_IsNumber(max_val)) {
                    return UCRA_ERR_INVALID_MANIFEST;
                }

                if (min_val->valuedouble >= max_val->valuedouble) {
                    return UCRA_ERR_INVALID_MANIFEST;
                }
            }
        } else if (strcmp(type_str, "enum") == 0) {
            /* Enum type must have values array */
            cJSON* values = cJSON_GetObjectItem(flag, "values");
            if (!values || !cJSON_IsArray(values) || cJSON_GetArraySize(values) == 0) {
                return UCRA_ERR_INVALID_MANIFEST;
            }

            /* All values must be strings */
            int values_count = cJSON_GetArraySize(values);
            for (int j = 0; j < values_count; j++) {
                cJSON* value = cJSON_GetArrayItem(values, j);
                if (!cJSON_IsString(value) || strlen(value->valuestring) == 0) {
                    return UCRA_ERR_INVALID_MANIFEST;
                }
            }
        }
    }

    return UCRA_SUCCESS;
}

/* Helper function to read entire file */
static UCRA_Result ucra_read_file(const char* filepath, char** out_content, size_t* out_size) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        return UCRA_ERR_FILE_NOT_FOUND;
    }

    /* Get file size */
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return UCRA_ERR_INTERNAL;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return UCRA_ERR_INTERNAL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return UCRA_ERR_INTERNAL;
    }

    /* Allocate buffer */
    char* content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    /* Read content */
    size_t read_size = fread(content, 1, file_size, file);
    content[read_size] = '\0';
    fclose(file);

    *out_content = content;
    *out_size = read_size;
    return UCRA_SUCCESS;
}

/* Parse a JSON array of integers */
static UCRA_Result ucra_parse_int_array(cJSON* array, uint32_t** out_values, uint32_t* out_count) {
    if (!cJSON_IsArray(array)) {
        return UCRA_ERR_INVALID_MANIFEST;
    }

    int count = cJSON_GetArraySize(array);
    if (count <= 0) {
        *out_values = NULL;
        *out_count = 0;
        return UCRA_SUCCESS;
    }

    uint32_t* values = malloc(count * sizeof(uint32_t));
    if (!values) {
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    for (int i = 0; i < count; i++) {
        cJSON* item = cJSON_GetArrayItem(array, i);
        if (!cJSON_IsNumber(item) || item->valueint < 0) {
            free(values);
            return UCRA_ERR_INVALID_MANIFEST;
        }
        values[i] = (uint32_t)item->valueint;
    }

    *out_values = values;
    *out_count = (uint32_t)count;
    return UCRA_SUCCESS;
}

/* Parse flags array */
static UCRA_Result ucra_parse_flags(cJSON* flags_array, UCRA_ManifestInternal* manifest) {
    if (!cJSON_IsArray(flags_array)) {
        manifest->public_manifest.flags = NULL;
        manifest->public_manifest.flags_count = 0;
        return UCRA_SUCCESS;
    }

    int count = cJSON_GetArraySize(flags_array);
    if (count <= 0) {
        manifest->public_manifest.flags = NULL;
        manifest->public_manifest.flags_count = 0;
        return UCRA_SUCCESS;
    }

    UCRA_ManifestFlag* flags = calloc(count, sizeof(UCRA_ManifestFlag));
    if (!flags) {
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    for (int i = 0; i < count; i++) {
        cJSON* flag = cJSON_GetArrayItem(flags_array, i);
        if (!cJSON_IsObject(flag)) {
            free(flags);
            return UCRA_ERR_INVALID_MANIFEST;
        }

        /* Parse required fields */
        cJSON* key = cJSON_GetObjectItem(flag, "key");
        cJSON* type = cJSON_GetObjectItem(flag, "type");
        cJSON* desc = cJSON_GetObjectItem(flag, "desc");

        if (!cJSON_IsString(key) || !cJSON_IsString(type) || !cJSON_IsString(desc)) {
            free(flags);
            return UCRA_ERR_INVALID_MANIFEST;
        }

        flags[i].key = ucra_strdup(key->valuestring);
        flags[i].type = ucra_strdup(type->valuestring);
        flags[i].desc = ucra_strdup(desc->valuestring);

        /* Parse optional default value */
        cJSON* default_val = cJSON_GetObjectItem(flag, "default");
        if (default_val) {
            if (cJSON_IsString(default_val)) {
                flags[i].default_val = ucra_strdup(default_val->valuestring);
            } else if (cJSON_IsNumber(default_val)) {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%.6f", default_val->valuedouble);
                flags[i].default_val = ucra_strdup(buffer);
            } else if (cJSON_IsBool(default_val)) {
                flags[i].default_val = ucra_strdup(cJSON_IsTrue(default_val) ? "true" : "false");
            }
        }

        /* Parse range for numeric types */
        cJSON* range = cJSON_GetObjectItem(flag, "range");
        if (range && cJSON_IsArray(range) && cJSON_GetArraySize(range) == 2) {
            float* range_values = malloc(2 * sizeof(float));
            if (range_values) {
                cJSON* min_val = cJSON_GetArrayItem(range, 0);
                cJSON* max_val = cJSON_GetArrayItem(range, 1);
                if (cJSON_IsNumber(min_val) && cJSON_IsNumber(max_val)) {
                    range_values[0] = (float)min_val->valuedouble;
                    range_values[1] = (float)max_val->valuedouble;
                    flags[i].range = range_values;
                } else {
                    free(range_values);
                }
            }
        }

        /* Parse values for enum type */
        cJSON* values = cJSON_GetObjectItem(flag, "values");
        if (values && cJSON_IsArray(values)) {
            int values_count = cJSON_GetArraySize(values);
            if (values_count > 0) {
                char** enum_values = malloc(values_count * sizeof(char*));
                if (enum_values) {
                    int valid_count = 0;
                    for (int j = 0; j < values_count; j++) {
                        cJSON* value = cJSON_GetArrayItem(values, j);
                        if (cJSON_IsString(value)) {
                            enum_values[valid_count++] = ucra_strdup(value->valuestring);
                        }
                    }
                    if (valid_count > 0) {
                        flags[i].values = (const char**)enum_values;
                        flags[i].values_count = valid_count;
                    } else {
                        free(enum_values);
                    }
                }
            }
        }
    }

    manifest->allocated_flags = flags;
    manifest->public_manifest.flags = flags;
    manifest->public_manifest.flags_count = (uint32_t)count;
    return UCRA_SUCCESS;
}

UCRA_Result ucra_manifest_load(const char* manifest_path, UCRA_Manifest** outManifest) {
    if (!manifest_path || !outManifest) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    *outManifest = NULL;

    /* Read file content */
    char* json_content = NULL;
    size_t content_size = 0;
    UCRA_Result result = ucra_read_file(manifest_path, &json_content, &content_size);
    if (result != UCRA_SUCCESS) {
        return result;
    }

    /* Parse JSON */
    cJSON* json = cJSON_Parse(json_content);
    free(json_content);

    if (!json) {
        return UCRA_ERR_INVALID_JSON;
    }

    /* Validate schema compliance */
    result = ucra_validate_required_fields(json);
    if (result != UCRA_SUCCESS) {
        cJSON_Delete(json);
        return result;
    }

    /* Validate flags array if present */
    cJSON* flags = cJSON_GetObjectItem(json, "flags");
    result = ucra_validate_flags_array(flags);
    if (result != UCRA_SUCCESS) {
        cJSON_Delete(json);
        return result;
    }

    /* Allocate internal manifest structure */
    UCRA_ManifestInternal* manifest = calloc(1, sizeof(UCRA_ManifestInternal));
    if (!manifest) {
        cJSON_Delete(json);
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    /* Parse basic string fields */
    cJSON* item;

    item = cJSON_GetObjectItem(json, "name");
    if (cJSON_IsString(item)) {
        manifest->public_manifest.name = ucra_strdup(item->valuestring);
    }

    item = cJSON_GetObjectItem(json, "version");
    if (cJSON_IsString(item)) {
        manifest->public_manifest.version = ucra_strdup(item->valuestring);
    }

    item = cJSON_GetObjectItem(json, "vendor");
    if (cJSON_IsString(item)) {
        manifest->public_manifest.vendor = ucra_strdup(item->valuestring);
    }

    item = cJSON_GetObjectItem(json, "license");
    if (cJSON_IsString(item)) {
        manifest->public_manifest.license = ucra_strdup(item->valuestring);
    }

    /* Parse entry object */
    cJSON* entry = cJSON_GetObjectItem(json, "entry");
    if (cJSON_IsObject(entry)) {
        item = cJSON_GetObjectItem(entry, "type");
        if (cJSON_IsString(item)) {
            manifest->public_manifest.entry.type = ucra_strdup(item->valuestring);
        }

        item = cJSON_GetObjectItem(entry, "path");
        if (cJSON_IsString(item)) {
            manifest->public_manifest.entry.path = ucra_strdup(item->valuestring);
        }

        item = cJSON_GetObjectItem(entry, "symbol");
        if (cJSON_IsString(item)) {
            manifest->public_manifest.entry.symbol = ucra_strdup(item->valuestring);
        }
    }

    /* Parse audio object */
    cJSON* audio = cJSON_GetObjectItem(json, "audio");
    if (cJSON_IsObject(audio)) {
        /* Parse rates array */
        cJSON* rates = cJSON_GetObjectItem(audio, "rates");
        result = ucra_parse_int_array(rates, &manifest->allocated_rates, &manifest->public_manifest.audio.rates_count);
        if (result == UCRA_SUCCESS) {
            manifest->public_manifest.audio.rates = manifest->allocated_rates;
        }

        /* Parse channels array */
        cJSON* channels = cJSON_GetObjectItem(audio, "channels");
        result = ucra_parse_int_array(channels, &manifest->allocated_channels, &manifest->public_manifest.audio.channels_count);
        if (result == UCRA_SUCCESS) {
            manifest->public_manifest.audio.channels = manifest->allocated_channels;
        }

        /* Parse streaming flag */
        item = cJSON_GetObjectItem(audio, "streaming");
        if (cJSON_IsBool(item)) {
            manifest->public_manifest.audio.streaming = cJSON_IsTrue(item);
        }
    }

    /* Parse flags array */
    cJSON* flags_obj = cJSON_GetObjectItem(json, "flags");
    if (flags_obj) {
        result = ucra_parse_flags(flags_obj, manifest);
        if (result != UCRA_SUCCESS) {
            cJSON_Delete(json);
            ucra_manifest_free((UCRA_Manifest*)manifest);
            return result;
        }
    }

    cJSON_Delete(json);
    *outManifest = (UCRA_Manifest*)manifest;
    return UCRA_SUCCESS;
}

void ucra_manifest_free(UCRA_Manifest* manifest) {
    if (!manifest) return;

    UCRA_ManifestInternal* internal = (UCRA_ManifestInternal*)manifest;

    /* Free string fields */
    free((void*)internal->public_manifest.name);
    free((void*)internal->public_manifest.version);
    free((void*)internal->public_manifest.vendor);
    free((void*)internal->public_manifest.license);
    free((void*)internal->public_manifest.entry.type);
    free((void*)internal->public_manifest.entry.path);
    free((void*)internal->public_manifest.entry.symbol);

    /* Free audio arrays */
    free(internal->allocated_rates);
    free(internal->allocated_channels);

    /* Free flags */
    if (internal->allocated_flags) {
        for (uint32_t i = 0; i < internal->public_manifest.flags_count; i++) {
            free((void*)internal->allocated_flags[i].key);
            free((void*)internal->allocated_flags[i].type);
            free((void*)internal->allocated_flags[i].desc);
            free((void*)internal->allocated_flags[i].default_val);
            free((void*)internal->allocated_flags[i].range);

            if (internal->allocated_flags[i].values) {
                for (uint32_t j = 0; j < internal->allocated_flags[i].values_count; j++) {
                    free((void*)internal->allocated_flags[i].values[j]);
                }
                free((void*)internal->allocated_flags[i].values);
            }
        }
        free(internal->allocated_flags);
    }

    /* Free the manifest structure itself */
    free(internal);
}
