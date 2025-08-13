# UCRA 간단한 사용법 예제

이 예제는 UCRA API의 가장 기본적인 사용법을 보여줍니다.

## 빌드 및 실행

### 방법 1: UCRA를 시스템에 설치한 경우 (권장)

```bash
# 메인 프로젝트 빌드 및 설치
cd ../../
mkdir build && cd build
cmake ..
make
sudo make install

# 이 예제 빌드 (매우 간단!)
cd ../examples/simple-usage
mkdir build && cd build
cmake ..
make

# 실행
./simple_usage
```

### 방법 2: 로컬 빌드 사용

```bash
# 메인 프로젝트를 먼저 빌드
cd ../../
mkdir build && cd build
cmake ..
make

# 이 예제 빌드
cd ../examples/simple-usage
mkdir build && cd build
cmake ..
make

# 실행
./simple_usage
```

## 예제 내용

### 1. basic_engine.c

엔진 생성, 정보 조회, 해제의 기본 생명주기를 보여줍니다.

### 2. manifest_usage.c

매니페스트 파일 로딩과 엔진 정보 읽기를 보여줍니다.

### 3. simple_render.c

가장 간단한 오디오 렌더링 예제입니다.

## 출력 예시

```text
=== UCRA 기본 엔진 예제 ===
엔진 생성 성공
엔진 정보: Test Engine v1.0 (Test rendering engine)
엔진 해제 완료

=== 매니페스트 사용 예제 ===
매니페스트 로드 성공: voicebank/resampler.json
엔진명: Test Voice Engine
버전: 1.0.0
제작자: UCRA Team
라이선스: MIT

=== 간단한 렌더링 예제 ===
렌더링 설정: 44100Hz, 1ch, A4 노트, 1초
렌더링 성공: 44100 프레임 생성
```

이 예제들을 통해 UCRA API의 기본 사용법을 쉽게 이해할 수 있습니다.
