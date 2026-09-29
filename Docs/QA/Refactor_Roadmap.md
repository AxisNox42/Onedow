# Onedow 코드 리팩터링 로드맵

기준일: 2026-09-29

게임의 시작·진행·종료 흐름과 기존 UI 배치는 유지한다. 역할이 분명한 코드는 별도 파일로 나누고, 확인된 폐기 코드만 제거한다. 남은 `main.cpp` 분리는 전체 재작성 없이 책임이 독립적인 단위부터 이어간다. 작업 중 발견한 기존 버그는 별도 보고하고, 요청 범위에 포함된 버그만 수정한다.

우선순위 순서: `Low` < `Medium` < `High` < `ExtraHigh` < `Critical`.

| 단계 | 우선순위 | 작업 | 상태 |
|---|---|---|---|
| A | Critical | 리팩터링 전 체크포인트 커밋 | 완료 · `f2a8e52` |
| B | Critical | Windows Release 출력 경로를 `build/windows/x64/Release`로 통일 | 완료 · Release 빌드 성공 |
| C | ExtraHigh | 동일 패턴 버튼의 그리기·히트 테스트·호버 처리를 공통 UI 함수로 묶기 | 완료 |
| D | ExtraHigh | 대형 장면 구현을 메뉴·설정·일시정지·런 설정·증강·런 종료 파일로 분리 | 완료 · `SceneInternal.h`로 내부 선언 분리 |
| E | ExtraHigh | 보스 전투·승리·전투 후 상점 전환 경로 제거 | 완료 · 로비 Armory는 유지 |
| F | ExtraHigh | 보스 전용 도감·업적·튜토리얼·시련과 폐기 몹/증강 제거 | 완료 · 구형 보스 저장 키는 읽고 무시 |
| G | High | 구형 시련 선택 및 직업 선택의 잔여 구현 제거, 현행 런 설정 흐름 유지 | 완료 |
| H | High | Light Step·Warlord·Bullet Rain Eternal·Rotor HP 효과와 설명 정렬 | 완료 · Light Step 피격 10초 타이머 누락도 수정 |
| I | High | 적 진행 수치 계산을 `EnemyProgression.h`로 분리하고 기존 조정값 유지 | 완료 |
| J | High | Visual Studio/CMake 소스 목록을 맞추고 누락된 셰이더 소스 추가 | 완료 · CMake 자체 빌드는 미검증 |
| K | High | 기존 몹 등장 램프와 게임 흐름 보존 여부 점검 | 완료 · 코드 수치 유지, Release 컴파일 통과 |
| L | High | 저장 호환·제거 코드 참조를 최종 검색 | 완료 · 보스 이름은 저장 키 무시 분기만 남음 |
| M | Medium | QA 시트의 완료 항목을 초록색 `Clear`로 표시하고 현행 항목과 대조 | 완료 · 181행 COMBO 재료 판정도 Clear 반영 |
| N | Medium | `main.cpp`의 입력 샘플링과 창 상태 처리를 독립 모듈로 분리 | 완료 · 입력 콜백·포커스 전이·커서 전이는 `System/Input`; 포커스 전후 플레이어/장면의 눌림·클릭 에지와 드래그 상태도 동기화 |
| O | Medium | 플레이어 이동·피격·스킬 업데이트 책임 분리 | 진행 중 · 이동·화면 경계·대시·쿨다운·피격 보호를 `Entity/PlayerRuntime`으로 분리; 스킬 발동 효과는 메인 루프에 유지 |
| P | Medium | 적 생성 스케줄·발사체·전투 업데이트 책임 분리 | 진행 중 · 적 스폰과 Rotor 특화 판정은 `Entity/EnemySpawner`, 유도·시간 효과·화면 이탈 처리는 `Entity/ProjectileSystem`; 충돌 판정은 기존 `CollisionSystem` 사용 |
| Q | Medium | 런 초기화·재시작·종료 처리를 명시적인 런 제어 모듈로 분리 | 진행 중 · 새 런의 HP/점수/레벨/1회 획득 장부와 적·탄환 초기화를 `GameManager::ResetRunProgress`에 모음; 재시작/포기 연출 조정은 메인 오케스트레이션에 유지 |
| R | Medium | `main.cpp`에 남은 HUD와 장면 렌더링 코드를 역할별로 분리 | 진행 중 · 메뉴 장면은 여러 `Scene*.cpp`로 분리, 플레이어 셸과 전투 시야 필드 렌더도 전용 함수로 이동; 전투 본문/HUD 렌더링은 후속 |
| S | Medium | 엔티티·렌더·시스템 간 의존 방향과 공용 선언 정리 | 진행 중 · `PlayerRuntime`, `ProjectileSystem`, `EnemySpawner`는 좁은 인자/전방 선언으로 의존; `GameContext`의 전역 선언 정리는 후속 |
| T | Medium | 자원 경로·로딩·정리 책임을 확인하고 필요한 부분만 분리 | 진행 중 · 폰트 선택·초기화·문자 예열을 `Render/GameFonts`로 이동 |
| U | Medium | 헤더 포함 관계와 중복 선언을 점검 | 진행 중 · 폰트 이동 후 main의 미사용 `EmbeddedResource.h` 포함 제거; 전체 include 점검은 후속 |
| V | Low | 참조 검색으로 남은 미사용 함수·변수·항등 계수 제거 | 진행 중 · `p2mult`, 미사용 조준 마커/방사형 게이지 제거; 잔여 참조 검색 계속 |
| W | Low | 빌드 문서와 패키징 스크립트가 컴파일 산출물 경로를 정확히 설명하는지 점검 | 후속 단계 · LTS 패키징은 별도 배포 단계 |
| X | Low | 기존 `bin/Onedow/WindowsOS` 파일을 안전하게 정리할 수 있는지 확인 | 보류 · 폴더 안의 사용자 저장 데이터 보존 필요 |
| Y | Low | 생성된 검사 산출물과 작업 트리 변경 범위를 정리 | 완료 · 검사 임시 파일 없음, 변경 파일과 빌드 산출물 위치 확인 |
| Z | Low | 완료 범위·미해결 QA·다음 리팩터링 단계를 기록 | 완료 · 현재 범위, 미검증 항목과 다음 단계를 기록 |

## 현재 QA에서 계속 확인할 항목

System QA 174행 포커스 처리에는 RUNNING 자동 일시정지와 포커스 이탈·복귀 시 키/마우스 에지 상태 초기화를 반영하고 Release 빌드까지 확인했다. 별도로 남아 있던 대시/Q·E·R, 증강 선택, 설정·도감·런 설정 화면의 키/드래그 에지도 초기화·동기화했다. 실제 Alt+Tab 후 입력이 붙지 않는지 수동 플레이 확인이 남아 있어 아직 `Clear`로 처리하지 않았다. 179행의 2048×1280 로비 메뉴는 현재 코드에서 5개 버튼 레일 높이 410px, 시작 위치 774px, 끝 위치 1184px로 화면 안에 배치된다. 소스만으로는 보고된 클리핑 원인을 재현하지 못했으며, 실행 화면 확인 전까지 미해결로 유지한다. 플레이어 외형 누락은 전투 렌더 경로에 무기 셸 호출을 복구했고, COMBO 레시피 재료는 런 중 실제 획득한 증강만 판정하도록 수정해 181행을 초록색 Clear로 갱신했다. 다른 남은 QA 항목으로 사망 연출 중 입력, Codex 검색 결과 없음 안내, 설정/오디오 옵션 정책이 있다.

## 빌드 경로와 기존 `bin` 폴더

Visual Studio Release와 Windows CMake 대상의 실행 파일 경로는 `build/windows/x64/Release`다. `scripts/package_lts.ps1`은 배포용 파일을 `bin/Onedow/WindowsOS`에 복사하는 별도 패키징 단계다. 그 폴더에는 사용자 저장 데이터가 있으므로 이번 작업에서 삭제하지 않았다.

## 빌드 검증

Visual Studio 2022 MSBuild 17.14.40, Windows SDK 10.0.26100.0으로 x64 Release를 빌드했다. 산출물은 `build/windows/x64/Release/WiNILL.exe`다. 포커스 입력·플레이어 런타임·적 스폰·발사체 모듈 변경이 포함된 빌드가 성공했다. CMake 소스 목록을 수정했지만 CMake 구성 자체는 실행하지 않았다.
