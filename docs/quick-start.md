# UCRA 빠른 시작 가이드

이 가이드는 UCRA를 처음 사용하는 개발자를 위한 단계별 튜토리얼입니다.

## 🚀 5분 만에 시작하기

### 1단계: 프로젝트 빌드

```bash
# 저장소 클론
git clone <repository-url> ucra
cd ucra

# 빌드
mkdir build && cd build
cmake ..
make

# 테스트 (선택사항)
ctest
```

### 2단계: 첫 번째 프로그램 작성

`hello_ucra.c` 파일을 만들어보세요:

```c
#include "ucra/ucra.h"
#include <stdio.h>

int main() {
    // 엔진 생성
    UCRA_Handle engine;
    if (ucra_engine_create(&engine, NULL, 0) != UCRA_SUCCESS) {
        printf("엔진 생성 실패!\n");
        return 1;
    }

    // 간단한 노트 설정
    UCRA_NoteSegment note = {
        .start_sec = 0.0,
        .duration_sec = 1.0,
        .midi_note = 69,    // A4 (440Hz)
        .velocity = 80,
        .lyric = "hello"
    };

    // 렌더링 설정
    UCRA_RenderConfig config = {
        .sample_rate = 44100,
        .channels = 1,
        .notes = &note,
        .note_count = 1
    };

    // 렌더링 실행
    UCRA_RenderResult result;
    if (ucra_render(engine, &config, &result) == UCRA_SUCCESS) {
        printf("성공! %llu 프레임 생성됨\n", result.frames);
    }

    // 정리
    ucra_engine_destroy(engine);
    return 0;
}
```

### 3단계: 컴파일 및 실행

```bash
# 컴파일 (프로젝트 루트에서)
gcc -I include hello_ucra.c build/libucra_impl.a build/libcjson.a -lm -o hello_ucra

# 실행
./hello_ucra
```

## 📚 단계별 학습

### 초급: 기본 API 익히기

1. **[simple-usage 예제](../examples/simple-usage/)**
   ```bash
   cd examples/simple-usage
   mkdir build && cd build
   cmake .. && make
   ./simple_usage
   ```

2. **핵심 개념 이해**
   - `UCRA_Handle`: 엔진 인스턴스
   - `UCRA_NoteSegment`: 음표 정보
   - `UCRA_RenderConfig`: 렌더링 설정
   - `UCRA_RenderResult`: 결과 오디오

### 중급: 실제 오디오 렌더링

1. **[basic-rendering 예제](../examples/basic-rendering/)**
   ```bash
   cd examples/basic-rendering
   mkdir build && cd build
   cmake .. && make
   ./basic_rendering
   ./multi_note_render
   ```

2. **학습 내용**
   - 화음 렌더링 (여러 노트 동시에)
   - 스테레오 오디오 생성
   - 오디오 품질 분석
   - WAV 파일 저장

### 고급: 엔진 통합

1. **[advanced 예제](../examples/advanced/)**
   - 복잡한 C++ 엔진 통합
   - 고품질 음성 합성
   - 산업용 수준 구현

## ⚡ 일반적인 사용 패턴

### 패턴 1: 단일 노트 렌더링

```c
// 1. 엔진 생성
UCRA_Handle engine;
ucra_engine_create(&engine, NULL, 0);

// 2. 노트 설정
UCRA_NoteSegment note = {
    .duration_sec = 1.0,
    .midi_note = 60,  // C4
    .velocity = 80
};

// 3. 렌더링
UCRA_RenderConfig config = {
    .sample_rate = 44100,
    .channels = 1,
    .notes = &note,
    .note_count = 1
};

UCRA_RenderResult result;
ucra_render(engine, &config, &result);

// 4. 정리
ucra_engine_destroy(engine);
```

### 패턴 2: 화음 렌더링

```c
// 여러 노트를 배열로 준비
UCRA_NoteSegment chord[3] = {
    {.midi_note = 60, .duration_sec = 1.0},  // C
    {.midi_note = 64, .duration_sec = 1.0},  // E
    {.midi_note = 67, .duration_sec = 1.0}   // G
};

// 여러 노트를 한 번에 렌더링
UCRA_RenderConfig config = {
    .sample_rate = 44100,
    .channels = 1,
    .notes = chord,
    .note_count = 3  // 중요: 노트 개수 지정
};
```

### 패턴 3: 스트리밍 API

```c
// 콜백 함수 정의
UCRA_Result stream_callback(void* user_data, UCRA_RenderConfig* out_config) {
    // 다음 오디오 블록을 위한 노트 정보 제공
    // out_config에 노트 정보 설정
    return UCRA_SUCCESS;
}

// 스트림 시작
UCRA_StreamHandle stream;
UCRA_RenderConfig base_config = {
    .sample_rate = 44100,
    .channels = 1,
    .block_size = 512
};

ucra_stream_open(&stream, &base_config, stream_callback, NULL);

// 오디오 데이터 읽기
float buffer[512];
uint32_t frames_read;
ucra_stream_read(stream, buffer, 512, &frames_read);

// 스트림 종료
ucra_stream_close(stream);
```

## 🛠️ 자주 사용하는 설정

### 고품질 오디오

```c
UCRA_RenderConfig config = {
    .sample_rate = 48000,  // 고품질
    .channels = 2,         // 스테레오
    .block_size = 256      // 낮은 지연시간
};
```

### 실시간 처리

```c
UCRA_RenderConfig config = {
    .sample_rate = 44100,
    .channels = 1,
    .block_size = 128      // 매우 낮은 지연시간
};
```

### 오프라인 렌더링

```c
UCRA_RenderConfig config = {
    .sample_rate = 44100,
    .channels = 2,
    .block_size = 2048     // 높은 품질, 지연시간 무관
};
```

## 🔧 문제 해결

### 빌드 오류

```bash
# 필요한 라이브러리 확인
ls build/
# libucra_impl.a와 libcjson.a가 있어야 함

# 깨끗한 재빌드
rm -rf build
mkdir build && cd build
cmake ..
make
```

### 링킹 오류

```bash
# Unix/Linux/macOS
gcc ... -lm -lpthread

# pthread가 필요한 경우
pkg-config --libs --cflags ucra
```

### 런타임 오류

```c
// 항상 반환값 확인
UCRA_Result result = ucra_engine_create(&engine, NULL, 0);
if (result != UCRA_SUCCESS) {
    printf("엔진 생성 실패: %d\n", result);
    return 1;
}
```

## 📖 다음 단계

1. **[examples/ 디렉토리](../examples/)** 탐색
2. **[API 레퍼런스](api-reference.md)** 상세 읽기
3. **매니페스트 파일 형식** 학습
4. **커스텀 엔진 통합** 시도

## 🎯 실용적인 프로젝트 아이디어

- 간단한 음성 재생기
- MIDI 파일을 음성으로 변환하는 도구
- 실시간 가사 입력 음성 합성기
- VST 플러그인 (고급)

더 자세한 정보는 [메인 README](../README.md)와 [examples](../examples/) 디렉토리를 참조하세요!
