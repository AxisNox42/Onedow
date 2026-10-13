# 전체 파일 구조 조사표

기준: 2026-10-12 로컬 작업 트리. 게임 98개+테스트 7개 전체의 구조 자료를 수집했다. 각 파일의 책임은 구조 분류이며 모든 실행 분기를 테스트했다는 의미가 아니다. 검토 ID는 본문 중복 목록과 연결되며 의존 파일까지 포함한다. ID가 없는 파일도 구조 조사 범위에 포함된다.

함수 목록은 어휘 분석으로 검출한 후보 전체이다. 람다·일부 생성자/연산자·매크로는 빠질 수 있다. 호출 수는 이름 기반 구문 위치, g_* 목록은 읽기/쓰기 구분 없는 참조다. include의 조건부 분기는 합산한다. 중복·의존 해석과 이관 순서는 [조사 본문](<D:/01_Study/02_OpenGL/Onedow/Docs/QA/CodeStructure_Audit_2026-10-12/README.md>)을 참고한다.

## 1. 전체 소스 책임/영향 표

| 파일 | 줄 | 함수 후보 | 직접 의존/피의존 | 책임 | 검토 ID |
|---|---:|---:|---:|---|---|
| [tests/blur_render_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/blur_render_test.cpp>) | 108 | 4 | 2/0 | GL 환경의 블러 캡처/framebuffer 상태 회귀 테스트 | D39 |
| [tests/genesis_launcher_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp>) | 397 | 16 | 1/0 | Genesis 투척·앵커/탄환 패턴 테스트 | D39 |
| [tests/magnetar_field_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp>) | 132 | 9 | 1/0 | Magnetar 장/탄환 상호작용 테스트 | D39 |
| [tests/mob_death_fx_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp>) | 397 | 8 | 3/0 | 몹 종류별 사망 효과/상태 테스트 | D39 |
| [tests/quasar_beam_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_beam_test.cpp>) | 155 | 7 | 1/0 | Quasar 빔 동작 테스트 | D39 |
| [tests/quasar_nova_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_nova_test.cpp>) | 109 | 6 | 1/0 | Quasar nova 패턴 테스트 | D39 |
| [tests/regulus_beam_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/regulus_beam_test.cpp>) | 102 | 6 | 1/0 | Regulus 빔 패턴 테스트 | D39 |
| [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>) | 187 | 7 | 2/8 | 업적 정의·해금 상태·진행 갱신 | D05 |
| [WiNILL/Core/AugmentSlots.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h>) | 39 | 6 | 2/7 | 증강 슬롯/보유 목록·교체 상태 | D07 |
| [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>) | 537 | 57 | 3/12 | 도감 대상/진행/검색/필터 및 해금 상태 | D01, D09, D12 |
| [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>) | 47 | 0 | 6/5 | 게임 전역 상태를 다른 영역에 노출하는 extern 접점 | 구조/인터페이스 조사 |
| [WiNILL/Core/GameManager.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp>) | 583 | 22 | 6/0 | 게임 상태 갱신·점수/XP·증강 추첨·내부 셰이더 초기화 | D01, D05, D07, D36 |
| [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>) | 102 | 2 | 3/15 | GameState와 GameManager 데이터/인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Core/GameVersion.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameVersion.h>) | 4 | 0 | 0/1 | 제품 버전 정의 | D40 |
| [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>) | 87 | 6 | 2/10 | 런 밖 메타 진행·구매 효과·플레이어 스탯 적용 | D02 |
| [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>) | 1883 | 33 | 34/0 | 증강 선택·미리보기·카드·소유 패널 UI와 표시 계산 | D02, D20, D23, D29, D32, D33 |
| [WiNILL/Core/Scenes/SceneBoot.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneBoot.cpp>) | 20 | 1 | 2/0 | 부팅/초기 화면 연출 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/SceneCodex.inl](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneCodex.inl>) | 820 | 1 | 0/1 | 독립 CODEX 화면의 도감 UI 구현 | D09, D19, D20, D22, D23 |
| [WiNILL/Core/Scenes/SceneCodexInline.inl](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneCodexInline.inl>) | 1255 | 1 | 0/1 | 메인 메뉴 안 도감 패널 UI 구현 | D09, D19, D20, D21, D22, D23, D25 |
| [WiNILL/Core/Scenes/SceneContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneContext.h>) | 44 | 1 | 1/5 | 화면에 전달하는 게임/렌더/상태 컨텍스트 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/SceneInternal.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneInternal.h>) | 58 | 0 | 1/8 | 화면 내부 공용 선언 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/SceneMainMenu.inl](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMainMenu.inl>) | 339 | 1 | 0/1 | 메인 메뉴 내용·선택·패널 진입 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>) | 1214 | 39 | 39/0 | 메뉴 공통 상태·연출·back 처리 및 여러 inl의 번역 단위 | D19, D22, D32, D33 |
| [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>) | 256 | 2 | 34/0 | 일시정지 화면 표시/입력 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/ScenePause.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.h>) | 17 | 0 | 1/2 | 일시정지용 좁은 컨텍스트/인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>) | 1595 | 5 | 31/0 | 직업/무기/시련/Creative 런 구성·능력표·로드아웃 확정 | D02, D08, D10, D19, D20, D21, D23, D25 |
| [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>) | 667 | 2 | 34/0 | 사망/게임오버 결과 UI | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/SceneRunEnd.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.h>) | 22 | 0 | 1/1 | 런 종료 화면용 컨텍스트/인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>) | 21 | 0 | 4/9 | 화면 진입점 묶음 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>) | 1623 | 4 | 34/0 | 설정 레이아웃·편집·스냅샷·취소/적용·변경 감지 | D19, D20, D21, D25, D26, D27 |
| [WiNILL/Core/Scenes/SceneShop.inl](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneShop.inl>) | 1303 | 2 | 0/1 | 상점 패널·선택/스크롤·구매 UI | D19, D20, D21, D25 |
| [WiNILL/Core/Scenes/SceneSkills.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.cpp>) | 37 | 4 | 1/0 | 스킬 슬롯 상태·장착/재장착·HUD | D06, D30 |
| [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>) | 17 | 0 | 1/9 | 스킬 타입/슬롯과 장착·표시 함수 선언 | D06 |
| [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>) | 268 | 2 | 33/0 | 튜토리얼/Ready/시작 화면 및 텍스트 처리 | D19, D24 |
| [WiNILL/Core/Scenes/SceneStart.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.h>) | 16 | 0 | 1/1 | 시작 화면용 컨텍스트/인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/SceneTextContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneTextContext.h>) | 9 | 0 | 0/3 | 텍스트 렌더러 중심의 좁은 화면 컨텍스트 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/SceneUI.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp>) | 411 | 18 | 4/0 | 화면 공통 UI 도형/텍스트/전환 기능 | D20 |
| [WiNILL/Core/Scenes/SceneUI.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.h>) | 62 | 0 | 0/9 | 공통 UI 인터페이스와 전환 관련 선언 | D20, D25, D33 |
| [WiNILL/Core/Scenes/WindowChrome.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.cpp>) | 32 | 1 | 5/0 | 앱 창 테두리/제목/버튼 그리기와 동작 | 구조/인터페이스 조사 |
| [WiNILL/Core/Scenes/WindowChrome.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.h>) | 9 | 0 | 0/9 | 창 UI 인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Core/TutorialText.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h>) | 200 | 8 | 1/7 | 언어별 튜토리얼 텍스트/선택 | D19 |
| [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>) | 463 | 20 | 3/15 | 증강 타입/희귀도/분류/자격/설명 선택의 공용 모델 | D07, D11, D28 |
| [WiNILL/Entity/AugmentCatalog.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/AugmentCatalog.cpp>) | 734 | 0 | 1/0 | 증강 카탈로그 슬롯/이름/설명/예약 데이터 | D11 |
| [WiNILL/Entity/AugmentDescKR.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/AugmentDescKR.h>) | 169 | 1 | 0/1 | 한국어 증강 상세 설명 오버라이드 | D11 |
| [WiNILL/Entity/Bullet.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h>) | 99 | 3 | 0/8 | 탄환 상태·이동/유도·수명·피해/반사 속성 | 구조/인터페이스 조사 |
| [WiNILL/Entity/CollisionSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h>) | 286 | 7 | 5/1 | 탄환-몹/플레이어 충돌·피해·처치·관통/도탄 | D01, D12, D13, D14, D17 |
| [WiNILL/Entity/EnemyProgression.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyProgression.h>) | 50 | 2 | 0/2 | 진행도에 따른 적 스펙 배율/상한 | D09 |
| [WiNILL/Entity/EnemyShield.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyShield.h>) | 119 | 5 | 1/2 | 적 실드 상태/피해 처리와 관련 효과 | 구조/인터페이스 조사 |
| [WiNILL/Entity/EnemySpawner.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.cpp>) | 178 | 3 | 6/0 | 일반/원거리 스폰 시간·종류 선택·진행도/개체 제한 | D09, D18 |
| [WiNILL/Entity/EnemySpawner.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.h>) | 10 | 0 | 0/2 | 스폰 초기화/갱신 인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Entity/ExpSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h>) | 34 | 1 | 0/9 | XP 획득/레벨 관련 계산·표현 접점 | 구조/인터페이스 조사 |
| [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>) | 1797 | 54 | 3/15 | 일반/특수 몹 데이터·스펙·AI 상태·사망 형상/연출 | D09, D15, D16, D31 |
| [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>) | 445 | 20 | 2/12 | 일반/원거리 몹 소유·갱신·검색·특수 패턴 조정 | D04, D12, D15, D17, D18 |
| [WiNILL/Entity/PlayerRuntime.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp>) | 157 | 12 | 3/0 | 플레이어 이동·대시·스킬 타이머·입력 edge·실행 상태 | D04, D06 |
| [WiNILL/Entity/PlayerRuntime.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.h>) | 62 | 0 | 0/2 | 플레이어 런타임 상태와 이동/타이머 인터페이스 | D06 |
| [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>) | 533 | 18 | 1/19 | 기본 스탯·증강 적용·누적 보너스·흡혈/스탯 계산 | D01, D02, D03, D11, D29 |
| [WiNILL/Entity/ProjectileSystem.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ProjectileSystem.cpp>) | 99 | 2 | 4/0 | 플레이어 투사체 발사·갱신·지우기 등 처리 | 구조/인터페이스 조사 |
| [WiNILL/Entity/ProjectileSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ProjectileSystem.h>) | 15 | 0 | 0/2 | 투사체 시스템 호출 인터페이스/입력 | 구조/인터페이스 조사 |
| [WiNILL/Entity/RangedMob.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h>) | 179 | 5 | 4/3 | 원거리 몹 상태·차징/발사·이동·피격/사망 상태 | D09, D16 |
| [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>) | 60 | 5 | 1/11 | 시작 무기 타입/기본 적용 규칙 | D02, D10 |
| [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>) | 4084 | 26 | 46/0 | 앱 수명·게임 루프·전환·런 시작/종료·전투·보상·HUD의 조정 중심 | D01, D02, D03, D04, D05, D10, D13, D15, D17, D27, D30 |
| [WiNILL/Render/BlurShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp>) | 447 | 8 | 3/0 | 블러 셰이더·캡처 framebuffer·렌더 상태 저장/복원 | D36, D38 |
| [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>) | 28 | 0 | 0/10 | 블러 초기화/캡처/그리기 인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Render/Camera.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.cpp>) | 8 | 0 | 1/0 | 카메라 좌표/흔들림 갱신·변환 | 구조/인터페이스 조사 |
| [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>) | 43 | 8 | 1/14 | 카메라 상태/인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>) | 257 | 16 | 1/22 | 공통 도형/배치/렌더 pass 기본 연산 | D32, D38 |
| [WiNILL/Render/EnemyParticles.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h>) | 192 | 10 | 1/2 | 적 파티클/폭발 등 상태와 생성·갱신·표시 | 구조/인터페이스 조사 |
| [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>) | 2282 | 46 | 10/0 | 몹·탄환·필드·특수 패턴 렌더와 가시성 계산 | D31, D32 |
| [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>) | 42 | 0 | 2/9 | 엔티티 렌더 진입점 | 구조/인터페이스 조사 |
| [WiNILL/Render/GameFonts.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.cpp>) | 129 | 2 | 5/0 | 플랫폼/내장 폰트 로드·텍스트 렌더러 설정 | D37 |
| [WiNILL/Render/GameFonts.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.h>) | 8 | 0 | 0/2 | 게임 폰트 로드/설정 인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>) | 663 | 22 | 2/10 | 아이콘 로딩/마스크·텍스처·셰이더·일반/회전/darken 그리기 | D11, D34, D35, D36, D37, D38 |
| [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>) | 232 | 14 | 5/8 | 피해 숫자·잔상·히트스톱·콤보·파티클; 전투 누적 부수효과 포함 | D01 |
| [WiNILL/Render/MainShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp>) | 348 | 11 | 3/0 | 메인 셰이더·gradient 텍스처·기하 제출 | D36, D37, D38 |
| [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>) | 28 | 0 | 0/12 | 메인 셰이더·색/기하 렌더 인터페이스 | D36, D38 |
| [WiNILL/Render/NebulaGlowShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.cpp>) | 133 | 2 | 3/0 | 성운 glow 셰이더/quad 초기화·그리기 | D36, D38 |
| [WiNILL/Render/NebulaGlowShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.h>) | 17 | 0 | 0/9 | 성운 glow 인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Render/PlanetShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.cpp>) | 132 | 2 | 3/0 | 행성 셰이더/quad 초기화·그리기 | D36, D38 |
| [WiNILL/Render/PlanetShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.h>) | 18 | 0 | 0/9 | 행성 셰이더 인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Render/PlayerWeaponShell.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp>) | 324 | 9 | 6/0 | 플레이어 무기 외형·선분 기하·표시 상태 참조 | D32 |
| [WiNILL/Render/PlayerWeaponShell.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.h>) | 5 | 0 | 0/3 | 플레이어 무기 표시 인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/Render/stb_impl.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/stb_impl.cpp>) | 8 | 0 | 0/0 | 외부 stb 라이브러리 구현 매크로의 단일 번역 단위 | 구조/인터페이스 조사 |
| [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>) | 566 | 23 | 1/14 | 폰트/glyph atlas·측정·줄바꿈 지원·일반/회전 텍스트·GL 수명 | D35, D36, D38 |
| [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>) | 125 | 7 | 1/9 | UI 색/알파 정책 | D33 |
| [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>) | 46 | 9 | 1/10 | UI 레이아웃/보간 계산 도구 | D20, D33 |
| [WiNILL/resource.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/resource.h>) | 3 | 0 | 0/0 | Windows 내장 리소스 ID | D40 |
| [WiNILL/System/Audio.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp>) | 312 | 18 | 2/0 | 오디오 엔진/장치·BGM/SFX·설정 적용 상태 | D27, D37 |
| [WiNILL/System/Audio.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.h>) | 29 | 0 | 0/9 | 오디오 재생/설정/수명 인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/System/CrashHandler.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/CrashHandler.h>) | 105 | 7 | 0/1 | 예외/크래시 처리와 진단 경계 | 구조/인터페이스 조사 |
| [WiNILL/System/DebugToolkit.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp>) | 1183 | 38 | 5/0 | 개발 패널·필드 편집·화면 캡처·테스트용 상태 조정 | D22, D29, D33 |
| [WiNILL/System/DebugToolkit.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h>) | 124 | 6 | 4/3 | 디버그 도구 상태/호출 인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/System/EmbeddedResource.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/EmbeddedResource.h>) | 42 | 2 | 0/3 | Windows 내장 리소스의 공통 읽기 helper | D37 |
| [WiNILL/System/FakeWindow.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/FakeWindow.cpp>) | 1 | 0 | 1/0 | 플레이어 가상 창 이동/충돌·위치/크기 조정 | 구조/인터페이스 조사 |
| [WiNILL/System/FakeWindow.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/FakeWindow.h>) | 10 | 0 | 0/4 | 가상 창 bounds/인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/System/GameplayTelemetry.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h>) | 389 | 18 | 3/1 | 런/증강 스탯 snapshot·차이·이벤트 기록 | D29 |
| [WiNILL/System/Input.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp>) | 318 | 20 | 3/0 | 키/마우스/텍스트 callback·Windows IME·화면별 입력 편집 | 구조/인터페이스 조사 |
| [WiNILL/System/Input.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.h>) | 24 | 0 | 0/4 | 입력 상태/callback·IME 관련 인터페이스 | 구조/인터페이스 조사 |
| [WiNILL/System/Platform.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h>) | 137 | 9 | 0/4 | 플랫폼별 창/타이머/OS 기능 분기 | 구조/인터페이스 조사 |
| [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>) | 288 | 6 | 4/8 | 설정·진행 저장/로드·초기화·런 결과 반영 | D05, D26, D28 |
| [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>) | 345 | 23 | 0/24 | 옵션 기본값·언어·시련 계산·여러 공용 게임 설정 상태 | D02, D08, D19, D26, D27, D28 |
| [WiNILL/System/SmokeCapture.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h>) | 191 | 8 | 2/1 | 자동 UI 스모크 캡처 설정/경로 | 구조/인터페이스 조사 |
| [WiNILL/System/SystemInfo.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SystemInfo.h>) | 64 | 2 | 0/7 | 시스템/환경 정보 수집 | 구조/인터페이스 조사 |
| [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>) | 236 | 1 | 1/9 | 언어별 공용 번역 데이터/선택 | D19 |
| [WiNILL/System/WindowFx.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp>) | 387 | 19 | 2/0 | 창/화면 효과 갱신과 적용 | D19 |
| [WiNILL/System/WindowFx.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.h>) | 17 | 0 | 0/2 | 창 효과 상태/인터페이스 | 구조/인터페이스 조사 |

## 2. 파일별 직접 의존·함수·전역 참조

### tests/blur_render_test.cpp

GL 환경의 블러 캡처/framebuffer 상태 회귀 테스트.

자체 include: [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `cstdio`, `cstdlib`, `glad/glad.h`.

함수 후보: [CompileGlShader:9](<D:/01_Study/02_OpenGL/Onedow/tests/blur_render_test.cpp:9>), [Check:18](<D:/01_Study/02_OpenGL/Onedow/tests/blur_render_test.cpp:18>), [TestCapture:21](<D:/01_Study/02_OpenGL/Onedow/tests/blur_render_test.cpp:21>), [main:79](<D:/01_Study/02_OpenGL/Onedow/tests/blur_render_test.cpp:79>).

참조 `g_*`: `g_BaseOrtho`, `g_MainVAO`, `g_VBO`.

이름 기반 호출 위치 19개. 간접 포함 의존 3파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### tests/genesis_launcher_test.cpp

Genesis 투척·앵커/탄환 패턴 테스트.

자체 include: [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>).

직접 피의존: 없음.

외부/표준 include: `cassert`, `cmath`, `cstdio`.

함수 후보: [CountThrown:11](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:11>), [Step:18](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:18>), [CheckLauncherCycle:25](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:25>), [CheckFlightHit:59](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:59>), [CheckLandingStun:74](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:74>), [CheckFlightPassesThrough:107](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:107>), [CheckThrownCap:129](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:129>), [CheckThrowRange:163](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:163>), [CheckShellStacks:185](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:185>), [CheckRandomThrows:221](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:221>), [CheckSpeedSpin:256](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:256>), [CheckLandingBlast:280](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:280>), [CheckHpScaling:300](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:300>), [CheckVolleyDamage:324](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:324>), [CheckThrowSpeedProfile:355](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:355>), [main:380](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp:380>).

참조 `g_*`: 없음.

이름 기반 호출 위치 82개. 간접 포함 의존 11파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### tests/magnetar_field_test.cpp

Magnetar 장/탄환 상호작용 테스트.

자체 include: [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>).

직접 피의존: 없음.

외부/표준 include: `cassert`, `cmath`, `cstdio`.

함수 후보: [ShotThrough:10](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp:10>), [StillMagnetar:20](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp:20>), [IntactFaces:27](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp:27>), [CheckFacing:34](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp:34>), [CheckFaceMapping:45](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp:45>), [CheckPerFaceBreak:60](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp:60>), [CheckFaceRestore:89](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp:89>), [CheckHitFlash:108](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp:108>), [main:124](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp:124>).

참조 `g_*`: 없음.

이름 기반 호출 위치 31개. 간접 포함 의존 9파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### tests/mob_death_fx_test.cpp

몹 종류별 사망 효과/상태 테스트.

자체 include: [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/RangedMob.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>).

직접 피의존: 없음.

외부/표준 include: `cassert`, `cstdio`.

함수 후보: [CheckGimbalAndWorldDiscCoordinates:15](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp:15>), [CheckBulletHaloLayers:77](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp:77>), [ActiveCount:99](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp:99>), [CheckBounds:105](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp:105>), [CheckRoster:121](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp:121>), [CheckAntaresBullfight:191](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp:191>), [CheckPriorityAndDelay:329](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp:329>), [main:374](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp:374>).

참조 `g_*`: `g_EnemyParts`, `g_ViewZoom`, `g_ViewZoomTarget`, `g_ZoomCX`, `g_ZoomCY`.

이름 기반 호출 위치 112개. 간접 포함 의존 12파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### tests/quasar_beam_test.cpp

Quasar 빔 동작 테스트.

자체 include: [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>).

직접 피의존: 없음.

외부/표준 include: `cassert`, `cmath`, `cstdio`.

함수 후보: [MakeQuasar:13](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_beam_test.cpp:13>), [AimAngle:25](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_beam_test.cpp:25>), [CheckAimChosenOnce:29](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_beam_test.cpp:29>), [CheckForcedLock:56](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_beam_test.cpp:56>), [CheckTurnTowardPlayerWhileFiring:77](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_beam_test.cpp:77>), [CheckBand:127](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_beam_test.cpp:127>), [main:148](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_beam_test.cpp:148>).

참조 `g_*`: 없음.

이름 기반 호출 위치 41개. 간접 포함 의존 9파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### tests/quasar_nova_test.cpp

Quasar nova 패턴 테스트.

자체 include: [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>).

직접 피의존: 없음.

외부/표준 include: `cassert`, `cmath`, `cstdio`.

함수 후보: [KillQuasar:13](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_nova_test.cpp:13>), [CheckHeldUntilBlast:26](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_nova_test.cpp:26>), [CheckNoAnimationReleasesNow:50](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_nova_test.cpp:50>), [CheckPendingBlocksRemoval:59](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_nova_test.cpp:59>), [CheckGenesisTiming:75](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_nova_test.cpp:75>), [main:102](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_nova_test.cpp:102>).

참조 `g_*`: 없음.

이름 기반 호출 위치 21개. 간접 포함 의존 11파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### tests/regulus_beam_test.cpp

Regulus 빔 패턴 테스트.

자체 include: [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>).

직접 피의존: 없음.

외부/표준 include: `cassert`, `cmath`, `cstdio`, `set`.

함수 후보: [Add:11](<D:/01_Study/02_OpenGL/Onedow/tests/regulus_beam_test.cpp:11>), [Step:19](<D:/01_Study/02_OpenGL/Onedow/tests/regulus_beam_test.cpp:19>), [CheckInstantGrant:28](<D:/01_Study/02_OpenGL/Onedow/tests/regulus_beam_test.cpp:28>), [CheckRateAndOnce:53](<D:/01_Study/02_OpenGL/Onedow/tests/regulus_beam_test.cpp:53>), [CheckRandomTarget:77](<D:/01_Study/02_OpenGL/Onedow/tests/regulus_beam_test.cpp:77>), [main:96](<D:/01_Study/02_OpenGL/Onedow/tests/regulus_beam_test.cpp:96>).

참조 `g_*`: 없음.

이름 기반 호출 위치 25개. 간접 포함 의존 11파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Achievements.h

업적 정의·해금 상태·진행 갱신.

자체 include: [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>).

외부/표준 include: 없음.

함수 후보: [IsPlayableJob:25](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h:25>), [AchName:152](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h:152>), [AchDesc:156](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h:156>), [JobName:160](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h:160>), [JobDesc:164](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h:164>), [JobUnlocked:170](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h:170>), [TryUnlockAch:180](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h:180>).

참조 `g_*`: `g_AchSaveNeeded`, `g_AchToastId`, `g_AchToastTimer`, `g_AchUnlocked`, `g_Coins`, `g_JobBought`, `g_Language`, `g_SelectedJob`.

이름 기반 호출 위치 1개. 간접 포함 의존 7파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/AugmentSlots.h

증강 슬롯/보유 목록·교체 상태.

자체 include: [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `vector`.

함수 후보: [AugUsesIdentitySlot:11](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h:11>), [AugIdxUsesIdentitySlot:16](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h:16>), [IdentitySlotMax:21](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h:21>), [CountIdentitySlotsUsed:26](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h:26>), [IdentitySlotsFull:33](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h:33>), [NeedsReplaceForAug:37](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h:37>).

참조 `g_*`: `g_MetaLv`, `g_OwnedAugs`.

이름 기반 호출 위치 6개. 간접 포함 의존 6파일, 간접 피의존 7파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Codex.h

도감 대상/진행/검색/필터 및 해금 상태.

자체 include: [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Entity/CollisionSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h>), [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>), [WiNILL/System/Input.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `algorithm`, `cwctype`, `string`.

함수 후보: [CodexFullReveal:16](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:16>), [CodexSearchHasSelection:30](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:30>), [CodexSearchSelectionStart:33](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:33>), [CodexSearchSelectionEnd:38](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:38>), [CodexSearchSetComposition:43](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:43>), [CodexSearchClearComposition:67](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:67>), [CodexSearchQuery:72](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:72>), [CodexSearchQueryCaret:87](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:87>), [CodexSearchCompositionStart:93](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:93>), [CodexSearchPreviousBoundary:97](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:97>), [CodexSearchNextBoundary:104](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:104>), [CodexSearchEraseRange:112](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:112>), [CodexSearchDeleteSelection:122](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:122>), [CodexSearchClear:126](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:126>), [CodexSearchMoveCaret:133](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:133>), [CodexSearchMoveLeft:143](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:143>), [CodexSearchMoveRight:148](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:148>), [CodexSearchMoveHome:153](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:153>), [CodexSearchMoveEnd:154](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:154>), [CodexSearchSelectAll:155](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:155>), [CodexSearchSelectedText:160](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:160>), [CodexSearchInsertText:165](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:165>), [CodexSearchInsertCodepoint:186](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:186>), [CodexSearchBackspace:197](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:197>), [CodexSearchDeleteForward:202](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:202>), [CodexMatch:208](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:208>), [CodexMatchAny:220](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:220>), [CodexSearchMatchesAny:228](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:228>), [MarkAugSeen:236](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:236>), [CodexAugSeen:241](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:241>), [CodexMobProfileFor:349](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:349>), [CodexMobProfileText:354](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:354>), [CodexMobTier:360](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:360>), [CodexMobBaseHp:365](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:365>), [CodexMobDamageValue:370](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:370>), [CodexMobBaseSpeed:375](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:375>), [CodexMobAttackPattern:380](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:380>), [CodexMobFormDescription:385](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:385>), [CodexMobIdForKind:397](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:397>), [ForEachCodexMobEntry:412](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:412>), [MarkMobSeen:421](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:421>), [MarkMobSeenId:425](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:425>), [RegisterCodexMobKill:430](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:430>), [RegisterCodexMobKill:435](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:435>), [CodexMobSeen:439](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:439>), [CodexMobKillCount:446](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:446>), [CodexMobKillThreshold:452](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:452>), [CodexMobDataStage:464](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:464>), [CodexLocalizedText:475](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:475>), [MobLocalizedNames:483](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:483>), [MobName:487](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:487>), [CodexMobNameUnlocked:492](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:492>), [CodexMobListLabel:495](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:495>), [MobDesc:501](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:501>), [MobThreatLabel:509](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:509>), [MobTierLabel:514](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:514>), [CodexMobKind:524](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h:524>).

참조 `g_*`: `g_AugSeen`, `g_CodexDirty`, `g_CodexSearch`, `g_CodexSearchAnchor`, `g_CodexSearchCaret`, `g_CodexSearchComposition`, `g_CodexSearchCompositionCaret`, `g_CodexSearchCompositionLen`, `g_CodexSearchInputEnabled`, `g_CodexSearchLen`, `g_CreativeMode`, `g_DevUnlocked`, `g_Language`, `g_MobKillCounts`, `g_MobSeen`, `g_RunMobKillCounts`, `g_SuppressMobSeen`.

이름 기반 호출 위치 78개. 간접 포함 의존 9파일, 간접 피의존 12파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/GameContext.h

게임 전역 상태를 다른 영역에 노출하는 extern 접점.

자체 include: [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Entity/Bullet.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h>), [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>).

직접 피의존: [WiNILL/Core/GameManager.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>).

외부/표준 include: `vector`.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: `g_AugExitSlot`, `g_AugExitT`, `g_Bullets`, `g_ConversionWeapon`, `g_CurrentWeapon`, `g_DeathReason`, `g_DevToastTimer`, `g_DevUnlocked`, `g_GameManager`, `g_GameOverFade`, `g_HoveredAug`, `g_LastRunRecord`, `g_LmbPrev`, `g_MonsterManager`, `g_OwnedAugs`, `g_PrevHP`, `g_RepExitSlot`, `g_RepExitT`, `g_RmbPrev`, `g_ScrollAccum`, `g_SettingsReturnTo`, `g_Stats`, `g_TextL`, `g_TextS`, `g_TextXL`, `g_VolBuf`, `g_VolEdit`, `g_VolLen`, `g_WindowSizeCur`.

이름 기반 호출 위치 0개. 간접 포함 의존 15파일, 간접 피의존 5파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/GameManager.cpp

게임 상태 갱신·점수/XP·증강 추첨·내부 셰이더 초기화.

자체 include: [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: 없음.

외부/표준 include: `algorithm`.

함수 후보: [DebuffCandidateAllowed:12](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:12>), [GameManager:32](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:32>), [Init:41](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:41>), [initShaders:47](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:47>), [initRenderData:67](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:67>), [setOrtho:91](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:91>), [drawQuad:103](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:103>), [HandleInput:113](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:113>), [UpdateStateSystem:153](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:153>), [ResetRunProgress:160](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:160>), [RarityWeight:179](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:179>), [RollOneAug:194](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:194>), [RollOneDebuff:302](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:302>), [PickAugChoices:333](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:333>), [PickRandomDebuffIndices:398](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:398>), [PickDebuffChoices:441](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:441>), [PickRandomAugIndices:457](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:457>), [QueueAugmentReward:489](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:489>), [ActivateNextAugmentReward:494](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:494>), [ClearAugmentRewardQueue:524](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:524>), [AddScore:535](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:535>), [Render:541](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp:541>).

참조 `g_*`: `g_AugExitSlot`, `g_AugExitT`, `g_CurrentWeapon`, `g_HoveredAug`, `g_RepExitSlot`, `g_RepExitT`, `g_SettingsReturnTo`, `g_Stats`, `g_TypeOwned`.

이름 기반 호출 위치 25개. 간접 포함 의존 23파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/GameManager.h

GameState와 GameManager 데이터/인터페이스.

자체 include: [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Entity/Bullet.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h>), [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>).

직접 피의존: [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/GameManager.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneBoot.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneBoot.cpp>), [WiNILL/Core/Scenes/SceneContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneContext.h>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Core/Scenes/WindowChrome.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.cpp>), [WiNILL/System/DebugToolkit.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h>), [WiNILL/System/Input.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `GLFW/glfw3.h`, `cstring`, `deque`, `glad/glad.h`, `vector`.

함수 후보: [ShouldUpdate:84](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h:84>), [GetState:88](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h:88>).

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 11파일, 간접 피의존 19파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/GameVersion.h

제품 버전 정의.

자체 include: 없음.

직접 피의존: [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 1파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Meta.h

런 밖 메타 진행·구매 효과·플레이어 스탯 적용.

자체 include: [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>), [WiNILL/Core/AugmentSlots.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>).

외부/표준 include: 없음.

함수 후보: [MetaNextCost:31](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h:31>), [MetaName:39](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h:39>), [AccentName:62](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h:62>), [ThemeOwned:67](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h:67>), [ApplyAccentTheme:73](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h:73>), [ApplyMeta:82](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h:82>).

참조 `g_*`: `g_AccentB`, `g_AccentG`, `g_AccentR`, `g_Coins`, `g_Language`, `g_MetaLv`, `g_ThemeOwned`, `g_ThemeSel`.

이름 기반 호출 위치 1개. 간접 포함 의존 5파일, 간접 피의존 11파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneAugment.cpp

증강 선택·미리보기·카드·소유 패널 UI와 표시 계산.

자체 include: [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>), [WiNILL/Core/AugmentSlots.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h>), [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Core/Scenes/SceneInternal.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneInternal.h>), [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>), [WiNILL/Core/Scenes/SceneUI.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.h>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>), [WiNILL/Core/Scenes/WindowChrome.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.h>), [WiNILL/Core/TutorialText.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h>), [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Entity/ExpSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/Render/NebulaGlowShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.h>), [WiNILL/Render/PlanetShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>), [WiNILL/System/Audio.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.h>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>), [WiNILL/System/SystemInfo.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SystemInfo.h>), [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `algorithm`, `cmath`, `functional`, `glad/glad.h`, `string`, `vector`.

함수 후보: [TarotClamp01:53](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:53>), [TarotEaseOut:57](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:57>), [TarotSegment:62](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:62>), [TarotHit:75](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:75>), [DrawAugmentOrbitRing:85](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:85>), [DrawAugmentShieldOrbit:122](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:122>), [DrawTarotEmblem:165](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:165>), [DrawTarotCardSurface:203](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:203>), [TarotWrap:278](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:278>), [DrawTarotDetailPanel:339](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:339>), [MakeAugPreviewBaseStats:427](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:427>), [MakeAugPreviewTrialStats:439](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:439>), [MakeAugPreviewOwnedStats:447](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:447>), [MakeAugPreviewCandidateStats:458](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:458>), [AugPreviewMetricValue:467](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:467>), [AugSelectionClamp:511](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:511>), [AugSelectionEase:515](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:515>), [AugSelectionSmooth:521](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:521>), [MakeAugSelectionLayout:526](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:526>), [AugIsWardVisual:551](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:551>), [AugIsMotionVisual:568](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:568>), [AugIsStrikeVisual:581](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:581>), [AugVisualFamilyOf:604](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:604>), [AugVisualFamilyLabel:618](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:618>), [DrawAugSelectionConstellation:631](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:631>), [BuildAugImpactRows:798](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:798>), [AugMetricLabel:814](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:814>), [FormatAugMetricValue:827](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:827>), [Scene_AugSelectConstellationPolished:840](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:840>), [Scene_AugSelect:1494](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:1494>), [Scene_AugReplaceCards:1497](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:1497>), [Scene_AugReplace:1663](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:1663>), [Scene_OwnedAugPanel:1666](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp:1666>).

참조 `g_*`: `g_AugExitSlot`, `g_AugExitT`, `g_CurrentWeapon`, `g_GameManager`, `g_HoveredAug`, `g_Language`, `g_LmbPrev`, `g_OwnedAugs`, `g_RepExitSlot`, `g_RepExitT`, `g_Scale`, `g_ScrollAccum`, `g_SmokeAugFocus`, `g_SmokeCapture`, `g_TextL`, `g_TextS`.

이름 기반 호출 위치 318개. 간접 포함 의존 46파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneBoot.cpp

부팅/초기 화면 연출.

자체 include: [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Scenes/SceneContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneContext.h>).

직접 피의존: 없음.

외부/표준 include: 없음.

함수 후보: [LaunchApp:12](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneBoot.cpp:12>).

참조 `g_*`: `g_AppOpen`, `g_BootAb`, `g_BootAg`, `g_BootAnim`, `g_BootAr`, `g_BootName`, `g_BootTarget`.

이름 기반 호출 위치 0개. 간접 포함 의존 13파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneCodex.inl

독립 CODEX 화면의 도감 UI 구현.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>).

외부/표준 include: 없음.

함수 후보: [Scene_Codex:3](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneCodex.inl:3>).

참조 `g_*`: `g_CodexEntryT`, `g_CodexSearchInputEnabled`, `g_GameManager`, `g_LmbPrev`, `g_ScrollAccum`, `g_TextL`, `g_TextS`.

이름 기반 호출 위치 196개. 간접 포함 의존 0파일, 간접 피의존 1파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneCodexInline.inl

메인 메뉴 안 도감 패널 UI 구현.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>).

외부/표준 include: 없음.

함수 후보: [Scene_CodexInline:3](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneCodexInline.inl:3>).

참조 `g_*`: `g_CodexEntryT`, `g_CodexSearch`, `g_CodexSearchCompositionLen`, `g_CodexSearchInputEnabled`, `g_GameManager`, `g_LmbPrev`, `g_MainMenuEntryT`, `g_RmbPrev`, `g_ScrollAccum`, `g_TextL`, `g_TextS`.

이름 기반 호출 위치 201개. 간접 포함 의존 0파일, 간접 피의존 1파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneContext.h

화면에 전달하는 게임/렌더/상태 컨텍스트.

자체 include: [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneBoot.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneBoot.cpp>), [WiNILL/Core/Scenes/SceneInternal.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneInternal.h>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>), [WiNILL/Core/Scenes/WindowChrome.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `functional`.

함수 후보: [StartSceneFade:39](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneContext.h:39>).

참조 `g_*`: `g_AppOpen`, `g_BootAb`, `g_BootAg`, `g_BootAnim`, `g_BootAr`, `g_BootName`, `g_BootTarget`, `g_FadeAlpha`, `g_FadeDir`, `g_FadeTarget`.

이름 기반 호출 위치 0개. 간접 포함 의존 12파일, 간접 피의존 13파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneInternal.h

화면 내부 공용 선언.

자체 include: [WiNILL/Core/Scenes/SceneContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneContext.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `string`, `vector`.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: `g_MainMenuEntryT`, `g_SettingsEntryT`.

이름 기반 호출 위치 0개. 간접 포함 의존 13파일, 간접 피의존 8파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneMainMenu.inl

메인 메뉴 내용·선택·패널 진입.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>).

외부/표준 include: 없음.

함수 후보: [Scene_MainMenu:3](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMainMenu.inl:3>).

참조 `g_*`: `g_BootAb`, `g_BootAg`, `g_BootAnim`, `g_BootAr`, `g_BootTarget`, `g_FadeAlpha`, `g_FadeDir`, `g_GameManager`, `g_LmbPrev`, `g_MainMenuEntryT`, `g_TextS`.

이름 기반 호출 위치 55개. 간접 포함 의존 0파일, 간접 피의존 1파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneMenus.cpp

메뉴 공통 상태·연출·back 처리 및 여러 inl의 번역 단위.

자체 include: [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>), [WiNILL/Core/AugmentSlots.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h>), [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Core/Scenes/SceneCodex.inl](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneCodex.inl>), [WiNILL/Core/Scenes/SceneCodexInline.inl](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneCodexInline.inl>), [WiNILL/Core/Scenes/SceneInternal.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneInternal.h>), [WiNILL/Core/Scenes/SceneMainMenu.inl](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMainMenu.inl>), [WiNILL/Core/Scenes/SceneShop.inl](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneShop.inl>), [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>), [WiNILL/Core/Scenes/SceneUI.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.h>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>), [WiNILL/Core/Scenes/WindowChrome.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.h>), [WiNILL/Core/TutorialText.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h>), [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Entity/ExpSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/Render/NebulaGlowShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.h>), [WiNILL/Render/PlanetShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>), [WiNILL/System/Audio.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.h>), [WiNILL/System/Input.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.h>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>), [WiNILL/System/SystemInfo.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SystemInfo.h>), [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `algorithm`, `cmath`, `functional`, `glad/glad.h`, `string`, `vector`.

함수 후보: [UiApproach:63](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:63>), [MainMenuRouteLabel:90](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:90>), [MainMenuMotionProfile:101](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:101>), [MotionProfileSettled:114](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:114>), [UpdateMainMenuConstellationMotion:123](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:123>), [ResetCodexUi:152](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:152>), [ResetShopUi:159](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:159>), [SmokeOpenMenuPanel:165](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:165>), [DrawMenuBackground:179](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:179>), [MainLogoTop:229](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:229>), [MainLogoHeight:233](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:233>), [LogoClamp01:240](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:240>), [SetSceneTextureReveal:246](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:246>), [LogoLine:250](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:250>), [DrawSceneLeftVignette:261](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:261>), [DrawPersistentSceneLeftVignette:271](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:271>), [DrawPersistentSceneSideVignettes:280](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:280>), [DrawSceneRadialVignette:295](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:295>), [DrawSettingsOrbitArc:302](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:302>), [MainMenuCommandStartY:321](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:321>), [OutgameButtonRailStartY:335](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:335>), [MainMenuButtonRailStartY:344](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:344>), [DrawSharedMenuDim:349](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:349>), [DrawAstralDataPlate:366](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:366>), [ConstellGeometryHalo:407](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:407>), [DrawVisibleConstellLine:411](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:411>), [DrawConstellationDisc:447](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:447>), [ShopStaticFieldPalette:469](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:469>), [MakeShopDetailLayout:500](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:500>), [DrawVisibleConstellNode:533](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:533>), [DrawArchiveConstellation:589](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:589>), [DrawUnknownConstellation:707](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:707>), [DrawMainOnedowLogo:774](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:774>), [MenuCommandReveal:938](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:938>), [DrawMainMenuAstralVeil:944](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:944>), [BuildInlineThreeColumnLayout:976](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:976>), [ProjectOrbitalPoint:1007](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:1007>), [DrawMainMenuOrbitalInstrument:1021](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:1021>), [TryEscNavigateBack:1185](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp:1185>).

참조 `g_*`: `g_BackdropBlurEnabled`, `g_CodexEntryT`, `g_CodexSearchInputEnabled`, `g_ConfigPanelTex`, `g_ConstellationCircleTex`, `g_ConstellationLineTex`, `g_GameManager`, `g_IconBatchProg`, `g_MainMenuEntryT`, `g_SceneTextureReveal`, `g_SettingsReturnTo`, `g_ShaderFx`, `g_ShopEntryT`, `g_VfxDensity`.

이름 기반 호출 위치 132개. 간접 포함 의존 51파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/ScenePause.cpp

일시정지 화면 표시/입력.

자체 include: [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>), [WiNILL/Core/AugmentSlots.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h>), [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Core/Scenes/SceneInternal.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneInternal.h>), [WiNILL/Core/Scenes/ScenePause.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.h>), [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>), [WiNILL/Core/Scenes/SceneUI.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.h>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>), [WiNILL/Core/Scenes/WindowChrome.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.h>), [WiNILL/Core/TutorialText.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h>), [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Entity/ExpSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/Render/NebulaGlowShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.h>), [WiNILL/Render/PlanetShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>), [WiNILL/System/Audio.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.h>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>), [WiNILL/System/SystemInfo.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SystemInfo.h>), [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `algorithm`, `cmath`, `functional`, `glad/glad.h`, `string`, `vector`.

함수 후보: [ApplyPauseAction:49](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp:49>), [Scene_Paused:74](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp:74>).

참조 `g_*`: `g_BaseOrtho`, `g_MainOrtho`, `g_MainProjLoc`.

이름 기반 호출 위치 32개. 간접 포함 의존 45파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/ScenePause.h

일시정지용 좁은 컨텍스트/인터페이스.

자체 include: [WiNILL/Core/Scenes/SceneTextContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneTextContext.h>).

직접 피의존: [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 1파일, 간접 피의존 10파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneRunConfig.cpp

직업/무기/시련/Creative 런 구성·능력표·로드아웃 확정.

자체 include: [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>), [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Core/Scenes/SceneInternal.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneInternal.h>), [WiNILL/Core/Scenes/SceneUI.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.h>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>), [WiNILL/Core/Scenes/WindowChrome.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.h>), [WiNILL/Core/TutorialText.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h>), [WiNILL/Entity/ExpSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/Render/NebulaGlowShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.h>), [WiNILL/Render/PlanetShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>), [WiNILL/System/Audio.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.h>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>), [WiNILL/System/SystemInfo.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SystemInfo.h>), [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `algorithm`, `cmath`, `functional`, `glad/glad.h`, `string`, `vector`.

함수 후보: [ResetRunConfigUi:47](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp:47>), [FinalizeLoadout:57](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp:57>), [FixedWeaponForSelectedJob:76](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp:76>), [Scene_CreativeConfig:84](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp:84>), [Scene_RunConfigInline:156](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp:156>).

참조 `g_*`: `g_BatchAlpha`, `g_CreativeMode`, `g_CreativeStartScore`, `g_CurrentWeapon`, `g_Difficulty`, `g_GameManager`, `g_LmbPrev`, `g_MainMenuEntryT`, `g_PrevHP`, `g_RmbPrev`, `g_ScrollAccum`, `g_SelectedJob`, `g_Stats`, `g_TextL`, `g_TextS`, `g_TrialPool`, `g_TrialPoolReady`, `g_TrialSelected`.

이름 기반 호출 위치 171개. 간접 포함 의존 44파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneRunEnd.cpp

사망/게임오버 결과 UI.

자체 include: [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>), [WiNILL/Core/AugmentSlots.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h>), [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Core/Scenes/SceneInternal.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneInternal.h>), [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>), [WiNILL/Core/Scenes/SceneUI.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.h>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>), [WiNILL/Core/Scenes/WindowChrome.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.h>), [WiNILL/Core/TutorialText.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h>), [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Entity/ExpSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/Render/NebulaGlowShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.h>), [WiNILL/Render/PlanetShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.h>), [WiNILL/Render/PlayerWeaponShell.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>), [WiNILL/System/Audio.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.h>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>), [WiNILL/System/SystemInfo.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SystemInfo.h>), [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `algorithm`, `cmath`, `functional`, `glad/glad.h`, `string`, `vector`.

함수 후보: [ApplyGameOverAction:50](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp:50>), [Scene_GameOver:68](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp:68>).

참조 `g_*`: `g_BestScore`, `g_Coins`, `g_Difficulty`, `g_RunStardust`, `g_SelectedJob`.

이름 기반 호출 위치 109개. 간접 포함 의존 46파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneRunEnd.h

런 종료 화면용 컨텍스트/인터페이스.

자체 include: [WiNILL/Core/Scenes/SceneTextContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneTextContext.h>).

직접 피의존: [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>).

외부/표준 include: `vector`.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 1파일, 간접 피의존 10파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/Scenes.h

화면 진입점 묶음.

자체 include: [WiNILL/Core/Scenes/SceneContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneContext.h>), [WiNILL/Core/Scenes/ScenePause.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.h>), [WiNILL/Core/Scenes/SceneRunEnd.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.h>), [WiNILL/Core/Scenes/SceneStart.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.h>).

직접 피의존: [WiNILL/Core/GameManager.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 17파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneSettings.cpp

설정 레이아웃·편집·스냅샷·취소/적용·변경 감지.

자체 include: [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>), [WiNILL/Core/AugmentSlots.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h>), [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Core/Scenes/SceneInternal.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneInternal.h>), [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>), [WiNILL/Core/Scenes/SceneUI.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.h>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>), [WiNILL/Core/Scenes/WindowChrome.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.h>), [WiNILL/Core/TutorialText.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h>), [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Entity/ExpSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/Render/NebulaGlowShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.h>), [WiNILL/Render/PlanetShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>), [WiNILL/System/Audio.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.h>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>), [WiNILL/System/SystemInfo.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SystemInfo.h>), [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `algorithm`, `cmath`, `functional`, `glad/glad.h`, `string`, `vector`.

함수 후보: [ResetSettingsUi:47](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp:47>), [SettingsText:55](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp:55>), [Scene_SettingsInline:61](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp:61>), [Scene_Settings:1620](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp:1620>).

참조 `g_*`: `g_AudioEngineEnabled`, `g_AudioMonoOutput`, `g_AutoFire`, `g_AutoSkill`, `g_BackdropBlurCaptureHz`, `g_BackdropBlurEnabled`, `g_BgmEnabled`, `g_DebugMode`, `g_FpsCap`, `g_GameManager`, `g_Language`, `g_LmbPrev`, `g_MainMenuEntryT`, `g_MobVisualStyle`, `g_RmbPrev`, `g_ScrollAccum`, `g_SettingsEntryT`, `g_SettingsReturnTo`, `g_SfxEnabled`, `g_ShaderFx`, `g_ShowCombo`, `g_ShowCrosshair`, `g_SoundVol`, `g_TextL`, `g_TextS`, `g_VfxDensity`.

이름 기반 호출 위치 174개. 간접 포함 의존 46파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneShop.inl

상점 패널·선택/스크롤·구매 UI.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>).

외부/표준 include: 없음.

함수 후보: [ShopText:4](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneShop.inl:4>), [Scene_Shop:10](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneShop.inl:10>).

참조 `g_*`: `g_BackdropBlurEnabled`, `g_Coins`, `g_ConstellationCircleTex`, `g_GameManager`, `g_LmbPrev`, `g_MainMenuEntryT`, `g_MetaLv`, `g_RmbPrev`, `g_ScrollAccum`, `g_ShopEntryT`, `g_TextL`, `g_TextS`, `g_ThemeOwned`.

이름 기반 호출 위치 140개. 간접 포함 의존 0파일, 간접 피의존 1파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneSkills.cpp

스킬 슬롯 상태·장착/재장착·HUD.

자체 include: [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>).

직접 피의존: 없음.

외부/표준 include: 없음.

함수 후보: [SkillForAug:6](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.cpp:6>), [EquipSkill:13](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.cpp:13>), [ClearEquippedSkills:24](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.cpp:24>), [ReequipSkillsFromOwned:29](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.cpp:29>).

참조 `g_*`: `g_SkillReplaceIdx`, `g_Skills`.

이름 기반 호출 위치 3개. 간접 포함 의존 5파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneSkills.h

스킬 타입/슬롯과 장착·표시 함수 선언.

자체 include: [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneSkills.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Entity/PlayerRuntime.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: `g_SkillReplaceIdx`, `g_Skills`.

이름 기반 호출 위치 0개. 간접 포함 의존 4파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneStart.cpp

튜토리얼/Ready/시작 화면 및 텍스트 처리.

자체 include: [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>), [WiNILL/Core/AugmentSlots.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h>), [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Core/Scenes/SceneInternal.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneInternal.h>), [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>), [WiNILL/Core/Scenes/SceneUI.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.h>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>), [WiNILL/Core/Scenes/WindowChrome.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.h>), [WiNILL/Core/TutorialText.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h>), [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Entity/ExpSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/Render/NebulaGlowShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.h>), [WiNILL/Render/PlanetShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>), [WiNILL/System/Audio.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.h>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>), [WiNILL/System/SystemInfo.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SystemInfo.h>), [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `algorithm`, `cmath`, `functional`, `glad/glad.h`, `string`, `vector`.

함수 후보: [Scene_Tutorial:44](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp:44>), [Scene_Ready:207](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp:207>).

참조 `g_*`: 없음.

이름 기반 호출 위치 62개. 간접 포함 의존 45파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneStart.h

시작 화면용 컨텍스트/인터페이스.

자체 include: [WiNILL/Core/Scenes/SceneTextContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneTextContext.h>).

직접 피의존: [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 1파일, 간접 피의존 10파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneTextContext.h

텍스트 렌더러 중심의 좁은 화면 컨텍스트.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/ScenePause.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.h>), [WiNILL/Core/Scenes/SceneRunEnd.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.h>), [WiNILL/Core/Scenes/SceneStart.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.h>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 13파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneUI.cpp

화면 공통 UI 도형/텍스트/전환 기능.

자체 include: [WiNILL/Core/Scenes/SceneUI.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `algorithm`, `cmath`, `string`, `utility`, `vector`.

함수 후보: [MotionForButton:30](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:30>), [DrawUIButtonFeedback:65](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:65>), [UiTextScale:103](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:103>), [MaxLocalizedTextWidth:114](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:114>), [DrawShadowedTextImmediate:132](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:132>), [DrawMenuCommandFeedback:152](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:152>), [DrawPanelButtonCore:194](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:194>), [UpdateMenuCommandHover:252](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:252>), [DrawShadowedText:257](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:257>), [FlushSceneTextCommands:277](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:277>), [BeginDeferredSceneText:294](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:294>), [EndDeferredSceneText:299](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:299>), [DrawPanelButton:306](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:306>), [DrawUnifiedMenuCommand:324](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:324>), [PanelButtonHit:343](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:343>), [UpdatePanelButtonHover:350](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:350>), [CenterTextX:361](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:361>), [UIButton:365](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp:365>).

참조 `g_*`: `g_DeferSceneText`, `g_SceneTextCommands`, `g_TextL`, `g_TextS`.

이름 기반 호출 위치 43개. 간접 포함 의존 5파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/SceneUI.h

공통 UI 인터페이스와 전환 관련 선언.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Core/Scenes/SceneUI.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/WindowChrome.cpp

앱 창 테두리/제목/버튼 그리기와 동작.

자체 include: [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Scenes/SceneContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneContext.h>), [WiNILL/Core/Scenes/WindowChrome.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>).

직접 피의존: 없음.

외부/표준 include: 없음.

함수 후보: [SceneAppWindow:7](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.cpp:7>).

참조 `g_*`: `g_AppOpen`.

이름 기반 호출 위치 4개. 간접 포함 의존 16파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/Scenes/WindowChrome.h

창 UI 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Core/Scenes/WindowChrome.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Core/TutorialText.h

언어별 튜토리얼 텍스트/선택.

자체 include: [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>).

외부/표준 include: 없음.

함수 후보: [LangIndexTutorial:7](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h:7>), [ReadyTitle:30](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h:30>), [ReadyBrief:31](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h:31>), [ReadyHint:32](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h:32>), [TutorialWinTitle:191](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h:191>), [TutorialPageTitle:192](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h:192>), [TutorialPageBody:196](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h:196>), [TutorialControls:200](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h:200>).

참조 `g_*`: `g_Language`.

이름 기반 호출 위치 7개. 간접 포함 의존 1파일, 간접 피의존 7파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/Augment.h

증강 타입/희귀도/분류/자격/설명 선택의 공용 모델.

자체 include: [WiNILL/Entity/AugmentDescKR.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/AugmentDescKR.h>), [WiNILL/System/Platform.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/Core/AugmentSlots.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h>), [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Entity/AugmentCatalog.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/AugmentCatalog.cpp>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Render/GameFonts.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.cpp>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `cstdlib`.

함수 후보: [CurLangIdx:146](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:146>), [AugName:151](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:151>), [AugStat:152](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:152>), [AugIndexOfType:173](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:173>), [AugRemoved:180](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:180>), [AugOnceOnly:259](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:259>), [GetRarityColor:282](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:282>), [AugIsSkillType:300](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:300>), [AugIsWeaponType:306](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:306>), [AugIsOrbitType:316](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:316>), [AugListGroupOf:330](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:330>), [AugListGroupOrder:342](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:342>), [AugListGroupLabel:357](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:357>), [OwnedAugListOrder:384](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:384>), [AugListIndexLess:399](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:399>), [AugTierIndexLess:411](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:411>), [GetRarityKR:419](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:419>), [GetAugTag:435](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:435>), [GetAugBadge:447](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:447>), [AugDesc:457](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h:457>).

참조 `g_*`: `g_Language`, `g_TypeOwned`.

이름 기반 호출 위치 20개. 간접 포함 의존 3파일, 간접 피의존 49파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/AugmentCatalog.cpp

증강 카탈로그 슬롯/이름/설명/예약 데이터.

자체 include: [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>).

직접 피의존: 없음.

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 4파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/AugmentDescKR.h

한국어 증강 상세 설명 오버라이드.

자체 include: 없음.

직접 피의존: [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>).

외부/표준 include: 없음.

함수 후보: [AugDescKR:3](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/AugmentDescKR.h:3>).

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 50파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/Bullet.h

탄환 상태·이동/유도·수명·피해/반사 속성.

자체 include: 없음.

직접 피의존: [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Entity/CollisionSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/ProjectileSystem.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ProjectileSystem.cpp>), [WiNILL/Entity/RangedMob.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `algorithm`, `cmath`, `glm/glm.hpp`.

함수 후보: [Bullet:53](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h:53>), [Update:63](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h:63>), [GetBulletHaloStyle:86](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h:86>).

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 37파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/CollisionSystem.h

탄환-몹/플레이어 충돌·피해·처치·관통/도탄.

자체 include: [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Entity/Bullet.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h>), [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>).

직접 피의존: [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `algorithm`, `cmath`, `cstdlib`, `glm/glm.hpp`.

함수 후보: [SpawnKillStardust:16](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h:16>), [ReleasePendingStardust:28](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h:28>), [SegDistSq:37](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h:37>), [SegDist:54](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h:54>), [RicochetTo:59](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h:59>), [CritRoll:92](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h:92>), [Update:102](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h:102>).

참조 `g_*`: 없음.

이름 기반 호출 위치 43개. 간접 포함 의존 17파일, 간접 피의존 1파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/EnemyProgression.h

진행도에 따른 적 스펙 배율/상한.

자체 include: 없음.

직접 피의존: [WiNILL/Entity/EnemySpawner.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `algorithm`.

함수 후보: [EnemySpeedRamp:16](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyProgression.h:16>), [EvaluateEnemyProgression:21](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyProgression.h:21>).

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 2파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/EnemyShield.h

적 실드 상태/피해 처리와 관련 효과.

자체 include: [WiNILL/Render/EnemyParticles.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h>).

직접 피의존: [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/RangedMob.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h>).

외부/표준 include: `algorithm`.

함수 후보: [ApplyMobDamage:8](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyShield.h:8>), [SpawnRegulusShieldGrantParticles:41](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyShield.h:41>), [SpawnRegulusShieldBreakParticles:56](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyShield.h:56>), [SpawnRegulusLinkBreakParticles:72](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyShield.h:72>), [ApplyEnemyDamageWithShield:87](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyShield.h:87>).

참조 `g_*`: 없음.

이름 기반 호출 위치 5개. 간접 포함 의존 2파일, 간접 피의존 37파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/EnemySpawner.cpp

일반/원거리 스폰 시간·종류 선택·진행도/개체 제한.

자체 include: [WiNILL/Entity/EnemyProgression.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyProgression.h>), [WiNILL/Entity/EnemySpawner.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: 없음.

외부/표준 include: `algorithm`, `cstdlib`.

함수 후보: [ApplyRotorSpawnSpecialization:18](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.cpp:18>), [ResetEnemySpawnState:104](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.cpp:104>), [UpdateEnemySpawns:108](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.cpp:108>).

참조 `g_*`: `g_EnemySpawnState`.

이름 기반 호출 위치 17개. 간접 포함 의존 13파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/EnemySpawner.h

스폰 초기화/갱신 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Entity/EnemySpawner.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 2파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/ExpSystem.h

XP 획득/레벨 관련 계산·표현 접점.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/System/GameplayTelemetry.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `cmath`.

함수 후보: [Required:26](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h:26>).

참조 `g_*`: `g_ExpSystem`.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/Monster.h

일반/특수 몹 데이터·스펙·AI 상태·사망 형상/연출.

자체 include: [WiNILL/Entity/Bullet.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h>), [WiNILL/Entity/EnemyShield.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyShield.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>).

직접 피의존: [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Entity/EnemySpawner.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.cpp>), [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>), [tests/magnetar_field_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/magnetar_field_test.cpp>), [tests/mob_death_fx_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp>), [tests/quasar_beam_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_beam_test.cpp>).

외부/표준 include: `algorithm`, `array`, `cmath`, `cstdlib`, `glm/glm.hpp`, `vector`.

함수 후보: [Project:36](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:36>), [FaceToward:46](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:46>), [MobKillReward:78](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:78>), [StardustRewardFor:95](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:95>), [MobRewardMult:110](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:110>), [MobXpBonus:114](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:114>), [MobExperienceReward:123](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:123>), [ApplyDamage:393](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:393>), [MakeKind:404](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:404>), [AntaresStage:511](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:511>), [AntaresRamRange:519](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:519>), [AntaresSpeedRatio:522](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:522>), [AntaresGhostCount:529](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:529>), [SpawnRotorDeathEffect:533](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:533>), [SpawnSwarmDeathEffect:545](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:545>), [SpawnGimbalDeathEffect:557](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:557>), [SpawnAntaresDeathEffect:573](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:573>), [SpawnRegulusDeathEffect:585](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:585>), [DeathEffectVisible:601](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:601>), [UpdateDeathEffect:609](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:609>), [EliteKillFxWeight:619](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:619>), [DeathRewardTime:634](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:634>), [TakePendingDust:641](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:641>), [SpawnGenesisInstabilityBurst:651](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:651>), [SpawnGenesisWaveBurst:663](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:663>), [DrawGravisDeathEffect:682](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:682>), [DrawQuasarDeathEffect:719](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:719>), [DrawGenesisDeathEffect:832](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:832>), [DrawDeathEffect:980](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:980>), [SpawnGravisDeathEffect:991](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:991>), [SpawnQuasarCollapseBurst:1008](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1008>), [SpawnQuasarBlastBurst:1022](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1022>), [SpawnStagedDeathBursts:1042](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1042>), [SpawnDeathEffect:1060](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1060>), [ConstrainGenesisToViewport:1107](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1107>), [ConstrainGimbalToViewport:1122](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1122>), [ConstrainRegulusToViewport:1134](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1134>), [MagnetarYaw:1158](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1158>), [MagnetarPitch:1159](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1159>), [MagnetarFaceToward:1160](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1160>), [QuasarPrecharge:1166](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1166>), [QuasarHeat:1172](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1172>), [GenesisStackFill:1186](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1186>), [GenesisStacks:1194](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1194>), [GenesisThrowAge:1197](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1197>), [ThrownFlightTime:1210](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1210>), [ThrownFlightFraction:1222](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1222>), [BeginThrownFlight:1233](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1233>), [UpdateThrownSwarm:1253](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1253>), [BeginGenesisEgress:1289](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1289>), [TryContact:1298](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1298>), [UpdateGimbal:1315](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1315>), [UpdateAntares:1399](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1399>), [Update:1507](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h:1507>).

참조 `g_*`: 없음.

이름 기반 호출 위치 92개. 간접 포함 의존 8파일, 간접 피의존 35파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/MonsterManager.h

일반/원거리 몹 소유·갱신·검색·특수 패턴 조정.

자체 include: [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/RangedMob.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h>).

직접 피의존: [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Entity/CollisionSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h>), [WiNILL/Entity/EnemySpawner.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.cpp>), [WiNILL/Entity/ProjectileSystem.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ProjectileSystem.cpp>), [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>), [WiNILL/Render/PlayerWeaponShell.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp>), [WiNILL/System/DebugToolkit.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>), [tests/genesis_launcher_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/genesis_launcher_test.cpp>), [tests/quasar_nova_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/quasar_nova_test.cpp>), [tests/regulus_beam_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/regulus_beam_test.cpp>).

외부/표준 include: `algorithm`, `cmath`, `cstdlib`, `vector`.

함수 후보: [ApplyDamage:22](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:22>), [ApplyDamage:28](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:28>), [MonsterManager:34](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:34>), [EdgePoint:38](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:38>), [SpawnMob:49](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:49>), [SpawnRangedMob:59](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:59>), [FindNearestEnemy:70](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:70>), [CollisionTier:101](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:101>), [UpdateDeathEffects:117](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:117>), [UpdateAll:123](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:123>), [Clear:179](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:179>), [RegulusTargetTier:187](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:187>), [HandleShieldBreak:205](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:205>), [HandleShieldBreak:209](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:209>), [ProcessPendingShieldBreaks:213](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:213>), [RegulusEligible:218](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:218>), [UpdateRegulusShots:225](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:225>), [UpdateRegulusBeams:282](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:282>), [UpdateGenesisLaunchers:293](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:293>), [ResolveMonsterCrowding:393](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h:393>).

참조 `g_*`: 없음.

이름 기반 호출 위치 28개. 간접 포함 의존 10파일, 간접 피의존 28파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/PlayerRuntime.cpp

플레이어 이동·대시·스킬 타이머·입력 edge·실행 상태.

자체 include: [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>), [WiNILL/Entity/PlayerRuntime.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>).

직접 피의존: 없음.

외부/표준 include: `algorithm`, `cmath`.

함수 후보: [PlayerSkillCooldownMax:10](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:10>), [ResetPlayerSkills:19](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:19>), [ResetPlayerInputEdges:34](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:34>), [UpdatePlayerRuntimeTimers:39](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:39>), [UpdatePlayerMovement:49](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:49>), [GetPlayerViewportBounds:68](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:68>), [ClampPlayerToViewport:85](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:85>), [ApplyPlayerDamageProtection:98](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:98>), [BeginPlayerDash:107](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:107>), [AdvancePlayerDash:125](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:125>), [ApplyPlayerKnockback:143](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:143>), [AdvancePlayerKnockback:148](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp:148>).

참조 `g_*`: `g_PlayerRuntime`, `g_PlayerShield`, `g_PlayerShieldTimer`, `g_SkillReplaceIdx`, `g_Skills`.

이름 기반 호출 위치 0개. 간접 포함 의존 7파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/PlayerRuntime.h

플레이어 런타임 상태와 이동/타이머 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Entity/PlayerRuntime.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: `g_PlayerRuntime`.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 2파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/PlayerStats.h

기본 스탯·증강 적용·누적 보너스·흡혈/스탯 계산.

자체 include: [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>).

직접 피의존: [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/GameManager.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp>), [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Entity/CollisionSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h>), [WiNILL/Entity/EnemySpawner.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.cpp>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/PlayerRuntime.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.cpp>), [WiNILL/Entity/RangedMob.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/System/DebugToolkit.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h>), [WiNILL/System/GameplayTelemetry.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `algorithm`, `cmath`.

함수 후보: [Apply:116](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:116>), [ApplyWarlordStack:410](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:410>), [RestoreWarlordStacks:418](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:418>), [RegisterKill:426](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:426>), [GetDamageTakenMult:433](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:433>), [GetRegenRate:439](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:439>), [GetVampireKillNeed:446](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:446>), [GetBaseDamage:449](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:449>), [GetDamageMultiplier:456](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:456>), [GetFireIntervalMult:460](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:460>), [GetBulletSpeedBonus:466](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:466>), [GetLifestealCap:468](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:468>), [GetLifestealPerKill:473](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:473>), [ApplyCommonMultBoost:481](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:481>), [GetMoveMultiplier:488](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:488>), [GrantPlayerShield:502](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:502>), [TickPlayerShield:508](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:508>), [HurtPlayer:518](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h:518>).

참조 `g_*`: `g_PlayerDamagePulse`, `g_PlayerDmgMult`, `g_PlayerShield`, `g_PlayerShieldTimer`.

이름 기반 호출 위치 3개. 간접 포함 의존 4파일, 간접 피의존 43파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/ProjectileSystem.cpp

플레이어 투사체 발사·갱신·지우기 등 처리.

자체 include: [WiNILL/Entity/Bullet.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h>), [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>), [WiNILL/Entity/ProjectileSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ProjectileSystem.h>), [WiNILL/System/FakeWindow.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/FakeWindow.h>).

직접 피의존: 없음.

외부/표준 include: `algorithm`, `cmath`.

함수 후보: [TurnAngleToward:10](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ProjectileSystem.cpp:10>), [UpdateProjectiles:21](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ProjectileSystem.cpp:21>).

참조 `g_*`: 없음.

이름 기반 호출 위치 4개. 간접 포함 의존 13파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/ProjectileSystem.h

투사체 시스템 호출 인터페이스/입력.

자체 include: 없음.

직접 피의존: [WiNILL/Entity/ProjectileSystem.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ProjectileSystem.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `vector`.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 2파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/RangedMob.h

원거리 몹 상태·차징/발사·이동·피격/사망 상태.

자체 include: [WiNILL/Entity/Bullet.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h>), [WiNILL/Entity/EnemyShield.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyShield.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>), [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>), [tests/mob_death_fx_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp>).

외부/표준 include: `cmath`, `cstdlib`, `glm/glm.hpp`, `vector`.

함수 후보: [ApplyDamage:59](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h:59>), [ExperienceReward:65](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h:65>), [SpawnDeathEffect:69](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h:69>), [pickNewTarget:89](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h:89>), [Update:100](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h:100>).

참조 `g_*`: `g_ArenaExX`, `g_ArenaExY`.

이름 기반 호출 위치 6개. 간접 포함 의존 8파일, 간접 피의존 30파일. 상세 위치는 TSV에 기록.

### WiNILL/Entity/Weapons.h

시작 무기 타입/기본 적용 규칙.

자체 include: [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>).

직접 피의존: [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>), [WiNILL/Core/GameManager.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/PlayerWeaponShell.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: [WeaponName:27](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h:27>), [WeaponDesc:28](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h:28>), [CurrentWeaponLabel:33](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h:33>), [CurrentWeaponDescText:40](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h:40>), [ApplyWeapon:47](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h:47>).

참조 `g_*`: `g_CurrentWeapon`, `g_Stats`.

이름 기반 호출 위치 6개. 간접 포함 의존 5파일, 간접 피의존 12파일. 상세 위치는 TSV에 기록.

### WiNILL/main.cpp

앱 수명·게임 루프·전환·런 시작/종료·전투·보상·HUD의 조정 중심.

자체 include: [WiNILL/Core/AugmentSlots.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/AugmentSlots.h>), [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Core/GameVersion.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameVersion.h>), [WiNILL/Core/Scenes/SceneContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneContext.h>), [WiNILL/Core/Scenes/SceneInternal.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneInternal.h>), [WiNILL/Core/Scenes/SceneSkills.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSkills.h>), [WiNILL/Core/Scenes/SceneUI.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.h>), [WiNILL/Core/Scenes/Scenes.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/Scenes.h>), [WiNILL/Core/Scenes/WindowChrome.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.h>), [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Entity/Bullet.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h>), [WiNILL/Entity/CollisionSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h>), [WiNILL/Entity/EnemyProgression.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyProgression.h>), [WiNILL/Entity/EnemySpawner.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.h>), [WiNILL/Entity/ExpSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>), [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>), [WiNILL/Entity/PlayerRuntime.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerRuntime.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/Entity/ProjectileSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ProjectileSystem.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/Render/GameFonts.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.h>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/Render/NebulaGlowShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.h>), [WiNILL/Render/PlanetShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.h>), [WiNILL/Render/PlayerWeaponShell.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>), [WiNILL/System/CrashHandler.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/CrashHandler.h>), [WiNILL/System/DebugToolkit.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h>), [WiNILL/System/FakeWindow.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/FakeWindow.h>), [WiNILL/System/GameplayTelemetry.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h>), [WiNILL/System/Input.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.h>), [WiNILL/System/Platform.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>), [WiNILL/System/SmokeCapture.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h>), [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>), [WiNILL/System/WindowFx.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `GLFW/glfw3native.h`, `algorithm`, `cmath`, `cstdarg`, `cstdio`, `cstdlib`, `cstring`, `ctime`, `cwctype`, `deque`, `dwmapi.h`, `functional`, `glad/glad.h`, `glm/glm.hpp`, `iostream`, `string`, `timeapi.h`, `vector`, `windows.h`.

함수 후보: [SpawnShockWave:176](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:176>), [TriggerMuzzle:193](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:193>), [EffectivePlayerWinSize:220](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:220>), [ClampPlayerWinSize:226](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:226>), [DrawPlayerRadialBars:235](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:235>), [DrawEdgeVignette:306](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:306>), [DrawGameplayCrosshair:328](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:328>), [SyncPlayerBoundsSize:361](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:361>), [EnsurePlayerBounds:386](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:386>), [UpdateGravisFields:390](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:390>), [InitChakramsFromStats:495](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:495>), [ApplyAugmentSideEffects:509](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:509>), [RebuildPlayerStatsFromOwned:540](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:540>), [ApplySingleAugIdx:597](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:597>), [BeginAugReplaceFlow:620](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:620>), [PushAugEffectsFront:637](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:637>), [ResetAugmentDerivedRuntime:644](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:644>), [FinishCurrentAugmentReward:652](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:652>), [FinishAugmentEffects:666](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:666>), [ProcessAugEffectQueue:689](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:689>), [CompleteAugReplaceFlow:779](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:779>), [CancelAugReplaceFlow:805](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:805>), [BeginGameplayTelemetryIfNeeded:818](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:818>), [SyncGameplayTelemetry:842](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:842>), [TriggerEliteKillHitStop:867](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:867>), [main:875](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp:875>).

참조 `g_*`: `g_AchSaveNeeded`, `g_AchToastId`, `g_AchToastTimer`, `g_AppOpen`, `g_ApproachOrbs`, `g_ArenaExX`, `g_ArenaExY`, `g_AudioEngineEnabled`, `g_AudioMonoOutput`, `g_AugEffectProcessing`, `g_AugEffectQueue`, `g_AugExitSlot`, `g_AugExitT`, `g_AutoFire`, `g_AutoSkill`, `g_BackdropBlurEnabled`, `g_BalanceTestMode`, `g_BaseOrtho`, `g_BgmEnabled`, `g_BulletRainTimer`, `g_Bullets`, `g_Chakrams`, `g_CodexDirty`, `g_CodexSearch`, `g_CodexSearchCaret`, `g_CodexSearchLen`, `g_Coins`, `g_Combo`, `g_ComboMilestone`, `g_ComboPulse`, `g_ComboTimer`, `g_ConversionWeapon`, `g_CreativeFreeGrab`, `g_CreativeGodmode`, `g_CreativeMode`, `g_CreativeStartPending`, `g_CreativeStartScore`, `g_CurrentFPS`, `g_CurrentWeapon`, `g_DeathBoomDone`, `g_DeathCX`, `g_DeathCY`, `g_DeathFlash`, `g_DeathReason`, `g_DeathWinW0`, `g_Debris`, `g_DebugMode`, `g_DebugToolkit`, `g_DevToastTimer`, `g_DevUnlocked`, `g_Difficulty`, `g_DmgNumbers`, `g_Drones`, `g_DyingTimer`, `g_EliteKillHitStop`, `g_EnemyParts`, `g_ExpSystem`, `g_FadeAlpha`, `g_FadeDir`, `g_FadeTarget`, `g_FlashColor`, `g_FlashIntensity`, `g_FontMemHandle`, `g_FpsCap`, `g_FpsFrames`, `g_FpsLastTime`, `g_GameBarH`, `g_GameManager`, `g_GameOverFade`, `g_GameTime`, `g_GameplayTelemetry`, `g_HitStopTimer`, `g_HoveredAug`, `g_HpBarPop`, `g_HurtVignette`, `g_LaserBeams`, `g_LaserTimer`, `g_LastRunCoins`, `g_LastRunRecord`, `g_LmbPrev`, `g_MacOptV1`, `g_MainFxLoc`, `g_MainOrtho`, `g_MainProjLoc`, `g_MainShader`, `g_MainVAO`, `g_MonsterManager`, `g_MuzzleAng`, `g_MuzzleTimer`, `g_MuzzleX`, `g_MuzzleY`, `g_OwnedAugs`, `g_PauseSelectedAug`, `g_PlayerDamagePulse`, `g_PlayerDmgMult`, `g_PlayerRuntime`, `g_PrevHP`, `g_RainKillAccum`, `g_RepExitSlot`, `g_RepExitT`, `g_RfwH`, `g_RfwW`, `g_RmbPrev`, `g_RunStardust`, `g_Scale`, `g_ScrollAccum`, `g_SelectedJob`, `g_SettingsReturnTo`, `g_SfxEnabled`, `g_ShaderFx`, `g_ShakeMag`, `g_ShakeTime`, `g_ShockWaves`, `g_ShowCombo`, `g_ShowCrosshair`, `g_ShowPauseInventory`, `g_Skills`, `g_SmokeAugFocus`, `g_SmokeRun`, `g_SmokeTimeScale`, `g_SoundVol`, `g_Sparks`, `g_StardustHudPulse`, `g_StardustPickups`, `g_StaticFieldPulseTimer`, `g_Stats`, `g_StrongMenuDim`, `g_TaskbarH`, `g_TextL`, `g_TextS`, `g_TextXL`, `g_TotalGames`, `g_Trail`, `g_TrialPool`, `g_TrialPoolReady`, `g_TrialSelected`, `g_TypeOwned`, `g_VBO`, `g_VfxDensity`, `g_ViewZoom`, `g_ViewZoomTarget`, `g_VolBuf`, `g_VolEdit`, `g_VolLen`, `g_WinPrevHP`, `g_WindowSizeCur`, `g_XpTimeAccum`, `g_ZoomCX`, `g_ZoomCY`, `g_aug1Released`, `g_aug2Released`, `g_aug3Released`.

이름 기반 호출 위치 527개. 간접 포함 의존 58파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/BlurShader.cpp

블러 셰이더·캡처 framebuffer·렌더 상태 저장/복원.

자체 include: [WiNILL/Render/BlurShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>).

직접 피의존: 없음.

외부/표준 include: `algorithm`, `chrono`, `cstdio`, `cstring`.

함수 후보: [MakeFBOTex:93](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp:93>), [LinkProg:134](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp:134>), [DrawNdcQuad:161](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp:161>), [InitBlurSystem:174](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp:174>), [ResizeBlurSystem:229](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp:229>), [CaptureBackdrop:233](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp:233>), [InvalidateBackdropCapture:389](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp:389>), [DrawBlurPanel:394](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp:394>).

참조 `g_*`: `g_BaseOrtho`, `g_MainVAO`, `g_VBO`.

이름 기반 호출 위치 17개. 간접 포함 의존 4파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/BlurShader.h

블러 초기화/캡처/그리기 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/BlurShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>), [tests/blur_render_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/blur_render_test.cpp>).

외부/표준 include: `glad/glad.h`.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 10파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/Camera.cpp

카메라 좌표/흔들림 갱신·변환.

자체 include: [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>).

직접 피의존: 없음.

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: `g_ViewZoom`, `g_ViewZoomTarget`, `g_ZoomCX`, `g_ZoomCY`.

이름 기반 호출 위치 0개. 간접 포함 의존 3파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/Camera.h

카메라 상태/인터페이스.

자체 include: [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>).

직접 피의존: [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/Camera.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.cpp>), [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/Render/PlayerWeaponShell.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>), [tests/mob_death_fx_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/mob_death_fx_test.cpp>).

외부/표준 include: `glad/glad.h`.

함수 후보: [ZCX:13](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h:13>), [ZCY:14](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h:14>), [W2SX:15](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h:15>), [W2SY:16](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h:16>), [ScreenToWorldX:17](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h:17>), [ScreenToWorldY:18](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h:18>), [ProjectWorldDisc:25](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h:25>), [WorldScissor:35](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h:35>).

참조 `g_*`: `g_ViewZoom`, `g_ViewZoomTarget`, `g_ZoomCX`, `g_ZoomCY`.

이름 기반 호출 위치 13개. 간접 포함 의존 2파일, 간접 피의존 16파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/DrawPrim.h

공통 도형/배치/렌더 pass 기본 연산.

자체 include: [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Core/Scenes/SceneUI.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp>), [WiNILL/Core/Scenes/WindowChrome.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.cpp>), [WiNILL/Render/BlurShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/MainShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp>), [WiNILL/Render/NebulaGlowShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.cpp>), [WiNILL/Render/PlanetShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.cpp>), [WiNILL/Render/PlayerWeaponShell.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>), [WiNILL/System/DebugToolkit.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>), [tests/blur_render_test.cpp](<D:/01_Study/02_OpenGL/Onedow/tests/blur_render_test.cpp>).

외부/표준 include: `cmath`, `cstdint`, `glad/glad.h`, `vector`.

함수 후보: [BindMainShader:32](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:32>), [BatchFlush:43](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:43>), [GfxVertOk:64](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:64>), [BatchVtx:67](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:67>), [BatchTri:76](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:76>), [BatchVerts:83](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:83>), [drawRect:87](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:87>), [drawTriangle:95](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:95>), [drawCircle:101](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:101>), [drawConeFan:120](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:120>), [drawMercedes:140](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:140>), [drawPentagon:162](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:162>), [drawDiamond:176](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:176>), [SetGlowFx:190](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:190>), [drawConstellFrame:198](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:198>), [drawNeonBorder:245](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h:245>).

참조 `g_*`: `g_Batch`, `g_BatchAlpha`, `g_BatchVBOFloats`, `g_GfxPass`, `g_MainFxLoc`, `g_MainOrtho`, `g_MainProjLoc`, `g_MainShader`, `g_MainVAO`, `g_VBO`.

이름 기반 호출 위치 53개. 간접 포함 의존 1파일, 간접 피의존 29파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/EnemyParticles.h

적 파티클/폭발 등 상태와 생성·갱신·표시.

자체 include: [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/Entity/EnemyShield.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemyShield.h>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>).

외부/표준 include: `algorithm`, `cmath`, `cstdlib`, `glm/glm.hpp`, `vector`.

함수 후보: [JuiceSparkCap:17](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h:17>), [SpawnSparks:27](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h:27>), [SpawnMobDeathSparks:43](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h:43>), [RandomEnemyParticleShape:53](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h:53>), [MobDeathParticleRadius:71](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h:71>), [ClampEnemyParticleRange:86](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h:86>), [FindEnemyParticleSlot:102](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h:102>), [SpawnEnemyParticleBurst:116](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h:116>), [UpdateEnemyParticles:166](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h:166>), [ResetEnemyParticles:190](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h:190>).

참조 `g_*`: `g_EnemyParts`, `g_Sparks`, `g_VfxDensity`.

이름 기반 호출 위치 9개. 간접 포함 의존 1파일, 간접 피의존 39파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/EntityDraw.cpp

몹·탄환·필드·특수 패턴 렌더와 가시성 계산.

자체 include: [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>), [WiNILL/Entity/RangedMob.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/EntityDraw.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.h>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/Render/UiColors.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `algorithm`, `cmath`, `vector`.

함수 후보: [ApplyMobStyleTint:19](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:19>), [ApplyMobHitFlash:26](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:26>), [drawLineQuad:35](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:35>), [DrawApproachOrb:47](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:47>), [DrawAppWindow:71](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:71>), [inWin:90](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:90>), [drawBullet:99](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:99>), [DrawEnemyWirePolygon:186](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:186>), [DrawDiscWithTexture:198](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:198>), [DrawBulletHalos:216](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:216>), [BeginEnemySightFrontBatch:274](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:274>), [QueueEnemySightFront:278](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:278>), [QueueMonsterSightFront:284](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:284>), [QueueRangedMobSightFront:322](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:322>), [FlushEnemySightFrontBatch:331](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:331>), [DrawBlackSightRear:364](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:364>), [DrawEnemySightRear:373](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:373>), [DrawEnemySightRearBatch:378](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:378>), [MonsterSightRearScale:422](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:422>), [DrawCombatSightFields:433](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:433>), [DrawMonsterSightRear:473](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:473>), [DrawRangedMobSightRear:483](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:483>), [DrawEnemyNode:496](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:496>), [DrawEnemyCore:508](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:508>), [DrawFilledHexagon:520](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:520>), [DrawGenesisCore:537](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:537>), [DrawQuasarEllipseArc:558](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:558>), [DrawEnemyArc:576](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:576>), [DrawThrownBlastZone:594](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:594>), [DrawGenesisLauncher:617](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:617>), [DrawThrownSwarmGhosts:912](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:912>), [DrawAegisMark:953](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:953>), [AegisMarkExtent:984](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:984>), [DrawRegulusPlates:1002](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1002>), [DrawRegulusCrest:1023](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1023>), [DrawRegulusBeams:1064](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1064>), [DrawAntaresFrame:1085](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1085>), [DrawAntares:1113](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1113>), [DrawMagnetar:1195](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1195>), [QuasarNoise:1425](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1425>), [QuasarHash:1430](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1430>), [DrawEllipseLoop:1438](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1438>), [DrawQuasarBody:1463](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1463>), [drawMob:1791](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:1791>), [drawRangedMob:2173](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:2173>), [drawCodexMobPreview:2240](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp:2240>).

참조 `g_*`: `g_ConstellationCircleTex`, `g_EnemySightFrontMarkers`, `g_IconBatchProg`, `g_IconProg`, `g_InGameCircleTex`, `g_MobVisualStyle`, `g_RainMissileTex`, `g_RfwW`, `g_SuppressMobSeen`, `g_TextS`.

이름 기반 호출 위치 290개. 간접 포함 의존 18파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/EntityDraw.h

엔티티 렌더 진입점.

자체 include: [WiNILL/Entity/Bullet.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Bullet.h>), [WiNILL/Entity/Monster.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Monster.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `vector`.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 9파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/GameFonts.cpp

플랫폼/내장 폰트 로드·텍스트 렌더러 설정.

자체 include: [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Render/GameFonts.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/System/EmbeddedResource.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/EmbeddedResource.h>), [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>).

직접 피의존: 없음.

외부/표준 include: `algorithm`, `cstdio`, `cwctype`, `string`.

함수 후보: [InitializeGameFonts:16](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.cpp:16>), [UpdateGameTextHalo:121](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.cpp:121>).

참조 `g_*`: `g_TextL`, `g_TextS`, `g_TextXL`.

이름 기반 호출 위치 17개. 간접 포함 의존 9파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/GameFonts.h

게임 폰트 로드/설정 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Render/GameFonts.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 2파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/IconSystem.h

아이콘 로딩/마스크·텍스처·셰이더·일반/회전/darken 그리기.

자체 include: [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>), [WiNILL/Render/PlayerWeaponShell.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `cstdio`, `cstring`, `glad/glad.h`, `stb_image.h`, `vector`, `windows.h`.

함수 후보: [IconNameForAug:26](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:26>), [IconCompile:110](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:110>), [InitIconGL:116](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:116>), [DrawIconBatch:214](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:214>), [IconTexFromRGBA:275](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:275>), [IconEnableMipmaps:290](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:290>), [IconLoadMem:297](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:297>), [IconLoadFile:305](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:305>), [IconLoadFileAlphaBoost:313](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:313>), [IconLoadResourceName:328](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:328>), [IconLoadResourceAlphaBoost:339](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:339>), [IconLoad:362](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:362>), [ResolveIconDir:376](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:376>), [LoadIcons:421](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:421>), [JobIcon:485](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:485>), [IconFor:491](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:491>), [DrawIcon:527](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:527>), [DrawIconDarken:561](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:561>), [DrawIconRot:596](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:596>), [MakeRocketSprite:628](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:628>), [InitRainMissileTex:642](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:642>), [DrawAugIcon:660](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h:660>).

참조 `g_*`: `g_ConfigPanelTex`, `g_ConstellationCircleTex`, `g_ConstellationLineTex`, `g_GfxPass`, `g_IconBaseDir`, `g_IconBatchCapacityFloats`, `g_IconBatchProg`, `g_IconBatchProjLoc`, `g_IconBatchVAO`, `g_IconBatchVBO`, `g_IconDarkenLoc`, `g_IconProg`, `g_IconProjLoc`, `g_IconTex`, `g_IconTintLoc`, `g_IconVAO`, `g_IconVBO`, `g_InGameCircleTex`, `g_JobIconNames`, `g_JobIconTex`, `g_LeftGradientTex`, `g_MainOrtho`, `g_RainMissileTex`.

이름 기반 호출 위치 37개. 간접 포함 의존 5파일, 간접 피의존 10파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/Juice.h

피해 숫자·잔상·히트스톱·콤보·파티클; 전투 누적 부수효과 포함.

자체 include: [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/EnemyParticles.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/System/Audio.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.h>), [WiNILL/System/Platform.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Entity/CollisionSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/CollisionSystem.h>).

외부/표준 include: `algorithm`, `cmath`, `cstdlib`, `cstring`, `glm/glm.hpp`, `vector`.

함수 후보: [JuiceDmgCap:28](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:28>), [JuiceTrailCap:35](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:35>), [SpawnDamageNumber:43](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:43>), [SpawnTrail:63](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:63>), [TriggerHitStop:72](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:72>), [TriggerFlash:79](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:79>), [ComboIsMilestone:90](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:90>), [AddKillCombo:96](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:96>), [SpawnEnemyExplosion:109](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:109>), [UpdateEnemyFx:137](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:137>), [ResetEnemyFx:141](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:141>), [ResetJuice:146](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:146>), [DrawStardustPickups:170](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:170>), [SpawnStardust:187](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h:187>).

참조 `g_*`: `g_Combo`, `g_ComboMilestone`, `g_ComboPulse`, `g_ComboTimer`, `g_DmgNumbers`, `g_EnemyParts`, `g_FlashColor`, `g_FlashIntensity`, `g_HitStopTimer`, `g_RainKillAccum`, `g_Sparks`, `g_StardustHudPulse`, `g_StardustPickups`, `g_Trail`.

이름 기반 호출 위치 12개. 간접 포함 의존 7파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/MainShader.cpp

메인 셰이더·gradient 텍스처·기하 제출.

자체 include: [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/System/EmbeddedResource.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/EmbeddedResource.h>).

직접 피의존: 없음.

외부/표준 include: `cstdio`, `cstring`, `stb_image.h`, `windows.h`.

함수 후보: [LoadMaskTexture:45](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:45>), [LoadRadGradTexture:57](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:57>), [LoadLinGradTexture:76](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:76>), [InitRadialGradShader:95](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:95>), [CompileGlShader:149](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:149>), [InitMainShaderPipeline:166](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:166>), [InitMainBatchGeometry:197](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:197>), [DrawRadialGradient:230](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:230>), [DrawRadialGradientRect:236](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:236>), [DrawLinearGradient:265](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:265>), [DrawLinearGradientRibbon:296](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp:296>).

참조 `g_*`: `g_BaseOrtho`, `g_Batch`, `g_LinGradTex`, `g_MainFxLoc`, `g_MainOrtho`, `g_MainProjLoc`, `g_MainResLoc`, `g_MainShader`, `g_MainVAO`, `g_RadGradAlpLoc`, `g_RadGradColLoc`, `g_RadGradProg`, `g_RadGradProjLoc`, `g_RadGradTex`, `g_RadGradTexLoc`, `g_VBO`.

이름 기반 호출 위치 17개. 간접 포함 의존 4파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/MainShader.h

메인 셰이더·색/기하 렌더 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/BlurShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/BlurShader.cpp>), [WiNILL/Render/MainShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp>), [WiNILL/Render/NebulaGlowShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.cpp>), [WiNILL/Render/PlanetShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `glad/glad.h`.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: `g_BaseOrtho`, `g_MainFxLoc`, `g_MainResLoc`.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 12파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/NebulaGlowShader.cpp

성운 glow 셰이더/quad 초기화·그리기.

자체 include: [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/Render/NebulaGlowShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.h>).

직접 피의존: 없음.

외부/표준 include: `cstdio`, `windows.h`.

함수 후보: [InitNebulaGlowShader:54](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.cpp:54>), [DrawNebulaGlow:98](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.cpp:98>).

참조 `g_*`: `g_BatchAlpha`, `g_MainOrtho`, `g_NGAlpha`, `g_NGCenter`, `g_NGColor`, `g_NGProg`, `g_NGProj`, `g_NGRadius`, `g_NGRainbow`, `g_NGRes`, `g_NGResH`, `g_NGResW`, `g_NGTime`, `g_NGVAO`, `g_NGVBO`.

이름 기반 호출 위치 5개. 간접 포함 의존 4파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/NebulaGlowShader.h

성운 glow 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/NebulaGlowShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/NebulaGlowShader.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `glad/glad.h`.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/PlanetShader.cpp

행성 셰이더/quad 초기화·그리기.

자체 include: [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/MainShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.h>), [WiNILL/Render/PlanetShader.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.h>).

직접 피의존: 없음.

외부/표준 include: `cstdio`, `windows.h`.

함수 후보: [InitPlanetShader:59](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.cpp:59>), [DrawPlanetSDF:104](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.cpp:104>).

참조 `g_*`: `g_BatchAlpha`, `g_MainOrtho`, `g_PAlpha`, `g_PCenter`, `g_PColor`, `g_PGlow`, `g_PProg`, `g_PProj`, `g_PRadius`, `g_PRes`, `g_PResH`, `g_PResW`, `g_PRing`, `g_PVAO`, `g_PVBO`.

이름 기반 호출 위치 5개. 간접 포함 의존 4파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/PlanetShader.h

행성 셰이더 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/PlanetShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlanetShader.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `glad/glad.h`.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/PlayerWeaponShell.cpp

플레이어 무기 외형·선분 기하·표시 상태 참조.

자체 include: [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>), [WiNILL/Entity/Weapons.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Weapons.h>), [WiNILL/Render/Camera.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Camera.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/IconSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/IconSystem.h>), [WiNILL/Render/PlayerWeaponShell.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `algorithm`, `cmath`.

함수 후보: [CurrentPlayerShellKind:17](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp:17>), [DrawPlayerRotRect:22](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp:22>), [DrawPlayerLine:36](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp:36>), [DrawPlayerHexFrame:44](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp:44>), [DrawPlayerTriangleFrame:64](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp:64>), [DrawPlayerCrossStarFrame:84](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp:84>), [DrawPlayerZigzagLine:112](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp:112>), [DrawInGameSignalFrame:132](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp:132>), [DrawPlayerWeaponShell:139](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp:139>).

참조 `g_*`: `g_CurrentWeapon`, `g_MonsterManager`, `g_MuzzleTimer`.

이름 기반 호출 위치 41개. 간접 포함 의존 16파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/PlayerWeaponShell.h

플레이어 무기 표시 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Render/PlayerWeaponShell.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/PlayerWeaponShell.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 3파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/stb_impl.cpp

외부 stb 라이브러리 구현 매크로의 단일 번역 단위.

자체 include: 없음.

직접 피의존: 없음.

외부/표준 include: `stb_image.h`, `stb_truetype.h`.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/TextRenderer.h

폰트/glyph atlas·측정·줄바꿈 지원·일반/회전 텍스트·GL 수명.

자체 include: [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>).

직접 피의존: [WiNILL/Core/GameContext.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameContext.h>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Core/Scenes/SceneUI.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp>), [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>), [WiNILL/Render/GameFonts.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.cpp>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/System/DebugToolkit.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `algorithm`, `cmath`, `cstdint`, `cstdio`, `cstring`, `glad/glad.h`, `stb_truetype.h`, `string`, `unordered_map`, `vector`.

함수 후보: [TextRenderer:32](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:32>), [LineHeightPixels:36](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:36>), [BaselineOffset:38](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:38>), [SetMinScale:43](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:43>), [SetHalo:47](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:47>), [EffectiveScale:102](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:102>), [InitGLPipeline:108](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:108>), [Compile:174](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:174>), [FinishInit:183](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:183>), [InitFromFiles:197](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:197>), [InitFromMemory:232](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:232>), [FaceForCodepoint:251](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:251>), [GetGlyph:261](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:261>), [CreateAtlasPage:310](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:310>), [AllocateAtlasRect:325](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:325>), [Cleanup:356](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:356>), [Width:368](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:368>), [PreloadText:375](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:375>), [Height:382](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:382>), [SetHaloUniforms:388](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:388>), [Draw:403](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:403>), [DrawRotated:467](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:467>), [Draw:542](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h:542>).

참조 `g_*`: `g_BatchAlpha`, `g_GfxPass`.

이름 기반 호출 위치 24개. 간접 포함 의존 2파일, 간접 피의존 16파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/UiColors.h

UI 색/알파 정책.

자체 include: [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: [GetUiTheme:45](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h:45>), [SetUiTheme:49](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h:49>), [ClampUiAlpha:103](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h:103>), [PulsedAmbientAlpha:111](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h:111>), [PulsedStructuralAlpha:115](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h:115>), [drawRectCol:119](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h:119>), [drawRectCol3:123](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiColors.h:123>).

참조 `g_*`: `g_UiThemeMode`.

이름 기반 호출 위치 4개. 간접 포함 의존 2파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/Render/UiLayout.h

UI 레이아웃/보간 계산 도구.

자체 include: [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Core/Scenes/SceneUI.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneUI.cpp>), [WiNILL/Core/Scenes/WindowChrome.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/WindowChrome.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `algorithm`.

함수 후보: [UiScale:5](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h:5>), [BottomInset:13](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h:13>), [HudY:14](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h:14>), [BottomLeftActionX:17](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h:17>), [BottomLeftActionY:18](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h:18>), [Smoothstep:31](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h:31>), [SceneTransitionEase:38](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h:38>), [CenterX:43](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h:43>), [WinOrigin:46](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h:46>).

참조 `g_*`: `g_GameBarH`, `g_TaskbarH`.

이름 기반 호출 위치 2개. 간접 포함 의존 1파일, 간접 피의존 10파일. 상세 위치는 TSV에 기록.

### WiNILL/resource.h

Windows 내장 리소스 ID.

자체 include: 없음.

직접 피의존: 없음.

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/System/Audio.cpp

오디오 엔진/장치·BGM/SFX·설정 적용 상태.

자체 include: [WiNILL/System/Audio.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.h>), [WiNILL/System/EmbeddedResource.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/EmbeddedResource.h>).

직접 피의존: 없음.

외부/표준 include: `chrono`, `cstdlib`, `cstring`, `miniaudio.h`, `string`, `windows.h`.

함수 후보: [ResolveBase:30](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:30>), [FullPath:34](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:34>), [ReleaseSoundObjects:60](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:60>), [InitializeEngine:79](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:79>), [LoadSoundEffects:85](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:85>), [StartBgm:117](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:117>), [Init:163](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:163>), [Shutdown:206](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:206>), [PlaySfx:213](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:213>), [PlayBgmMain:239](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:239>), [StopBgm:241](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:241>), [SetEnabled:250](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:250>), [IsEnabled:260](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:260>), [SetBgmEnabled:262](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:262>), [SetSfxEnabled:267](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:267>), [SetMonoOutput:276](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:276>), [IsMonoOutput:306](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:306>), [SetVolume:308](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp:308>).

참조 `g_*`: `g_base`, `g_bgm`, `g_bgmActive`, `g_bgmDecoder`, `g_bgmDecoderActive`, `g_bgmEnabled`, `g_bgmPath`, `g_enabled`, `g_engine`, `g_inited`, `g_masterVolume`, `g_monoOutput`, `g_sfx`, `g_sfxEnabled`, `g_sfxLastMs`.

이름 기반 호출 위치 18개. 간접 포함 의존 2파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/System/Audio.h

오디오 재생/설정/수명 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/System/Audio.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 11파일. 상세 위치는 TSV에 기록.

### WiNILL/System/CrashHandler.h

예외/크래시 처리와 진단 경계.

자체 include: 없음.

직접 피의존: [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `cstdio`, `cstring`, `ctime`, `dbghelp.h`, `windows.h`.

함수 후보: [SetBreadcrumb:24](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/CrashHandler.h:24>), [writeLog:36](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/CrashHandler.h:36>), [writeDump:66](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/CrashHandler.h:66>), [Filter:84](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/CrashHandler.h:84>), [Install:95](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/CrashHandler.h:95>), [SetBreadcrumb:102](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/CrashHandler.h:102>), [Install:103](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/CrashHandler.h:103>).

참조 `g_*`: `g_Breadcrumb`.

이름 기반 호출 위치 4개. 간접 포함 의존 0파일, 간접 피의존 1파일. 상세 위치는 TSV에 기록.

### WiNILL/System/DebugToolkit.cpp

개발 패널·필드 편집·화면 캡처·테스트용 상태 조정.

자체 include: [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/TextRenderer.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/TextRenderer.h>), [WiNILL/System/DebugToolkit.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h>), [WiNILL/System/Input.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: 없음.

외부/표준 include: `algorithm`, `array`, `chrono`, `cmath`, `cstddef`, `cstdint`, `ctime`, `cwchar`, `filesystem`, `fstream`, `glad/glad.h`, `sstream`, `string`, `vector`.

함수 후보: [GetPanelLayout:66](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:66>), [Clamp01:72](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:72>), [SpawnGridStep:76](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:76>), [SpawnButtonHeight:82](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:82>), [FormatFloat:86](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:86>), [FormatInt:92](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:92>), [FormatLongLong:98](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:98>), [Crc32:104](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:104>), [Adler32:114](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:114>), [AppendU32BE:124](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:124>), [AppendPngChunk:131](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:131>), [SaveRgbaPng:142](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:142>), [Timestamp:202](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:202>), [LocalizedFieldName:222](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:222>), [Fields:314](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:314>), [CaptureStates:381](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:381>), [FieldTypeLabel:400](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:400>), [MobLabel:410](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:410>), [SceneLabel:420](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:420>), [Configure:439](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:439>), [ResetTransientInput:447](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:447>), [SetMessage:454](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:454>), [Hit:458](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:458>), [BeginFieldEdit:464](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:464>), [BeginRuntimeEdit:490](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:490>), [ApplyFieldEdit:502](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:502>), [HandleNumericKeys:593](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:593>), [BeginInput:653](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:653>), [DrawButton:877](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:877>), [RenderStatsTab:905](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:905>), [RenderSpawnTab:1014](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:1014>), [RenderCaptureTab:1061](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:1061>), [Render:1089](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:1089>), [RequestCurrentScreenshot:1121](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:1121>), [RequestNamedScreenshot:1127](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:1127>), [RequestAllScreenshots:1133](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:1133>), [CaptureRenderState:1141](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:1141>), [CaptureFrameAfterRender:1147](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp:1147>).

참조 `g_*`: `g_BatchAlpha`, `g_DebugMode`, `g_DebugToolkit`, `g_ScrollAccum`, `g_TextL`, `g_TextS`.

이름 기반 호출 위치 163개. 간접 포함 의존 17파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/System/DebugToolkit.h

디버그 도구 상태/호출 인터페이스.

자체 include: [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/Entity/MonsterManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/MonsterManager.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/System/FakeWindow.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/FakeWindow.h>).

직접 피의존: [WiNILL/System/DebugToolkit.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp>), [WiNILL/System/SmokeCapture.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `GLFW/glfw3.h`, `functional`, `glad/glad.h`, `string`, `vector`.

함수 후보: [IsVisible:47](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h:47>), [SetVisible:48](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h:48>), [IsInputCaptured:52](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h:52>), [SuppressOverlayForCapture:53](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h:53>), [CaptureInProgress:62](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h:62>), [LastMessage:66](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h:66>).

참조 `g_*`: `g_DebugToolkit`.

이름 기반 호출 위치 1개. 간접 포함 의존 13파일, 간접 피의존 3파일. 상세 위치는 TSV에 기록.

### WiNILL/System/EmbeddedResource.h

Windows 내장 리소스의 공통 읽기 helper.

자체 include: 없음.

직접 피의존: [WiNILL/Render/GameFonts.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.cpp>), [WiNILL/Render/MainShader.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/MainShader.cpp>), [WiNILL/System/Audio.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Audio.cpp>).

외부/표준 include: `cstddef`, `windows.h`.

함수 후보: [valid:18](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/EmbeddedResource.h:18>), [LoadEmbeddedResource:21](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/EmbeddedResource.h:21>).

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 3파일. 상세 위치는 TSV에 기록.

### WiNILL/System/FakeWindow.cpp

플레이어 가상 창 이동/충돌·위치/크기 조정.

자체 include: [WiNILL/System/FakeWindow.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/FakeWindow.h>).

직접 피의존: 없음.

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 1파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/System/FakeWindow.h

가상 창 bounds/인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Entity/ProjectileSystem.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ProjectileSystem.cpp>), [WiNILL/System/DebugToolkit.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h>), [WiNILL/System/FakeWindow.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/FakeWindow.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 6파일. 상세 위치는 TSV에 기록.

### WiNILL/System/GameplayTelemetry.h

런/증강 스탯 snapshot·차이·이벤트 기록.

자체 include: [WiNILL/Entity/ExpSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/ExpSystem.h>), [WiNILL/Entity/PlayerStats.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/PlayerStats.h>), [WiNILL/System/Platform.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h>).

직접 피의존: [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `algorithm`, `cmath`, `ctime`, `fstream`, `iomanip`, `sstream`, `string`.

함수 후보: [SetSampleIntervalSeconds:67](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:67>), [SampleIntervalSeconds:72](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:72>), [IsActive:73](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:73>), [Capture:75](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:75>), [BeginRun:118](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:118>), [RecordExperience:159](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:159>), [RecordLevelUp:164](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:164>), [RecordAugment:196](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:196>), [Update:251](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:251>), [EndRun:270](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:270>), [OutputFilePath:294](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:294>), [BuildName:301](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:301>), [Timestamp:309](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:309>), [RequiredForLevel:322](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:322>), [WriteSample:327](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:327>), [WriteFloatDiff:359](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:359>), [WriteIntDiff:366](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:366>), [WriteBoolDiff:373](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h:373>).

참조 `g_*`: `g_ExpSystem`.

이름 기반 호출 위치 53개. 간접 포함 의존 6파일, 간접 피의존 1파일. 상세 위치는 TSV에 기록.

### WiNILL/System/Input.cpp

키/마우스/텍스트 callback·Windows IME·화면별 입력 편집.

자체 include: [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameManager.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.h>), [WiNILL/System/Input.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `GLFW/glfw3native.h`, `algorithm`, `cmath`, `cstring`, `cwctype`, `glad/glad.h`, `imm.h`, `string`, `vector`, `windows.h`.

함수 후보: [CodexTextInputActive:45](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:45>), [ApplyCodexImeAnchor:51](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:51>), [ReadImeString:67](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:67>), [ReadCodexImeComposition:78](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:78>), [InputWindowProc:102](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:102>), [InputUpdateTextInput:126](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:126>), [InputSetCodexImeAnchor:144](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:144>), [CodexWideToUtf8:163](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:163>), [CodexUtf8ToWide:174](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:174>), [CodexCheckDevPhrase:185](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:185>), [CodexCopySelection:196](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:196>), [CodexPaste:203](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:203>), [InputKeyCallback:209](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:209>), [InputCharCallback:239](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:239>), [InputScrollCallback:253](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:253>), [InputRegisterCallbacks:257](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:257>), [InputUnregisterCallbacks:276](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:276>), [InputClearState:290](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:290>), [InputUpdateWindowFocus:296](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:296>), [InputUpdateGameplayCursor:306](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp:306>).

참조 `g_*`: `g_CodexSearch`, `g_CodexSearchCompositionLen`, `g_CodexSearchInputEnabled`, `g_CodexSearchLen`, `g_DevToastTimer`, `g_DevUnlocked`, `g_GameManager`, `g_ScrollAccum`, `g_VolBuf`, `g_VolEdit`, `g_VolLen`.

이름 기반 호출 위치 43개. 간접 포함 의존 14파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/System/Input.h

입력 상태/callback·IME 관련 인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/System/DebugToolkit.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp>), [WiNILL/System/Input.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Input.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: `g_ScrollAccum`.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 4파일. 상세 위치는 TSV에 기록.

### WiNILL/System/Platform.h

플랫폼별 창/타이머/OS 기능 분기.

자체 include: 없음.

직접 피의존: [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Render/Juice.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/Juice.h>), [WiNILL/System/GameplayTelemetry.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/GameplayTelemetry.h>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `chrono`, `cstdint`, `cstdio`, `ctime`, `cwchar`, `direct.h`, `libgen.h`, `mach-o/dyld.h`, `string`, `sys/stat.h`, `thread`, `timeapi.h`, `unistd.h`, `windows.h`.

함수 후보: [wcscpy_s:34](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h:34>), [wcsncpy_s:39](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h:39>), [localtime_s:49](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h:49>), [PlatformTimerBegin:54](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h:54>), [PlatformTimerEnd:59](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h:59>), [PlatformSleepMs:64](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h:64>), [PlatformPathIsDir:73](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h:73>), [PlatformExeDirectory:84](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h:84>), [PlatformChdirToExeDir:113](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Platform.h:113>).

참조 `g_*`: 없음.

이름 기반 호출 위치 3개. 간접 포함 의존 0파일, 간접 피의존 51파일. 상세 위치는 TSV에 기록.

### WiNILL/System/SaveSystem.h

설정·진행 저장/로드·초기화·런 결과 반영.

자체 include: [WiNILL/Core/Achievements.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Achievements.h>), [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `cstdio`, `cstdlib`, `cstring`, `filesystem`, `string`, `system_error`.

함수 후보: [OdwCrypt:20](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h:20>), [SaveFilePath:42](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h:42>), [SaveGame:44](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h:44>), [LoadGame:127](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h:127>), [ResetSaveProgress:242](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h:242>), [RecordRunResult:264](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h:264>).

참조 `g_*`: `g_AchUnlocked`, `g_AudioEngineEnabled`, `g_AudioMonoOutput`, `g_AugSeen`, `g_AutoFire`, `g_AutoSkill`, `g_BackdropBlurCaptureHz`, `g_BackdropBlurEnabled`, `g_BestScore`, `g_BgmEnabled`, `g_Coins`, `g_DebugMode`, `g_FpsCap`, `g_JobBought`, `g_Language`, `g_LastRunCoins`, `g_MacOptV1`, `g_MetaLv`, `g_MobKillCounts`, `g_MobSeen`, `g_MobVisualStyle`, `g_RunMobKillCounts`, `g_RunStardust`, `g_SfxEnabled`, `g_ShaderFx`, `g_ShowCombo`, `g_ShowCrosshair`, `g_ShowDamageNumbers`, `g_SmokeCapture`, `g_SoundVol`, `g_StrongMenuDim`, `g_ThemeOwned`, `g_ThemeSel`, `g_TotalGames`, `g_TotalKills`, `g_VfxDensity`, `g_WeaponBestKills`, `g_WeaponBestScore`, `g_WeaponRunCount`, `g_WeaponTotalKills`.

이름 기반 호출 위치 8개. 간접 포함 의존 13파일, 간접 피의존 8파일. 상세 위치는 TSV에 기록.

### WiNILL/System/Settings.h

옵션 기본값·언어·시련 계산·여러 공용 게임 설정 상태.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Codex.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Codex.h>), [WiNILL/Core/GameManager.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/GameManager.cpp>), [WiNILL/Core/Meta.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Meta.h>), [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Core/TutorialText.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/TutorialText.h>), [WiNILL/Entity/Augment.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/Augment.h>), [WiNILL/Entity/EnemySpawner.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/EnemySpawner.cpp>), [WiNILL/Entity/RangedMob.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Entity/RangedMob.h>), [WiNILL/Render/DrawPrim.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/DrawPrim.h>), [WiNILL/Render/EnemyParticles.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EnemyParticles.h>), [WiNILL/Render/EntityDraw.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/EntityDraw.cpp>), [WiNILL/Render/UiLayout.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/UiLayout.h>), [WiNILL/System/DebugToolkit.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.cpp>), [WiNILL/System/SaveSystem.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SaveSystem.h>), [WiNILL/System/SmokeCapture.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h>), [WiNILL/System/Translations.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h>), [WiNILL/System/WindowFx.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: [LangIndex:24](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:24>), [LanguageFace:52](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:52>), [LanguageFontChain:57](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:57>), [GetDifficultyParams:73](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:73>), [TrialDefUnimplemented:119](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:119>), [TrialDefRetired:123](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:123>), [TrialStageForDef:135](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:135>), [TrialScoreBonusForDef:143](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:143>), [TrialActive:158](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:158>), [TrialCount:167](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:167>), [TrialScoreMult:174](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:174>), [ResetTrials:183](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:183>), [TrialPlayerMaxHpMult:193](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:193>), [TrialSpawnRateMult:199](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:199>), [TrialEnemyHpMult:208](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:208>), [TrialRangedIntervalMult:216](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:216>), [TrialRangedMaxBonus:224](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:224>), [CombatKeylineStrength:286](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:286>), [BackdropBlurCaptureIntervalSeconds:290](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:290>), [VfxIntervalMult:308](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:308>), [EnemyParticleCap:312](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:312>), [GfxCircleSegs:321](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:321>), [GfxArcSegs:330](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h:330>).

참조 `g_*`: `g_AccentB`, `g_AccentG`, `g_AccentR`, `g_ArenaExX`, `g_ArenaExY`, `g_AudioEngineEnabled`, `g_AudioMonoOutput`, `g_AutoFire`, `g_AutoSkill`, `g_BackdropBlurCaptureHz`, `g_BackdropBlurEnabled`, `g_BalanceTestMode`, `g_BgmEnabled`, `g_CombatKeylineScale`, `g_CreativeFreeGrab`, `g_CreativeGodmode`, `g_CreativeMode`, `g_CreativeStartScore`, `g_DebugMode`, `g_Difficulty`, `g_FpsCap`, `g_GameBarH`, `g_Language`, `g_MacOptV1`, `g_MobVisualStyle`, `g_Scale`, `g_SfxEnabled`, `g_ShaderFx`, `g_ShowCombo`, `g_ShowCrosshair`, `g_ShowDamageNumbers`, `g_SmokeAugFocus`, `g_SmokeCapture`, `g_SmokeTimeScale`, `g_SoundVol`, `g_StrongMenuDim`, `g_TaskbarH`, `g_ThemeOwned`, `g_ThemeSel`, `g_TrialPool`, `g_TrialPoolReady`, `g_TrialSelected`, `g_VfxDensity`.

이름 기반 호출 위치 19개. 간접 포함 의존 0파일, 간접 피의존 68파일. 상세 위치는 TSV에 기록.

### WiNILL/System/SmokeCapture.h

자동 UI 스모크 캡처 설정/경로.

자체 include: [WiNILL/System/DebugToolkit.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/DebugToolkit.h>), [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: `GLFW/glfw3.h`, `cstdio`, `cstdlib`, `cstring`, `functional`, `string`, `vector`.

함수 후보: [SmokeEnv:30](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h:30>), [Init:56](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h:56>), [Active:83](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h:83>), [Width:85](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h:85>), [Height:86](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h:86>), [Frame:90](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h:90>), [ParseSteps:143](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h:143>), [FileName:182](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SmokeCapture.h:182>).

참조 `g_*`: `g_BackdropBlurEnabled`, `g_CombatKeylineScale`, `g_DebugMode`, `g_DebugToolkit`, `g_Language`, `g_SmokeCapture`, `g_SmokeRun`.

이름 기반 호출 위치 13개. 간접 포함 의존 14파일, 간접 피의존 1파일. 상세 위치는 TSV에 기록.

### WiNILL/System/SystemInfo.h

시스템/환경 정보 수집.

자체 include: 없음.

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>).

외부/표준 include: `cwchar`, `string`, `windows.h`.

함수 후보: [GetSystemSpecLine1:12](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SystemInfo.h:12>), [GetSystemSpecLine2:44](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/SystemInfo.h:44>).

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 7파일. 상세 위치는 TSV에 기록.

### WiNILL/System/Translations.h

언어별 공용 번역 데이터/선택.

자체 include: [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>).

직접 피의존: [WiNILL/Core/Scenes/SceneAugment.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneAugment.cpp>), [WiNILL/Core/Scenes/SceneMenus.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneMenus.cpp>), [WiNILL/Core/Scenes/ScenePause.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/ScenePause.cpp>), [WiNILL/Core/Scenes/SceneRunConfig.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunConfig.cpp>), [WiNILL/Core/Scenes/SceneRunEnd.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneRunEnd.cpp>), [WiNILL/Core/Scenes/SceneSettings.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneSettings.cpp>), [WiNILL/Core/Scenes/SceneStart.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Core/Scenes/SceneStart.cpp>), [WiNILL/Render/GameFonts.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Render/GameFonts.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: [T:232](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Translations.h:232>).

참조 `g_*`: `g_Language`.

이름 기반 호출 위치 0개. 간접 포함 의존 1파일, 간접 피의존 9파일. 상세 위치는 TSV에 기록.

### WiNILL/System/WindowFx.cpp

창/화면 효과 갱신과 적용.

자체 include: [WiNILL/System/Settings.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/Settings.h>), [WiNILL/System/WindowFx.h](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.h>).

직접 피의존: 없음.

외부/표준 include: `GLFW/glfw3.h`, `GLFW/glfw3native.h`, `cstdarg`, `cstdio`, `dwmapi.h`, `shellapi.h`, `shobjidl.h`, `windows.h`.

함수 후보: [TransparencyLog:7](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:7>), [FxText:23](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:23>), [MarkShellFullscreen:38](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:38>), [UpdateWindowTaskbarPolicy:56](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:56>), [ReleaseWindowTaskbarPolicy:77](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:77>), [EnableWindowTransparency:81](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:81>), [ReadTransparencySetting:136](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:136>), [NotifyTransparencyChanged:144](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:144>), [Restore:156](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:156>), [TemporaryTransparency:181](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:181>), [EnableSystemTransparency:186](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:186>), [OpenBlurSettings:215](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:215>), [RefreshBlurEnvironment:232](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:232>), [ResolveBackdropProc:297](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:297>), [ConfigureWindowBackdropBlur:307](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:307>), [UpdateWindowTaskbarPolicy:375](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:375>), [ReleaseWindowTaskbarPolicy:376](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:376>), [EnableWindowTransparency:380](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:380>), [ConfigureWindowBackdropBlur:384](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp:384>).

참조 `g_*`: 없음.

이름 기반 호출 위치 28개. 간접 포함 의존 2파일, 간접 피의존 0파일. 상세 위치는 TSV에 기록.

### WiNILL/System/WindowFx.h

창 효과 상태/인터페이스.

자체 include: 없음.

직접 피의존: [WiNILL/System/WindowFx.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/System/WindowFx.cpp>), [WiNILL/main.cpp](<D:/01_Study/02_OpenGL/Onedow/WiNILL/main.cpp>).

외부/표준 include: 없음.

함수 후보: 없음/검출 범위 밖.

참조 `g_*`: 없음.

이름 기반 호출 위치 0개. 간접 포함 의존 0파일, 간접 피의존 2파일. 상세 위치는 TSV에 기록.

## 3. 빌드·리소스·스크립트 전체 목록

| 파일 | 줄 | 책임/검토 |
|---|---:|---|
| [.github/workflows/macos-build.yml](<D:/01_Study/02_OpenGL/Onedow/.github/workflows/macos-build.yml>) | 76 | 빌드·패키징·릴리스 설정; D40 |
| [CMakeLists.txt](<D:/01_Study/02_OpenGL/Onedow/CMakeLists.txt>) | 103 | 빌드·패키징·릴리스 설정; D40 |
| [glad.c](<D:/01_Study/02_OpenGL/Onedow/glad.c>) | 1140 | 생성된 외부 OpenGL loader; 변경/중복 통합 제외 |
| [scripts/build_macos.sh](<D:/01_Study/02_OpenGL/Onedow/scripts/build_macos.sh>) | 30 | 빌드·패키징·릴리스 설정; D40 |
| [scripts/build_windows.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/build_windows.ps1>) | 19 | 빌드·패키징·릴리스 설정; D40 |
| [scripts/package_lts.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/package_lts.ps1>) | 32 | 빌드·패키징·릴리스 설정; D40 |
| [scripts/package_macos.sh](<D:/01_Study/02_OpenGL/Onedow/scripts/package_macos.sh>) | 22 | 빌드·패키징·릴리스 설정; D40 |
| [scripts/package_windows.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/package_windows.ps1>) | 3 | 빌드·패키징·릴리스 설정; D40 |
| [scripts/release_macos.sh](<D:/01_Study/02_OpenGL/Onedow/scripts/release_macos.sh>) | 4 | 빌드·패키징·릴리스 설정; D40 |
| [scripts/release_windows.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/release_windows.ps1>) | 4 | 빌드·패키징·릴리스 설정; D40 |
| [scripts/smoke_ui.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/smoke_ui.ps1>) | 38 | 빌드·패키징·릴리스 설정; D40 |
| [scripts/test_blur_windows.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/test_blur_windows.ps1>) | 18 | 테스트 빌드/실행; D39 |
| [scripts/test_genesis_launcher.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/test_genesis_launcher.ps1>) | 19 | 테스트 빌드/실행; D39 |
| [scripts/test_magnetar_field.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/test_magnetar_field.ps1>) | 19 | 테스트 빌드/실행; D39 |
| [scripts/test_mob_death_fx.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/test_mob_death_fx.ps1>) | 16 | 테스트 빌드/실행; D39 |
| [scripts/test_quasar_beam.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/test_quasar_beam.ps1>) | 19 | 테스트 빌드/실행; D39 |
| [scripts/test_quasar_nova.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/test_quasar_nova.ps1>) | 19 | 테스트 빌드/실행; D39 |
| [scripts/test_regulus_beam.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/test_regulus_beam.ps1>) | 19 | 테스트 빌드/실행; D39 |
| [scripts/zip_onedow.ps1](<D:/01_Study/02_OpenGL/Onedow/scripts/zip_onedow.ps1>) | 44 | 빌드·패키징·릴리스 설정; D40 |
| [scripts/zip_onedow.sh](<D:/01_Study/02_OpenGL/Onedow/scripts/zip_onedow.sh>) | 15 | 빌드·패키징·릴리스 설정; D40 |
| [WiNILL/Resource.rc](<D:/01_Study/02_OpenGL/Onedow/WiNILL/Resource.rc>) | 87 | 리소스/IDE 메타데이터; D40 |
| [WiNILL/WiNILL.vcxproj](<D:/01_Study/02_OpenGL/Onedow/WiNILL/WiNILL.vcxproj>) | 289 | 빌드·패키징·릴리스 설정; D40 |
| [WiNILL/WiNILL.vcxproj.filters](<D:/01_Study/02_OpenGL/Onedow/WiNILL/WiNILL.vcxproj.filters>) | 261 | 리소스/IDE 메타데이터; D40 |
| [WiNILL.sln](<D:/01_Study/02_OpenGL/Onedow/WiNILL.sln>) | 31 | 빌드·패키징·릴리스 설정; D40 |

## 4. 외부 경계

외부 코드/헤더 파일 목록과 해시는 `external_boundaries.tsv`에 별도 기록했다. 외부 내부 구현의 중복 제거는 이번 범위가 아니다. 소스가 직접 포함한 외부 이름은 위 파일별 include와 includes.tsv에서 확인할 수 있다.

| 경계 | 파일 수 |
|---|---:|
| GLFW | 2 |
| Include/stb_image.h | 1 |
| Include/stb_truetype.h | 1 |
| KHR | 1 |
| WiNILL/System/miniaudio.h | 1 |
| glad | 1 |
| glad.c | 1 |
| glm | 433 |

## 5. 자료 교차 확인

- 소스 표 105행, 함수 후보 831행, 호출 위치 3739행, 보조 파일 24행.
- 모든 소스 파일에 수동 책임 분류가 있으며 모든 함수 후보를 파일별 목록에 연결했다.
- 본문의 D01~D40과 자동 후보 판정표를 함께 읽어야 한다. 식별자 정규화 후보 수를 실제 중복 규칙 수로 사용하지 않는다.
- 조사 시점 원본 무변경 검증은 verification.json에 기록한다. 빌드/실행 검증과 구분한다.
