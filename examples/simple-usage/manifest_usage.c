#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== 매니페스트 사용 예제 ===\n");

    // 1. 매니페스트 파일 로드 (샘플 보이스뱅크 경로)
    const char* manifest_path = "../../examples/sample-voicebank/resampler.json";
    UCRA_Manifest* manifest;
    UCRA_Result result = ucra_manifest_load(manifest_path, &manifest);

    if (result != UCRA_SUCCESS) {
        printf("매니페스트 로드 실패: 오류 코드 %d\n", result);
        printf("파일 경로를 확인하세요: %s\n", manifest_path);
        return 1;
    }

    printf("매니페스트 로드 성공: %s\n", manifest_path);

    // 2. 매니페스트 정보 출력
    printf("엔진명: %s\n", manifest->name ? manifest->name : "없음");
    printf("버전: %s\n", manifest->version ? manifest->version : "없음");
    printf("제작자: %s\n", manifest->vendor ? manifest->vendor : "없음");
    printf("라이선스: %s\n", manifest->license ? manifest->license : "없음");

    // 3. 오디오 지원 정보
    printf("지원되는 샘플 레이트: ");
    for (uint32_t i = 0; i < manifest->audio.rates_count; i++) {
        printf("%u", manifest->audio.rates[i]);
        if (i < manifest->audio.rates_count - 1) printf(", ");
    }
    printf(" Hz\n");

    printf("지원되는 채널 수: ");
    for (uint32_t i = 0; i < manifest->audio.channels_count; i++) {
        printf("%u", manifest->audio.channels[i]);
        if (i < manifest->audio.channels_count - 1) printf(", ");
    }
    printf("\n");

    printf("스트리밍 지원: %s\n", manifest->audio.streaming ? "예" : "아니오");

    // 4. 엔트리 포인트 정보
    printf("엔트리 타입: %s\n", manifest->entry.type ? manifest->entry.type : "없음");
    printf("엔트리 경로: %s\n", manifest->entry.path ? manifest->entry.path : "없음");

    // 5. 엔진 플래그 정보
    printf("지원되는 플래그 수: %u\n", manifest->flags_count);
    for (uint32_t i = 0; i < manifest->flags_count && i < 3; i++) {  // 최대 3개만 출력
        const UCRA_ManifestFlag* flag = &manifest->flags[i];
        printf("  플래그 %u: %s (%s) - %s\n",
               i + 1,
               flag->key ? flag->key : "없음",
               flag->type ? flag->type : "없음",
               flag->desc ? flag->desc : "설명 없음");
    }

    // 6. 매니페스트 해제
    ucra_manifest_free(manifest);
    printf("매니페스트 해제 완료\n");

    return 0;
}
