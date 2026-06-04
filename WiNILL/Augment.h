#pragma once
#include <cstdlib>
#include "Settings.h"   // g_Language, LANG_COUNT

enum class AugType {
    // ── 일반 (7) ──────────────────────────────
    DMG_UP, RATE_UP, SPD_UP, MOVE_UP, VISION_UP, REGEN_UP,
    // ── 희귀 ──
    GLASS_CANNON, LIGHT_AMMO, LIGHT_STEP, GUN_RUNNER,
    BULLET_RAIN,
    CANNON,                // 전설 → 희귀
    CRIT, LIFESTEAL, BERSERK,   // 핵앤슬래쉬 (치명타/흡혈/광전사)
    // ── 에픽 ──
    VAMPIRE, BROKEN_SIGHT, SNIPER, BAYONET,
    MINIATURIZE, GIGANTIFY, PIERCE, TWIN, CHAKRAM,
    BULLET_RAIN_2, CHAKRAM_2,
    DRONE,                 // 희귀 → 에픽
    MINIGUN, HACK_RANGED, SHOTGUN,    // 신규
    PROB_CHAIN,            // 확률적 연쇄 작용 (30% 3튕김)
    DEATH_BLAST,           // 연쇄 폭발 (적 사망 시 폭발)
    SKILL_CLOSE, SKILL_OVERCLOCK,     // 액티브 스킬 (창 닫기 / 과부하)
    // ── 전설 ──
    RANDOM_AUG, SOUL_HARVEST,
    BULLET_RAIN_3, DRONE_2, CHAKRAM_3,
    MK2, HACK_BOMBER,                  // 신규
    CHAIN,                 // 연쇄 작용 (무조건 2튕김, -30% 데미지)
    SKILL_TIMESTOP,        // 액티브 스킬 (시간 정지)
    // ── 디버프 ──
    D_RMOB_MAX, D_RMOB_HP, D_RMOB_DELAY,
    D_MOB_SPAWN, D_APPROACH, D_MOB_SPEED,
    D_GLASS_HEART, D_BULLET_STUCK, D_DRUNK,
    D_BOMBER_BLAST, D_BOMBER_BUFF, D_BOMBER_SPEED,   // 신규
    D_MOB_HP, D_SLOW_MOVE,                            // 신규
    D_SPLITTER, D_BLINKER,                            // 신규 적 — 분열체 / 점멸체
    D_ORBITER, D_SPAWNER, D_SHIELDED,                 // 신규 적 — 공전체 / 소환체 / 보호막체
    D_BLEED, D_WEAKEN,                                // 출혈 / 약화
    // ── 특수 (2) ──────────────────────────────
    S_CHAOS, S_PANDORA,
    // ── 조합 (COMBO) — 레시피 충족 시에만 등장. 일반 추첨 X ──
    CB_EXECUTIONER,   // 치명타 + 광전사
    CB_BLOODLORD,     // 흡혈탄 + 흡혈마
    CB_PIERCE_TWIN,   // 더블 + 관통
    CB_STORMCALLER,   // 탄환세례 + 드론
    // ── deprecated (ALL_AUGS 에서 제외, 코드는 남음) ──
    SIEGE_TANK, D_RMOB_DMG, XP_UP
};

enum class AugRarity { COMMON, RARE, EPIC, LEGENDARY, DEBUFF, SPECIAL, COMBO };

// 고유 카테고리 — 같은 카테고리 내에서 1개만 선택 가능
enum class AugUnique { NONE, SIZE, DISTANCE };

struct AugDef {
    AugType        type;
    AugRarity      rarity;
    AugUnique      unique;
    const char*    name;                  // 영문 코드명 (윈도우 타이틀/디버그)
    const wchar_t* locName[LANG_COUNT];   // [KR, EN, JP] 카드 이름
    const wchar_t* locDesc[LANG_COUNT];   // [KR, EN, JP] 카드 설명
};

// 현재 언어 인덱스 (범위 클램프)
inline int CurLangIdx() {
    int li = (int)g_Language;
    if (li < 0 || li >= LANG_COUNT) li = 0;
    return li;
}
inline const wchar_t* AugName(const AugDef& d) { return d.locName[CurLangIdx()]; }
inline const wchar_t* AugDesc(const AugDef& d) { return d.locDesc[CurLangIdx()]; }

static const AugDef ALL_AUGS[] = {
    // ── 일반 ───────────────────────────────────────────
    { AugType::DMG_UP,        AugRarity::COMMON,    AugUnique::NONE, "DMG+8%",
      { L"공격력 증가", L"Attack Up", L"攻撃力アップ" },
      { L"공격력 +8%  (곱연산 누적)", L"Attack +8%  (multiplicative)", L"攻撃力 +8%  (乗算で累積)" } },
    { AugType::RATE_UP,       AugRarity::COMMON,    AugUnique::NONE, "RATE+4%",
      { L"연사 속도 증가", L"Fire Rate Up", L"連射速度アップ" },
      { L"연사 속도 +4%", L"Fire rate +4%", L"連射速度 +4%" } },
    { AugType::SPD_UP,        AugRarity::COMMON,    AugUnique::NONE, "BSPD+30",
      { L"탄속 증가", L"Bullet Speed Up", L"弾速アップ" },
      { L"탄환 속도 +30", L"Bullet speed +30", L"弾速 +30" } },
    { AugType::MOVE_UP,       AugRarity::COMMON,    AugUnique::NONE, "MOVE+5%",
      { L"이동속도 증가", L"Move Speed Up", L"移動速度アップ" },
      { L"이동 속도 +5%", L"Move speed +5%", L"移動速度 +5%" } },
    { AugType::VISION_UP,     AugRarity::COMMON,    AugUnique::NONE, "VISION+70",
      { L"시야 증가", L"Vision Up", L"視界アップ" },
      { L"시야(창 크기) +70  /  최대 5중첩 (+350)",
        L"View (window) +70  /  max 5 stacks (+350)",
        L"視界(ウィンドウ) +70  /  最大5重 (+350)" } },
    { AugType::REGEN_UP,      AugRarity::COMMON,    AugUnique::NONE, "REGEN",
      { L"체력 재생", L"Regeneration", L"体力リジェネ" },
      { L"약 3초마다 체력 1 추가 회복  (중첩 가능)",
        L"Heal ~1 HP every 3s  (stackable)",
        L"約3秒ごとに体力1回復  (重複可)" } },

    // ── 희귀 ───────────────────────────────────────────
    { AugType::GLASS_CANNON,  AugRarity::RARE,      AugUnique::NONE, "GLASSCANNON",
      { L"유리대포", L"Glass Cannon", L"ガラスの大砲" },
      { L"공격력 +50%  /  최대 체력 -35%", L"Attack +50%  /  Max HP -35%", L"攻撃力 +50%  /  最大体力 -35%" } },
    { AugType::LIGHT_AMMO,    AugRarity::RARE,      AugUnique::NONE, "LIGHTAMMO",
      { L"가벼운 탄환", L"Light Ammo", L"軽量弾" },
      { L"연사 +10%  /  탄속 +30%  /  공격력 -10%",
        L"Fire rate +10%  /  speed +30%  /  Attack -10%",
        L"連射 +10%  /  弾速 +30%  /  攻撃力 -10%" } },
    { AugType::LIGHT_STEP,    AugRarity::RARE,      AugUnique::NONE, "LIGHTSTEP",
      { L"가벼운 발걸음", L"Light Step", L"軽い足取り" },
      { L"이동 속도 +50%  /  피격 시 10초간 이 효과 정지",
        L"Move speed +50%  /  disabled 10s when hit",
        L"移動速度 +50%  /  被弾時10秒間 無効" } },
    { AugType::GUN_RUNNER,    AugRarity::RARE,      AugUnique::NONE, "GUNRUNNER",
      { L"건 앤 러너", L"Gun Runner", L"ガンランナー" },
      { L"사격하지 않는 동안 이동 속도 +80%",
        L"Move speed +80% while not firing",
        L"非射撃中 移動速度 +80%" } },
    { AugType::BULLET_RAIN,   AugRarity::RARE,      AugUnique::NONE, "BULLETRAIN",
      { L"탄환 세례", L"Bullet Rain", L"弾幕の雨" },
      { L"15초마다 유도탄 20발 일제 발사  /  발당 데미지 50%",
        L"Every 15s fire 20 homing shots  /  50% dmg each",
        L"15秒ごとに誘導弾20発  /  1発50%ダメージ" } },
    { AugType::CRIT,          AugRarity::RARE,      AugUnique::NONE, "CRIT",
      { L"치명타", L"Critical Strike", L"クリティカル" },
      { L"25% 확률로 치명타 (피해 ×2.5)  (중첩 시 확률 +25%)",
        L"25% chance to crit (×2.5 dmg)  (stacks +25%)",
        L"25%でクリティカル (×2.5)  (重複で+25%)" } },
    { AugType::LIFESTEAL,     AugRarity::RARE,      AugUnique::NONE, "LIFESTEAL",
      { L"흡혈탄", L"Lifesteal", L"吸血弾" },
      { L"적 처치마다 체력 0.12 회복  (중첩 가능)",
        L"Heal 0.12 HP per kill  (stackable)",
        L"撃破毎に体力0.12回復  (重複可)" } },
    { AugType::BERSERK,       AugRarity::RARE,      AugUnique::NONE, "BERSERK",
      { L"광전사", L"Berserker", L"バーサーカー" },
      { L"체력이 낮을수록 공격력 증가  (최대 +60% · 빈사 시)",
        L"Lower HP = more attack  (up to +60% near death)",
        L"低HPほど攻撃力上昇  (瀕死時 最大+60%)" } },

    // ── 에픽 ───────────────────────────────────────────
    { AugType::VAMPIRE,       AugRarity::EPIC,      AugUnique::NONE, "VAMPIRE",
      { L"흡혈마", L"Vampire", L"吸血鬼" },
      { L"10킬마다 체력 +1  /  최대 체력 -10%  /  획득 즉시 현재 체력 -20%",
        L"+1 HP per 10 kills  /  Max HP -10%  /  -20% current HP on pickup",
        L"10キル毎に体力+1  /  最大体力 -10%  /  取得時 現体力 -20%" } },
    { AugType::BROKEN_SIGHT,  AugRarity::EPIC,      AugUnique::NONE, "BROKENSIGHT",
      { L"고장난 조준선", L"Broken Sight", L"壊れた照準" },
      { L"마우스 무시, 황금 오브 방향으로 자동 발사  /  공격력 +250%",
        L"Auto-fire toward golden orb (ignores mouse)  /  Attack +250%",
        L"マウス無視・金色オーブへ自動射撃  /  攻撃力 +250%" } },
    { AugType::BAYONET,       AugRarity::EPIC,      AugUnique::DISTANCE, "BAYONET",
      { L"총검", L"Bayonet", L"銃剣" },
      { L"200px 이내의 적에게 피해 +50%  (고유 · 거리)",
        L"+50% dmg to enemies within 200px  (unique · range)",
        L"200px以内の敵にダメージ +50%  (固有・距離)" } },
    { AugType::MINIATURIZE,   AugRarity::EPIC,      AugUnique::SIZE, "MINI",
      { L"축소화", L"Miniaturize", L"小型化" },
      { L"최대 체력 10 고정  /  보유 증강당 공격력 +10·연사 +2%  /  이속 +20%  /  크기 -20%  (고유 · 크기)  [원거리 한 방에 사망 주의]",
        L"Max HP fixed 10  /  per aug: ATK +10·rate +2%  /  move +20%  /  size -20%  (unique · size)  [one ranged hit can kill]",
        L"最大体力10固定  /  強化1つ毎 攻撃+10・連射+2%  /  移動+20%  /  サイズ-20%  (固有・サイズ)  [遠距離一撃死注意]" } },
    { AugType::GIGANTIFY,     AugRarity::EPIC,      AugUnique::SIZE, "GIGA",
      { L"거대화", L"Gigantify", L"巨大化" },
      { L"최대 체력 ×2  /  초당 체력 약 1.3 회복  /  이속 -40%  /  크기 +50%  (고유 · 크기)",
        L"Max HP ×2  /  ~1.3 HP/s regen  /  move -40%  /  size +50%  (unique · size)",
        L"最大体力 ×2  /  毎秒 約1.3回復  /  移動 -40%  /  サイズ +50%  (固有・サイズ)" } },
    { AugType::PIERCE,        AugRarity::EPIC,      AugUnique::NONE, "PIERCE",
      { L"관통", L"Pierce", L"貫通" },
      { L"명중 시 30% 확률로 적·장애물 관통",
        L"30% chance to pierce on hit",
        L"命中時30%で敵・障害物を貫通" } },
    { AugType::TWIN,          AugRarity::EPIC,      AugUnique::NONE, "TWIN",
      { L"더블", L"Twin", L"ダブル" },
      { L"한 번에 2발 발사  /  공격력 -40%", L"Fire 2 shots at once  /  Attack -40%", L"一度に2発発射  /  攻撃力 -40%" } },
    { AugType::CHAKRAM,       AugRarity::EPIC,      AugUnique::NONE, "CHAKRAM",
      { L"차크람", L"Chakram", L"チャクラム" },
      { L"넓게 공전하는 차크람 1개  /  닿은 잡몹·자폭병 즉사, 원거리 큰 피해·적탄 막기 (공전체 카운터)  /  파괴 시 6초 후 재생성",
        L"1 wide-orbiting chakram  /  instakills mobs/bombers, big dmg to gunners, blocks bullets (orbiter counter)  /  respawns 6s",
        L"広く公転するチャクラム1個  /  雑魚・自爆兵即死, 遠距離に大ダメージ・敵弾を防ぐ (公転体対策)  /  破壊後6秒で再生成" } },
    { AugType::BULLET_RAIN_2, AugRarity::EPIC,      AugUnique::NONE, "BULLETRAIN II",
      { L"탄환 세례 II", L"Bullet Rain II", L"弾幕の雨 II" },
      { L"탄환 세례 쿨다운 15초 → 10초  (요구: 탄환 세례)",
        L"Bullet Rain cooldown 15s → 10s  (req: Bullet Rain)",
        L"弾幕の雨 クールダウン 15秒 → 10秒  (要: 弾幕の雨)" } },
    { AugType::CHAKRAM_2,     AugRarity::EPIC,      AugUnique::NONE, "CHAKRAM II",
      { L"차크람 II", L"Chakram II", L"チャクラム II" },
      { L"차크람 1개 → 2개  (요구: 차크람)", L"1 → 2 chakrams  (req: Chakram)", L"1個 → 2個  (要: チャクラム)" } },
    { AugType::DRONE,         AugRarity::EPIC,      AugUnique::NONE, "DRONE",
      { L"드론", L"Drone", L"ドローン" },
      { L"자동 조준 드론 1기 소환  /  연사·탄속 50%",
        L"Summon 1 auto-aim drone  /  50% fire rate·speed",
        L"自動照準ドローン1機  /  連射・弾速50%" } },
    { AugType::MINIGUN,       AugRarity::EPIC,      AugUnique::NONE, "MINIGUN",
      { L"미니건", L"Minigun", L"ミニガン" },
      { L"연사 ×2  /  공격력 -30%  /  20% 확률 관통", L"Fire rate ×2  /  Attack -30%  /  20% pierce", L"連射 ×2  /  攻撃力 -30%  /  20%貫通" } },
    { AugType::HACK_RANGED,   AugRarity::EPIC,      AugUnique::NONE, "HACK_RANGED",
      { L"해킹: 원거리", L"Hack: Ranged", L"ハック: 遠距離" },
      { L"원거리 몹 처치 시 20% 확률로 유도탄 5발  (적에게만 피해)",
        L"On ranged-mob kill, 20% chance: 5 homing shots  (enemy-only dmg)",
        L"遠距離敵 撃破時20%で誘導弾5発  (敵のみ)" } },
    { AugType::PROB_CHAIN,    AugRarity::EPIC,      AugUnique::NONE, "PROBCHAIN",
      { L"확률적 연쇄 작용", L"Chance Ricochet", L"確率的連鎖" },
      { L"명중 시 30% 확률로 가까운 적에게 튕김  (최대 3회 · 뮤탈 글레이브)",
        L"On hit, 30% chance to ricochet to nearest enemy  (up to 3x)",
        L"命中時30%で近い敵に跳弾  (最大3回)" } },
    { AugType::DEATH_BLAST,   AugRarity::LEGENDARY, AugUnique::NONE, "DEATHBLAST",
      { L"연쇄 폭발", L"Death Blast", L"連鎖爆発" },
      { L"적 처치 시 폭발 — 주변 적에게 공격력 30% 피해 (폭발로 죽은 적은 추가 폭발 X)",
        L"On kill, explode — 30% ATK dmg nearby (blast-killed don't re-explode)",
        L"撃破時に爆発 — 周囲に攻撃力30%ダメージ (爆死は再爆発しない)" } },
    { AugType::SKILL_CLOSE,   AugRarity::EPIC,      AugUnique::NONE, "SKILL_CLOSE",
      { L"[스킬] 창 닫기", L"[Skill] Close Window", L"[スキル] ウィンドウを閉じる" },
      { L"액티브 스킬 획득 — 플레이어 중심 대폭발(넉백+피해)  (쿨 16초)",
        L"Active skill — explosion at the player (knockback+dmg)  (16s CD)",
        L"アクティブスキル — 自分中心の大爆発(ノックバック+ダメージ)  (CD16秒)" } },
    { AugType::SKILL_OVERCLOCK,AugRarity::EPIC,     AugUnique::NONE, "SKILL_OVCLK",
      { L"[스킬] 과부하", L"[Skill] Overclock", L"[スキル] オーバークロック" },
      { L"액티브 스킬 획득 — 5초간 연사 ×2 · 공격력 +50%  (쿨 20초)",
        L"Active skill — 5s: fire rate x2, attack +50%  (20s CD)",
        L"アクティブスキル — 5秒間 連射×2・攻撃+50%  (CD20秒)" } },

    // ── 전설 ───────────────────────────────────────────
    { AugType::RANDOM_AUG,    AugRarity::LEGENDARY, AugUnique::NONE, "RANDOM",
      { L"랜덤 증강", L"Random Augment", L"ランダム強化" },
      { L"등급 무관 랜덤 버프 3개 즉시 획득  (디버프 없음)",
        L"Instantly gain 3 random buffs (any rarity, no debuff)",
        L"等級無関係のバフ3個を即獲得  (デバフ無し)" } },
    { AugType::SOUL_HARVEST,  AugRarity::LEGENDARY, AugUnique::NONE, "SOULHARVEST",
      { L"영혼 수확", L"Soul Harvest", L"魂の収穫" },
      { L"100킬마다 공격력 +5% · 연사 +2% · 탄속 +2%  (영구 누적)",
        L"Per 100 kills: ATK +5% · rate +2% · speed +2%  (permanent)",
        L"100キル毎に 攻撃+5%・連射+2%・弾速+2%  (永続累積)" } },
    { AugType::MK2,           AugRarity::LEGENDARY, AugUnique::NONE, "MK2",
      { L"MK2 내장", L"MK2 Core", L"MK2 内蔵" },
      { L"사망 시 공격력 비례 대폭발 + 풀 HP 부활  (1회, 페널티 없음)",
        L"On death: ATK-scaled blast + full-HP revive  (once, no penalty)",
        L"死亡時 攻撃力比例の大爆発+全回復で復活  (1回・ペナルティ無)" } },
    { AugType::HACK_BOMBER,   AugRarity::LEGENDARY, AugUnique::NONE, "HACK_BMBR",
      { L"해킹: 자폭병", L"Hack: Bomber", L"ハック: 自爆兵" },
      { L"자폭병 처치 시 20% 확률로 폭발  (적에게만 피해)",
        L"On bomber kill, 20% chance to explode  (enemy-only dmg)",
        L"自爆兵 撃破時20%で爆発  (敵のみ)" } },
    { AugType::CHAIN,         AugRarity::LEGENDARY, AugUnique::NONE, "CHAIN",
      { L"연쇄 작용", L"Chain Reaction", L"連鎖反応" },
      { L"모든 총알이 무조건 2회 튕김  /  총알 공격력 -30%",
        L"All bullets always ricochet 2x  /  bullet damage -30%",
        L"全弾が必ず2回跳弾  /  弾ダメージ -30%" } },
    { AugType::SKILL_TIMESTOP,AugRarity::LEGENDARY, AugUnique::NONE, "SKILL_TSTOP",
      { L"[스킬] 시간 정지", L"[Skill] Time Stop", L"[スキル] 時間停止" },
      { L"액티브 스킬 획득 — 1.5초간 적·적탄막 정지 (나는 계속 행동)  (쿨 28초)",
        L"Active skill — freeze enemies & enemy bullets 1.5s  (28s CD)",
        L"アクティブスキル — 1.5秒 敵と敵弾を停止  (CD28秒)" } },
    { AugType::BULLET_RAIN_3, AugRarity::LEGENDARY, AugUnique::NONE, "BULLETRAIN III",
      { L"탄환 세례 III", L"Bullet Rain III", L"弾幕の雨 III" },
      { L"탄환 세례 쿨다운 10초 → 5초  (요구: 탄환 세례 II)",
        L"Bullet Rain cooldown 10s → 5s  (req: Bullet Rain II)",
        L"弾幕の雨 クールダウン 10秒 → 5秒  (要: 弾幕の雨 II)" } },
    { AugType::DRONE_2,       AugRarity::LEGENDARY, AugUnique::NONE, "DRONE II",
      { L"드론 II", L"Drone II", L"ドローン II" },
      { L"드론 1기 → 2기  (요구: 드론)  /  [조합] 대포 보유 시 포탑 배치로 전환",
        L"1 → 2 drones  (req: Drone)  /  [combo] with Cannon: deploys turret",
        L"1機 → 2機  (要: ドローン)  /  [組合] 大砲所持で砲台配置に変化" } },
    { AugType::CHAKRAM_3,     AugRarity::LEGENDARY, AugUnique::NONE, "CHAKRAM III",
      { L"차크람 III", L"Chakram III", L"チャクラム III" },
      { L"차크람 2개 → 3개  (요구: 차크람 II)", L"2 → 3 chakrams  (req: Chakram II)", L"2個 → 3個  (要: チャクラム II)" } },

    // ── 디버프 ─────────────────────────────────────────
    { AugType::D_RMOB_MAX,    AugRarity::DEBUFF,    AugUnique::NONE, "D_RMOBMAX",
      { L"원거리 몹 증원", L"Ranged Reinforce", L"遠距離増援" },
      { L"원거리 몹 최대 수 +1 · 스폰 0.5초 빨라짐 · 원거리 처치 EXP +12",
        L"Ranged max +1 · spawn 0.5s faster · ranged-kill EXP +12",
        L"遠距離敵 最大+1・スポーン0.5秒短縮・遠距離撃破EXP +12" } },
    { AugType::D_RMOB_HP,     AugRarity::DEBUFF,    AugUnique::NONE, "D_RMOBPOW",
      { L"원거리 몹 강화", L"Ranged Empower", L"遠距離強化" },
      { L"원거리 몹 체력·공격력 +20% · 원거리 처치 EXP +12",
        L"Ranged HP·ATK +20% · ranged-kill EXP +12",
        L"遠距離敵 体力・攻撃+20%・遠距離撃破EXP +12" } },
    { AugType::D_RMOB_DELAY,  AugRarity::DEBUFF,    AugUnique::NONE, "D_RMOBDELAY",
      { L"원거리 몹 가속", L"Ranged Haste", L"遠距離加速" },
      { L"원거리 몹 이동 속도 +20% · 원거리 처치 EXP +5",
        L"Ranged move +20% · ranged-kill EXP +5",
        L"遠距離敵 移動+20%・遠距離撃破EXP +5" } },
    { AugType::D_MOB_SPAWN,   AugRarity::DEBUFF,    AugUnique::NONE, "D_MOBSPAWN",
      { L"잡몹 폭주", L"Mob Surge", L"雑魚暴走" },
      { L"잡몹 생성 빈도 증가 · 동시 한도 +200 · 처치 EXP +1",
        L"Mobs spawn more often · cap +200 · kill EXP +1",
        L"雑魚の出現頻度増加・同時上限+200・撃破EXP +1" } },
    { AugType::D_APPROACH,    AugRarity::DEBUFF,    AugUnique::NONE, "D_APPROACH",
      { L"다가오는 죽음", L"Approaching Death", L"迫りくる死" },
      { L"무적 빨간 오브가 영원히 추격 · 초당 EXP +0.5  (중복 시 오브 속도 +20%)",
        L"An invincible red orb chases forever · EXP +0.5/s  (stacks: orb +20% speed)",
        L"無敵の赤いオーブが永遠に追跡・毎秒EXP +0.5  (重複でオーブ速度+20%)" } },
    { AugType::D_MOB_SPEED,   AugRarity::DEBUFF,    AugUnique::NONE, "D_MOBSPD",
      { L"잡몹 가속", L"Mob Haste", L"雑魚加速" },
      { L"잡몹 이동 속도 +10% · 초당 EXP +0.5",
        L"Mob move speed +10% · EXP +0.5/s",
        L"雑魚の移動+10%・毎秒EXP +0.5" } },
    { AugType::D_GLASS_HEART, AugRarity::DEBUFF,    AugUnique::NONE, "D_GLASS",
      { L"유리 심장", L"Glass Heart", L"ガラスの心臓" },
      { L"최대 체력 -20% · 전체 EXP +5%", L"Max HP -20% · all EXP +5%", L"最大体力 -20%・全EXP +5%" } },
    { AugType::D_BULLET_STUCK,AugRarity::DEBUFF,    AugUnique::NONE, "D_STUCK",
      { L"총알 걸림", L"Jammed", L"弾詰まり" },
      { L"연사 속도 -20% · 전체 EXP +5%", L"Fire rate -20% · all EXP +5%", L"連射速度 -20%・全EXP +5%" } },
    { AugType::D_DRUNK,       AugRarity::DEBUFF,    AugUnique::NONE, "D_DRUNK",
      { L"취함", L"Drunk", L"酩酊" },
      { L"20초마다 5초간 조준이 랜덤 방향 · 전체 EXP +10%  (중복: 지속 +1초·쿨 -2초)",
        L"Every 20s, aim goes random for 5s · all EXP +10%  (stacks: +1s dur·-2s cd)",
        L"20秒毎に5秒間 照準がランダム・全EXP +10%  (重複: 持続+1秒・CD-2秒)" } },
    { AugType::D_BOMBER_BLAST,AugRarity::DEBUFF,    AugUnique::NONE, "D_BMB_BLAST",
      { L"자폭병 폭발 확장", L"Bigger Blast", L"自爆範囲拡大" },
      { L"자폭병 폭발 반경 ×1.5 · 자폭병 처치 EXP +12",
        L"Bomber blast radius ×1.5 · bomber-kill EXP +12",
        L"自爆兵の爆発範囲 ×1.5・自爆兵撃破EXP +12" } },
    { AugType::D_BOMBER_BUFF, AugRarity::DEBUFF,    AugUnique::NONE, "D_BMB_BUFF",
      { L"자폭병 강화", L"Tough Bomber", L"自爆兵強化" },
      { L"자폭병 체력 ×1.5 · 자폭병 처치 EXP +10",
        L"Bomber HP ×1.5 · bomber-kill EXP +10",
        L"自爆兵の体力 ×1.5・自爆兵撃破EXP +10" } },
    { AugType::D_BOMBER_SPEED,AugRarity::DEBUFF,    AugUnique::NONE, "D_BMB_SPD",
      { L"자폭병 가속", L"Fast Bomber", L"自爆兵加速" },
      { L"자폭병 이동 속도 +30% · 자폭병 처치 EXP +5",
        L"Bomber move speed +30% · bomber-kill EXP +5",
        L"自爆兵の移動+30%・自爆兵撃破EXP +5" } },
    { AugType::D_MOB_HP,      AugRarity::DEBUFF,    AugUnique::NONE, "D_MOBHP",
      { L"잡몹 체력 강화", L"Tough Mobs", L"雑魚体力強化" },
      { L"잡몹 체력 +20% · 처치 EXP +1", L"Mob HP +20% · kill EXP +1", L"雑魚の体力+20%・撃破EXP +1" } },
    { AugType::D_SLOW_MOVE,   AugRarity::DEBUFF,    AugUnique::NONE, "D_SLOWMV",
      { L"이동속도 너프", L"Heavy Legs", L"鈍足" },
      { L"플레이어 이동 속도 -5% · 전체 EXP +10%", L"Player move speed -5% · all EXP +10%", L"プレイヤー移動 -5%・全EXP +10%" } },
    { AugType::D_SPLITTER,    AugRarity::DEBUFF,    AugUnique::NONE, "D_SPLITTER",
      { L"분열체 출현", L"Splitters", L"分裂体出現" },
      { L"일부 잡몹이 분열체로 등장 (처치 시 작은 2마리로 쪼개짐, 2세대까지) · 처치 EXP +3",
        L"Some mobs become splitters (split into 2 on death, up to 2 gens) · kill EXP +3",
        L"一部の雑魚が分裂体に (撃破で2体に分裂・2世代まで)・撃破EXP +3" } },
    { AugType::D_BLINKER,     AugRarity::DEBUFF,    AugUnique::NONE, "D_BLINKER",
      { L"점멸체 출현", L"Blinkers", L"点滅体出現" },
      { L"일부 잡몹이 점멸체로 등장 (잔상 경고 후 플레이어 쪽으로 순간이동) · 처치 EXP +6",
        L"Some mobs become blinkers (teleport toward you after a ghost telegraph) · kill EXP +6",
        L"一部の雑魚が点滅体に (残像予告後プレイヤーへ瞬間移動)・撃破EXP +6" } },
    { AugType::D_ORBITER,     AugRarity::DEBUFF,    AugUnique::NONE, "D_ORBITER",
      { L"공전체 출현", L"Orbiters", L"公転体出現" },
      { L"일부 잡몹이 공전체로 등장 (플레이어 주위를 돌며 서서히 좁혀옴) · 처치 EXP +5",
        L"Some mobs become orbiters (circle you, spiraling inward) · kill EXP +5",
        L"一部の雑魚が公転体に (周囲を回り徐々に接近)・撃破EXP +5" } },
    { AugType::D_SPAWNER,     AugRarity::DEBUFF,    AugUnique::NONE, "D_SPAWNER",
      { L"소환체 출현", L"Spawners", L"召喚体出現" },
      { L"일부 잡몹이 소환체로 등장 (느리지만 작은 잡몹을 계속 소환) · 처치 EXP +7",
        L"Some mobs become spawners (slow, keep summoning small mobs) · kill EXP +7",
        L"一部の雑魚が召喚体に (低速・小雑魚を召喚し続ける)・撃破EXP +7" } },
    { AugType::D_SHIELDED,    AugRarity::DEBUFF,    AugUnique::NONE, "D_SHIELDED",
      { L"보호막체 출현", L"Shielded", L"防御体出現" },
      { L"일부 잡몹이 보호막체로 등장 (방패 ON 동안 피해 대폭 감소, 주기적으로 OFF) · 처치 EXP +5",
        L"Some mobs become shielded (huge damage cut while shield is up, cycles off) · kill EXP +5",
        L"一部の雑魚が防御体に (盾ON中は被害激減・周期的にOFF)・撃破EXP +5" } },
    { AugType::D_BLEED,       AugRarity::DEBUFF,    AugUnique::NONE, "D_BLEED",
      { L"출혈", L"Bleed", L"出血" },
      { L"초당 체력 0.8 감소 (회복으로 상쇄 가능) · 전체 EXP +12%",
        L"Lose 0.8 HP/s (regen can offset) · all EXP +12%",
        L"毎秒 体力0.8減少 (回復で相殺可)・全EXP +12%" } },
    { AugType::D_WEAKEN,      AugRarity::DEBUFF,    AugUnique::NONE, "D_WEAKEN",
      { L"약화", L"Weaken", L"弱体化" },
      { L"공격력 -12% · 전체 EXP +10%", L"Attack -12% · all EXP +10%", L"攻撃力 -12%・全EXP +10%" } },

    // ── 특수 ───────────────────────────────────────────
    { AugType::S_CHAOS,       AugRarity::SPECIAL,   AugUnique::NONE, "CHAOS",
      { L"대혼란", L"Chaos", L"大混乱" },
      { L"보유 증강을 모두 잊고 같은 개수만큼 랜덤 재배분  (약 60% 버프 / 40% 디버프)",
        L"Forget all augments, redistribute the same count randomly  (~60% buff / 40% debuff)",
        L"所持強化を全て忘れ、同数をランダム再配分  (約60%バフ / 40%デバフ)" } },
    { AugType::S_PANDORA,     AugRarity::SPECIAL,   AugUnique::NONE, "PANDORA",
      { L"판도라의 상자", L"Pandora's Box", L"パンドラの箱" },
      { L"버프 3개 + 디버프 2개 즉시 획득", L"Instantly gain 3 buffs + 2 debuffs", L"バフ3個 + デバフ2個を即獲得" } },

    // ── 조합 (COMBO) — 레시피 충족 시에만 카드로 등장 ───────
    { AugType::CB_EXECUTIONER, AugRarity::COMBO,    AugUnique::NONE, "CB_EXEC",
      { L"처형자", L"Executioner", L"処刑者" },
      { L"[조합] 치명타 확률 +35% · 치명타 배율 +1.5 · 공격력 +20%",
        L"[Combo] Crit chance +35% · crit mult +1.5 · attack +20%",
        L"[組合] クリ率+35%・クリ倍率+1.5・攻撃+20%" } },
    { AugType::CB_BLOODLORD,   AugRarity::COMBO,    AugUnique::NONE, "CB_BLOOD",
      { L"피의 군주", L"Bloodlord", L"血の君主" },
      { L"[조합] 처치당 회복 +0.3 · 최대 체력 +25 · 초당 재생 ↑",
        L"[Combo] Heal +0.3/kill · Max HP +25 · regen up",
        L"[組合] 撃破毎+0.3回復・最大HP+25・再生↑" } },
    { AugType::CB_PIERCE_TWIN, AugRarity::COMBO,    AugUnique::NONE, "CB_PTWIN",
      { L"관통 쌍둥이", L"Piercing Twins", L"貫通の双子" },
      { L"[조합] 관통 확률 60% · 공격력 +40% (더블 패널티 상쇄)",
        L"[Combo] 60% pierce chance · attack +40%",
        L"[組合] 貫通60% · 攻撃+40%" } },
    { AugType::CB_STORMCALLER, AugRarity::COMBO,    AugUnique::NONE, "CB_STORM",
      { L"폭풍 소환사", L"Stormcaller", L"嵐の召喚士" },
      { L"[조합] 탄환 세례 쿨다운 4초 · 드론 +1 · 연사 +15%",
        L"[Combo] Bullet Rain CD 4s · +1 drone · fire rate +15%",
        L"[組合] 弾幕の雨CD4秒・ドローン+1・連射+15%" } },
};

static constexpr int AUG_TOTAL = 67;  // +조합4

// ── 조합 레시피 — result 는 COMBO 등급 AugType, reqs 를 모두 보유하면 등장 ──
struct ComboDef {
    AugType result;
    AugType reqs[3];
    int     reqCount;
};
inline const ComboDef COMBO_DEFS[] = {
    { AugType::CB_EXECUTIONER, { AugType::CRIT,        AugType::BERSERK }, 2 },
    { AugType::CB_BLOODLORD,   { AugType::LIFESTEAL,   AugType::VAMPIRE }, 2 },
    { AugType::CB_PIERCE_TWIN, { AugType::TWIN,        AugType::PIERCE  }, 2 },
    { AugType::CB_STORMCALLER, { AugType::BULLET_RAIN, AugType::DRONE   }, 2 },
};
inline const int COMBO_COUNT = (int)(sizeof(COMBO_DEFS) / sizeof(COMBO_DEFS[0]));

// ALL_AUGS 에서 특정 AugType 의 인덱스 (없으면 -1)
inline int AugIndexOfType(AugType t) {
    for (int i = 0; i < AUG_TOTAL; i++)
        if (ALL_AUGS[i].type == t) return i;
    return -1;
}

// 현재 런에서 해당 AugType 을 보유 중인지 ((int)AugType 인덱스). 조합 레시피 판정용.
//   main 의 applyByIdx 가 갱신, ResetForNewGame 이 초기화.
inline bool g_TypeOwned[128] = { false };

// 한 번만 등장해야 하는 증강/디버프 (스택 불가 플래그형) — 픽 풀에서 takenOnce 로 제외
inline bool AugOnceOnly(AugType t, AugRarity r) {
    if (r == AugRarity::EPIC || r == AugRarity::LEGENDARY || r == AugRarity::COMBO)
        return true;
    switch (t) {
    // 티어드 (등급 무관 1회씩)
    case AugType::BULLET_RAIN: case AugType::BULLET_RAIN_2: case AugType::BULLET_RAIN_3:
    case AugType::DRONE:       case AugType::DRONE_2:
    case AugType::CHAKRAM:     case AugType::CHAKRAM_2:     case AugType::CHAKRAM_3:
    // 희귀 불리언 (중첩 의미 없음)
    case AugType::LIGHT_STEP:  case AugType::GUN_RUNNER:    case AugType::BERSERK:
    // 적 출현형 디버프 (플래그 1개 — 재등장 불필요)
    case AugType::D_SPLITTER:  case AugType::D_BLINKER:
    case AugType::D_ORBITER:   case AugType::D_SPAWNER:     case AugType::D_SHIELDED:
        return true;
    default:
        return false;
    }
}

// 등급별 카드 색상 (배경 RGB)
inline void GetRarityColor(AugRarity r, float& cr, float& cg, float& cb) {
    switch (r) {
    case AugRarity::COMMON:    cr = 0.15f; cg = 0.65f; cb = 0.20f; break; // 녹
    case AugRarity::RARE:      cr = 0.10f; cg = 0.30f; cb = 0.85f; break; // 청
    case AugRarity::EPIC:      cr = 0.55f; cg = 0.10f; cb = 0.80f; break; // 보라
    case AugRarity::LEGENDARY: cr = 0.95f; cg = 0.70f; cb = 0.10f; break; // 금
    case AugRarity::DEBUFF:    cr = 0.55f; cg = 0.10f; cb = 0.10f; break; // 어두운 빨강
    case AugRarity::SPECIAL:   cr = 0.85f; cg = 0.10f; cb = 0.55f; break; // 마젠타
    case AugRarity::COMBO:     cr = 0.10f; cg = 0.85f; cb = 0.80f; break; // 청록(시안)
    }
}

// 등급 라벨 (현재 언어)
inline const wchar_t* GetRarityKR(AugRarity r) {
    int li = CurLangIdx();
    switch (r) {
    case AugRarity::COMMON:    { static const wchar_t* s[3]={L"일반",L"Common",L"コモン"};       return s[li]; }
    case AugRarity::RARE:      { static const wchar_t* s[3]={L"희귀",L"Rare",L"レア"};           return s[li]; }
    case AugRarity::EPIC:      { static const wchar_t* s[3]={L"에픽",L"Epic",L"エピック"};        return s[li]; }
    case AugRarity::LEGENDARY: { static const wchar_t* s[3]={L"전설",L"Legendary",L"レジェンド"}; return s[li]; }
    case AugRarity::DEBUFF:    { static const wchar_t* s[3]={L"디버프",L"Debuff",L"デバフ"};      return s[li]; }
    case AugRarity::SPECIAL:   { static const wchar_t* s[3]={L"특수",L"Special",L"スペシャル"};   return s[li]; }
    case AugRarity::COMBO:     { static const wchar_t* s[3]={L"조합",L"Combo",L"組合"};           return s[li]; }
    }
    return L"?";
}
