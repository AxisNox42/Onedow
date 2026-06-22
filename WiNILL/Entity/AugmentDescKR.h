#pragma once

inline const wchar_t* AugDescKR(AugType t) {
    switch (t) {

    // ── Common ──
    case AugType::DMG_UP:
        return L"공격력 +9 (고정 가산) / 중첩마다 기본 피해가 누적 상승 / 초반·후반 모두 안정적인 화력 보강";
    case AugType::RATE_UP:
        return L"연사 속도 +4% (백분율 누적) / 중첩마다 발사 간격 단축 / 안정적인 화력 보강";
    case AugType::SPD_UP:
        return L"탄환 속도 +30 / 멀리 날아가는 탄의 실효 사거리·관통 체감 향상 / 중첩 가능";
    case AugType::MOVE_UP:
        return L"이동 속도 +5% (백분율 누적) / 회피·진입·후퇴 기동성 상승 / 중첩 가능";
    case AugType::VISION_UP:
        return L"시야(플레이어 창 크기) +70 / 최대 5중첩 (합 +350) / 넓은 시야로 적·탄·보상 탐지 유리";
    case AugType::REGEN_UP:
        return L"재생 +0.28/s (중첩 가능) / 약 3.5초마다 체력 1 회복 / 출혈 등 지속 피해를 서서히 상쇄";

    // ── Rare ──
    case AugType::GLASS_CANNON:
        return L"공격력 +50% / 최대 체력 -35% / 한 방 화력 극대화·생존 리스크 동반 (1회만 획득)";
    case AugType::LIGHT_AMMO:
        return L"연사 +10% · 탄속 +30% / 공격력 -20% / 가볍고 빠른 탄환 빌드 (1회만 획득)";
    case AugType::LIGHT_STEP:
        return L"이동 속도 +30% / 피격 시 10초간 보너스 해제 / 기동전·카이팅에 유리 (1회만 획득)";
    case AugType::GUN_RUNNER:
        return L"사격하지 않는 동안 이동 속도 +80% / 사격 중에는 효과 없음 / 포지셔닝·도주에 특화 (1회만 획득)";
    case AugType::BULLET_RAIN:
        return L"15초마다 유도탄 20발 일제 발사 / 발당 피해 50% / 주변 적 일괄 처리·군중 제어";
    case AugType::CRIT:
        return L"치명타 확률 +15%p (최대 75%) / 치명타 배율 ×2.0 / 행운형 화력 증폭";
    case AugType::LIFESTEAL:
        return L"처치당 HP 회복 (스택당 +0.06, 최대 4중첩) / 흡혈 한도 0.24 / 지속 전투 생존 보조";
    case AugType::BERSERK:
        return L"체력이 낮을수록 공격력 증가 / 빈사 시 최대 +60% / 위기 역전·고위험 고보상 (1회만 획득)";
    case AugType::OVERDRIVE:
        return L"공격력 +14 (고정 가산) / 희귀 등급 단일 스탯 부스트 / 안정적인 화력 상승";

    // ── Epic ──
    case AugType::CORE_OVERLOAD:
        return L"공격력 +24 (고정 가산) / 에픽 등급 고화력 가산 / 오버드라이브보다 큰 일회성 화력";
    case AugType::VAMPIRE:
        return L"흡혈 한도 0.48로 상향 / 최대 체력 +20 / 10킬마다 HP +1 (장기 생존·흡혈 빌드 핵심)";
    case AugType::BROKEN_SIGHT:
        return L"마우스 무시, 황금 오브 방향으로 자동 발사 / 공격력 +250% / 조준 부담↓·화력 극대 (픽 풀 제외 가능)";
    case AugType::BAYONET:
        return L"200px 이내 적에게 피해 +50% / 근접전·총검 빌드 / 거리형 증강과 중복 불가";
    case AugType::MINIATURIZE:
        return L"최대 체력 1/2 · 크기 -20% · 이동 +20% / 보유 증강 1개당 공격 +10·연사 +2% / 작고 빠른 스케일링 빌드";
    case AugType::GIGANTIFY:
        return L"최대 체력 ×2 · 크기 +50% / 재생 +1.0/s · 이동 -40% / 거대·탱커 생존 빌드";
    case AugType::PIERCE:
        return L"명중 시 30% 확률로 적·장애물 관통 / 밀집 적 관통 처리 / 관통 II·조합과 시너지";
    case AugType::TWIN:
        return L"한 번에 2발 동시 발사 / 공격력 -40% / 화력 밀도↑·단발 위력↓";
    case AugType::CHAKRAM:
        return L"넓게 공전하는 차크람 1개 / 잡몹·자폭병 즉사, 원거리 큰 피해·적탄 차단 / 파괴 시 6초 후 재생성";
    case AugType::BULLET_RAIN_2:
        return L"탄환 세례 쿨다운 15초 → 10초 / 선행: 탄환 세례 필요 / 세례 빈도 대폭 상승";
    case AugType::CHAKRAM_2:
        return L"차크람 1개 → 2개 / 선행: 차크람 필요 / 공전 커버리지·압박력 2배";
    case AugType::DRONE:
        return L"자동 조준 드론 1기 소환 / 플레이어 연사·탄속의 50% 화력 / 손 안 쏴도 지속 딜";
    case AugType::LASER:
        return L"0.85초마다 조준 방향 관통 레이저 (사거리 560) / 직선상 적 일소·군중 제어 / 오빗 빔 계열";
    case AugType::PURGE_NOVA:
        return L"2.4초마다 주변 정화 펄스 (반경 240) / 범위 내 적에 강력 피해 / 중첩 시 주기↓·범위↑";
    case AugType::LASER_2:
        return L"레이저 간격 0.85→0.55초 · 사거리 560→760 / 선행: 스캔 레이저 / 연사·원거리 레이저 강화";
    case AugType::PIERCE_2:
        return L"관통 확률 +30%p (최대 100%) / 선행: 관통 필요 / 관통 빌드 완성 단계";
    case AugType::TWIN_2:
        return L"동시 발사 2발 → 3발 (트리플 샷) / 선행: 더블 필요 / 화력 밀도 최대화";
    case AugType::MELEE_WIDE:
        return L"근접 공격 범위(호) 확대 / 사거리·부채꼴 커버리지 상승 / 근접 빌드 보조";
    case AugType::BLADE_WIND:
        return L"근접 공격마다 전방 관통 칼바람 발사 / 근접+원거리 견제 / 범위 딜 보강";
    case AugType::POWER_DRAW:
        return L"차징 속도 +40% / 완충(풀 차지) 위력 증가 / 강한 단발 화력";
    case AugType::MULTISHOT:
        return L"완충 발사 시 3발 부채꼴 / 단발 화력을 면제역으로 / 다탄 화력";
    case AugType::MINIGUN:
        return L"연사 +75% / 탄 퍼짐 +0.15 / 정조준·소총과 반대 축 — 근거리 탄막 특화";
    case AugType::HACK_RANGED:
        return L"원거리 몹 처치 시 20% 확률 / 유도탄 5발 추가 (적에게만 피해) / 원거리 격파 연쇄";
    case AugType::HACK_FIREWALL:
        return L"보호막체 처치 시 10% 확률 / 3초간 최대 체력 20% 보호막 / 해킹 생존 연계";
    case AugType::PROB_CHAIN:
        return L"명중 시 30% 확률로 가까운 적에게 튕김 / 최대 3회 연쇄 / 확률형 멀티히트";
    case AugType::DEATH_BLAST:
        return L"적 처치 시 주변 폭발 (공격력 30% 피해) / 폭발로 죽은 적은 재폭발 없음 / 연쇄 처치 유도";
    case AugType::SKILL_CLOSE:
        return L"[스킬] Q/E/R 빈 슬롯에 장착 / 플레이어 중심 대폭발 (넉백+피해, 반경 380) / 재사용 16초";
    case AugType::SKILL_OVERCLOCK:
        return L"[스킬] Q/E/R 빈 슬롯에 장착 / 5초간 시야 +50%·적·적탄 -30% 속도·대시 쿨 2초 / 재사용 20초";

    // ── Legendary ──
    case AugType::POWER_SURGE:
        return L"공격력 +5% (중첩) / 3스택까지 효율 좋음, 이후 스택은 +3% / 장기 화력 성장";
    case AugType::RANDOM_AUG:
        return L"등급 무관 랜덤 버프 3개 즉시 획득 / 디버프는 포함되지 않음 / 한 번에 빌드 다각화";
    case AugType::SOUL_HARVEST:
        return L"1500킬마다 공격력 +5%·연사 +2%·탄속 +2% (최대 7스택) / 장기전 스노우볼 성장 / 영구 누적";
    case AugType::MK2:
        return L"사망 시 공격력 비례 대폭발 + 풀 HP 부활 / 1회 한정·페널티 없음 / 최후의 안전장치";
    case AugType::HACK_BOMBER:
        return L"자폭병 처치 시 20% 확률로 폭발 / 적에게만 피해 / 자폭병 밀집 구역 연쇄 처리";
    case AugType::CHAIN:
        return L"모든 총알이 무조건 2회 튕김 (100%) / 총알 피해 -30% / 확실한 멀티히트·화력 트레이드오프";
    case AugType::SKILL_TIMESTOP:
        return L"[스킬] Q/E/R 빈 슬롯에 장착 / 1.5초간 적·적탄 정지 (본인은 행동 가능) / 재사용 28초";
    case AugType::BULLET_RAIN_3:
        return L"탄환 세례 쿨다운 10초 → 5초 / 선행: 탄환 세례 II / 고빈도 세례 완성";
    case AugType::DRONE_2:
        return L"드론 1기 → 2기 / 선행: 드론 필요 / 자동 화력 2배";
    case AugType::CHAKRAM_3:
        return L"차크람 2개 → 3개 / 선행: 차크람 II / 공전 방어막·압박 최대";

    // ── Debuff ──
    case AugType::D_RMOB_MAX:
        return L"원거리 몹 동시 존재 +1 · 스폰 0.5초 가속 / 원거리 처치 EXP +12 / 원거리 압박↑·보상↑";
    case AugType::D_RMOB_HP:
        return L"원거리 몹 체력·공격력 +20% / 원거리 처치 EXP +6 / 강한 원거리 적·경험치 보상";
    case AugType::D_RMOB_DELAY:
        return L"원거리 몹 이동 속도 +20% (최대 10중첩) / 원거리 처치 EXP +5 / 빠른 원거리 위협";
    case AugType::D_MOB_SPAWN:
        return L"잡몹 스폰 빈도 증가 · 동시 한도 +200 / 잡몹 처치 EXP +1 / 화면 혼잡·경험치 소량 보상";
    case AugType::D_APPROACH:
        return L"무적 빨간 오브가 영원히 추격 / 초당 EXP +0.5 / 중복 시 오브 속도 +20% — 고위험·고보상";
    case AugType::D_MOB_SPEED:
        return L"잡몹 이동 속도 +10% / 초당 EXP +0.5 / 빠른 잡몹 러시·지속 경험치 보상";
    case AugType::D_GLASS_HEART:
        return L"최대 체력 -20% / 전체 EXP +5% / 얇은 체력·빠른 성장 트레이드오프";
    case AugType::D_BULLET_STUCK:
        return L"연사 속도 -20% / 전체 EXP +5% / 느린 사격·경험치 보상";
    case AugType::D_DRUNK:
        return L"20초마다 5초간 조준 랜덤·피해 -40% / 전체 EXP +5% / 중복: 지속 +1초·쿨 -2초";
    case AugType::D_BOMBER_BLAST:
        return L"자폭병 폭발 반경 ×1.5 / 자폭병 처치 EXP +12 / 넓은 폭발·높은 보상";
    case AugType::D_BOMBER_BUFF:
        return L"자폭병 체력 ×1.5 / 자폭병 처치 EXP +5 / 단단한 자폭병";
    case AugType::D_BOMBER_SPEED:
        return L"자폭병 이동 속도 +30% / 자폭병 처치 EXP +3 / 빠른 접근 자폭병";
    case AugType::D_MOB_HP:
        return L"잡몹 체력 +45% / 잡몹 처치 EXP +2 / 단단한 잡몹·보상 소폭 상향";
    case AugType::D_SLOW_MOVE:
        return L"플레이어 이동 속도 -5% / 전체 EXP +10% / 둔한 기동·높은 경험치";
    case AugType::D_SPLITTER:
        return L"일부 프로세스가 웜으로 변이 / 처치 시 작은 2마리로 분열 (2세대까지) / 처치 EXP +2";
    case AugType::D_SPLITTER_BOOST:
        return L"웜 분열 3세대까지 · 분열된 개체마다 처치 보상 별도 / 디버프 EXP 보너스 없음 / 선행: 웜 침투";
    case AugType::D_BLINKER:
        return L"일부 프로세스가 트로이목마로 변이 / 잔상 예고 후 순간이동 추격 / 처치 EXP +3";
    case AugType::D_ORBITER:
        return L"일부 프로세스가 스파이웨어로 변이 / 주위를 돌며 서서히 좁혀옴 / 처치 EXP +5";
    case AugType::D_SPAWNER:
        return L"일부 프로세스가 봇넷으로 변이 / 작은 프로세스를 계속 소환 / 처치 EXP +7";
    case AugType::D_SHIELDED:
        return L"일부 프로세스가 방화벽으로 변이 / 방패 ON 시 피해 대폭 감소·주기적 OFF / 처치 EXP +5";
    case AugType::D_BLEED:
        return L"초당 체력 0.8 감소 (재생으로 상쇄 가능) / 전체 EXP +12% / 지속 피해·높은 경험치";
    case AugType::D_WEAKEN:
        return L"공격력 -12% / 전체 EXP +10% / 약화·경험치 보상";
    case AugType::D_MOB_PACK:
        return L"잡몹 스폰 시 추가 +2마리 동시 등장 / 잡몹 처치 EXP +6 / 개체 수 폭증·보상↑";
    case AugType::D_MOB_ELITE:
        return L"엘리트 변종(신속/강인/폭발) 출현 확률 대폭↑ / 잡몹 처치 EXP +3 / 변종 난이도·보상";
    case AugType::D_MOB_FRENZY:
        return L"특수 잡몹(돌진/회피/거대) 출현 확률↑ / 잡몹 처치 EXP +4 / 다양한 위협·보상";

    // ── Special ──
    case AugType::S_CHAOS:
        return L"보유 증강을 모두 잊고 같은 개수만큼 랜덤 재배분 / 약 60% 버프 · 40% 디버프 / 빌드 전면 리셋·도박";
    case AugType::S_PANDORA:
        return L"버프 3개 + 디버프 2개 즉시 획득 / 한 번에 대량 변동 / 고위험 고보상 선택";

    // ── Combo ──
    case AugType::CB_EXECUTIONER:
        return L"[조합] 치명타+광전사 / 치명타 확률 +30%p·배율 +1.2·공격력 +20% / 처형·크리 빌드 완성";
    case AugType::CB_BLOODLORD:
        return L"[조합] 흡혈탄 II+흡혈마 / 최대 체력 +15 · 재생 +0.25/s / 피의 군주 생존 패키지";
    case AugType::CB_PIERCE_TWIN:
        return L"[조합] 더블+관통 / 관통 확률 60% · 공격력 +40% / 더블 패널티 상쇄·관통 쌍둥이";
    case AugType::CB_STORMCALLER:
        return L"[조합] 탄환 세례+드론 / 세례 쿨 4초 · 드론 +1 · 연사 +15% / 폭풍 오빗 화력";
    case AugType::CB_RAILGUN:
        return L"[조합] 저격+관통+관통 II / 관통 +15%p · 거리 보너스 +20%p · 공격 +30% · 탄속 +35% / 원거리 관통 일점사";
    case AugType::CB_GLASS_REAPER:
        return L"[조합] 유리대포+흡혈탄 / 공격 ×1.2 · 처치당 흡혈 +0.2 · 최대 HP +20 / 유리 리스크 완화";
    case AugType::CB_WARLORD:
        return L"[조합] 광전사+연쇄폭발+연쇄 II / 공격력 +15% · 영혼 수확 능력 부여 / 장기전 스케일링 군주";
    case AugType::CB_TEMPEST:
        return L"[조합] 차크람+드론 / 차크람 +1 · 드론 +1 · 연사 +10% / 공전 오케스트라 난기류";
    case AugType::CB_OVERLORD:
        return L"[조합] 오버드라이브+코어 과부하 / 공격 +35 (가산) · 공격 ×1.12 / 과부하 군주 화력";
    case AugType::CB_HELLFIRE:
        return L"[조합] 연쇄 폭발+탄환 세례 / 폭발 반경 ×1.6 · 세례 쿨 5초 / 지옥불 연쇄·세례";
    case AugType::CB_TURRET:
        return L"[조합] 대포+드론 II+HE탄 / 드론 공전 대신 자동 포탑 배치 / 1초마다 배치·5초 지속·소총 화력";
    case AugType::CB_BASTION:
        return L"[조합] 거대화+MK2+방화벽 / 최대 HP +15% · 재생 +0.35/s · 받는 피해 -8%p / 철벽 생존";
    case AugType::CB_LIFEBUOY:
        return L"[조합] 재생 II+흡혈마+가벼운 발걸음 / 재생 +0.25/s · 이동 +12% · 7킬마다 HP+1 / 피격 시 가벼운 발걸음 6초 정지";

    case AugType::MINIGUN_2:
        return L"연사 +12%p / 탄 퍼짐 -25% / 선행: 미니건 / 제어 조금 나아지지만 여전히 산탄기";
    case AugType::MINIGUN_CYCLONE:
        return L"명중마다 연사 쿨 0.1초 단축(누적 최대 0.6초) / 선행: 미니건 II / 관통 없음·순수 연사 스노우볼";
    case AugType::DEATH_BLAST_2:
        return L"처치 폭발 피해 30%→40% · 반경 +40% / 선행: 연쇄 폭발 / 연쇄폭발 완성";
    case AugType::CB_TANWOO:
        return L"[조합] 미니건+관통 II / 관통 70% · 연사 +15% / 탄막 관통 빌드";

    // ── Mythic ──
    case AugType::BULLET_RAIN_ETERNAL:
        return L"탄환 세례 쿨 최대 8초 / 적 처치마다 쿨다운 0.4초 감소 / 몰아칠수록 세례가 더 자주 — 무한 세례";
    case AugType::DRONE_HIVE:
        return L"드론 최대 2기 고정 / 초고속 사격 (발사 간격 대폭 단축) / 군집 지능 오토 화력";
    case AugType::LASER_CONVERGE:
        return L"스캔 레이저 거의 연속 발사 (0.18초) / 초장거리·광폭 빔 / 직선 일소 수렴";
    case AugType::PIERCE_RAILSLUG:
        return L"관통 확률 90% 고정 / 공격 +25 (가산) · 탄속 +50% / 멈추지 않는 철갑탄";
    case AugType::CHAKRAM_SINGULARITY:
        return L"차크람이 적을 끌어당김 · 접촉 지속 피해 / 선행: 차크람 III / 특이점 중력·DoT";

    // ── Debuff (extended) ──
    case AugType::D_SCHEDULER:
        return L"특수 잡몹(돌진/회피/거대 등) 체력 +10% (중첩) / 잡몹 처치 EXP +2 / 스케줄러 강화";
    case AugType::D_TROJAN_BOOST:
        return L"[트로이목마 침투 보유 시] 트로이 점멸 쿨다운 단축 / 잡몹 처치 EXP +2 / 더 빠른 순간이동";
    case AugType::D_CRASHER_BOOST:
        return L"크래셔 돌진 중 받는 피해 -10% / 잡몹 처치 EXP +4 / 단단한 돌진체";
    case AugType::D_BADSECTOR:
        return L"배드 섹터 프로세스 출현 / 처치 시 잠시 감속 구역 잔류 / 잡몹 처치 EXP +8";
    case AugType::D_REGERROR:
        return L"레지스트리 에러 노드 출현 / 주변 적 강화 오라 / 잡몹 처치 EXP +10";
    case AugType::D_DDOS:
        return L"디도스 프로세스 창(스웜) 출현 / 다수 소형 위협 / 잡몹 처치 EXP +4";
    case AugType::D_WEAVER_BOOST:
        return L"회피체(위버) 지그재그·속도 +15% / 잡몹 처치 EXP +3 / 회피 난이도↑";
    case AugType::D_BRUTE_BOOST:
        return L"거대체 HP +25% · 접촉 피해 +20% / 잡몹 처치 EXP +4 / 브루트 강화";

    // ── Tier / Weapon / Skill ──
    case AugType::LIFESTEAL_2:
        return L"흡혈 한도 0.24 → 0.36 / 10킬마다 HP +1 (흡혈마와 연동) / 선행: 흡혈탄 (1회만)";
    case AugType::CHAIN_2:
        return L"튕김 2회 → 3회 · 총알 피해 -30% → -20% / 선행: 연쇄 작용 / 연쇄 II 완성";
    case AugType::SHOTGUN_SPREAD:
        return L"[샷건] 펠릿 5→7발 / 사거리 700→630 (-10%) / 산탄 확장·근거리 화력";
    case AugType::REVOLVER_OVERLOAD:
        return L"[리볼버] 6발 장전 / 6번째 탄 치명타 (은탄환 보유 시 화상으로 대체) / 과장전";
    case AugType::REVOLVER_SILVER:
        return L"[리볼버] 6번째 탄 은탄환 — 명중 대상에 공격력 120% 화상(1초) / 선행: 과장전";
    case AugType::HE_SHELLS:
        return L"[대포] 관통 종료 시 소형 폭발 (공격력 25%, 반경 80) / 관통 끝마다 범위 피해";
    case AugType::HE_SHELLS_2:
        return L"[대포] 폭발 25%→35% · 반경 80→110 / 선행: HE탄 / 대포 II";
    case AugType::SKILL_FOCUS:
        return L"[스킬] Q/E/R 슬롯 · [저격] 전용 / 0.4초 정지 후 다음 1발 ×2.5·관통 +30%p / 재사용 14초";
    case AugType::SKILL_DASH_UP:
        return L"[스킬] SHIFT 대시 강화 / 대시 시 유도탄 3~5발 · 이후 3발 ×2 피해 / 기동+화력 연계";
    case AugType::SMG_COMPRESSOR:
        return L"[SMG] 탄 흩어짐 -50% / 연사 +8% / 근거리 탄막 정밀도·연사";
    case AugType::RIFLE_STABILITY:
        return L"[소총] 흩어짐 완전 제거 / 공격력 +12 (가산) / 정조준 원거리 화력";
    case AugType::SNIPER_AMPLIFIER:
        return L"[저격] 거리 보너스 +30%p / 저격총·저격 증강과 시너지 / 장거리 일격 강화";

    // ── Survival ──
    case AugType::HP_UP:
        return L"최대 체력 +15 (중첩 가능) / 생존 여유·흡혈·재생 효율 상승 / 기본 탱킹";
    case AugType::FIREWALL:
        return L"받는 피해 -12%p (중첩 가능) / 감소 효율 점감·최대 35% / 방어 스택 (1회만)";
    case AugType::REGEN_2:
        return L"재생 +0.45/s / 체력 40% 이하 시 재생 ×2 / 재생 빌드 핵심 (1회만)";

    default:
        return nullptr;
    }
}
