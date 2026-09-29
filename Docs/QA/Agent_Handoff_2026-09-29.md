# Onedow 인수인계 — 2026-09-29

## 다음 작업

설정 화면의 텍스트 정렬을 실제 실행 화면에서 확인하고, 통과한 내용만 QA 문서에 반영합니다.

- 설정 우측의 각 행에서 컨트롤 텍스트를 해당 컨트롤 영역의 세로 중앙에 둡니다. 화면 전체의 절대 Y를 통일하는 뜻은 아닙니다.
- 기존 가로 배치를 유지합니다. 좌측 카테고리와 하위 목록은 왼쪽 정렬입니다.
- 구현 위치: `WiNILL/Core/Scenes/SceneSettings.cpp`의 `controlY` 계산부.
- 실제 화면·호버·클릭은 아직 확인하지 않았습니다. 확인 전에는 QA 항목을 `Clear`로 표시하지 않습니다.

## 현재 상태

- Release 빌드 성공: `build/windows/x64/Release/WiNILL.exe`.
- 빌드 시 기존 `APIENTRY` 매크로 재정의 경고가 있습니다.
- 브랜치 `main`, 기준 커밋 `f2a8e52`. 작업 트리에 다수의 기존 미커밋 변경이 있으므로 삭제·초기화하지 말고 먼저 `git status --short`를 확인합니다.
- 빌드 명령: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build_windows.ps1`.
- 설정 UI 확인 후 [Onedow.xlsx](Onedow.xlsx)와 [UI 텍스트 계획](UI_Text_Audit_Plan.md)을 갱신합니다. 이후 아키텍처 작업은 [리팩터링 로드맵](Refactor_Roadmap.md)의 O–S부터 이어갑니다.
