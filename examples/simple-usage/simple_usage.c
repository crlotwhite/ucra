#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>

// 개별 예제 함수들을 호출하는 통합 예제
int run_basic_engine_example(void);
int run_manifest_example(void);
int run_simple_render_example(void);

int main() {
    printf("========================================\n");
    printf("          UCRA 간단한 사용법 예제        \n");
    printf("========================================\n\n");

    int overall_result = 0;

    // 1. 기본 엔진 예제
    printf("1. 기본 엔진 생명주기 예제\n");
    printf("---------------------------\n");
    int result1 = run_basic_engine_example();
    if (result1 != 0) {
        printf("❌ 기본 엔진 예제 실패\n");
        overall_result = 1;
    } else {
        printf("✅ 기본 엔진 예제 성공\n");
    }
    printf("\n");

    // 2. 매니페스트 예제
    printf("2. 매니페스트 사용 예제\n");
    printf("----------------------\n");
    int result2 = run_manifest_example();
    if (result2 != 0) {
        printf("❌ 매니페스트 예제 실패 (파일이 없을 수 있음)\n");
    } else {
        printf("✅ 매니페스트 예제 성공\n");
    }
    printf("\n");

    // 3. 간단한 렌더링 예제
    printf("3. 간단한 렌더링 예제\n");
    printf("--------------------\n");
    int result3 = run_simple_render_example();
    if (result3 != 0) {
        printf("❌ 렌더링 예제 실패\n");
        overall_result = 1;
    } else {
        printf("✅ 렌더링 예제 성공\n");
    }
    printf("\n");

    // 결과 요약
    printf("========================================\n");
    printf("           결과 요약                     \n");
    printf("========================================\n");
    printf("기본 엔진 예제: %s\n", result1 == 0 ? "✅ 성공" : "❌ 실패");
    printf("매니페스트 예제: %s\n", result2 == 0 ? "✅ 성공" : "❌ 실패");
    printf("렌더링 예제: %s\n", result3 == 0 ? "✅ 성공" : "❌ 실패");
    printf("\n");

    if (overall_result == 0) {
        printf("🎉 모든 핵심 예제가 성공적으로 실행되었습니다!\n");
        printf("\n다음 단계:\n");
        printf("- examples/basic-rendering/ 에서 더 자세한 오디오 렌더링 예제를 확인하세요\n");
        printf("- examples/advanced/ 에서 고급 엔진 통합 예제를 확인하세요\n");
    } else {
        printf("⚠️  일부 예제에서 오류가 발생했습니다.\n");
        printf("메인 UCRA 프로젝트가 올바르게 빌드되었는지 확인하세요.\n");
    }

    return overall_result;
}

int run_basic_engine_example(void) {
    // 엔진 생성
    UCRA_Handle engine;
    UCRA_Result result = ucra_engine_create(&engine, NULL, 0);

    if (result != UCRA_SUCCESS) {
        printf("엔진 생성 실패: 오류 코드 %d\n", result);
        return 1;
    }

    printf("✓ 엔진 생성 성공\n");

    // 엔진 정보 조회
    char info_buffer[256];
    result = ucra_engine_getinfo(engine, info_buffer, sizeof(info_buffer));

    if (result == UCRA_SUCCESS) {
        printf("✓ 엔진 정보: %s\n", info_buffer);
    } else {
        printf("⚠ 엔진 정보 조회 실패: 오류 코드 %d\n", result);
    }

    // 엔진 해제
    ucra_engine_destroy(engine);
    printf("✓ 엔진 해제 완료\n");

    return 0;
}

int run_manifest_example(void) {
    // 매니페스트 파일 로드
    UCRA_Manifest* manifest;
    UCRA_Result result = ucra_manifest_load("../../voicebank/resampler.json", &manifest);

    if (result != UCRA_SUCCESS) {
        // 현재 디렉토리에서도 시도
        result = ucra_manifest_load("voicebank/resampler.json", &manifest);
        if (result != UCRA_SUCCESS) {
            printf("⚠ 매니페스트 파일을 찾을 수 없습니다\n");
            printf("  시도한 경로: ../../voicebank/resampler.json, voicebank/resampler.json\n");
            return 1;
        }
    }

    printf("✓ 매니페스트 로드 성공\n");

    // 기본 정보 출력
    printf("  엔진명: %s\n", manifest->name ? manifest->name : "없음");
    printf("  버전: %s\n", manifest->version ? manifest->version : "없음");
    printf("  제작자: %s\n", manifest->vendor ? manifest->vendor : "없음");

    // 매니페스트 해제
    ucra_manifest_free(manifest);
    printf("✓ 매니페스트 해제 완료\n");

    return 0;
}

int run_simple_render_example(void) {
    // 엔진 생성
    UCRA_Handle engine;
    UCRA_Result result = ucra_engine_create(&engine, NULL, 0);

    if (result != UCRA_SUCCESS) {
        printf("엔진 생성 실패: 오류 코드 %d\n", result);
        return 1;
    }

    // 노트 설정 (A4, 440Hz, 0.5초)
    UCRA_NoteSegment note = {
        .start_sec = 0.0,
        .duration_sec = 0.5,    // 짧게 설정
        .midi_note = 69,        // A4 (440Hz)
        .velocity = 80,
        .lyric = "a",
        .f0_override = NULL,
        .env_override = NULL
    };

    // 렌더링 설정
    UCRA_RenderConfig config = {
        .sample_rate = 44100,
        .channels = 1,
        .block_size = 512,
        .flags = 0,
        .notes = &note,
        .note_count = 1,
        .options = NULL,
        .option_count = 0
    };

    printf("✓ 렌더링 설정: %uHz, %uch, A4 노트, %.1f초\n",
           config.sample_rate, config.channels, note.duration_sec);

    // 오디오 렌더링
    UCRA_RenderResult render_result;
    result = ucra_render(engine, &config, &render_result);

    if (result != UCRA_SUCCESS) {
        printf("렌더링 실패: 오류 코드 %d\n", result);
        ucra_engine_destroy(engine);
        return 1;
    }

    printf("✓ 렌더링 성공: %llu 프레임 생성\n", render_result.frames);

    // 엔진 해제
    ucra_engine_destroy(engine);
    printf("✓ 엔진 해제 완료\n");

    return 0;
}
