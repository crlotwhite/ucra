# GitHub Pages 설정 가이드

UCRA 프로젝트의 Doxygen 문서가 GitHub Actions를 통해 자동으로 배포되도록 설정되었습니다.

## GitHub Pages 활성화 방법

1. **GitHub 저장소 페이지로 이동**
   - 웹 브라우저에서 `https://github.com/crlotwhite/ucra` 을 방문합니다.

2. **Settings 탭 클릭**
   - 저장소 상단의 "Settings" 탭을 클릭합니다.

3. **Pages 설정 찾기**
   - 왼쪽 사이드바에서 "Pages"를 클릭합니다.

4. **Source 설정**
   - "Source" 섹션에서 "Deploy from a branch" 선택
   - Branch: `gh-pages` 선택
   - Folder: `/ (root)` 선택
   - "Save" 버튼 클릭

## 문서 배포 프로세스

### 자동 배포 트리거
다음 조건에서 문서가 자동으로 생성되고 배포됩니다:

- `main` 또는 `master` 브ranch에 push할 때
- 다음 파일들이 변경되었을 때:
  - `include/**` (헤더 파일)
  - `src/**` (소스 파일)
  - `docs/**` (문서 파일)
  - `README.md`
  - `Doxyfile`
  - `CMakeLists.txt`
  - `.github/workflows/docs.yml`

### 수동 배포
GitHub Actions 페이지에서 "Documentation" 워크플로우를 선택하고 "Run workflow" 버튼을 클릭하여 수동으로 문서를 배포할 수 있습니다.

### Pull Request에서의 문서 미리보기
Pull Request에서는 문서가 GitHub Pages에 배포되지 않고, 대신 아티팩트로 업로드됩니다. PR 댓글에서 "documentation-preview" 아티팩트를 다운로드하여 로컬에서 확인할 수 있습니다.

## 문서 접근 방법

문서 배포가 완료되면 다음 URL에서 접근할 수 있습니다:

```
https://crlotwhite.github.io/ucra/docs/
```

## 문서 구조

배포된 문서는 다음과 같은 구조로 구성됩니다:

- **API Reference**: C 함수들과 구조체 문서
- **Quick Start Guide**: 빠른 시작 가이드
- **Examples**: 코드 예제들
- **File Documentation**: 소스 파일별 상세 문서

## 트러블슈팅

### 배포가 실패하는 경우
1. GitHub Actions의 "Actions" 탭에서 실패한 워크플로우 확인
2. 로그를 확인하여 오류 원인 파악
3. Doxygen 설정(`Doxyfile`)이나 CMake 설정(`CMakeLists.txt`) 검토

### 문서가 업데이트되지 않는 경우
1. GitHub Pages 설정이 올바른지 확인
2. `gh-pages` 브랜치가 존재하고 최신 콘텐츠가 있는지 확인
3. 브라우저 캐시 삭제 후 재시도

### 링크 체크 실패
Pull Request에서 링크 체크가 실패하는 경우, 외부 링크가 깨졌거나 문서 내 상대 경로가 잘못되었을 수 있습니다.

## 추가 설정

### 커스텀 도메인 설정
GitHub Pages에서 커스텀 도메인을 사용하려면 Settings > Pages에서 "Custom domain" 설정을 추가하고, DNS 설정을 업데이트해야 합니다.

### HTTPS 강제
Settings > Pages에서 "Enforce HTTPS" 옵션을 활성화하는 것을 권장합니다.
