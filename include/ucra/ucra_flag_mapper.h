#ifndef UCRA_FLAG_MAPPER_H
#define UCRA_FLAG_MAPPER_H

#include "ucra/ucra.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flag mapping rule types */
typedef enum {
    UCRA_TRANSFORM_COPY,
    UCRA_TRANSFORM_SCALE,
    UCRA_TRANSFORM_MAP,
    UCRA_TRANSFORM_CONSTANT
} UCRA_TransformKind;

/* Flag mapping rule structure */
typedef struct UCRA_FlagRule {
    char* source_name;
    char* target_name;
    UCRA_TransformKind transform_kind;

    /* Transform parameters */
    double scale_min;
    double scale_max;
    char** map_keys;
    char** map_values;
    uint32_t map_count;
    char* constant_value;
    char* default_value;
} UCRA_FlagRule;

/* Flag mapping ruleset */
typedef struct UCRA_FlagMapper {
    char* engine_name;
    char* version;
    UCRA_FlagRule* rules;
    uint32_t rule_count;
} UCRA_FlagMapper;

/* Flag mapping result */
typedef struct UCRA_FlagMapResult {
    UCRA_KeyValue* flags;
    uint32_t flag_count;
    char** warnings;
    uint32_t warning_count;
} UCRA_FlagMapResult;

/* API functions */
UCRA_Result ucra_flag_mapper_load(const char* json_path, UCRA_FlagMapper** mapper);
void ucra_flag_mapper_free(UCRA_FlagMapper* mapper);

UCRA_Result ucra_flag_mapper_apply(const UCRA_FlagMapper* mapper,
                                  const UCRA_KeyValue* legacy_flags, uint32_t legacy_count,
                                  UCRA_FlagMapResult* result);
void ucra_flag_map_result_free(UCRA_FlagMapResult* result);

/* Utility function to parse legacy flag string "key1=val1;key2=val2" */
UCRA_Result ucra_parse_legacy_flags(const char* flag_str, UCRA_KeyValue** flags, uint32_t* count);
void ucra_free_legacy_flags(UCRA_KeyValue* flags, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* UCRA_FLAG_MAPPER_H */
