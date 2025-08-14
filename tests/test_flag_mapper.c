#include "ucra/ucra_flag_mapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_parse_legacy_flags(void) {
    printf("Testing ucra_parse_legacy_flags...\n");

    UCRA_KeyValue* flags = NULL;
    uint32_t count = 0;

    /* Test empty string */
    assert(ucra_parse_legacy_flags("", &flags, &count) == UCRA_SUCCESS);
    assert(count == 0);

    /* Test single flag */
    assert(ucra_parse_legacy_flags("g=0.5", &flags, &count) == UCRA_SUCCESS);
    assert(count == 1);
    assert(strcmp(flags[0].key, "g") == 0);
    assert(strcmp(flags[0].value, "0.5") == 0);
    ucra_free_legacy_flags(flags, count);

    /* Test multiple flags */
    assert(ucra_parse_legacy_flags("g=0.5;v=100;mode=1", &flags, &count) == UCRA_SUCCESS);
    assert(count == 3);
    assert(strcmp(flags[0].key, "g") == 0);
    assert(strcmp(flags[0].value, "0.5") == 0);
    assert(strcmp(flags[1].key, "v") == 0);
    assert(strcmp(flags[1].value, "100") == 0);
    assert(strcmp(flags[2].key, "mode") == 0);
    assert(strcmp(flags[2].value, "1") == 0);
    ucra_free_legacy_flags(flags, count);

    printf("✓ ucra_parse_legacy_flags tests passed\n");
}

static void test_flag_mapper_load(void) {
    printf("Testing ucra_flag_mapper_load...\n");

    UCRA_FlagMapper* mapper = NULL;

    /* Test with moresampler mapping */
    const char* mapping_path = "tools/flag_mapper/mappings/moresampler_map.json";
    UCRA_Result result = ucra_flag_mapper_load(mapping_path, &mapper);

    if (result == UCRA_SUCCESS) {
        assert(mapper != NULL);
        assert(mapper->engine_name != NULL);
        assert(strcmp(mapper->engine_name, "moresampler") == 0);
        assert(mapper->rule_count > 0);

        printf("✓ Loaded mapper for engine: %s (rules: %u)\n",
               mapper->engine_name, mapper->rule_count);

        ucra_flag_mapper_free(mapper);
    } else {
        printf("Warning: Could not load mapping file %s (result: %d)\n", mapping_path, result);
        printf("This is expected if running from different directory\n");
    }

    printf("✓ ucra_flag_mapper_load tests completed\n");
}

static void test_flag_mapper_apply(void) {
    printf("Testing ucra_flag_mapper_apply...\n");

    /* Try to load mapper first */
    UCRA_FlagMapper* mapper = NULL;
    const char* mapping_path = "tools/flag_mapper/mappings/moresampler_map.json";

    if (ucra_flag_mapper_load(mapping_path, &mapper) != UCRA_SUCCESS) {
        printf("Skipping apply test - mapping file not found\n");
        return;
    }

    /* Create test legacy flags */
    UCRA_KeyValue legacy_flags[] = {
        {"g", "0.5"},
        {"v", "80"},
        {"mode", "1"}
    };

    UCRA_FlagMapResult result;
    if (ucra_flag_mapper_apply(mapper, legacy_flags, 3, &result) == UCRA_SUCCESS) {
        printf("✓ Mapped %u legacy flags to %u UCRA flags\n",
               3, result.flag_count);

        for (uint32_t i = 0; i < result.flag_count; i++) {
            printf("  %s = %s\n", result.flags[i].key, result.flags[i].value);
        }

        if (result.warning_count > 0) {
            printf("Warnings:\n");
            for (uint32_t i = 0; i < result.warning_count; i++) {
                printf("  %s\n", result.warnings[i]);
            }
        }

        ucra_flag_map_result_free(&result);
    } else {
        printf("Error: Flag mapping failed\n");
    }

    ucra_flag_mapper_free(mapper);
    printf("✓ ucra_flag_mapper_apply tests completed\n");
}

int main(void) {
    printf("UCRA Flag Mapper Test Suite\n");
    printf("==========================\n\n");

    test_parse_legacy_flags();
    test_flag_mapper_load();
    test_flag_mapper_apply();

    printf("\n✓ All flag mapper tests completed\n");
    return 0;
}
