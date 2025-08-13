# UCRA 기본 렌더링 예제

이 예제는 UCRA를 사용한 실제적인 오디오 렌더링 방법을 보여줍니다.

## 빌드 및 실행

```bash
# 메인 프로젝트를 먼저 빌드
cd ../../
mkdir build && cd build
cmake ..
make

# 이 예제 빌드
cd ../examples/basic-rendering
mkdir build && cd build
cmake ..
make

# 실행
./basic_rendering
./multi_note_render
./streaming_example
```

## 예제 내용

### 1. basic_rendering.c

단일 노트를 렌더링하고 결과를 분석하는 기본 예제입니다.

### 2. multi_note_render.c

여러 노트를 동시에 렌더링하는 예제입니다.

### 3. streaming_example.c

스트리밍 API를 사용한 실시간 오디오 처리 예제입니다.

### 4. wav_output.c

렌더링 결과를 WAV 파일로 저장하는 예제입니다. (간단한 WAV 헤더 생성 포함)

## 특징

- 실제 오디오 데이터 분석 및 검증
- 다양한 렌더링 설정 테스트
- 오디오 품질 확인 방법
- 간단한 WAV 파일 출력
- 스트리밍 API 실전 사용법

## 출력 예시

```text
=== UCRA 기본 렌더링 예제 ===
A4 노트 (440Hz) 렌더링...
✓ 1초 분량 오디오 생성: 44100 샘플
✓ 오디오 품질 검증 통과
✓ 평균 진폭: 0.234
✓ 최대 진폭: 0.567

=== 다중 노트 렌더링 ===
C-E-G 화음 렌더링...
✓ 3개 노트 동시 렌더링 성공
✓ 총 길이: 2초 (88200 샘플)

=== 스트리밍 예제 ===
스트리밍 세션 시작...
✓ 10개 블록 처리 완료
✓ 총 5120 샘플 스트리밍
```

이 예제들을 통해 UCRA로 실제 음악 애플리케이션을 만드는 방법을 배울 수 있습니다.
