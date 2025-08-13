/*
 * UCRA Basic Audio Example
 * 기본 오디오 렌더링을 통한 간단한 예제
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "ucra/ucra.h"

int main(void) {
    printf("UCRA Basic Audio Example\n");
    printf("========================\n\n");

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
        .duration_sec = 1.0f,
        .midi_note = 60, // C4
        .velocity = 100,
        .lyric = "la",
        .f0_override = NULL,
        .env_override = NULL
    };

    // 기본 렌더 설정
    UCRA_RenderConfig config;
    memset(&config, 0, sizeof(config));
    config.sample_rate = 44100;
    config.channels = 1; // 모노
    config.notes = &note;
    config.note_count = 1;

    printf("\n기본 오디오 렌더링 중...\n");

    UCRA_RenderResult render_result;
    result = ucra_render(engine, &config, &render_result);
    if (result != UCRA_SUCCESS) {
        printf("❌ 렌더링 실패: %d\n", result);
        ucra_engine_destroy(engine);
        return 1;
    }

    printf("✓ 렌더링 성공\n");
    printf("  - 프레임 수: %llu\n", render_result.frames);
    printf("  - 채널 수: %u\n", render_result.channels);
    printf("  - 샘플레이트: %u Hz\n", render_result.sample_rate);
    printf("  - 길이: %.2f 초\n", (float)render_result.frames / render_result.sample_rate);

    // 오디오 데이터 품질 확인
    if (render_result.pcm && render_result.frames > 0) {
        float peak = 0.0f;
        for (uint64_t i = 0; i < render_result.frames * render_result.channels; i++) {
            float sample = fabsf(render_result.pcm[i]);
            if (sample > peak) peak = sample;
        }
        printf("  - 피크 레벨: %.3f\n", peak);

        if (peak > 0.001f) {
            printf("  ✓ 오디오 신호 감지됨\n");
        } else {
            printf("  ⚠ 오디오 신호가 매우 낮거나 무음\n");
        }
    }

    printf("\n정리 중...\n");
    ucra_engine_destroy(engine);
    printf("✓ 완료\n");

    return 0;
}
