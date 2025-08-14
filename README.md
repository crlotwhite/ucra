# UCRA (Universal Choir Rendering API)

UCRA는 음성 합성 엔진을 위한 범용 C API입니다. UTAU, OpenUtau와 같은 음성 편집기와 다양한 음성 합성 엔진을 연결하는 표준화된 인터페이스를 제공합니다.

## ✨ 주요 기능

- **🎵 음성 합성 엔진 추상화**: 다양한 음성 합성 엔진을 통합된 API로 사용
- **📝 매니페스트 시스템**: JSON 기반 엔진 설정 및 메타데이터 관리
- **🔄 스트리밍 API**: 실시간 오디오 스트리밍 지원
- **🛠️ UTAU 호환성**: 기존 UTAU resampler.exe 완전 호환
- **🌐 크로스 플랫폼**: Windows, macOS, Linux 지원
- **⚡ C99 호환**: 최대 호환성을 위한 표준 C 인터페이스

## 🚀 빠른 시작

### 필요 조건

- CMake 3.18+
- C99 호환 컴파일러 (GCC, Clang, MSVC)
- Git

### 빌드

```bash
git clone <repository-url> ucra
cd ucra
mkdir build && cd build
cmake ..
make

# 선택사항: 시스템에 설치 (예제 빌드가 더 간단해짐)
sudo make install
```

### 빌드 옵션

UCRA는 다양한 빌드 옵션을 제공합니다:

```bash
# 기본 빌드 (라이브러리와 예제만)
cmake ..

# 예제 비활성화
cmake -DUCRA_BUILD_EXAMPLES=OFF ..

# 검증 및 테스트 도구 활성화 (기본값: OFF)
cmake -DUCRA_BUILD_TOOLS=ON ..

# 모든 옵션 함께 사용
cmake -DUCRA_BUILD_EXAMPLES=ON -DUCRA_BUILD_TOOLS=ON ..
```

#### 검증 및 테스트 도구 (`UCRA_BUILD_TOOLS=ON`)

tools 옵션을 활성화하면 다음 도구들이 빌드됩니다:

- **validation_suite**: 종합 검증 도구
- **f0_rmse_calc**: F0 RMSE 계산 유틸리티
- **mcd_calc**: MCD(13) 계산 유틸리티
- **audio_compare**: 오디오 파일 비교 도구
- **golden_runner**: Golden 테스트 하네스

자세한 내용은 [tools/README.md](tools/README.md)를 참조하세요.

### 테스트 실행

```bash
# 모든 테스트 실행
ctest

# 개별 테스트 실행
./test_suite
./test_manifest
./test_streaming_integration
```

## 📖 기본 사용법

### 1. 엔진 생성 및 기본 렌더링

```c
#include "ucra/ucra.h"

int main() {
    // 엔진 생성
    UCRA_Handle engine;
    UCRA_Result result = ucra_engine_create(&engine, NULL, 0);
    if (result != UCRA_SUCCESS) {
        printf("엔진 생성 실패: %d\n", result);
        return 1;
    }

    // 노트 설정
    UCRA_NoteSegment note = {
        .start_sec = 0.0,
        .duration_sec = 1.0,
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

    // 오디오 렌더링
    UCRA_RenderResult render_result;
    result = ucra_render(engine, &config, &render_result);
    if (result == UCRA_SUCCESS) {
        printf("렌더링 완료: %llu 프레임\n", render_result.frames);
        // render_result.pcm에 오디오 데이터 포함
    }

    // 정리
    ucra_engine_destroy(engine);
    return 0;
}
```

### 2. 매니페스트 파일 사용

```c
#include "ucra/ucra.h"

int main() {
    // 매니페스트 로드
    UCRA_Manifest* manifest;
    UCRA_Result result = ucra_manifest_load("voicebank/resampler.json", &manifest);

    if (result == UCRA_SUCCESS) {
        printf("엔진: %s v%s\n", manifest->name, manifest->version);
        printf("제작자: %s\n", manifest->vendor);

        // 지원되는 샘플 레이트 확인
        for (uint32_t i = 0; i < manifest->audio.rates_count; i++) {
            printf("지원 샘플 레이트: %u Hz\n", manifest->audio.rates[i]);
        }
    }

    // 매니페스트 해제
    ucra_manifest_free(manifest);
    return 0;
}
```

### 3. 스트리밍 API

```c
#include "ucra/ucra.h"

// 스트리밍 콜백 함수
UCRA_Result stream_callback(void* user_data, UCRA_RenderConfig* out_config) {
    // 다음 오디오 블록을 위한 노트 정보 제공
    static UCRA_NoteSegment note = {
        .start_sec = 0.0,
        .duration_sec = 0.1,  // 짧은 블록
        .midi_note = 69,
        .velocity = 80,
        .lyric = "a"
    };

    out_config->notes = &note;
    out_config->note_count = 1;
    return UCRA_SUCCESS;
}

int main() {
    UCRA_RenderConfig config = {
        .sample_rate = 44100,
        .channels = 1,
        .block_size = 512
    };

    // 스트림 열기
    UCRA_StreamHandle stream;
    UCRA_Result result = ucra_stream_open(&stream, &config, stream_callback, NULL);

    if (result == UCRA_SUCCESS) {
        float buffer[512];
        uint32_t frames_read;

        // 오디오 블록 읽기
        result = ucra_stream_read(stream, buffer, 512, &frames_read);
        if (result == UCRA_SUCCESS) {
            printf("읽은 프레임: %u\n", frames_read);
        }

        // 스트림 닫기
        ucra_stream_close(stream);
    }

    return 0;
}
```

## 🛠️ CLI 도구 (resampler.exe 호환)

UCRA는 기존 UTAU resampler.exe와 완전 호환되는 CLI 도구를 제공합니다:

```bash
# 기본 렌더링
./resampler --input input.wav --output output.wav --note C4 --tempo 120

# 고급 옵션
./resampler --input input.wav --output output.wav \
           --note C4 --tempo 120 \
           --flags "g+10,Y50" \
           --f0-curve f0.txt \
           --vb-root /path/to/voicebank \
           --rate 48000
```

## 📁 프로젝트 구조

```text
ucra/
├── include/ucra/           # 공개 헤더 파일
│   └── ucra.h             # 메인 API 헤더
├── src/                   # 구현 소스 코드
│   ├── ucra_manifest.c    # 매니페스트 파싱
│   ├── ucra_streaming.c   # 스트리밍 API
│   └── resampler_cli.c    # CLI 도구
├── examples/              # 사용 예제
│   ├── simple-usage/      # 기본 API 사용법
│   ├── basic-rendering/   # 오디오 렌더링 예제
│   └── advanced/          # 고급 예제 (WORLD 엔진 등)
├── tests/                 # 테스트 코드
├── schemas/               # JSON 스키마
├── voicebank/            # 테스트용 보이스뱅크
└── docs/                 # 문서
```

## 📚 문서

- [빠른 시작 가이드](docs/quick-start.md)
- [API 레퍼런스](docs/api-reference.md)
- [매니페스트 파일 형식](schemas/resampler.schema.json)
- [예제 모음](examples/)

## 🎯 사용 사례

- **음성 편집기**: UTAU, OpenUtau와 같은 편집기와 엔진 연동
- **플러그인 개발**: VST/AU 플러그인에서 음성 합성 엔진 활용
- **스탠드얼론 애플리케이션**: 음성 합성 기능이 필요한 애플리케이션
- **연구 및 실험**: 새로운 음성 합성 알고리즘 프로토타이핑

## 🔧 빌드 결과물

빌드 후 다음 파일들이 생성됩니다:

- **libucra_impl.a**: UCRA 정적 라이브러리
- **resampler**: UTAU 호환 CLI 도구
- **test_\***: 각종 테스트 실행파일

## 🤝 기여하기

1. 이 저장소를 포크하세요
2. 기능 브랜치를 만드세요 (`git checkout -b feature/AmazingFeature`)
3. 변경사항을 커밋하세요 (`git commit -m 'Add some AmazingFeature'`)
4. 브랜치에 푸시하세요 (`git push origin feature/AmazingFeature`)
5. Pull Request를 열어주세요

## 📄 라이선스

이 프로젝트는 [라이선스명] 라이선스 하에 배포됩니다. 자세한 내용은 `LICENSE` 파일을 참조하세요.

## 🙏 감사의 말

- cJSON 라이브러리 개발팀
- UTAU 및 OpenUtau 커뮤니티
- 모든 기여자분들

---

더 자세한 정보는 [examples/](examples/) 디렉토리의 예제들을 참조하세요.
