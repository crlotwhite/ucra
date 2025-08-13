#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    printf("=== UCRA 다중 노트 렌더링 예제 ===\n\n");

    // 엔진 생성
    UCRA_Handle engine;
    UCRA_Result result = ucra_engine_create(&engine, NULL, 0);

    if (result != UCRA_SUCCESS) {
        printf("❌ 엔진 생성 실패: 오류 코드 %d\n", result);
        return 1;
    }

    printf("✓ 엔진 생성 성공\n\n");

    // 1. 순차적 노트 렌더링 (멜로디)
    printf("1. 순차적 멜로디 렌더링\n");
    printf("----------------------\n");
    printf("Do-Re-Mi-Fa-Sol (C4-D4-E4-F4-G4) 순서로 렌더링\n\n");

    // C 메이저 스케일의 첫 5개 노트
    int melody_notes[] = {60, 62, 64, 65, 67};  // C4, D4, E4, F4, G4
    const char* note_names[] = {"C4", "D4", "E4", "F4", "G4"};
    const char* lyrics[] = {"do", "re", "mi", "fa", "sol"};
    int num_melody_notes = sizeof(melody_notes) / sizeof(melody_notes[0]);

    for (int i = 0; i < num_melody_notes; i++) {
        UCRA_NoteSegment note = {
            .start_sec = 0.0,
            .duration_sec = 0.8,
            .midi_note = melody_notes[i],
            .velocity = 90,
            .lyric = lyrics[i],
            .f0_override = NULL,
            .env_override = NULL
        };

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

        printf("렌더링 중: %s (%s)\n", note_names[i], lyrics[i]);

        UCRA_RenderResult render_result;
        result = ucra_render(engine, &config, &render_result);

        if (result == UCRA_SUCCESS) {
            printf("  ✓ %llu 프레임 생성\n", render_result.frames);

            // 간단한 품질 검사
            if (render_result.pcm && render_result.frames > 0) {
                float max_amplitude = 0.0f;
                uint64_t total_samples = render_result.frames * render_result.channels;

                for (uint64_t j = 0; j < total_samples; j++) {
                    float abs_sample = fabsf(render_result.pcm[j]);
                    if (abs_sample > max_amplitude) {
                        max_amplitude = abs_sample;
                    }
                }
                printf("  📊 최대 진폭: %.3f\n", max_amplitude);
            }
        } else {
            printf("  ❌ 렌더링 실패: 오류 코드 %d\n", result);
        }
        printf("\n");
    }

    // 2. 동시 노트 렌더링 (화음)
    printf("2. 동시 화음 렌더링\n");
    printf("------------------\n");
    printf("C 메이저 화음 (C4-E4-G4) 동시 렌더링\n\n");

    // C 메이저 화음
    UCRA_NoteSegment chord_notes[3] = {
        {
            .start_sec = 0.0,
            .duration_sec = 2.0,
            .midi_note = 60,        // C4
            .velocity = 85,
            .lyric = "do",
            .f0_override = NULL,
            .env_override = NULL
        },
        {
            .start_sec = 0.0,
            .duration_sec = 2.0,
            .midi_note = 64,        // E4
            .velocity = 80,
            .lyric = "mi",
            .f0_override = NULL,
            .env_override = NULL
        },
        {
            .start_sec = 0.0,
            .duration_sec = 2.0,
            .midi_note = 67,        // G4
            .velocity = 75,
            .lyric = "sol",
            .f0_override = NULL,
            .env_override = NULL
        }
    };

    UCRA_RenderConfig chord_config = {
        .sample_rate = 44100,
        .channels = 1,
        .block_size = 1024,
        .flags = 0,
        .notes = chord_notes,
        .note_count = 3,           // 3개 노트 동시에!
        .options = NULL,
        .option_count = 0
    };

    printf("렌더링 중: C 메이저 화음 (C4 + E4 + G4)\n");

    UCRA_RenderResult chord_result;
    result = ucra_render(engine, &chord_config, &chord_result);

    if (result == UCRA_SUCCESS) {
        printf("✓ 화음 렌더링 성공!\n");
        printf("  프레임 수: %llu\n", chord_result.frames);
        printf("  길이: %.2f초\n", (float)chord_result.frames / chord_result.sample_rate);

        // 화음의 복잡성 분석
        if (chord_result.pcm && chord_result.frames > 0) {
            float rms = 0.0f;
            uint64_t total_samples = chord_result.frames * chord_result.channels;

            for (uint64_t i = 0; i < total_samples; i++) {
                rms += chord_result.pcm[i] * chord_result.pcm[i];
            }
            rms = sqrtf(rms / total_samples);

            printf("  📊 RMS 레벨: %.3f (화음의 풍부함 지표)\n", rms);
        }
    } else {
        printf("❌ 화음 렌더링 실패: 오류 코드 %d\n", result);
    }
    printf("\n");

    // 3. 복잡한 시퀀스 (겹치는 노트들)
    printf("3. 복잡한 노트 시퀀스\n");
    printf("--------------------\n");
    printf("시간차를 두고 시작하는 여러 노트들\n\n");

    UCRA_NoteSegment sequence_notes[4] = {
        {
            .start_sec = 0.0,
            .duration_sec = 1.5,
            .midi_note = 60,        // C4
            .velocity = 90,
            .lyric = "do",
            .f0_override = NULL,
            .env_override = NULL
        },
        {
            .start_sec = 0.5,       // 0.5초 후 시작
            .duration_sec = 1.5,
            .midi_note = 64,        // E4
            .velocity = 85,
            .lyric = "mi",
            .f0_override = NULL,
            .env_override = NULL
        },
        {
            .start_sec = 1.0,       // 1초 후 시작
            .duration_sec = 1.5,
            .midi_note = 67,        // G4
            .velocity = 80,
            .lyric = "sol",
            .f0_override = NULL,
            .env_override = NULL
        },
        {
            .start_sec = 1.5,       // 1.5초 후 시작
            .duration_sec = 1.0,
            .midi_note = 72,        // C5
            .velocity = 95,
            .lyric = "do",
            .f0_override = NULL,
            .env_override = NULL
        }
    };

    UCRA_RenderConfig sequence_config = {
        .sample_rate = 44100,
        .channels = 2,             // 스테레오로 렌더링
        .block_size = 1024,
        .flags = 0,
        .notes = sequence_notes,
        .note_count = 4,
        .options = NULL,
        .option_count = 0
    };

    printf("렌더링 중: 4개 노트의 시간차 시퀀스 (스테레오)\n");
    printf("  C4 (0.0s) → E4 (0.5s) → G4 (1.0s) → C5 (1.5s)\n");

    UCRA_RenderResult sequence_result;
    result = ucra_render(engine, &sequence_config, &sequence_result);

    if (result == UCRA_SUCCESS) {
        printf("✓ 시퀀스 렌더링 성공!\n");
        printf("  총 프레임: %llu\n", sequence_result.frames);
        printf("  총 길이: %.2f초\n", (float)sequence_result.frames / sequence_result.sample_rate);
        printf("  채널 수: %u (스테레오)\n", sequence_result.channels);

        // 스테레오 정보 분석
        if (sequence_result.pcm && sequence_result.frames > 0 && sequence_result.channels == 2) {
            float left_rms = 0.0f, right_rms = 0.0f;

            for (uint64_t i = 0; i < sequence_result.frames; i++) {
                float left = sequence_result.pcm[i * 2];
                float right = sequence_result.pcm[i * 2 + 1];
                left_rms += left * left;
                right_rms += right * right;
            }

            left_rms = sqrtf(left_rms / sequence_result.frames);
            right_rms = sqrtf(right_rms / sequence_result.frames);

            printf("  📊 좌측 채널 RMS: %.3f\n", left_rms);
            printf("  📊 우측 채널 RMS: %.3f\n", right_rms);
        }
    } else {
        printf("❌ 시퀀스 렌더링 실패: 오류 코드 %d\n", result);
    }
    printf("\n");

    // 4. 정리
    ucra_engine_destroy(engine);
    printf("✓ 엔진 해제 완료\n\n");

    printf("🎉 다중 노트 렌더링 예제가 완료되었습니다!\n");
    printf("\n학습한 내용:\n");
    printf("- 순차적 멜로디 렌더링\n");
    printf("- 동시 화음 렌더링 (여러 노트를 한 번에)\n");
    printf("- 시간차가 있는 복잡한 시퀀스\n");
    printf("- 스테레오 렌더링\n");
    printf("- 오디오 품질 분석 기법\n");

    return 0;
}
