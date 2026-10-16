# 점검표 작업 기록

[Review_Checklist.md](Review_Checklist.md) 6장의 단위별 기록. 항목은 증거가 모두 갖춰졌을 때만 완료로 표시한다.

## FIX-01 — macOS CI 빌드 복구 (진행 중, 미완료)

| 항목 | 내용 |
|---|---|
| 기준 | HEAD `abb8143` + 미커밋 로컬 변경. FIX-01 파일은 커밋하지 않음(사용자 결정, 2026-10-13) |
| 변경 파일 (sha256) | `WiNILL/System/Platform.h` 3afa697b…85e7<br>`WiNILL/System/Input.cpp` 16bf5f43…b3de<br>`tests/platform_format_test.cpp` 8420d0b0…1e5c (신규)<br>`scripts/test_platform_format.ps1` 7cf9d051…29d9 (신규) |
| 분리 | 위 4개 파일에는 FIX-01 변경만 있음. 작업 트리의 다른 미커밋 변경(몹 연출·Quasar 빔 등)과 파일 단위로 겹치지 않아 별도 커밋 가능 |
| 줄바꿈 | 저장소는 `.gitattributes`(`text=auto`)로 LF 정규화(`git ls-files --eol`: i/lf) — 작업 파일의 CRLF/LF와 무관하게 커밋 내용 동일. (2026-10-13 정정: 앞선 'CRLF 유지' 판단은 Git Bash `grep -c $'\r'`가 모든 줄을 세는 잘못된 검사에 근거했음) |

### 변경 내용
- `Platform.h`: 비Windows `swprintf_s` 매크로(`sizeof(buf)`로 크기 추론)를 `PortableWideFormat::swprintf_s` 두 오버로드로 교체. 배열 호출은 크기를 추론하고, 포인터+명시 크기 호출은 그 크기를 그대로 쓴다. 넘침 시 -1 반환 + 끝 NUL 보장.
- `Input.cpp`: Win32 IME(HWND/HIMC, WndProc 서브클래싱)와 `windows.h`/`imm.h`를 `#ifdef _WIN32`로 분리. 비Windows는 `InputUpdateTextInput`/`InputSetCodexImeAnchor`를 빈 구현으로 두고, 클립보드 UTF-8 ↔ UTF-16 변환을 `std::wstring_convert<codecvt_utf8_utf16<wchar_t>>`로 수행(도감 검색 버퍼는 모든 플랫폼에서 UTF-16 단위로 저장 — `CodexSearchInsertCodepoint`). macOS는 Cmd(SUPER)도 복사/붙여넣기/전체선택 수정키로 인정.

### 검증 결과 (2026-10-13)
| 검증 | 명령/방법 | 결과 |
|---|---|---|
| 두 호출 형태·짧은 버퍼 | `scripts/test_platform_format.ps1` (MSVC로 `PortableWideFormat` 직접 컴파일) | 통과 |
| 기존 회귀 테스트 7종 | `scripts/test_{quasar_nova,mob_death_fx,quasar_beam,genesis_launcher,magnetar_field,regulus_beam,blur_windows}.ps1` | 모두 통과 |
| Windows Release 빌드 | `scripts/build_windows.ps1` | 통과 (기존 APIENTRY 재정의 경고만) |
| 비Windows 경로 문법 검사 (로컬 대용) | VS 동봉 `clang-tidy` 19.1.5의 clang 프런트엔드로 CMake 등록 31개 소스를 `-U_WIN32 -D__APPLE__`로 파싱. POSIX 전용 헤더(unistd/libgen/mach-o)는 선언 stub 사용 | 수정 전 `Platform.h`: CI와 같은 종류의 `swprintf` 오류 11건 재현(main.cpp). 수정 후: `swprintf` 관련 0건. 남은 오류는 모두 검사 환경 부산물 — UCRT의 `localtime_s`·`wcscpy_s` 선언 충돌, `_MSC_VER` 블록의 `DWORD`, miniaudio의 `pthread.h` 부재 |
| 넓은 포맷 `%s` 이식성 | `L"…%s"` 검색 | 0건. 문자열 인자 33곳 모두 `%ls` 사용(macOS에서 출력 깨짐 없음) |
| 게임 실행·입력 경로 스모크 | `smoke_ui.ps1 -Steps "MENU;CODEX;SEARCH=quasar;SEARCH=" -Langs KR,JP -Tag fix01_input` | KR/JP 모두 정상 실행·검색 결과 1건 표시·정상 종료. 캡처 `build/smoke/fix01_input/` |

### 남은 완료 조건
1. **macOS CI (필수)**: 최신 변경 SHA에서 Configure/build/Verify binary 통과. 로컬 검사는 MSVC 헤더 위의 대용이라 libc++·macOS SDK 차이(예: `wstring_convert` 사용 중단 경고, 32비트 `wchar_t`)를 보장하지 못한다. 커밋·push 후 실행 필요.
2. **Windows KR/JP IME 실제 조합 입력 (수동)**: 도감 검색에서 한글·일본어 조합 입력, 후보창 위치, 포커스 이탈/복귀. IME 코드는 내용 변경 없이 `#ifdef` 안으로 이동했지만 자동 스모크(`SEARCH=`)는 IME를 거치지 않는다.
3. **macOS 텍스트 입력 (잔여 위험)**: Mac 실기가 없으면 확인 불가. 빌드 통과 후 잔여 위험으로 기록.
4. **규칙 확인**: macOS에서 Cmd+C/V/A를 Ctrl과 같이 처리하는 입력 UX 변경 — 사용자 확인 필요(점검표 "입력 UX 변경 시 규칙").

검토자: 미지정.

## FIX-02 — 텍스트 렌더러 GL 자원 수명 (완료 조건 충족, 검토 대기)

| 항목 | 내용 |
|---|---|
| 기준 | HEAD `abb8143` + 미커밋 로컬 변경. 커밋하지 않음 |
| 변경 파일 (sha256) | `WiNILL/Render/TextRenderer.h` 1cd7b9b4…79c5<br>`WiNILL/Render/GameFonts.h` 57d0c0ef…f715<br>`WiNILL/Render/GameFonts.cpp` 2f75baee…a6<br>`WiNILL/main.cpp` 2c2e2c9e…686c<br>`tests/text_shutdown_test.cpp` 16fbc632…99ac (신규)<br>`scripts/test_text_shutdown.ps1` f63be940…c7 (신규) |
| 분리 | `TextRenderer.h`·`GameFonts.*`·신규 2개는 FIX-02 변경만 있음. `main.cpp`에는 다른 미커밋 변경(엘리트 처치 주석, `g_MagnetarFieldRipple` 제거)이 함께 있어 커밋 시 hunk 단위 분리 필요 — FIX-02는 종료부의 `ShutdownGameFonts()` 1개 hunk |
| 줄바꿈 | 저장소는 `.gitattributes`(`text=auto`)로 LF 정규화(`git ls-files --eol`: i/lf) — 작업 파일의 CRLF/LF와 무관하게 커밋 내용 동일. (2026-10-13 정정: 앞선 'CRLF 유지' 판단은 Git Bash `grep -c $'\r'`가 모든 줄을 세는 잘못된 검사에 근거했음) |

### 조사 결과
- `TextRenderer` 인스턴스는 `main.cpp`의 전역 `g_TextL`/`g_TextS`/`g_TextXL` 3개뿐. 초기화는 모두 `InitializeGameFonts`(메모리 폰트, 실패 시 디스크 폰트 체인).
- 종료부는 `g_TextL`/`g_TextS`만 `Cleanup()` 후 `glfwTerminate()`. `g_TextXL`은 정리되지 않아 전역 소멸자(`~TextRenderer` → `Cleanup`)가 컨텍스트가 사라진 뒤 `glDeleteTextures`/`VertexArrays`/`Buffers`/`Program`을 호출했다.
- 다른 종료 경로(`exit`/`abort`)는 `main.cpp`에 없음. `Cleanup()`은 핸들을 0으로 만들고 atlas를 비우므로 두 번 호출해도 GL 호출이 없다.

### 변경 내용
- `GameFonts`: `InitializeGameFonts`의 짝으로 `ShutdownGameFonts()` 추가 — 세 폰트를 모두 정리. 초기화와 정리의 소유자가 같아 폰트가 늘어도 누락되지 않는다.
- `main.cpp`: 종료부의 `g_TextL/S.Cleanup()` 두 줄을 `ShutdownGameFonts()`로 교체(`glfwTerminate` 이전 위치 유지).
- `TextRenderer`: 검증용 읽기 전용 `HasGlResources()` 추가, `Cleanup`의 컨텍스트 전제를 주석으로 명시. 동작 변경 없음.

### 검증 결과 (2026-10-13)
| 검증 | 명령/방법 | 결과 |
|---|---|---|
| 실제 GL 회귀 테스트 | `scripts/test_text_shutdown.ps1` — 숨김 GLFW 창(3.3 core)에서 `InitializeGameFonts` → `ShutdownGameFonts` 2회 → `glfwTerminate` → glad의 GL delete 포인터를 호출 카운터로 교체 → 전역 소멸자와 같은 `Cleanup` 실행 | 통과: 세 폰트 모두 해제, 재호출 안전, 종료 후 GL 호출 0회 |
| 검출기 대조군 | 같은 테스트 안에서 정리하지 않은 렌더러를 종료 후 `Cleanup` | GL delete 4회 감지(검출기가 실제로 작동함) |
| 수정 전 동작 재현 | 테스트 복사본에서 종료 순서를 기존 `main`(L/S만 정리)으로 되돌려 실행(스크래치, 저장소 밖) | `FAIL: released fonts make no GL calls after glfwTerminate` — 결함 재현 |
| Windows Release 빌드 | `scripts/build_windows.ps1` | 통과 |
| 전체 테스트 9종 | text_shutdown, platform_format 및 기존 7종 | 모두 통과 |
| 초기화 후 종료 스모크 | `ONEDOW_SMOKE="MENU~1.0"`, `"MENU~0.8;RUN~1.5;PAUSED~0.5"`로 Release 실행, 프로세스 종료 코드 확인 | 두 경우 모두 정상 종료, 종료 코드 0, 새 크래시 로그 없음 |

### 잔여 위험 / 별도 항목 후보
- **같은 유형 결함 (FIX-02 범위 밖)**: 전역 `g_GameManager`의 소멸자(`GameManager.cpp` `~GameManager`)도 `glfwTerminate` 이후 VAO/VBO/EBO/overlay/셰이더를 삭제한다. FIX-02 범위(`TextRenderer`)에 포함하지 않았으며, 새 FIX 항목으로 등록해 같은 방식(컨텍스트 유효 구간의 명시 정리 + 같은 회귀 테스트 틀)으로 처리할 것을 권고.
- 실제 크래시 재현은 하지 않음(드라이버에 따라 컨텍스트 없는 GL 호출은 무시되거나 실패). 판정 근거는 호출 순서와 카운터 테스트.

검토자: 미지정.

## FIX-03 — 패키징 포함 내용 계약 (로컬 완료 조건 충족, macOS CI 확인 대기)

| 항목 | 내용 |
|---|---|
| 기준 | HEAD `abb8143` + 미커밋 로컬 변경. 커밋하지 않음 |
| 변경 파일 (sha256 앞8…뒤4) | `scripts/package_manifest.txt` 2fb06b95…17ae (신규)<br>`scripts/verify_package_zip.py` ac015ff8…6451 (신규)<br>`scripts/zip_onedow.ps1` e44a1dbe…da94<br>`scripts/zip_onedow.sh` 99097851…8b3b<br>`scripts/release_macos.sh` 05f4261d…edf0<br>`scripts/package_macos.sh` 7711e0a3…8426<br>`.github/workflows/macos-build.yml` 0d9eaf3a…db75<br>`scripts/test_package_zip.ps1` 75de1947…9d1b (신규) |
| 분리 | 모든 파일이 FIX-03 변경만 포함 — 파일 단위로 별도 커밋 가능 |

### 조사 결과
- ZIP 생성 경로 3개: `release_windows.ps1`→`zip_onedow.ps1`(허용 목록 staging + `Compress-Archive`), `release_macos.sh`→`zip_onedow.sh`(`bin/Onedow` 전체 `zip -r`), macOS CI Package 단계(같은 전체 `zip -r`을 인라인으로).
- 실제 `bin/Onedow`에 배포 대상이 아닌 파일 존재: `onedow_save.cfg`(루트·`WindowsOS/` 2개), `WindowsOS/Onedow.pdb/.lib/.exp`, 오래된 `WindowsOS/Resource/`. 게임이 실행 위치에 쓰는 사용자 파일: 세이브, `onedow_crash.log`/`*.dmp`, `Screenshots/`, `onedow_gameplay_telemetry.txt`.
- `Compress-Archive`는 Unix 권한을 기록하지 못해 Windows에서 만든 ZIP의 `macOS/Onedow`가 실행 불가로 풀린다.
- 추가 발견: `scripts/*.sh` 4개가 git에 `100644`(실행 비트 없음)로 저장되어 있어, `release_macos.sh`/`package_macos.sh`의 `"…/x.sh"` 직접 실행은 Mac에서 권한 오류가 난다.

### 변경 내용
- `scripts/package_manifest.txt`: ZIP 포함 목록의 단일 기준(README.md·Resource/ 필수, WindowsOS/Onedow.exe·macOS/Onedow(exec)·macOS/README.txt 선택). 목록 밖 파일은 포함되지 않음.
- `zip_onedow.ps1`: 목록 기반 staging, 압축은 `System32\tar.exe`(bsdtar)+mtree로 Unix 권한 기록(`macOS/Onedow` 0755, 나머지 0644, 폴더 0755). 검증용 선택 인자 `-Source`/`-Output`(기본값은 기존과 동일).
- `zip_onedow.sh`: 같은 목록으로 임시 폴더 staging 후 `zip -r`, exec 항목 `chmod 755`. 선택 인자 `[원본] [출력]`. Mac 바이너리 필수 검사 추가.
- macOS CI: Package 단계가 `bash scripts/zip_onedow.sh`를 사용, 새 `Verify package` 단계에서 `verify_package_zip.py --require macOS/Onedow`와 `zipinfo`로 내용·실행 권한 확인.
- `release_macos.sh`/`package_macos.sh`: 하위 스크립트를 `bash`로 실행(실행 비트와 무관).
- `scripts/verify_package_zip.py`: 목록 밖 파일, 필수 누락, exec 항목 권한(Unix host + 실행 비트)을 검사.

### 검증 결과 (2026-10-13)
| 검증 | 명령/방법 | 결과 |
|---|---|---|
| 임시 fixture 회귀 | `scripts/test_package_zip.ps1` — `build/tests/package`에 실제 Resource + 더미 바이너리 + 사용자/빌드 파일 11종(세이브 2, pdb/lib/exp, stale Resource, Screenshots, crash log/dmp, telemetry, .DS_Store) | 통과: 두 스크립트가 같은 76개 파일, 11종 모두 제외, PS ZIP의 `macOS/Onedow` host=Unix·0755 |
| shell 경로 | 위 테스트에서 Git Bash로 `zip_onedow.sh` 실행. Info-ZIP `zip`이 없어 테스트 전용 대체 도구 사용 | 내용 통과. 권한은 대체 도구로 판단 불가 → CI `Verify package`가 확인 |
| 수정 전 재현 | HEAD 스크립트를 같은 fixture에 실행(`build/tests/package/oldroot`) | PS: `not executable: macOS/Onedow (host=0, mode=0o0)`. sh: 11종 전부 포함 → 결함 재현 |
| 실제 `bin/Onedow` | `zip_onedow.ps1 -Output build/tests/package/out/real_bin.zip` (기존 `bin/Onedow.zip`은 덮어쓰지 않음) | 통과: 기존 릴리스 ZIP과 파일 75개 동일, 세이브·pdb·stale Resource 제외 |
| 문법 | `bash -n` (sh 3개), CI YAML 단계 구성 육안 확인 | 이상 없음 |

### 남은 완료 조건 / 결정
1. **macOS CI**: push 후 `Verify package` 단계 통과(실제 Info-ZIP 권한 확인). FIX-01과 함께 확인 가능.
2. **배포 포함 정책(사용자 결정)**: 현재 목록 = README.md, Resource/, WindowsOS/Onedow.exe, macOS/Onedow, macOS/README.txt. 다른 파일(예: 라이선스 문서)을 배포에 넣으려면 목록에 추가해야 한다.
3. Windows ZIP 생성 도구가 `Compress-Archive`→bsdtar로 바뀜(Windows 10 1803 이상 내장). 압축 방식은 둘 다 deflate.

검토자: 미지정.

## BEH-01 — 일반 스웜 처치 흡혈 100% + 레이저 흡혈 누락 교정 (완료 조건 충족, 검토 대기)

| 항목 | 내용 |
|---|---|
| 기준 | HEAD `abb8143` + 미커밋 로컬 변경. 커밋하지 않음 |
| 변경 파일 (sha256 앞8…뒤4) | `WiNILL/Entity/PlayerStats.h` 9b79880c…6acf<br>`WiNILL/Entity/CollisionSystem.h` 2e427601…2dff<br>`WiNILL/main.cpp` 2f975b36…bbb9<br>`tests/kill_lifesteal_test.cpp` b3430b80…4415 (신규)<br>`scripts/test_kill_lifesteal.ps1` 6b5cf322…e361 (신규) |
| 분리 | `PlayerStats.h`·`CollisionSystem.h`·신규 2개는 BEH-01 변경만 포함. `main.cpp`는 다른 미커밋 변경(FIX-02 등)과 함께 있어 hunk 단위 분리 필요 — BEH-01은 `creditKill`, 레이저 일반 몹 처치, `lOnKill` 3개 hunk |
| 작은 helper | `PlayerStats::ApplyKillLifesteal(hp)` — 점검표 2장 "BEH에 꼭 필요한 작은 helper는 diff에 명시" 조항. 처치 보상 전체 통합(D01)은 하지 않음 |

### 수정 전 상태 (일반 스웜 1마리, `GetLifestealPerKill()`=0.16 기준)
| 경로 | 흡혈 | 흡혈마 N킬 누적 |
|---|---|---|
| 탄환 → 일반 몹 (`CollisionSystem.h`) | 0.032 (스웜 보상 배율 0.2가 흡혈에도 곱해짐) | 있음 |
| 탄환 → Scope (`CollisionSystem.h`) | 0.16 | 있음 |
| 공통 수거 `creditKill` (`main.cpp`) | 0.16 (Creative 제외) | 있음 |
| 레이저 → 일반 몹 (`main.cpp`) | 없음 (흡혈 호출 자체가 없음; `scored`를 먼저 세워 공통 수거도 건너뜀) | 없음 |
| 레이저 → Scope `lOnKill` (`main.cpp`) | 0 (legacy 필드 `lifestealPerKill`(항상 0) 사용) | 있음 |

### 변경 내용
- `PlayerStats::ApplyKillLifesteal(hp)` 추가: `GetLifestealPerKill()` 전액, `maxHP` 상한. 몹별 보상 배율을 곱하지 않음.
- 5개 경로 모두 이 helper로 회복. 탄환→일반 몹의 `* rewardMult` 제거(점수 배율 `baseScore *= rewardMult`는 유지), `lOnKill`의 legacy 필드 읽기 교체, 레이저→일반 몹에 흡혈 추가.
- 유지한 것: 스웜 점수/XP/스타더스트 배율, 흡혈마 N킬 회복 로직, `creditKill`의 Creative 가드(DEC-02 미결).
- 레이저→일반 몹의 새 흡혈은 `!m->genesisThrown`일 때만 적용 — 투척 스웜은 흡혈 제외(BEH-02 목표와 일치, 이 경로의 나머지 투척 스웜 보상은 BEH-02에서 처리).

### 검증 결과 (2026-10-13)
| 검증 | 명령/방법 | 결과 |
|---|---|---|
| 실패 사례 먼저 | `scripts/test_kill_lifesteal.ps1`을 수정 전 코드에 실행 | `bullet kill heal: swarm 0.032, rotor 0.160` → 스웜 assert 실패(결함 재현) |
| 수정 후 회귀 | 같은 스크립트 — `CollisionSystem::Update`로 탄환 경로(스웜·로터·Scope) 처치, HP 증가·`maxHP` 상한·스웜 점수 배율 확인 | 통과: 스웜·로터·Scope 모두 0.16, `maxHP` 초과 없음, 스웜 점수 = 기본 × 0.2 유지 |
| `main.cpp` 경로 (람다라 직접 호출 불가) | 같은 스크립트의 소스 검사: `main.cpp` 3곳·`CollisionSystem.h` 2곳이 `ApplyKillLifesteal` 호출, 그 밖에 `GetLifestealPerKill()`/`.lifestealPerKill` 직접 읽기 없음(정의부·디버그 툴 제외) | 통과 |
| 빌드·전체 테스트 | `build_windows.ps1` + 테스트 10종 | 모두 통과 |
| 게임 스모크 | `RUN;GIVE=LIFESTEAL×2;GIVE=LASER;HP=0.5;AUTOFIRE;SPAWN=2×6;HOLD;KILL;SPAWN=4;KILL`, 종료 코드 확인 | 정상 진행(6콤보), 종료 코드 0. HUD에 HP 수치가 없어 회복량은 테스트로 판정. 캡처 `build/smoke/beh01/` |

### 남은 항목 / 결정 필요
- **레이저→일반 몹 처치의 흡혈마 N킬 누적 누락**: 다른 4개 경로는 누적하지만 이 경로만 없다. 점검표의 "흡혈마 N킬 회복은 별도"에 따라 이번에 바꾸지 않음 — 사용자 결정 후 별도 항목(또는 D01)으로 처리.
- 레이저 경로는 실제 호출 테스트가 없음(소스 검사로 대체). REG-01의 경로 harness는 람다 추출(D01 선행 단계) 이후 가능.
- Creative 모드 흡혈 가드가 경로마다 다름(공통 수거만 가드) — DEC-02.

검토자: 미지정.

## BEH-02 — 투척 스웜 처치는 콤보·도감만 (완료 조건 충족, 검토 대기)

| 항목 | 내용 |
|---|---|
| 결정 | **DEC-04 = 제외** (투척 스웜 처치는 무한 세례 처치 누적에 넣지 않음). 점검표 권고안, 사용자 진행 지시(2026-10-13)로 확정 |
| 기준 | HEAD `abb8143` + 미커밋 로컬 변경. 커밋하지 않음 |
| 변경 파일 (sha256 앞8…뒤4) | `WiNILL/Render/Juice.h` 6d336f62…389c<br>`WiNILL/Entity/CollisionSystem.h` be9d1852…6f51<br>`WiNILL/main.cpp` 14cc4096…d27c<br>`tests/thrown_swarm_kill_test.cpp` 7cc819f7…8fac (신규)<br>`scripts/test_thrown_swarm_kill.ps1` 4679df2d…9d30 (신규) |
| 분리 | `Juice.h`와 신규 2개는 BEH-02만. `CollisionSystem.h`는 BEH-01과, `main.cpp`는 FIX-02·BEH-01 등과 함께 있어 hunk 단위 분리 필요. BEH-02 hunk: `CollisionSystem.h` helper 추가·탄환 분기 / `main.cpp` 공통 수거 분기·레이저 분기(BEH-01의 임시 `!m->genesisThrown` 조건 제거 포함) |
| 작은 helper | `CreditThrownSwarmKill(Monster&)`(CollisionSystem.h)와 `AddKillCombo(bool countsForRain = true)` — 점검표 2장 조항. 기존 `AddKillCombo()` 호출 6곳은 기본값으로 동작 동일 |

### 수정 전 상태 (투척 스웜 1마리 처치)
| 경로 | 콤보 | 도감 | 점수·XP·별가루·흡혈·런 킬·흡혈마 | 무한 세례 누적 |
|---|---|---|---|---|
| 탄환 (`CollisionSystem.h`) | O | O | 없음 | **+1** (`AddKillCombo` 부수효과) |
| 공통 수거 (`main.cpp`, 지속 피해·드론 등) | **없음** | **없음** | 없음 | 없음 |
| 레이저 (`main.cpp`) | O | O | **일반 처치와 동일하게 지급** (투척 여부 미확인) | +1 |

### 변경 내용
- `CreditThrownSwarmKill`: `scored` 설정 + 콤보(무한 세례 누적 없음) + 도감. 세 경로가 모두 호출.
- 공통 수거·레이저에 투척 분기 추가(레이저는 일반 보상 분기 앞에서 분리), 탄환 분기는 helper로 교체.

### 검증 결과 (2026-10-13)
| 검증 | 명령/방법 | 결과 |
|---|---|---|
| 실패 사례 먼저 | `scripts/test_thrown_swarm_kill.ps1`을 수정 전 코드에 실행(탄환 경로) | `thrown swarm: … rain +1 …` → `b.rain == a.rain` 실패(DEC-04 위반 재현). 나머지 항목은 이미 목표대로 |
| 수정 후 회귀 | 같은 스크립트 — 탄환으로 일반 스웜(대조군)과 투척 스웜 처치, HP·점수·무한 세례·콤보·런 킬·흡혈마·도감·별가루 스냅샷 비교 | 통과: 일반 스웜은 전부 +, 투척 스웜은 콤보 +1·도감 +1 외 모두 0 |
| `main.cpp` 경로 (람다라 직접 호출 불가) | 같은 스크립트의 소스 검사: `main.cpp` 2곳·`CollisionSystem.h` 1곳 helper 호출, 투척 판정 분기 3곳이 모두 helper로 이어짐 | 통과 |
| 빌드·전체 테스트 | `build_windows.ps1` + 테스트 11종(신규 포함) | 모두 통과 |
| 게임 스모크 | `RUN;GIVE=LIFESTEAL;GIVE=LASER;AUTOFIRE;SPAWN=1(Genesis);HOLD×3;KILL`, 종료 코드 확인 | 정상 진행, 종료 코드 0. 캡처로는 투척 스웜의 처치 수단을 구분할 수 없어 보상 판정은 테스트 근거. 캡처 `build/smoke/beh02/` |

### 남은 항목
- 레이저·공통 수거 경로는 실제 호출 테스트가 없음(소스 검사로 대체) — REG-01 harness는 처치 람다 추출(D01 선행) 후.
- Creative 모드에서는 `RegisterCodexMobKill`이 도감을 올리지 않음(기존 동작, DEC-02 범위).

검토자: 미지정.

## REG-01 — 처치 보상 회귀 기준 (D01 선행, 통과)

| 항목 | 내용 |
|---|---|
| 기준 | HEAD `abb8143` + 미커밋 로컬 변경(FIX-01~03, BEH-01/02 포함). 커밋하지 않음 |
| 변경 파일 (sha256 앞8…뒤4) | `WiNILL/Entity/KillRewards.h` 8648e357…896c (신규)<br>`WiNILL/main.cpp` 16efa6af…0fbd<br>`WiNILL/WiNILL.vcxproj` d704809b…d07a / `.filters` 5012ab4a…a802 (헤더 등록)<br>`tests/reg01_kill_rewards_test.cpp` f5d5411b…80b7 (신규)<br>`scripts/test_reg01_kill_rewards.ps1` 95529304…478c (신규)<br>BEH-01/02 테스트·스크립트 소스 검사 위치 갱신: `tests/kill_lifesteal_test.cpp` 950c33be…535a, `scripts/test_kill_lifesteal.ps1` cc4aeaf9…c386, `tests/thrown_swarm_kill_test.cpp` 203b8e69…7756, `scripts/test_thrown_swarm_kill.ps1` 20b04cbe…3618 |

### 선행 단계: 추출 전용 이동 (동작 변경 없음)
점검표 3장은 "5개 진입점을 실제로 호출하는 harness"를 요구하지만, 3개 진입점이 `main()` 안의 람다·인라인 코드라 호출할 수 없었다. 검토(2026-10-12)에서 제안한 (a)안대로 **보상 코드를 문장 그대로 옮기는 단계**를 먼저 수행했다.
- 신규 `KillRewards.h`: `CreditRegularKill`(←`creditKill` 람다), `CollectMonsterKill`/`CollectScopeKill`(←사망 수거 루프의 보상 부분), `CreditLaserMonsterKill`/`CreditLaserScopeKill`(←레이저 처치 블록·`lOnKill`). 전역 대신 `KillRewardSink`(HP·점수·스탯·플레이어 위치·Creative)를 받는다.
- 치환 외 변경 없음: `g_GameManager.playerHP/scoreAccum/score`→`s.*`, `g_Stats`→`s.stats`, `pCX/pCY`→`s.playerCX/Y`, `g_CreativeMode`→`s.creative`, `m->`→`m.`. 경로 간 차이(Creative 가드는 수거만, 레이저 일반 몹은 흡혈마 누적 없음)도 그대로 유지 — 통합은 D01.
- `main.cpp`는 루프 안에서 위 함수만 호출. 사망 연출·히트스톱·연쇄 폭발 등 보상 외 코드는 그대로.
- 동등성 근거: 위 치환 규칙의 문장 단위 이동(검토로 확인), 이동 전에 작성한 BEH-01/02 테스트 재통과, 아래 REG-01 결과가 이동 전 확인된 동작(BEH 기록의 경로별 표)과 일치, 게임 스모크.

### REG-01 테스트 (`scripts/test_reg01_kill_rewards.ps1`)
- 진입점 5개를 실제 호출: 탄환→일반 몹/Scope(`CollisionSystem::Update`), 공통 수거(`CollectMonsterKill`/`CollectScopeKill`), 레이저(`CreditLaserMonsterKill`/`CreditLaserScopeKill`).
- 대상 5종 × 수단 3종 = 15조합, 관찰 11종: 점수, XP, 즉시 별가루, 지연 별가루(값·XP), HP, 런 킬, 전쟁군주 스택, 흡혈마 누적, 콤보, 도감, 무한 세례 누적.
- 기대값은 규칙에서 직접 계산한 상수(기본 스탯): 로터 100점·XP 2·별가루 1 / 스웜 2.4점·XP 1·별가루 1 / Scope 300점·XP 8·별가루 3 / 퀘이사 480점·지연 7·XP 12 / 흡혈 0.16 / 흡혈마 10킬 / 전쟁군주 1000킬. 보상 함수로 기대값을 다시 계산하지 않음.
- 특수 사례: 같은 사망 재처리(수거 2회) 무지급, `maxHP` 상한, 흡혈마 9→10킬 경계, 전쟁군주 998/999→다음 킬, 투척 스웜은 999에서도 스택 없음, Creative.
- 미정 규칙은 현재값 고정(OPEN 표시): 레이저→일반 몹 흡혈마 누적 없음 / Creative는 공통 수거만 흡혈 없음, 도감은 모든 경로에서 미집계(DEC-02).
- 구조 검사: `main.cpp`가 4개 추출 함수를 호출하고 자체 보상 코드(`MobKillReward(`, `ApplyKillLifesteal`, `RegisterKill(`, `AddKillCombo(`, `scoreAccum +=` 등)가 없음.

### 결과 (2026-10-13)
| 검증 | 결과 |
|---|---|
| REG-01 | 통과 — 15조합 표와 특수 사례 모두 기대값 일치 |
| 테스트 12종 (REG-01, BEH-01/02, FIX-02/01 포함) | 모두 통과 |
| Windows Release 빌드 | 통과 (새 경고 없음) |
| 게임 스모크 | `RUN;GIVE=LIFESTEAL;GIVE=LASER;HP=0.5;AUTOFIRE;SPAWN=Genesis·Swarm×2·Scope·Rotor;HOLD×2;KILL;SPAWN=Quasar;KILL` — 점수·콤보·별가루 정상 증가, 종료 코드 0. 캡처 `build/smoke/reg01/` |

### 남은 것
- DYING(플레이어 사망 시 일괄 처리)은 보상 진입점이 아니라 `main.cpp`에 남음(모두 `scored`만 세우고 무지급) — REG-01 대상 밖, REG-05에서 다룸.
- OPEN 2건(레이저 흡혈마 누적, DEC-02)은 결정 시 테스트 기대값을 갱신.

검토자: 미지정.

## REG-02 / REG-03 — 스탯 경로·런 상태 보존 회귀 기준 (통과)

| 항목 | 내용 |
|---|---|
| 기준 | HEAD `abb8143` + 미커밋 로컬 변경. 커밋하지 않음 |
| 변경 파일 (sha256 앞8…뒤4) | `WiNILL/Entity/RunStats.h` ce9da5d7…4c98 (신규)<br>`WiNILL/main.cpp` a422788c…6578<br>`WiNILL/Core/Scenes/SceneRunConfig.cpp` caa3e867…7ac2<br>`WiNILL/Core/Scenes/SceneAugment.cpp` 523ba523…2dba<br>`WiNILL/WiNILL.vcxproj` 1ab7880f…fbe3 / `.filters` 57e5cffc…e439<br>`tests/reg02_03_stat_builds_test.cpp` e24467f9…f149 (신규)<br>`scripts/test_reg02_03_stat_builds.ps1` 03a108dc…0ea4 (신규) |

### 선행 단계: 스탯 계산 추출 (경로별 구성 유지)
- `RunStats.h`: `ResetRunStats`(새 런 리셋), `ApplyLoadoutStats`(FinalizeLoadout), `RebuildStats`(교체 재계산·재시작, 런 필드 보존 포함), `ChaosStats`, `Preview*Stats`(미리보기 4종). 문장이 완전히 같던 두 묶음만 공용화: `ApplyAugStats`(4곳), `ApplyTrialMaxHp`(3곳).
- `RebuildPlayerStatsFromOwned`: 스탯을 먼저 계산한 뒤 런타임 객체 루프 실행. 루프에서 g_Stats를 읽는 곳은 `ApplyAugmentSideEffects`(창 크기, 차크람 수)뿐이며 최종 스탯으로 같은 최종 상태에 도달(차크람은 루프 뒤 최종 재초기화, 창은 같은 중심으로 최종 크기). `ResetPlayerSkills`/`EquipSkill`/takenOnce는 스탯을 읽지 않음.

### 결과 (현재값 고정, OPEN 표시)
| 경로 (SMG+메타+시련10) | 최대 HP | baseFireInterval |
|---|---|---|
| 새 런 / 미리보기 | 113.4 (시련 → 카드) | 0.20 / 0.15(미설정) |
| 재시작 | 110.7 (카드 → 시련) — OPEN DEC-03 | 0.10 |
| 증강 교체 재계산 | 135 (시련 미적용) — OPEN DEC-03, 실제 영향 있음 | 0.10 |
- 연사·공격력·탄속·시야는 모든 경로·조합(무기 2 × 메타 2 × 시련 2 × 배율 2)에서 일치. 후보 미리보기 = 실제 획득 = 교체 획득.
- `baseFireInterval`은 게임 코드에서 읽히지 않음(디버그 표시만) — 경로 차이는 동작 영향 없음.
- REG-03: 재계산은 5개 런 필드 보존(전쟁군주 스택·효과 포함), 전쟁군주 교체로 제거 시 스택 소실, 재획득 시 0부터(현재 동작), 재시작은 모두 초기화, 대혼란은 4개 보존·전쟁군주 스택 미보존(OPEN DEC-01). takenOnce는 main.cpp에서 true로만 쓰이고 `GameManager::ResetRunProgress`만 초기화(소스 검사).

### 검증 (2026-10-13)
- `scripts/test_reg02_03_stat_builds.ps1` 통과, 테스트 13종 모두 통과, Release 빌드 통과.
- 게임 스모크(`RUN;GIVE=RATE+4%;GIVE=HP_UP;GIVE=CB_WAR;AUTOFIRE;SPAWN…;PAUSED`) 종료 코드 0. 교체·재시작·대혼란은 스모크 단계로 유발 불가 → 테스트 근거. 캡처 `build/smoke/reg02/`.

### 다음
- 결정 필요: DEC-03(교체 재계산·대혼란의 시련 HP 누락, 재시작의 적용 순서), DEC-01(대혼란 전쟁군주 스택).
- 남은 REG: 04·05(런 초기화·포기·결과 — 람다 추출 필요), 06(스킬 슬롯 — 바로 테스트 가능), 26(설정 저장·복원).

검토자: 미지정.

## REG-06 — 스킬 슬롯·런타임 초기화 회귀 기준 (통과)

| 항목 | 내용 |
|---|---|
| 변경 파일 | `tests/reg06_skill_reset_test.cpp` ded1c557…b198 (신규), `scripts/test_reg06_skill_reset.ps1` 47570f5c…12bf (신규). 게임 코드 변경 없음(이미 일반 함수) |
| 대상 | `ClearEquippedSkills`, `ResetPlayerSkills`, `ReequipSkillsFromOwned`, `EquipSkill` — `SceneSkills.cpp`·`PlayerRuntime.cpp`를 직접 링크 |

결과 (2026-10-13, 모든 슬롯·런타임 필드를 채운 뒤 실행):
- `ClearEquippedSkills`: 슬롯 3개(타입·쿨다운)와 교체 인덱스만 초기화, 대시·실드·시간정지 런타임은 그대로.
- `ResetPlayerSkills`: 슬롯 + 대시 쿨다운/무적/진행/부스트 탄, 넉백, 실드·실드 타이머, 시간정지·하이퍼포커스 타이머 초기화. 증강 선택 직후 유예(`postPickGrace`), 대시 시작·끝 좌표, 입력 edge(`ResetPlayerInputEdges` 담당)는 유지.
- `ReequipSkillsFromOwned`: 먼저 비우고 보유 순서대로 장착, 중복·비스킬 카드·잘못된 인덱스는 건너뜀, 런타임 무변경. 슬롯이 찼을 때 새 스킬은 교체 인덱스부터 순환 교체.
- D06 목표 기록(강제 아님): Entity → Scene include 현재 1건 `PlayerRuntime.cpp:3 SceneSkills.h`.

## REG-05 — 런 결과 확정 회귀 기준 (통과)

| 항목 | 내용 |
|---|---|
| 변경 파일 (sha256 앞8…뒤4) | `WiNILL/Core/RunResult.h` 4df5bf65…44c5 (신규)<br>`WiNILL/main.cpp` e24afc14…e8fc<br>`WiNILL/WiNILL.vcxproj` b6f25536…25bf / `.filters` cabd9138…7a6c<br>`tests/reg05_run_result_test.cpp` eac1c174…b7a9 (신규)<br>`scripts/test_reg05_run_result.ps1` 48e1403f…7fd3 (신규) |

### 선행 단계: 결과 확정 추출 (동작 변경 없음)
포기(`AbandonCurrentRun`)와 사망(DYING→GAMEOVER) 경로에 문장 단위로 동일하던 결과 확정 블록을 `CommitRunResult(creative, difficulty, score, kills, weaponIdx)`로 옮김. `g_LastRunRecord`는 main.cpp 정의라 반환값으로 돌려주고 호출부가 대입. 실행 1회 보장 조건(포기: GAMEOVER면 즉시 반환 / 사망: 타이머 만료 분기에서 GAMEOVER로 전환한 뒤 확정)은 호출부에 그대로.

### 테스트 (`scripts/test_reg05_run_result.ps1`)
- 저장 stub: `g_SmokeCapture`로 `SaveGame` 무동작 + 빈 임시 폴더에서 실행 후 생성 파일 0개 확인(실제 세이브 접근 없음).
- 결과(기대값은 공식에서 직접 계산): 일반 런 점수 12345·처치 77·보통 → 최고 기록, 누적 처치·판수, 무기별 기록, 도감 런 처치의 누적 이동, 코인 60 = (12+38)×1.2. 낮은 2회차는 기록 아님·코인 12. 범위 밖 난이도는 보통, 어려움 ×1.5. 10번째 판에서 업적 1회 해금(+200코인), 저장 플래그 해제, 재해금 없음. Creative는 아무것도 기록·지급하지 않고 런 도감 처치도 이동하지 않음.
- 소스 검사: main.cpp에 `CommitRunResult` 호출 2곳, `RecordRunResult` 직접 호출 없음, 포기 람다 첫 줄이 GAMEOVER 반환 가드, 사망 경로 호출이 `g_DyingTimer <= 0` 분기의 GAMEOVER 전환 뒤.

### 결과 (2026-10-13)
- REG-05 통과, 테스트 15종 모두 통과, Release 빌드 통과.
- 게임 스모크 `RUN;SPAWN=0;HOLD;GAMEOVER;MENU`(GAMEOVER 단계 = 포기 경로): 결과 화면 정상(사망 원인 "플레이 포기", 획득 별가루 0 = (248/1000+0)×1.2), 종료 코드 0. 캡처 `build/smoke/reg05/`.
- 남은 것: 저장 횟수(SaveGame 호출 수) 자체는 stub 때문에 세지 못함 — 판수·코인으로 1회 확정을 확인. 메뉴 왕복 재진입은 소스 검사 수준.

## REG-04 — 보류 (진행 방식 결정 필요)
`ResetForNewGame`이 건드리는 전역 54개 중 35개(`g_GameManager`, `g_MonsterManager`, 드론·차크람·레이저·오브, 사망 연출 상태 등)와 `main()` 지역 변수(`fireTimer`, `playerWin`)가 main.cpp에 정의되어 있어, 테스트에서 호출하려면 정의 위치부터 옮겨야 함 — 이는 D04(런 상태 소유권) 자체. 선택지: (a) 스모크 명령으로 상태 채우기 → 새 런/재계산/재시작/포기 실행 → 상태 스냅샷 파일 기록(main.cpp에 테스트 명령 추가), (b) D04 설계와 함께 진행.

## BEH-03 — 레이저 일반 몹 처치의 흡혈마 누적 (2026-10-13)

사용자 결정: 레이저로 일반 몹을 처치해도 흡혈마 N킬 누적에 포함한다. REG-01의 레이저 일반 몹 기대값을 누적 +1로 바꾸어 기존 코드에서 실패를 확인한 뒤, `KillRewards.h`의 세 보상 경로가 `CreditVampireKill`을 공유하도록 수정했다. Genesis 투척 스웜은 기존 조기 분기로 누적 대상에서 제외한다.

- `scripts/test_reg01_kill_rewards.ps1`: 15조합과 9→10킬 경계 통과. 레이저 일반 몹의 누적 +1, HP 추가 +1, 투척 스웜 누적 0 확인.
- `scripts/test_kill_lifesteal.ps1`, `scripts/test_thrown_swarm_kill.ps1`, `scripts/build_windows.ps1`: 통과.
- FIX-01 입력 검증은 사용자가 완료했다고 보고했다. 확인한 플랫폼과 입력 방식의 세부 기록은 아직 받지 않았으므로 macOS CI 빌드 검증과 혼동하지 않는다.
- 위 REG-01·BEH-01 기록에 남은 레이저 흡혈마 `OPEN` 표기는 당시의 결과이며 BEH-03으로 해소했다. DEC-02(Creative/DYING)는 별도 미결이다.

## REG-04 — 런 초기화 경로 회귀 기준 (방식 (a), 통과)

| 항목 | 내용 |
|---|---|
| 방식 | 사용자 결정 (a): 런 상태 전역 35개가 main.cpp 정의라 함수 추출 대신 스모크 명령으로 실제 게임에서 검증 |
| 변경 파일 | `WiNILL/main.cpp`: 스모크 처리기에 `REG04=…`/`REG04DUMP=…` 명령 추가(게임 동작 무관, 스모크 세션에서만 실행)<br>`scripts/test_reg04_run_reset.ps1` (신규): Release 빌드를 스모크로 실행하고 `Screenshots/reg04_state.txt` 스냅샷을 기대표와 비교<br>`scripts/test_reg02_03_stat_builds.ps1`: takenOnce 검사를 쓰기(`true` 외 대입·`memset`)만 보도록 정밀화(스냅샷의 읽기를 오인하던 문제) |

### 방법
`REG04=<경로>`는 같은 단계 안에서 ① 드론·차크람·레이저·오브·스킬 슬롯·대시/실드/시간정지·입력 edge·유예·탄환·몹·Scope·별가루·충격파·잔해·증강 큐·보유 증강·획득 기록·점수/XP/레벨/HP/킬·무기·게임 시간을 모두 서로 다른 값으로 채우고 ② 경로 하나를 실행하고 ③ 스냅샷을 기록(사이에 프레임 없음). READY 재진입은 프레임 자체의 GAMEOVER→READY 처리를 거치도록 상태만 바꾸고 다음 단계에서 기록, 같은 동작을 두 번 수행.

### 결과 (2026-10-13, 6개 경로 모두 기대표 일치)
| 경로 | 비움 | 유지 |
|---|---|---|
| 새 런 | 증강 객체·스킬 런타임·전투 객체·연출·큐·보유 증강·획득 기록·점수/XP/레벨/킬/무기/시간, HP 100 | **입력 edge·증강 선택 유예 (OPEN)** |
| 재시작 | 새 런과 같음 | 보유 증강·무기·그 증강의 스킬, 일회성 카드만 획득 기록 재잠금, HP = 최대 HP |
| 증강 교체 재계산 | 증강 객체, 스킬 런타임 (**진행 중 실드·시간정지·대시 쿨다운 포함, OPEN**) | 전투 객체·점수·보유 증강·획득 기록·킬 등 나머지 전부 |
| 포기 | 전투장(몹·탄환·별가루·충격파·증강 객체) | 빌드·점수·스킬·큐(결과 화면용), HP 0, GAMEOVER |
| READY 진입 1회차 | 새 런과 같음 (프레임 전이에서 1회) | — |
| READY 진입 2회차 | 아무것도 비우지 않음 → **중복 초기화 없음** | 채운 값 전부 |
- 사용자 세이브의 메타 레벨에 따라 달라지는 재시작 HP는 "최대 HP로 채움"만 확인.
- 소유 객체 중복 삭제: 몹 포인터를 채운 상태에서 각 경로 실행 후 정상 종료(종료 코드 0)로 확인.

### 새로 드러난 현재 동작 (결정 필요, 테스트에 현재값 고정)
1. 어떤 초기화 경로도 `ResetPlayerInputEdges`·`postPickGrace`를 부르지 않아 대시/스킬 누름 상태와 선택 유예가 다음 런으로 넘어감 — 점검표 "이전 런 입력 잔존 없음"과 불일치.
2. 증강 교체 재계산이 `ResetPlayerSkills`를 부르면서 진행 중인 실드·시간정지·대시 쿨다운을 0으로 만듦.

### 참고: 진행 중 외부 변경 (2026-10-13 08:51)
REG-04 작업 중 다른 작업이 `WiNILL/Entity/KillRewards.h`(sha256 aead89ea…)와 `tests/reg01_kill_rewards_test.cpp`(90ac8b5e…)를 수정함 — 흡혈마 누적을 `CreditVampireKill`로 공통화하고 레이저→일반 몹에도 적용(REG-01의 OPEN 항목). 이 두 파일과 REG-01 실행은 건드리지 않음. REG-01 기록의 해시(8648e357…, f5d5411b…)는 그 이전 상태.

## REG-01 재실행 — 레이저 흡혈마 누적 반영 (통과)
- 사용자 확인(2026-10-13): `KillRewards.h`의 외부 변경(레이저로 일반 몹 처치 시 흡혈마 누적, `CreditVampireKill` 공통화)은 의도한 변경. 해당 OPEN 항목 종료.
- 현재 파일: `WiNILL/Entity/KillRewards.h` aead89ea…01da, `tests/reg01_kill_rewards_test.cpp` 401d7cb5…538d.
- Release 재빌드 후 REG-01 통과: 15조합 모두 흡혈마 +1(투척 스웜 제외), 레이저 9/10 경계에서 HP +1.16. 테스트 16종 전체 통과.
- 참고: REG-01 테스트 머리 주석(11행)에 "OPEN (vampire streak on laser kills…)" 문구가 남아 있음 — 다음 수정 때 정리.

## REG-26 — 설정 저장·복원 회귀 기준 (통과)

| 항목 | 내용 |
|---|---|
| 변경 파일 (sha256 앞8…뒤4) | `WiNILL/System/SettingsSnapshot.h` 7f8c25ee…8715 (신규)<br>`WiNILL/Core/Scenes/SceneSettings.cpp` 5278a9cb…e370<br>`WiNILL/WiNILL.vcxproj` 6c46c31b…b5b0 / `.filters` e7aaec2a…79e8<br>`tests/reg26_settings_test.cpp` 674b6a09…b507 (신규)<br>`scripts/test_reg26_settings.ps1` 7accb117…65a4 (신규) |

### 선행 단계: 스냅샷 추출 (동작 변경 없음)
설정 화면 함수 안의 스냅샷 구조체, 문장 단위로 같던 캡처 3곳(진입·저장 버튼·저장 후 뒤로), 복원·변경 감지 람다 본문을 `SettingsSnapshot.h`의 `CaptureSettingsSnapshot`/`RestoreSettingsSnapshot`/`SettingsSnapshotChanged`로 이동. 화면의 정적 상태(`savedSettings`, 유효 플래그)와 행별 변경 표시는 그대로.

### 테스트 (`scripts/test_reg26_settings.ps1`, 빈 임시 폴더에서 실행)
- 왕복: 옵션 19개 전부 기본값이 아닌 값 + 진행도(최고 점수·누적·코인·무기 기록·메타·업적·증강/몹 발견·도감 처치·직업·테마) → `SaveGame`(난독화 형식, 매직 확인) → 초기화 → `LoadGame` → 모두 일치.
- 예전 평문 세이브: 일부 키만 있으면 나머지는 기본값 유지, 알 수 없는 키·깨진 줄 무시.
- 범위 밖 값: fps/언어/블러 주기/몹 스타일/효과 밀도는 무효 값 거부, 볼륨 0~100·메타 최대 레벨·코인 음수→0·도감 처치 음수 무시. **OPEN: 테마 선택 인덱스(42 그대로)와 기록 값(최고 점수·판수 음수 그대로)은 검증 없이 수용.**
- 스냅샷: 변경 감지 → 버리기 시 17개 옵션 원복 + 오디오 장치 호출 5종(BGM·효과음·마스터 ON/OFF·볼륨·모노)이 원래 값으로 재적용, 17개 필드 각각 단독 변경도 감지. 장치가 모노 전환을 거부하면 장치 값으로 남아 변경으로 표시됨(현재 동작).
- 구조 검사: 설정 화면이 바꾸는 옵션 15개(화면 애니메이션용 3개 제외)가 모두 스냅샷 17개에 포함, 스냅샷 옵션 전부가 저장·로드 키를 가짐. (`strongmenudim`·`macopt`는 화면에서 바꾸지 않는 값이라 스냅샷 대상 아님.)
- UI 스모크 `SETTINGS;MENU;SETTINGS` KR/EN: 재진입 시 스냅샷 재설정으로 "변경 저장" 비활성, 정상 표시. 캡처 `build/smoke/reg26/`.

### 결과 (2026-10-13)
- REG-26 통과, 테스트 17종 전체 통과, Release 빌드 통과. 실제 세이브 파일 접근 없음.

## 점검표 3장 REG 현황
REG-01·02·03·04·05·06·26 모두 회귀 기준 확보. 결정 대기: DEC-01(대혼란 전쟁군주), DEC-02(Creative), DEC-03(시련 HP), REG-04 OPEN 2건(입력 edge 잔존, 교체 시 실드 소멸), REG-26 OPEN 1건(테마·기록 값 검증).

## 전체 재점검 (2026-10-13)

### 1. 추출 코드의 동등성 — HEAD(`abb8143`) 원본과 토큰 단위 대조
주석·공백 제거와 이름 치환(`g_Stats`→`s.stats` 등) 후 토큰 비교(스크립트: 세션 스크래치 `equiv.py`).
| 대상 | 결과 |
|---|---|
| `SettingsSnapshot.h` 복원·변경 감지·구조체, 캡처 3곳 | 모두 동일 |
| `RunStats.h` 미리보기 4종, 대혼란 | 모두 동일 |
| `RunStats.h` 재계산·FinalizeLoadout | 스탯 문장 순서 대조로 동일 확인(런타임 객체 루프는 별도, REG-02/04로 검증) |
| `RunResult.h` | HEAD의 포기 블록과 게임오버 블록이 서로 동일 → 하나로 합친 것이 타당 |
| `KillRewards.h` 공통 수거 Scope | 동일 |
| `KillRewards.h` creditKill·공통 수거 일반 몹·레이저 2종 | 차이는 의도한 변경뿐: BEH-01 흡혈 helper, BEH-02 투척 스웜 처리, 레이저 흡혈마 누적(사용자 승인), 흡혈마 helper(`+=1; if >max` → `min`, 결과 동일) |

### 2. 재검증
- Release 빌드 통과, 테스트 18종 전부 통과(종료 코드/예외 기준; REG-04·platform_format은 성공 시 요약 줄이 없어 별도 확인).
- macOS 경로 clang 대체 검사(CMake 31개 소스): 새 오류 없음(검사 환경 부산물 25줄 동일).
- 통합 스모크(`RUN;GIVE×3;AUTOFIRE;SPAWN 4종;HOLD;KILL;GAMEOVER;SETTINGS;MENU`) 종료 코드 0, 결과 화면 정상. 캡처 `build/smoke/final/`.
- 낡은 주석 1건 정리: `tests/reg01_kill_rewards_test.cpp` 머리말의 레이저 흡혈마 OPEN 문구.

### 3. 남은 중복 (이번 작업에서 의도적으로 남김 / 새로 생김)
| 위치 | 상태 | 처리 제안 |
|---|---|---|
| 흡혈마 N킬 처리 | `KillRewards.h` 3경로는 `CreditVampireKill` 공유, 탄환 경로(`CollisionSystem.h` 일반 몹·Scope) 2곳은 인라인 사본 | `ApplyKillLifesteal`처럼 PlayerStats 쪽 helper로 옮겨 5경로 공유 (D01, 동작 동일) |
| 처치 보상 순서 | 탄환 2경로(CollisionSystem)와 레이저·수거(KillRewards)가 같은 단계를 각자 수행 | D01 본 작업 (REG-01이 기준) |
| 스탯 기본 단계 `PlayerStats+ApplyMeta+scale(+무기)` | `RunStats.h` 4곳 반복 — 경로 차이 보존을 위해 유지 | D02에서 공통 기본 함수 + 경로별 단계로 정리 (REG-02가 기준) |
| 증강 객체 초기화(드론·차크람·레이저·오브) | main.cpp 4곳(재계산, `ResetAugmentDerivedRuntime`, 새 런, 포기) — 재계산의 묶음은 `ResetAugmentDerivedRuntime`과 동일 | D04 (REG-04가 기준) |
| 설정 행별 변경 표시 | `SceneSettings.cpp` 15개 비교가 `SettingsSnapshotChanged`와 필드 중복 | D26 (REG-26이 기준) |
| BEH 테스트 2종 | `kill_lifesteal`/`thrown_swarm_kill`이 REG-01과 대부분 겹침 | 소스 검사만 REG-01 스크립트로 옮기고 정리 가능 |
| 테스트 스크립트 | 컴파일형 15개가 vswhere/vcvars 준비 ~9줄 반복, 2개(`blur_windows`, `mob_death_fx`)는 PATH 보정 없음(경고만, 통과) | D39 공통 runner |
| 테스트 fixture | Audio 스텁·`g_BaseOrtho`·탄환 helper·`IdxOf`가 2~5개 테스트에 반복 | `tests/TestSupport.h` 후보 (독립성과 저울질) |
| REG-04 스모크 명령 | main.cpp에 테스트 전용 105줄 | D04 이후 디버그 모듈로 이동 검토 |

### 4. 판정
추출·이동은 모두 원본과 동등(차이는 기록된 의도 변경뿐)하며 회귀 기준이 모든 경로를 덮는다. 남은 중복은 점검표 D 항목의 범위로, 각 항목에 회귀 테스트가 준비되어 있다.

## 흡혈마 N킬 처리 통합 (재점검 3절 첫 항목, 완료)

| 항목 | 내용 |
|---|---|
| 변경 파일 (sha256 앞8…뒤4) | `WiNILL/Entity/PlayerStats.h` 6d3b9611…45ad — `ApplyVampireKill(hp)` 추가<br>`WiNILL/Entity/CollisionSystem.h` 16c5e78f…6f71 — 탄환 2경로의 인라인 사본 교체<br>`WiNILL/Entity/KillRewards.h` 9484e518…f39a — `CreditVampireKill` 제거, 3경로 교체<br>`tests/reg01_kill_rewards_test.cpp` 4f275b16…bc07, `scripts/test_reg01_kill_rewards.ps1` 4d85f695…441e |
| 동작 | 변경 없음. 판정(카드 보유 → 누적 → 필요 수 도달 시 0으로, HP +1, maxHP 상한)이 5곳 사본과 같고, 경로별 흡혈과의 순서도 유지(탄환: 흡혈마→흡혈 / 그 외: 흡혈→흡혈마; 둘 다 비음수 가산 후 상한이라 결과 동일) |

검증 (2026-10-13):
- REG-01 통과(변경 전에 고정한 15조합 값 그대로). 추가: Scope 처치의 9/10 경계를 탄환·수거·레이저 3경로 모두 확인 + 1 HP 회복의 maxHP 상한.
- REG-01 스크립트에 구조 검사 추가: `ApplyVampireKill` 호출 CollisionSystem 2·KillRewards 3·main 0, PlayerStats 밖에서 누적·비교·`GetVampireKillNeed` 사용 없음.
- Release 빌드, 테스트 18종, 게임 스모크(흡혈마·레이저·스웜·Scope·로터·일괄 처치·포기) 모두 통과, 종료 코드 0.

참고 — 진행 중 외부 변경 (09:24): 다른 작업이 `tests/reg02_03_stat_builds_test.cpp`(sha256 4f6c157e…)를 DEC-03 확정 기준으로 수정 중. `RunStats.h`는 아직 미변경(ce9da5d7…). 이 파일들은 건드리지 않음 — 그 작업 완료 후 REG-02/03 재실행 필요.

## REG-02 → REG-04 → REG-03 → FIX-03 후속 작업 (2026-10-13)

- **REG-02 / DEC-03 확정:** 사용자 결정에 따라 시련 10의 최대 HP 감산을 HP 증강보다 먼저 적용하고, `baseFireInterval`은 무기 적용 후·증강 전 값으로 통일했다. `RunStats.h`의 재계산·대혼란에 시련 감산을 추가하고, 재시작의 뒤늦은 감산을 제거했다. SMG+메타+시련10+HP 증강에서 새 런·재계산·재시작·미리보기 모두 최대 HP 113.4, 기본 발사 간격 0.10. 대혼란은 HP 증강 제거 후 98.4. REG-02/03 테스트는 변경 전 실패와 수정 후 통과를 확인했다.
- **REG-04 새 런:** 입력 edge와 선택 유예의 이전 런 잔존을 스모크 테스트에서 재현하고 `ResetForNewGame`에서 비웠다. 새 런·재시작·GAMEOVER→READY 모두 0/false, READY 중복 진입은 재초기화하지 않음.
- **REG-04 증강 교체:** 사용자 결정에 따라 재계산 시 기본 대시 쿨다운·활성 실드를 유지하고, 재장착된 스킬은 같은 종류의 쿨다운·활성 시간을 유지한다. 제거된 시간정지/하이퍼포커스 효과와 대시 강화 잔여 탄환은 지운다. 스모크에 시간정지 보유/제거 두 사례를 추가했고 7경로 통과. 재시작은 새 런이므로 보존하지 않는다.
- **REG-03 일회성 콤보:** `GameManager::PickAugChoices`의 콤보 주입이 `takenOnce`를 건너뛰던 문제를 재현했다. 후보 판정을 `ComboRewardEligible`로 모아 현재 보유·동일 타입의 획득 장부·재료를 검사한다. 전쟁군주 교체 후 재등장 방지 테스트와 게임 호출 경로 소스 검사 통과.
- **FIX-03 검증:** `verify_package_zip.py`의 비 ASCII 성공 문자를 ASCII로 바꿔 CP949 콘솔 오류를 제거했다. 기본 환경의 PowerShell·shell ZIP이 동일한 76개 파일을 담고 사용자/빌드 파일 11종을 제외함을 확인했다. Git Bash는 샌드박스의 MSYS 객체 생성 차단으로 1회 실패하여 허용된 실행 환경에서 같은 테스트를 다시 실행했다. 실제 macOS CI의 실행 권한 검증은 여전히 대기 중이다.
- Windows Release 빌드와 변경 경로 회귀 테스트 통과. 커밋·푸시는 하지 않았다. D02~04 구조 통합 전체는 별도 작업이며 DEC-01(대혼란 전쟁군주 스택)과 DEC-02(Creative/DYING)는 미결이다.

## REG-02 / REG-03 재실행 — DEC-03 반영 후 (통과, 2026-10-13 09:5x)
- 외부 작업(09:24~09:34)이 DEC-03을 반영: 시련 최대 HP 감소를 모든 경로(새 런·교체 재계산·대혼란·미리보기)에서 카드 적용 전에 적용, 재시작의 별도 감소 제거, `baseFireInterval`을 모든 경로에서 무기 적용 후 값으로 기록. 관련 파일: `RunStats.h`, `PlayerStats.h`, `main.cpp`, `GameManager.cpp`, REG-02/03 테스트·스크립트(콤보 주입의 획득 기록 검사 추가), REG-04 스크립트, `verify_package_zip.py`.
- 파일이 17분간 변경 없음을 확인한 뒤 Release 재빌드 → REG-02/03 통과: 무기 2 × 메타 2 × 시련 2 × 배율 2 전 조합에서 4경로(새 런·재계산·재시작·미리보기)의 최대 HP·발사 간격·기본 발사 간격·공격력·탄속·시야가 모두 일치(예: SMG+메타+시련 113.4 / 0.10). REG-03 결과는 이전과 동일(DEC-01 대혼란 전쟁군주 미보존은 OPEN 유지).
- 테스트 18종 전부 통과, 게임 스모크 종료 코드 0.
- DEC-03 종료. 남은 결정: DEC-01, DEC-02, REG-04 OPEN 2건, REG-26 OPEN 1건.

## 결정 반영 — DEC-01, REG-04 2건, REG-26 1건 (2026-10-13 10시)

| 항목 | 결정·처리 | 근거 |
|---|---|---|
| DEC-01 대혼란 전쟁군주 스택 | **초기화**(사용자 결정). 현재 동작과 같아 코드 변경 없음 — REG-02/03 테스트·`RunStats.h` 주석의 OPEN을 결정 사항으로 변경 | REG-03: chaos warlord 0 |
| REG-04 ① 새 런에 입력 상태 잔존 | 외부 작업(09:30)이 반영: `ResetForNewGame`에서 `ResetPlayerInputEdges()` + `postPickGrace = 0` | REG-04: NEWRUN/RESTART/READY1 dashHeld 0·skillHeld 0·grace 0.00 |
| REG-04 ② 교체 재계산이 실드·시간정지·대시 쿨다운 소멸 | 외부 작업(09:30)이 반영: 재계산이 런타임·실드를 보존하고 다시 장착된 스킬의 쿨다운 복원(교체로 쿨다운 초기화 악용 방지), 교체로 빠진 시간정지·하이퍼포커스 타이머만 정지 | REG-04: REBUILD dashCd 1.50·timeStop 0.80·shield 25·timeCd 11.00 / REBUILD_NO_TIME timeStop 0 |
| REG-26 테마 인덱스·기록 값 | `SaveSystem.h` `LoadGame`: 최고 점수·누적 처치/판수·무기별 기록 음수 → 0, `themesel`은 `ACCENT_COUNT` 범위만 수용(보유 검사는 로드 직후 `ApplyAccentTheme`, main.cpp 889행) | REG-26: 범위 밖 값 전부 0/기본, 미보유 테마 1 → 적용 시 0, 보유(themeowned=3) 시 1 유지 |

정정: 직전 보고(REG-02/03 재실행)에서 REG-04 2건을 "남은 결정"으로 적었으나, 그 시점에 이미 외부 작업으로 반영·REG-04 통과 상태였음.

변경 파일 (sha256 앞8…뒤4):
- a87d944c…0d84 *WiNILL/System/SaveSystem.h
- 00495ad4…bd4b *tests/reg26_settings_test.cpp
- 000a7085…4e8c *tests/reg02_03_stat_builds_test.cpp
- fc5a28b3…f353 *WiNILL/Entity/RunStats.h
- e27a56e1…3fc6 *WiNILL/main.cpp
- 09c1b6f7…4201 *scripts/test_reg04_run_reset.ps1

검증: Release 빌드, 테스트 18종 전부 통과(REG-04는 실행 파일 일시 잠금으로 1회 재실행 — 7경로 통과), 게임 스모크 종료 코드 0.
남은 결정: DEC-02(Creative 흡혈·도감) — 설명 후 결정 대기.

## 커밋 확인 · macOS CI · DEC-02 반영 (2026-10-13 11시)

### 커밋 상태
- 이 점검표 작업(FIX-01~03, BEH-01/02, REG-01~06·26, 흡혈마 통합, DEC-01/03, REG-04·26 수정, 작업 기록)은 전부 `b8f6a08 release: prepare Onedow 0.1.18`(10-13 10:28)에 포함. 확인한 35개 파일 모두 추적 중, 커밋 후 수정 없음(REG-26 테마 검증·DEC-01 주석·`ApplyVampireKill`까지 내용 대조).
- 참고: 점검표의 "변경 ID별 별도 커밋" 원칙과 달리 릴리스 커밋 하나에 묶여 들어감.

### macOS CI (FIX-01 / FIX-03 남은 완료 조건)
- `b8f6a08` push의 macOS Build 런 38332579992: **성공** — 0.1.15 이후 첫 성공. Configure & build, Verify binary, Package, Verify package 모두 통과.
- `Verify package`: `PASS: bin/Onedow.zip - 75 files, all within the manifest`, `zipinfo` → `-rwxr-xr-x … unx … Onedow/macOS/Onedow`.
- FIX-01: macOS 빌드·바이너리 검증 충족 → 완료(잔여: Windows IME 실기 입력, macOS 실기 텍스트 입력은 수동 확인 항목).
- FIX-03: CI에서 실제 Info-ZIP 내용·실행 권한 확인 → 완료.

### DEC-02 Creative 흡혈·도감 — A안 (사용자 결정)
- `WiNILL/Entity/KillRewards.h`: 공통 수거(`CreditRegularKill`)의 `if (!s.creative)` 제거 → Creative에서도 5개 처치 경로 모두 흡혈·흡혈마 동일. 도감은 기존대로 Creative 처치 미집계(`RegisterCodexMobKill`).
- `KillRewardSink::creative`는 이제 쓰이지 않음. 제거하려면 `main.cpp`의 초기화 2곳도 고쳐야 하는데, 레벨업 작업이 `main.cpp`를 수정 중(10:58)이라 보류 — 해당 작업 종료 후 함께 정리.
- `tests/reg01_kill_rewards_test.cpp`: Creative 기대값을 로터·Scope × 3경로 모두 흡혈 +0.16·흡혈마 +1·도감 0으로 갱신, OPEN 표기 정리.
- 검증: REG-01 통과, kill_lifesteal·thrown_swarm_kill·REG-02/03·REG-05·REG-26 통과, Release 빌드 통과(레벨업 작업의 미커밋 변경 포함 상태).

### 남은 것
- `KillRewardSink::creative` 필드 제거(레벨업 작업 후, main.cpp 2줄).
- 결정 대기 항목 없음. 점검표 4장(D 항목 구조 정리)으로 진행 가능.
