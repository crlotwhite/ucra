/*
 * WORLD Engine Example Stub
 * This file demonstrates how a third-party engine could be wrapped.
 * The original full implementation was removed from core; this is a placeholder.
 */
#include "ucra/ucra.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

extern "C" {

UCRA_Result ucra_engine_create(UCRA_Handle* outEngine,
                               const UCRA_KeyValue* options,
                               uint32_t option_count) {
    (void)options; (void)option_count;
    if(!outEngine) return UCRA_ERR_INVALID_ARGUMENT;
    *outEngine = reinterpret_cast<UCRA_Handle>(0x1); // dummy non-null
    return UCRA_SUCCESS;
}

void ucra_engine_destroy(UCRA_Handle engine) {
    (void)engine; // nothing
}

UCRA_Result ucra_engine_getinfo(UCRA_Handle engine,
                                char* outBuffer,
                                size_t buffer_size) {
    if(!engine || !outBuffer || buffer_size < 8) return UCRA_ERR_INVALID_ARGUMENT;
    const char* info = "WORLD Stub 0.1";
    if(strlen(info) >= buffer_size) return UCRA_ERR_INVALID_ARGUMENT;
    std::strcpy(outBuffer, info);
    return UCRA_SUCCESS;
}

UCRA_Result ucra_render(UCRA_Handle engine,
                        const UCRA_RenderConfig* config,
                        UCRA_RenderResult* outResult) {
    if(!engine || !config || !outResult) return UCRA_ERR_INVALID_ARGUMENT;
    // Produce silent buffer for demonstration
    uint32_t frames = 512;
    float* pcm = (float*)std::calloc(frames * config->channels, sizeof(float));
    if(!pcm) return UCRA_ERR_OUT_OF_MEMORY;
    outResult->pcm = pcm;
    outResult->frames = frames;
    outResult->channels = config->channels;
    outResult->sample_rate = config->sample_rate;
    outResult->metadata = nullptr;
    outResult->metadata_count = 0;
    outResult->status = UCRA_SUCCESS;
    return UCRA_SUCCESS;
}

} // extern "C"
