#include "ucra/ucra.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== UCRA 기본 엔진 예제 ===\n");

    // 1. 엔진 생성
    UCRA_Handle engine;
    UCRA_Result result = ucra_engine_create(&engine, NULL, 0);

    if (result != UCRA_SUCCESS) {
        printf("엔진 생성 실패: 오류 코드 %d\n", result);
        return 1;
    }

    printf("엔진 생성 성공\n");

    // 2. 엔진 정보 조회
    char info_buffer[256];
    result = ucra_engine_getinfo(engine, info_buffer, sizeof(info_buffer));

    if (result == UCRA_SUCCESS) {
        printf("엔진 정보: %s\n", info_buffer);
    } else {
        printf("엔진 정보 조회 실패: 오류 코드 %d\n", result);
    }

    // 3. 엔진 해제
    ucra_engine_destroy(engine);
    printf("엔진 해제 완료\n");

    return 0;
}
