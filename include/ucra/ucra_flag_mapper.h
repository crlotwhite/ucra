/**
 * @file ucra_flag_mapper.h
 * @brief UCRA Flag Mapping API
 * @author UCRA Development Team
 * @date 2025
 *
 * This header defines the flag mapping system for converting between
 * different flag formats and engines. The flag mapper allows translation
 * of legacy flag formats to modern UCRA-compatible formats.
 */
#ifndef UCRA_FLAG_MAPPER_H
#define UCRA_FLAG_MAPPER_H

#include "ucra/ucra.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Flag mapping rule types
 *
 * Specifies how a source flag should be transformed to a target flag.
 */
typedef enum {
    UCRA_TRANSFORM_COPY,      /**< Copy value directly without transformation */
    UCRA_TRANSFORM_SCALE,     /**< Scale numeric value to different range */
    UCRA_TRANSFORM_MAP,       /**< Map discrete values using lookup table */
    UCRA_TRANSFORM_CONSTANT   /**< Use constant value regardless of input */
} UCRA_TransformKind;

/**
 * @brief Flag mapping rule structure
 *
 * Defines how a single flag should be transformed from source to target format.
 */
typedef struct UCRA_FlagRule {
    char* source_name;        /**< Source flag name */
    char* target_name;        /**< Target flag name */
    UCRA_TransformKind transform_kind; /**< Type of transformation to apply */

    /** @brief Transform parameters */
    double scale_min;         /**< Minimum value for scaling transform */
    double scale_max;         /**< Maximum value for scaling transform */
    char** map_keys;          /**< Keys for map transform */
    char** map_values;        /**< Values for map transform */
    uint32_t map_count;       /**< Number of map entries */
    char* constant_value;     /**< Constant value for constant transform */
    char* default_value;      /**< Default value if source not found */
} UCRA_FlagRule;

/**
 * @brief Flag mapping ruleset
 *
 * Complete set of rules for mapping flags from one engine to another.
 */
typedef struct UCRA_FlagMapper {
    char* engine_name;        /**< Target engine name */
    char* version;           /**< Mapper version */
    UCRA_FlagRule* rules;    /**< Array of mapping rules */
    uint32_t rule_count;     /**< Number of rules */
} UCRA_FlagMapper;

/**
 * @brief Flag mapping result
 *
 * Result of applying flag mapping rules, including the mapped flags
 * and any warnings encountered during the process.
 */
typedef struct UCRA_FlagMapResult {
    UCRA_KeyValue* flags;    /**< Mapped flags array */
    uint32_t flag_count;     /**< Number of mapped flags */
    char** warnings;         /**< Warning messages array */
    uint32_t warning_count;  /**< Number of warnings */
} UCRA_FlagMapResult;

/**
 * @brief Flag Mapping API Functions
 * @defgroup FlagMapperAPI Flag Mapping Functions
 * @{
 */

/**
 * @brief Load flag mapper from JSON file
 *
 * Parses a JSON file containing flag mapping rules and creates
 * a UCRA_FlagMapper instance.
 *
 * @param json_path Path to the JSON mapping file
 * @param mapper Pointer to store the created mapper
 * @return UCRA_SUCCESS on successful loading
 */
UCRA_API UCRA_Result UCRA_CALL ucra_flag_mapper_load(const char* json_path, UCRA_FlagMapper** mapper);

/**
 * @brief Free flag mapper resources
 *
 * Releases all memory associated with the flag mapper.
 *
 * @param mapper Mapper to free (may be NULL)
 */
UCRA_API void UCRA_CALL ucra_flag_mapper_free(UCRA_FlagMapper* mapper);

/**
 * @brief Apply flag mapping rules
 *
 * Transforms legacy flags according to the mapping rules and
 * produces a result with the mapped flags.
 *
 * @param mapper Flag mapper containing the rules
 * @param legacy_flags Array of legacy flags to transform
 * @param legacy_count Number of legacy flags
 * @param result Result structure to populate
 * @return UCRA_SUCCESS on successful mapping
 */
UCRA_API UCRA_Result UCRA_CALL ucra_flag_mapper_apply(const UCRA_FlagMapper* mapper,
                                                     const UCRA_KeyValue* legacy_flags, uint32_t legacy_count,
                                                     UCRA_FlagMapResult* result);

/**
 * @brief Free flag mapping result
 *
 * Releases memory allocated for the mapping result.
 *
 * @param result Result to free
 */
UCRA_API void UCRA_CALL ucra_flag_map_result_free(UCRA_FlagMapResult* result);

/**
 * @brief Parse legacy flag string
 *
 * Utility function to parse legacy flag string in the format
 * "key1=val1;key2=val2" into an array of UCRA_KeyValue pairs.
 *
 * @param flag_str Flag string to parse
 * @param flags Pointer to store the parsed flags array
 * @param count Pointer to store the number of parsed flags
 * @return UCRA_SUCCESS on successful parsing
 */
UCRA_API UCRA_Result UCRA_CALL ucra_parse_legacy_flags(const char* flag_str, UCRA_KeyValue** flags, uint32_t* count);

/**
 * @brief Free legacy flags array
 *
 * Releases memory allocated by ucra_parse_legacy_flags().
 *
 * @param flags Flags array to free
 * @param count Number of flags in the array
 */
UCRA_API void UCRA_CALL ucra_free_legacy_flags(UCRA_KeyValue* flags, uint32_t count);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* UCRA_FLAG_MAPPER_H */
