/*
 * Test WORLD Engine Lifecycle Functions
 * Tests ucra_engine_create, ucra_engine_getinfo, and ucra_engine_destroy
 */

#include "ucra/ucra.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int test_engine_lifecycle() {
    printf("Testing WORLD engine lifecycle...\n");

    UCRA_Handle engine = NULL;
    UCRA_Result result;

    /* Test engine creation */
    printf("1. Testing engine creation...\n");
    result = ucra_engine_create(&engine, NULL, 0);

    if (result != UCRA_SUCCESS) {
        printf("   ❌ Engine creation failed with error %d\n", result);
        return 1;
    }

    if (engine == NULL) {
        printf("   ❌ Engine handle is NULL after creation\n");
        return 1;
    }

    printf("   ✅ Engine created successfully\n");

    /* Test engine info */
    printf("2. Testing engine info...\n");
    char info_buffer[512];
    result = ucra_engine_getinfo(engine, info_buffer, sizeof(info_buffer));

    if (result != UCRA_SUCCESS) {
        printf("   ❌ Failed to get engine info with error %d\n", result);
        ucra_engine_destroy(engine);
        return 1;
    }

    printf("   ✅ Engine info: %s\n", info_buffer);

    /* Test engine creation with options */
    printf("3. Testing engine creation with options...\n");
    UCRA_KeyValue options[] = {
        {"sample_rate", "48000"},
        {"frame_period", "10.0"}
    };

    UCRA_Handle engine2 = NULL;
    result = ucra_engine_create(&engine2, options, 2);

    if (result != UCRA_SUCCESS) {
        printf("   ❌ Engine creation with options failed with error %d\n", result);
        ucra_engine_destroy(engine);
        return 1;
    }

    char info_buffer2[512];
    result = ucra_engine_getinfo(engine2, info_buffer2, sizeof(info_buffer2));

    if (result == UCRA_SUCCESS) {
        printf("   ✅ Engine with options info: %s\n", info_buffer2);
    }

    /* Test engine destruction */
    printf("4. Testing engine destruction...\n");
    ucra_engine_destroy(engine);
    ucra_engine_destroy(engine2);
    printf("   ✅ Engines destroyed successfully\n");

    /* Test invalid arguments */
    printf("5. Testing invalid arguments...\n");

    result = ucra_engine_create(NULL, NULL, 0);
    if (result != UCRA_ERR_INVALID_ARGUMENT) {
        printf("   ❌ Expected INVALID_ARGUMENT for NULL engine handle\n");
        return 1;
    }

    result = ucra_engine_getinfo(NULL, info_buffer, sizeof(info_buffer));
    if (result != UCRA_ERR_INVALID_ARGUMENT) {
        printf("   ❌ Expected INVALID_ARGUMENT for NULL engine in getinfo\n");
        return 1;
    }

    printf("   ✅ Invalid argument tests passed\n");

    return 0;
}

int main() {
    printf("=== UCRA WORLD Engine Lifecycle Tests ===\n\n");

    int result = test_engine_lifecycle();

    if (result == 0) {
        printf("\n🎉 All engine lifecycle tests passed!\n");
    } else {
        printf("\n❌ Some tests failed!\n");
    }

    return result;
}
