# 제네시스 협공 출격 QA

검증일: 2026-10-18

이전 협공 버전 보관 자료다. 현재 협공은 로터 연속 출격으로 교체되었으며 아래 전용 협공 회귀와 스모크 단계는 현재 코드에서 제거했다. 최신 검증은 `../Rotor_v1/`을 본다.

- Windows x64 Release 빌드 성공. 기존 `APIENTRY` 매크로 재정의 경고 있음.
- 전용 회귀: `scripts/test_genesis_assault.ps1` 통과.
- 기존 투척, 소환체 처치 보상, 퀘이사 폭격·빔 회귀 통과.
- 1440×900, 생산 시작 0.00–4.48초를 0.08초 간격으로 실제 렌더러에서 캡처.
- 최종 빌드에서 1280×720도 생산·잠금·돌격·회복 네 시점으로 추가 스모크 확인. 경로가 경계에 맞춰 반대쪽 배치를 선택하며 두 분대와 통로를 유지했다.
- `Assault_Preview.gif`: 57개 실제 캡처, 각 프레임 80ms. 중간 프레임 합성 없음.
- `Motion_ContactSheet.png`: 개방·생산·전개·잠금·돌격·회복의 대표 프레임.

확인한 동작: 세 외피의 개방·복귀, 외피당 두 차례 생산, 3+3 분대 전개, 고정된 두 경로, 완만한 가속·감속, 잠금 뒤 방향 유지. 생성 시 크기 변화와 추적 복귀 시 방향·속도 보간을 적용했다. 본체 사망 시 잔상 샘플을 현재 위치로 초기화하는 것은 전용 회귀에서도 확인한다.

재현 예:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke_ui.ps1 -Steps 'GENESISASSAULT=1.2;GENESISASSAULT=2.1;GENESISASSAULT=3.45;GENESISASSAULT=4.3' -Tag genesis_review -Size 1440x900
python scripts/preview_genesis_assault.py build/smoke/genesis_assault_verified_v1 Docs/QA/Genesis_Elite/Assault_v1
```

검증 한계: 고정된 플레이어를 쓰는 재현 장면이다. 혼합 웨이브의 체감 난이도, 큰 플레이어 빌드의 통로 여유, 이동·사격을 병행하는 실전 난이도를 확정하지 않는다. 피해·취소·격추·시간 정지는 자동 회귀에서 별도로 검증한다.
