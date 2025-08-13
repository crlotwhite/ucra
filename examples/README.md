# UCRA Examples

이 디렉토리는 UCRA (Universal Choir Rendering API)의 다양한 사용법을 보여주는 예제들을 포함합니다.

## 📚 사용 가이드

### 초보자용 예제

#### 1. simple-usage/
UCRA API의 가장 기본적인 사용법을 배울 수 있습니다.

**포함 내용:**
- 엔진 생성과 해제
- 매니페스트 파일 로딩
- 간단한 오디오 렌더링

**시작하기:**
```bash
cd simple-usage
mkdir build && cd build
cmake ..
make
./simple_usage
```

#### 2. basic-rendering/
실제적인 오디오 렌더링 방법을 배울 수 있습니다.

**포함 내용:**
- 단일 노트 렌더링
- 다중 노트 동시 렌더링 (화음)
- 스트리밍 API 사용법
- WAV 파일 출력

**시작하기:**
```bash
cd basic-rendering
mkdir build && cd build
cmake ..
make
./basic_rendering
./multi_note_render
```

### 고급 사용자용 예제

#### 3. advanced/ucra-world-engine/
WORLD vocoder 엔진과의 완전한 통합 예제입니다.

**포함 내용:**
- 고품질 음성 합성 엔진 통합
- C++ 래퍼 구현
- 복잡한 빌드 시스템
- 종합적인 테스트 스위트

**요구사항:**
- CMake 3.10+
- C++11 호환 컴파일러
- Git (WORLD 라이브러리 다운로드용)

**시작하기:**
```bash
cd advanced/ucra-world-engine
mkdir build && cd build
cmake ..
make
ctest
```

## 🎯 학습 경로

1. **첫 단계**: `simple-usage/`로 UCRA API 기초 익히기
2. **두 번째**: `basic-rendering/`으로 실제 오디오 렌더링 배우기
3. **고급 단계**: `advanced/`에서 복잡한 엔진 통합 방법 학습

## 🛠️ 빌드 순서

모든 예제를 실행하기 전에 메인 UCRA 프로젝트를 먼저 빌드해야 합니다:

```bash
# 1. 메인 프로젝트 빌드
cd ../../
mkdir build && cd build
cmake ..
make

# 2. 원하는 예제 빌드 및 실행
cd ../examples/simple-usage
mkdir build && cd build
cmake ..
make
./simple_usage
```

## 📖 예제별 상세 설명

### simple-usage/
- **basic_engine.c**: 엔진 생명주기 관리
- **manifest_usage.c**: 매니페스트 파일 처리
- **simple_render.c**: 기본 렌더링
- **simple_usage.c**: 통합 예제

### basic-rendering/
- **basic_rendering.c**: 상세한 렌더링 분석
- **multi_note_render.c**: 화음과 멜로디 렌더링
- **streaming_example.c**: 실시간 스트리밍
- **wav_output.c**: 파일 출력 방법

### advanced/ucra-world-engine/
- 완전한 C++ 엔진 통합
- 고품질 음성 합성
- 산업용 수준의 구현

## 🎵 사용 사례별 가이드

### 간단한 음성 재생이 필요한 경우
→ `simple-usage/simple_render.c` 참조

### 화음이나 멜로디 렌더링이 필요한 경우
→ `basic-rendering/multi_note_render.c` 참조

### 실시간 오디오 스트리밍이 필요한 경우
→ `basic-rendering/streaming_example.c` 참조

### 파일로 오디오를 저장해야 하는 경우
→ `basic-rendering/wav_output.c` 참조

### 고품질 음성 합성 엔진이 필요한 경우
→ `advanced/ucra-world-engine/` 참조

## 🤝 기여하기

새로운 예제를 추가할 때:

1. 적절한 디렉토리에 배치 (simple-usage, basic-rendering, advanced)
2. 포괄적인 README.md 포함
3. 완전한 빌드 지침 제공
4. 테스트 커버리지 포함
5. 이 인덱스 파일 업데이트

## 📄 라이선스

예제들은 메인 UCRA 프로젝트와 동일한 라이선스를 따릅니다. 단, 일부 예제는 서드파티 라이브러리와 통합되어 별도의 라이선스 요구사항이 있을 수 있습니다.
