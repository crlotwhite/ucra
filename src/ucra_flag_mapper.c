#include "ucra/ucra_flag_mapper.h"
#include "../third-party/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static char* ucra_strdup(const char* src) {
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char* dst = malloc(len);
    if (dst) {
        memcpy(dst, src, len);
    }
    return dst;
}

static UCRA_TransformKind ucra_parse_transform_kind(const char* kind_str) {
    if (!kind_str) return UCRA_TRANSFORM_COPY;
    if (strcmp(kind_str, "scale") == 0) return UCRA_TRANSFORM_SCALE;
    if (strcmp(kind_str, "map") == 0) return UCRA_TRANSFORM_MAP;
    if (strcmp(kind_str, "constant") == 0) return UCRA_TRANSFORM_CONSTANT;
    return UCRA_TRANSFORM_COPY;
}

static UCRA_Result ucra_parse_rule(const cJSON* rule_json, UCRA_FlagRule* rule) {
    if (!rule_json || !rule) return UCRA_ERR_INVALID_ARGUMENT;

    memset(rule, 0, sizeof(UCRA_FlagRule));

    /* Parse source */
    const cJSON* source = cJSON_GetObjectItem(rule_json, "source");
    if (!source) return UCRA_ERR_INVALID_ARGUMENT;

    const cJSON* source_name = cJSON_GetObjectItem(source, "name");
    if (!cJSON_IsString(source_name)) return UCRA_ERR_INVALID_ARGUMENT;

    rule->source_name = ucra_strdup(cJSON_GetStringValue(source_name));
    if (!rule->source_name) return UCRA_ERR_OUT_OF_MEMORY;

    /* Parse target */
    const cJSON* target = cJSON_GetObjectItem(rule_json, "target");
    if (!target) return UCRA_ERR_INVALID_ARGUMENT;

    const cJSON* target_name = cJSON_GetObjectItem(target, "name");
    if (!cJSON_IsString(target_name)) return UCRA_ERR_INVALID_ARGUMENT;

    rule->target_name = ucra_strdup(cJSON_GetStringValue(target_name));
    if (!rule->target_name) return UCRA_ERR_OUT_OF_MEMORY;

    /* Parse default value if present */
    const cJSON* default_val = cJSON_GetObjectItem(target, "default");
    if (cJSON_IsString(default_val)) {
        rule->default_value = ucra_strdup(cJSON_GetStringValue(default_val));
    } else if (cJSON_IsNumber(default_val)) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.6g", cJSON_GetNumberValue(default_val));
        rule->default_value = ucra_strdup(buffer);
    }

    /* Parse transform */
    const cJSON* transform = cJSON_GetObjectItem(rule_json, "transform");
    if (transform) {
        const cJSON* kind = cJSON_GetObjectItem(transform, "kind");
        if (cJSON_IsString(kind)) {
            rule->transform_kind = ucra_parse_transform_kind(cJSON_GetStringValue(kind));
        }

        /* Parse scale parameters */
        if (rule->transform_kind == UCRA_TRANSFORM_SCALE) {
            const cJSON* scale = cJSON_GetObjectItem(transform, "scale");
            if (cJSON_IsArray(scale) && cJSON_GetArraySize(scale) >= 2) {
                rule->scale_min = cJSON_GetNumberValue(cJSON_GetArrayItem(scale, 0));
                rule->scale_max = cJSON_GetNumberValue(cJSON_GetArrayItem(scale, 1));
            }
        }

        /* Parse map parameters */
        if (rule->transform_kind == UCRA_TRANSFORM_MAP) {
            const cJSON* map = cJSON_GetObjectItem(transform, "map");
            if (cJSON_IsObject(map)) {
                uint32_t count = (uint32_t)cJSON_GetArraySize(map);
                if (count > 0) {
                    rule->map_keys = malloc(count * sizeof(char*));
                    rule->map_values = malloc(count * sizeof(char*));
                    if (rule->map_keys && rule->map_values) {
                        uint32_t i = 0;
                        const cJSON* item;
                        cJSON_ArrayForEach(item, map) {
                            rule->map_keys[i] = ucra_strdup(item->string);
                            rule->map_values[i] = ucra_strdup(cJSON_GetStringValue(item));
                            i++;
                        }
                        rule->map_count = count;
                    }
                }
            }
        }

        /* Parse constant value */
        if (rule->transform_kind == UCRA_TRANSFORM_CONSTANT) {
            const cJSON* value = cJSON_GetObjectItem(transform, "value");
            if (cJSON_IsString(value)) {
                rule->constant_value = ucra_strdup(cJSON_GetStringValue(value));
            }
        }
    }

    return UCRA_SUCCESS;
}

static void ucra_free_rule(UCRA_FlagRule* rule) {
    if (!rule) return;

    free(rule->source_name);
    free(rule->target_name);
    free(rule->default_value);
    free(rule->constant_value);

    if (rule->map_keys) {
        for (uint32_t i = 0; i < rule->map_count; i++) {
            free(rule->map_keys[i]);
            free(rule->map_values[i]);
        }
        free(rule->map_keys);
        free(rule->map_values);
    }

    memset(rule, 0, sizeof(UCRA_FlagRule));
}

UCRA_Result ucra_flag_mapper_load(const char* json_path, UCRA_FlagMapper** mapper) {
    if (!json_path || !mapper) return UCRA_ERR_INVALID_ARGUMENT;

    FILE* file = fopen(json_path, "r");
    if (!file) return UCRA_ERR_FILE_NOT_FOUND;

    /* Get file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    /* Read file */
    char* json_data = malloc((size_t)file_size + 1);
    if (!json_data) {
        fclose(file);
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    size_t read_size = fread(json_data, 1, (size_t)file_size, file);
    json_data[read_size] = '\0';
    fclose(file);

    /* Parse JSON */
    cJSON* json = cJSON_Parse(json_data);
    free(json_data);

    if (!json) {
        return UCRA_ERR_INVALID_ARGUMENT;
    }

    /* Allocate mapper */
    UCRA_FlagMapper* m = calloc(1, sizeof(UCRA_FlagMapper));
    if (!m) {
        cJSON_Delete(json);
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    /* Parse engine name */
    const cJSON* engine = cJSON_GetObjectItem(json, "engine");
    if (cJSON_IsString(engine)) {
        m->engine_name = ucra_strdup(cJSON_GetStringValue(engine));
    }

    /* Parse version */
    const cJSON* version = cJSON_GetObjectItem(json, "version");
    if (cJSON_IsString(version)) {
        m->version = ucra_strdup(cJSON_GetStringValue(version));
    }

    /* Parse rules */
    const cJSON* rules = cJSON_GetObjectItem(json, "rules");
    if (cJSON_IsArray(rules)) {
        uint32_t rule_count = (uint32_t)cJSON_GetArraySize(rules);
        if (rule_count > 0) {
            m->rules = calloc(rule_count, sizeof(UCRA_FlagRule));
            if (!m->rules) {
                ucra_flag_mapper_free(m);
                cJSON_Delete(json);
                return UCRA_ERR_OUT_OF_MEMORY;
            }

            uint32_t parsed_count = 0;
            const cJSON* rule_json;
            cJSON_ArrayForEach(rule_json, rules) {
                if (ucra_parse_rule(rule_json, &m->rules[parsed_count]) == UCRA_SUCCESS) {
                    parsed_count++;
                }
            }
            m->rule_count = parsed_count;
        }
    }

    cJSON_Delete(json);
    *mapper = m;
    return UCRA_SUCCESS;
}

void ucra_flag_mapper_free(UCRA_FlagMapper* mapper) {
    if (!mapper) return;

    free(mapper->engine_name);
    free(mapper->version);

    if (mapper->rules) {
        for (uint32_t i = 0; i < mapper->rule_count; i++) {
            ucra_free_rule(&mapper->rules[i]);
        }
        free(mapper->rules);
    }

    free(mapper);
}

static char* ucra_apply_transform(const UCRA_FlagRule* rule, const char* input_value, char** warning) {
    if (!rule || !input_value) return NULL;

    switch (rule->transform_kind) {
        case UCRA_TRANSFORM_COPY:
            return ucra_strdup(input_value);

        case UCRA_TRANSFORM_SCALE: {
            char* endptr;
            double val = strtod(input_value, &endptr);
            if (*endptr != '\0') {
                if (warning) {
                    *warning = ucra_strdup("scale: invalid number format");
                }
                return NULL;
            }
            double scaled = rule->scale_min + (rule->scale_max - rule->scale_min) * val;
            char* result = malloc(32);
            if (result) {
                snprintf(result, 32, "%.6g", scaled);
            }
            return result;
        }

        case UCRA_TRANSFORM_MAP:
            for (uint32_t i = 0; i < rule->map_count; i++) {
                if (strcmp(input_value, rule->map_keys[i]) == 0) {
                    return ucra_strdup(rule->map_values[i]);
                }
            }
            if (warning) {
                char* msg = malloc(128);
                if (msg) {
                    snprintf(msg, 128, "map: value '%s' not found in mapping", input_value);
                    *warning = msg;
                }
            }
            return NULL;

        case UCRA_TRANSFORM_CONSTANT:
            return ucra_strdup(rule->constant_value);

        default:
            return ucra_strdup(input_value);
    }
}

UCRA_Result ucra_flag_mapper_apply(const UCRA_FlagMapper* mapper,
                                  const UCRA_KeyValue* legacy_flags, uint32_t legacy_count,
                                  UCRA_FlagMapResult* result) {
    if (!mapper || !result) return UCRA_ERR_INVALID_ARGUMENT;

    memset(result, 0, sizeof(UCRA_FlagMapResult));

    /* Allocate result arrays */
    result->flags = calloc(mapper->rule_count, sizeof(UCRA_KeyValue));
    result->warnings = calloc(mapper->rule_count, sizeof(char*));
    if (!result->flags || !result->warnings) {
        ucra_flag_map_result_free(result);
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    uint32_t flag_count = 0;
    uint32_t warning_count = 0;

    /* Apply each rule */
    for (uint32_t i = 0; i < mapper->rule_count; i++) {
        const UCRA_FlagRule* rule = &mapper->rules[i];
        const char* input_value = NULL;

        /* Find input value */
        for (uint32_t j = 0; j < legacy_count; j++) {
            if (strcmp(legacy_flags[j].key, rule->source_name) == 0) {
                input_value = legacy_flags[j].value;
                break;
            }
        }

        char* warning = NULL;
        char* output_value = NULL;

        if (input_value) {
            output_value = ucra_apply_transform(rule, input_value, &warning);
        } else if (rule->default_value) {
            output_value = ucra_strdup(rule->default_value);
        }

        if (output_value) {
            result->flags[flag_count].key = ucra_strdup(rule->target_name);
            result->flags[flag_count].value = output_value;
            flag_count++;
        }

        if (warning) {
            result->warnings[warning_count] = warning;
            warning_count++;
        }
    }

    result->flag_count = flag_count;
    result->warning_count = warning_count;

    return UCRA_SUCCESS;
}

void ucra_flag_map_result_free(UCRA_FlagMapResult* result) {
    if (!result) return;

    if (result->flags) {
        for (uint32_t i = 0; i < result->flag_count; i++) {
            free((void*)result->flags[i].key);
            free((void*)result->flags[i].value);
        }
        free(result->flags);
    }

    if (result->warnings) {
        for (uint32_t i = 0; i < result->warning_count; i++) {
            free(result->warnings[i]);
        }
        free(result->warnings);
    }

    memset(result, 0, sizeof(UCRA_FlagMapResult));
}

UCRA_Result ucra_parse_legacy_flags(const char* flag_str, UCRA_KeyValue** flags, uint32_t* count) {
    if (!flag_str || !flags || !count) return UCRA_ERR_INVALID_ARGUMENT;

    *flags = NULL;
    *count = 0;

    if (strlen(flag_str) == 0) return UCRA_SUCCESS;

    /* Count semicolons to estimate number of flags */
    uint32_t estimated_count = 1;
    for (const char* p = flag_str; *p; p++) {
        if (*p == ';') estimated_count++;
    }

    /* Allocate array */
    UCRA_KeyValue* kv_array = calloc(estimated_count, sizeof(UCRA_KeyValue));
    if (!kv_array) return UCRA_ERR_OUT_OF_MEMORY;

    /* Parse string */
    char* str_copy = ucra_strdup(flag_str);
    if (!str_copy) {
        free(kv_array);
        return UCRA_ERR_OUT_OF_MEMORY;
    }

    uint32_t actual_count = 0;
    char* token = strtok(str_copy, ";");
    while (token && actual_count < estimated_count) {
        char* equals = strchr(token, '=');
        if (equals) {
            *equals = '\0';
            char* key = token;
            char* value = equals + 1;

            /* Trim whitespace */
            while (*key == ' ' || *key == '\t') key++;
            while (*value == ' ' || *value == '\t') value++;

            kv_array[actual_count].key = ucra_strdup(key);
            kv_array[actual_count].value = ucra_strdup(value);
            actual_count++;
        }
        token = strtok(NULL, ";");
    }

    free(str_copy);

    *flags = kv_array;
    *count = actual_count;

    return UCRA_SUCCESS;
}

void ucra_free_legacy_flags(UCRA_KeyValue* flags, uint32_t count) {
    if (!flags) return;

    for (uint32_t i = 0; i < count; i++) {
        free((void*)flags[i].key);
        free((void*)flags[i].value);
    }
    free(flags);
}
