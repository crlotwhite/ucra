/*
 * UCRA Basic Rendering Example
 * UCRA SDK의 기본 렌더링 기능을 보여주는 예제
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ucra/ucra.h"

// 오디오 분석 도우미 함수
void analyze_audio(const float* pcm, uint64_t frames, uint32_t channels, uint32_t sample_rate) {
    if (!pcm || frames == 0) {
        printf("  ⚠ 오디오 데이터가 비어있음\n");
        return;
    }

    // 피크와 RMS 계산
    float peak = 0.0f;
    float rms_sum = 0.0f;

    for (uint64_t i = 0; i < frames * channels; i++) {
        float sample = fabsf(pcm[i]);
        if (sample > peak) peak = sample;
        rms_sum += sample * sample;
    }

    float rms = sqrtf(rms_sum / (frames * channels));

    printf("  📊 오디오 분석:\n");
    printf("     - 총 프레임: %llu\n", frames);
    printf("     - 채널: %u\n", channels);
    printf("     - 샘플레이트: %u Hz\n", sample_rate);
    printf("     - 길이: %.2f 초\n", (float)frames / sample_rate);
    printf("     - 피크: %.3f\n", peak);
    printf("     - RMS: %.3f\n", rms);

    if (peak > 0.95f) {
        printf("     ⚠ 클리핑 가능성\n");
    } else if (peak < 0.01f) {
        printf("     ⚠ 매우 낮은 볼륨\n");
    } else {
        printf("     ✓ 정상 레벨\n");
    }
}

int main(void) {
    printf("UCRA Basic Rendering Example\n");
    printf("============================\n\n");

    // 1. UCRA 엔진 생성
    printf("1. UCRA 엔진 초기화\n");
    printf("------------------\n");

    UCRA_Handle engine = NULL;
    UCRA_Result result = ucra_engine_create(&engine, NULL, 0);

    if (result != UCRA_SUCCESS) {
        printf("❌ UCRA 엔진 생성 실패: 오류 코드 %d\n", result);
        return 1;
    }

    printf("✓ UCRA 엔진 생성됨\n\n");

    // 2. 엔진 정보 조회
    printf("2. 엔진 정보\n");
    printf("-----------\n");

    char engine_info[512];
    result = ucra_engine_getinfo(engine, engine_info, sizeof(engine_info));

    if (result == UCRA_SUCCESS) {
        printf("✓ 엔진 정보 조회 성공\n");
        printf("  %s\n", engine_info);
    } else {
        printf("⚠ 엔진 정보 조회 실패: 오류 코드 %d\n", result);
    }
    printf("\n");

    // 3. 기본 노트 렌더링
    printf("3. 기본 노트 렌더링\n");
    printf("------------------\n");

    // 노트 세그먼트 정의
    UCRA_NoteSegment note_c4 = {
        .start_sec = 0.0f,
        .duration_sec = 1.5f,
        .midi_note = 60,        // C4
        .velocity = 80,
        .lyric = "do",
        .f0_override = NULL,
        .env_override = NULL
    };

    // 렌더링 설정
    UCRA_RenderConfig config;
    memset(&config, 0, sizeof(config));
    config.sample_rate = 44100;
    config.channels = 1;  // 모노
    config.notes = &note_c4;
    config.note_count = 1;

    printf("렌더링 시작: C4 (261.63Hz), 1.5초\n");

    UCRA_RenderResult render_result;
    result = ucra_render(engine, &config, &render_result);

    if (result != UCRA_SUCCESS) {
        printf("❌ C4 렌더링 실패: 오류 코드 %d\n", result);
        ucra_engine_destroy(engine);
        return 1;
    }

    printf("✓ C4 렌더링 성공\n");
    analyze_audio(render_result.pcm, render_result.frames, render_result.channels, render_result.sample_rate);
    printf("\n");

    // 4. 스테레오 렌더링
    printf("4. 스테레오 렌더링\n");
    printf("-----------------\n");

    // 스테레오 설정으로 변경
    config.channels = 2;

    printf("렌더링 시작: C4 스테레오\n");

    result = ucra_render(engine, &config, &render_result);

    if (result != UCRA_SUCCESS) {
        printf("❌ 스테레오 렌더링 실패: 오류 코드 %d\n", result);
    } else {
        printf("✓ 스테레오 렌더링 성공\n");
        analyze_audio(render_result.pcm, render_result.frames, render_result.channels, render_result.sample_rate);
    }
    printf("\n");

    // 5. 높은 노트 렌더링
    printf("5. 높은 노트 렌더링\n");
    printf("------------------\n");

    UCRA_NoteSegment note_c5 = {
        .start_sec = 0.0f,
        .duration_sec = 1.0f,
        .midi_note = 72,        // C5 (한 옥타브 위)
        .velocity = 100,
        .lyric = "do",
        .f0_override = NULL,
        .env_override = NULL
    };

    config.channels = 1;  // 모노로 복원
    config.notes = &note_c5;

    printf("렌더링 시작: C5 (523.25Hz), 1.0초\n");

    result = ucra_render(engine, &config, &render_result);

    if (result != UCRA_SUCCESS) {
        printf("❌ C5 렌더링 실패: 오류 코드 %d\n", result);
    } else {
        printf("✓ C5 렌더링 성공\n");
        analyze_audio(render_result.pcm, render_result.frames, render_result.channels, render_result.sample_rate);
    }
    printf("\n");

    // 6. 정리
    printf("6. 정리\n");
    printf("------\n");

    ucra_engine_destroy(engine);
    printf("✓ UCRA 엔진 해제됨\n");
    printf("\n🎵 기본 렌더링 예제 완료!\n");

    return 0;
}
