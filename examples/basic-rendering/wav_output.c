/*
 * UCRA WAV Output Example
 * 오디오를 WAV 파일로 출력하는 예제
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ucra/ucra.h"

// 간단한 WAV 파일 헤더 구조체
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t chunk_size;
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];           // "data"
    uint32_t data_size;
} WAVHeader;

// WAV 헤더 생성
void create_wav_header(WAVHeader* header, uint32_t sample_rate, uint32_t num_samples) {
    memcpy(header->riff, "RIFF", 4);
    memcpy(header->wave, "WAVE", 4);
    memcpy(header->fmt, "fmt ", 4);
    memcpy(header->data, "data", 4);

    header->fmt_size = 16;
    header->audio_format = 1; // PCM
    header->num_channels = 1; // 모노
    header->sample_rate = sample_rate;
    header->bits_per_sample = 16;
    header->block_align = header->num_channels * header->bits_per_sample / 8;
    header->byte_rate = header->sample_rate * header->block_align;
    header->data_size = num_samples * header->block_align;
    header->chunk_size = 36 + header->data_size;
}

int main(void) {
    printf("UCRA WAV Output Example\n");
    printf("=======================\n\n");

    // UCRA 엔진 생성
    UCRA_Handle engine = NULL;
    UCRA_Result result = ucra_engine_create(&engine, NULL, 0);
    if (result != UCRA_SUCCESS) {
        printf("❌ UCRA 엔진 생성 실패: %d\n", result);
        return 1;
    }

    printf("✓ UCRA 엔진 생성됨\n");

    // 노트 세그먼트 정의
    UCRA_NoteSegment note = {
        .start_sec = 0.0f,
        .duration_sec = 2.0f, // 2초
        .midi_note = 67, // G4
        .velocity = 120,
        .lyric = "sol",
        .f0_override = NULL,
        .env_override = NULL
    };

    // 렌더 설정
    UCRA_RenderConfig config;
    memset(&config, 0, sizeof(config));
    config.sample_rate = 44100;
    config.channels = 1; // 모노
    config.notes = &note;
    config.note_count = 1;

    printf("음성 렌더링 중 (노트: G4, 2초)...\n");

    // 렌더링 실행
    UCRA_RenderResult render_result;
    result = ucra_render(engine, &config, &render_result);
    if (result != UCRA_SUCCESS) {
        printf("❌ 렌더링 실패: %d\n", result);
        ucra_engine_destroy(engine);
        return 1;
    }

    printf("✓ 렌더링 완료 (%llu 프레임)\n", render_result.frames);

    // WAV 파일로 저장
    const char* filename = "output.wav";
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("❌ 파일 생성 실패: %s\n", filename);
        ucra_engine_destroy(engine);
        return 1;
    }

    // WAV 헤더 작성
    WAVHeader header;
    create_wav_header(&header, config.sample_rate, render_result.frames);
    fwrite(&header, sizeof(header), 1, file);

    // 오디오 데이터를 16비트 PCM으로 변환하여 작성
    for (uint64_t i = 0; i < render_result.frames; i++) {
        // float를 16비트 정수로 변환 (-1.0 ~ 1.0 -> -32768 ~ 32767)
        float sample = render_result.pcm[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        int16_t pcm_sample = (int16_t)(sample * 32767.0f);
        fwrite(&pcm_sample, sizeof(pcm_sample), 1, file);
    }

    fclose(file);
    printf("✓ WAV 파일 저장됨: %s\n", filename);

    // 정리
    ucra_engine_destroy(engine);
    printf("\n✓ 완료! 'play %s' 명령으로 재생할 수 있습니다.\n", filename);

    return 0;
}
