# A 항목 관련 전역 쓰기와 alias 경로

2026-10-12. 기존 global_references.tsv는 읽기/쓰기 미구분 자료다. 이 문서는 **직접 쓰기, 메서드 내부 쓰기, 참조/포인터를 통한 쓰기, 읽기**를 구분하여 보완한다. 자동 후보의 정확한 현재 줄/코드는 `focus_write_candidates.tsv`에 기록한다. 함수 내부 람다는 lexical 목록에서 main으로 표시될 수 있어 아래에 호출 경로를 별도로 적었다.

## 1. 직접 쓰기와 수정 메서드

| 상태 | 쓰는 파일:함수/분기 | 변경 내용 | 관련 ID |
|---|---|---|---|
| g_Stats 전체/빌드 | main:RebuildPlayerStatsFromOwned, ProcessAugEffectQueue의 S_CHAOS, ResetForNewGame | PlayerStats() 재대입, 메타/무기 적용, windowSize/baseFireInterval, 보유 증강 재적용 | D02~04 |
| g_Stats 증강/누적 복원 | main:ApplySingleAugIdx / RebuildPlayerStatsFromOwned / S_CHAOS | Apply/ApplyCommonMultBoost, killCount/vampireKillStreak/mk2Used/lightStepDisableTimer 복원, 조건부 RestoreWarlordStacks | D02/03 |
| g_Stats 런 구성 | SceneRunConfig:FinalizeLoadout / main:재시작 분기 | ApplyWeapon, baseFireInterval, 시련 maxHP 배율 | D02 |
| g_Stats 창 크기/디버그 보정 | main:SyncPlayerBoundsSize / DebugToolkit Configure의 syncRuntime 콜백 | windowSize, maxHP의 유효값 보정 | D02/26, API 계약 |
| g_Stats 처치 누적 | main:creditKill / lOnKill / 레이저 일반·Scope 분기 | RegisterKill, vampireKillStreak 증가/임계 reset. RegisterKill은 추가로 전쟁군주 보너스 스탯도 변경 | D01/03, BEH |
| g_Stats 실행 상태 | main:MK2 발동 / RUNNING 타이머 처리 | mk2Used=true, lightStepDisableTimer 감소 | D03/04 |
| g_Combo | Juice.h:AddKillCombo / ResetJuice, main:콤보 만료 | ++/0 reset. HUD는 읽기 | D01/04 |
| g_ComboTimer/Pulse/Milestone | Juice.h:AddKillCombo / ResetJuice, main:콤보 타이머 | 시간창·pulse·milestone 설정/감소/0 보정 | D01/04 |
| g_RainKillAccum | Juice.h:AddKillCombo / ResetJuice, main:탄환 세례 갱신 | 처치당 +1 / 런 초기화0 / 쿨다운 반영 후0 | D01/04, BEH-02 |
| g_Skills/g_SkillReplaceIdx | SceneSkills:EquipSkill / ClearEquippedSkills / ReequipSkillsFromOwned; PlayerRuntime:ResetPlayerSkills / UpdatePlayerRuntimeTimers; main:스킬 발동 | 슬롯/type/cd/교체 인덱스 설정, timer 감소/발동 cooldown | D04/06 |

직접 쓰기 표에는 초기화 선언도 포함한다. `Apply*`, `RegisterKill`은 쓰기 메서드로 별도 표시했다. 전체 g_*를 자동으로 ‘쓰기’로 세지 않는다.

## 2. g_Stats가 이름을 바꾸어 수정되는 경로

| 원본 → 전달 경로 | 실제 쓰기 위치 | 확인 사항 |
|---|---|---|
| main의 g_Stats → CollisionSystem::Update(... PlayerStats& stats, ...) | CollisionSystem.h:Update의 stats.RegisterKill / vampireKillStreak ++·reset | 콤보·도감과 별개로 런 카운터/전쟁군주·흡혈마가 바뀐다. 일반/원거리 두 경로 모두 확인 |
| g_Stats.RegisterKill → PlayerStats::RegisterKill | PlayerStats.h:killCount ++ → 1000킬 임계 ApplyWarlordStack | warlordStacks와 damageMultiplier/fireInterval/bulletSpeed까지 연쇄 수정 |
| g_Stats.RestoreWarlordStacks → PlayerStats::RestoreWarlordStacks | warlordStacks=0 → 스택별 ApplyWarlordStack | 카운터만 복원하는 메서드가 아님. 수치 재적용 순서가 중요 |
| g_Stats → ApplyMeta(PlayerStats& s) / ApplyWeapon(PlayerStats& s) | Meta.h / Weapons.h의 s 필드 쓰기 | 같은 함수도 preview의 로컬 PlayerStats에 적용할 때는 전역 쓰기가 아니다. 호출자 입력으로 구분 |
| g_Stats → DebugToolkitContext.stats(&g_Stats) → context_.stats + offsetof | DebugToolkit.cpp:ApplyFieldEdit의 float/int/long long 재해석 포인터 대입, HandleNumericKeys의 bool 토글 | 이름 기반 g_Stats 검색으로 놓치는 실제 writer. 편집 뒤 syncRuntime 콜백이 추가 보정 수행 |
| g_Stats.lightStepDisableTimer → ApplyPlayerDamageProtection의 float& | PlayerRuntime.cpp:ApplyPlayerDamageProtection | 필드 주소를 넘기는 쓰기. 보호/회복 처리가 바꾸는 필드와 실제 HP 변경을 구분 |

g_Stats 참조를 소비하는 Get*/HUD/텔레메트리와 GameManager의 증강 자격 읽기는 쓰기로 세지 않는다. EnemySpawner/ProjectileSystem 등은 서명과 구현에서 const 여부를 대조한다. `const_cast`, 저장된 포인터, 매크로·복잡한 alias까지 완전한 데이터 흐름 분석을 수행한 것은 아니다.

## 3. D05/D26의 추가 쓰기 소유권

| 경로 | 상태/부수효과 | 이관 시 관찰 |
|---|---|---|
| main:AbandonCurrentRun / DYING → SaveSystem:RecordRunResult | g_LastRunRecord/Coins, 누적 게임/최고 기록·코인·도감 관련 저장, 업적 처리와 SaveGame | REG-05에서 writer 호출 횟수와 런 결과 snapshot을 비교. 결과 화면을 그리는 것만으로 재지급되면 안 됨 |
| SceneSettings의 편집/취소·main의 적용 동기화 → Settings/Audio | 원하는 옵션·snapshot 복원·Audio 내부 적용값·저장 파일 | REG-26에서 UI 값과 장치 적용값을 각각 관찰. SaveGame은 상태를 직렬화하고 LoadGame은 옵션/진행에 씀 |

위 두 영역은 모든 전역 필드의 쓰기 줄을 열거한 완전한 목록이 아니다. A 항목 선행 검증에서 추적할 writer/부수효과 경계를 명시한 표다. 세부 직렬화/옵션 전역의 완전한 writer 분석은 D26 설계 시 필드 명세와 함께 확장한다.

## 4. 자동 후보의 범위와 갱신

`generate_appendix.py`는 선택한 8개 전역의 직접 대입/복합 대입/증감, 알려진 g_Stats 수정 메서드 및 ApplyMeta/ApplyWeapon 호출을 추출한다. 비교식과 일반 읽기를 writer로 잡지 않는 작은 자체 검사를 포함한다. `focus_write_candidates.tsv`는 이 범위의 **후보**이며 위 alias 경로를 대신하지 않는다. 읽기만 하는 줄은 별도 global_references.tsv와 대조한다.

기준 소스가 바뀌면 audit_structure.py → generate_appendix.py를 재실행하고, 전달 인자/디버그 포인터/필드 참조 경로도 다시 검토한다. D01~D06의 완료 판정은 자동 쓰기 후보 개수 감소가 아니라 기대한 소유자와 검증 결과로 판단한다.
