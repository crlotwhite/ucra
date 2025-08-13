/* Include the public API */
#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>

/* Provide stub implementations to validate signatures and linkage */
UCRA_Result UCRA_CALL
ucra_engine_create(UCRA_Handle* outEngine, const UCRA_KeyValue* options, uint32_t option_count) {
    (void)options; (void)option_count;
    if (!outEngine) return UCRA_ERR_INVALID_ARGUMENT;
    *outEngine = NULL; /* not allocated in stub */
    return UCRA_SUCCESS;
}

void UCRA_CALL
ucra_engine_destroy(UCRA_Handle engine) {
    (void)engine;
}

UCRA_Result UCRA_CALL
ucra_engine_getinfo(UCRA_Handle engine, char* outBuffer, size_t buffer_size) {
    (void)engine;
    if (!outBuffer || buffer_size == 0) return UCRA_ERR_INVALID_ARGUMENT;
    const char* s = "ucra-stub/0.0";
    size_t i = 0; while (s[i] && i + 1 < buffer_size) { outBuffer[i] = s[i]; i++; }
    outBuffer[i] = '\0';
    return UCRA_SUCCESS;
}

UCRA_Result UCRA_CALL
ucra_render(UCRA_Handle engine, const UCRA_RenderConfig* config, UCRA_RenderResult* outResult) {
    (void)engine; (void)config; (void)outResult;
    return UCRA_ERR_NOT_SUPPORTED;
}

int main(void) {
    printf("sizeof(UCRA_KeyValue)=%zu\n", (size_t)sizeof(UCRA_KeyValue));
    printf("sizeof(UCRA_F0Curve)=%zu\n", (size_t)sizeof(UCRA_F0Curve));
    printf("sizeof(UCRA_EnvCurve)=%zu\n", (size_t)sizeof(UCRA_EnvCurve));
    printf("sizeof(UCRA_NoteSegment)=%zu\n", (size_t)sizeof(UCRA_NoteSegment));
    printf("sizeof(UCRA_RenderConfig)=%zu\n", (size_t)sizeof(UCRA_RenderConfig));
    printf("sizeof(UCRA_RenderResult)=%zu\n", (size_t)sizeof(UCRA_RenderResult));
    return 0;
}
