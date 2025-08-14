# UCRA Validation and Testing Tools

이 폴더에는 UCRA 라이브러리의 품질 검증 및 테스팅을 위한 도구들이 포함되어 있습니다.

## 빌드 방법

기본적으로 tools는 빌드되지 않습니다. 다음과 같이 CMake 옵션을 사용하여 활성화할 수 있습니다:

```bash
cmake -DUCRA_BUILD_TOOLS=ON ..
make
```

## 포함된 도구들

### 1. validation_suite
메인 검증 도구로, 다른 도구들을 조율하여 종합적인 품질 검증을 수행합니다.

### 2. f0_rmse_calc
F0 (기본 주파수) RMSE 계산 유틸리티입니다.

### 3. mcd_calc
MCD(13) (Mel-Cepstral Distortion) 계산 유틸리티입니다.

### 4. audio_compare
오디오 파일 비교 모듈입니다.

### 5. golden_runner
Golden 테스트 하네스로, 표준 출력과 비교하여 회귀 테스트를 수행합니다.

## 사용법

각 도구는 독립적으로 실행할 수 있으며, `--help` 옵션을 사용하여 사용법을 확인할 수 있습니다.

```bash
./validation_suite --help
./golden_runner --help
```

## 테스트

tools가 빌드된 경우, 다음 테스트들이 자동으로 포함됩니다:
- validation_suite_test
- f0_rmse_calc_test
- mcd_calc_test
- audio_compare_test
- golden_runner_test
