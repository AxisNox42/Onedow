# macOS 빌드 실패 근거 — FIX-01 / A

2026-10-12 GitHub CLI로 원격 실행 목록과 실패 로그를 직접 읽었다. 로컬에서 macOS 빌드를 수행하거나 CI를 재실행하지는 않았다. 최신 5건은 모두 `failure`다. 원격 commit의 줄 번호와 현재 작업 트리의 줄 번호는 구분한다.

| 실행 | UTC 시작 / ref | SHA | 로그에서 확인한 오류 |
|---|---|---|---|
| [38211533566](https://github.com/AxisNox42/Onedow/actions/runs/38211533566) | 2026-10-12 06:14:03 / main, release 0.1.17 | abb8143630fc1cd38ff71bbeb833d80cb2affc31 | main.cpp:3964, swprintf 오버로드 불일치 |
| [37793287391](https://github.com/AxisNox42/Onedow/actions/runs/37793287391) | 2026-10-08 14:32:20 / main, release 0.1.16 | 36e42bac86230b7327bd6e48cd3d9792377bfe13 | main.cpp:3905, 같은 오류 |
| [37574220615](https://github.com/AxisNox42/Onedow/actions/runs/37574220615) | 2026-10-07 05:00:48 / main, release 0.1.15 | ff19d168de6e0b5fb7600284fbce77f183f318c7 | SceneAugment.cpp:832/834/836, 같은 매크로 확장 오류 |
| [37455405760](https://github.com/AxisNox42/Onedow/actions/runs/37455405760) | 2026-10-06 11:17:23 / main, release 1.1.15(실행 제목 그대로) | f9ad13542696395f6cd2c79c26b3eb76efac3463 | SceneAugment.cpp:832/834/836, 같은 오류 |
| [37401832718](https://github.com/AxisNox42/Onedow/actions/runs/37401832718) | 2026-10-06 01:58:18 / main, HUD/UI 수정 | 1b757a581f272f92db121e5974ce27f209b71128 | SceneAugment.cpp:795/797/799, 같은 오류 |

원본 실패 로그는 이 폴더의 `ci_<run_id>_failed.log` 5개에 보관했다. 확인 명령: `gh run list --repo AxisNox42/Onedow --workflow macos-build.yml --limit 5 --json databaseId,headSha,headBranch,displayTitle,conclusion,status,createdAt,url`, 각 ID의 `gh run view <id> --repo AxisNox42/Onedow --log-failed`.

최신 실행의 작업은 [build job 114686577293](https://github.com/AxisNox42/Onedow/actions/runs/38211533566/job/114686577293)이며 `Configure & build Onedow` 단계가 실패, `Verify binary`/`Package`/`Upload artifact`가 skipped다. 로그에서 main.cpp 컴파일은 9%, 마지막 C++ 컴파일 진행 출력은 ProjectileSystem.cpp의 18%다. 어느 로그에도 Input.cpp의 C++ 컴파일 시작은 나타나지 않는다. 따라서 첫 실패를 Input.cpp 때문이라고 기록할 수 없다.

직접 원인: 호출 `swprintf_s(buf, n, L"%d", ...)`에 대해 Platform.h의 `#define swprintf_s(buf, ...) swprintf((buf), sizeof(buf)/sizeof((buf)[0]), __VA_ARGS__)`가 크기 인자를 한 번 더 삽입한다. 확장 결과는 `swprintf(buf, 추론크기, n, format, ...)`이고 세 번째 인자가 포맷 문자열이어야 하는 표준 함수와 맞지 않는다. 포인터 인자에 sizeof를 적용하는 문제도 있으므로 한 호출 줄만 수정하기보다 배열/명시 크기 두 호출 계약을 검사해야 한다.

Input.cpp의 무조건 Windows native/IME include와 HWND/HIMC 코드는 **현재 소스의 별도 플랫폼 장애 근거**다. FIX-01은 (1) 문자열 포맷 호환, (2) 플랫폼 IME 경계 두 소단계로 나누되, 최신 SHA의 전체 macOS 빌드와 Windows 입력 회귀 검증까지 끝나야 완료한다. 아직 수정/재실행하지 않았다.
