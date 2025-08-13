#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== 간단한 렌더링 예제 ===\n");

    // 1. 엔진 생성
    UCRA_Handle engine;
    UCRA_Result result = ucra_engine_create(&engine, NULL, 0);

    if (result != UCRA_SUCCESS) {
        printf("엔진 생성 실패: 오류 코드 %d\n", result);
        return 1;
    }

    // 2. 노트 설정 (A4, 440Hz, 1초)
    UCRA_NoteSegment note = {
        .start_sec = 0.0,
        .duration_sec = 1.0,
        .midi_note = 69,        // A4 (440Hz)
        .velocity = 80,
        .lyric = "a",
        .f0_override = NULL,
        .env_override = NULL
    };

    // 3. 렌더링 설정
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

    printf("렌더링 설정: %uHz, %uch, A4 노트, %.1f초\n",
           config.sample_rate, config.channels, note.duration_sec);

    // 4. 오디오 렌더링
    UCRA_RenderResult render_result;
    result = ucra_render(engine, &config, &render_result);

    if (result != UCRA_SUCCESS) {
        printf("렌더링 실패: 오류 코드 %d\n", result);
        ucra_engine_destroy(engine);
        return 1;
    }

    printf("렌더링 성공: %llu 프레임 생성\n", render_result.frames);
    printf("오디오 데이터 주소: %p\n", (void*)render_result.pcm);
    printf("실제 채널 수: %u\n", render_result.channels);
    printf("실제 샘플 레이트: %u Hz\n", render_result.sample_rate);

    // 5. 첫 번째 오디오 샘플들 출력 (디버깅용)
    if (render_result.pcm && render_result.frames > 0) {
        printf("첫 10개 오디오 샘플: ");
        uint64_t samples_to_show = render_result.frames < 10 ? render_result.frames : 10;
        for (uint64_t i = 0; i < samples_to_show; i++) {
            printf("%.3f ", render_result.pcm[i]);
        }
        printf("\n");
    }

    // 6. 메타데이터 정보 (있는 경우)
    if (render_result.metadata_count > 0) {
        printf("메타데이터 항목 수: %u\n", render_result.metadata_count);
        for (uint32_t i = 0; i < render_result.metadata_count && i < 3; i++) {
            printf("  %s: %s\n",
                   render_result.metadata[i].key ? render_result.metadata[i].key : "없음",
                   render_result.metadata[i].value ? render_result.metadata[i].value : "없음");
        }
    }

    // 7. 엔진 해제
    ucra_engine_destroy(engine);
    printf("엔진 해제 완료\n");

    return 0;
}
