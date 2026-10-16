#pragma once
// ─────────────────────────────────────────────────────────────
// 도감(Codex) — 발견 추적 + 적 정보
//   증강/조합/적은 "처음 획득/조우" 해야 도감에 공개. 그 전엔 ??? (정보 비공개).
//   발견 상태는 SaveSystem 으로 영구 저장.
// ─────────────────────────────────────────────────────────────
#include "Augment.h"   // AUG_TOTAL
#include "Monster.h"   // MobKind
#include "Settings.h"  // g_Language, g_CreativeMode
#include <string>
#include <cwctype>
#include <algorithm>

extern bool g_DevUnlocked;   // main.cpp — 도감 develop_mod 해금

inline bool CodexFullReveal() {
    return g_CreativeMode || g_DevUnlocked;
}

extern wchar_t g_CodexSearch[64];
extern int     g_CodexSearchLen;
inline bool    g_CodexSearchInputEnabled = false;
inline wchar_t g_CodexSearchComposition[64] = {};
inline int     g_CodexSearchCompositionLen = 0;
inline int     g_CodexSearchCompositionCaret = 0;

inline int g_CodexSearchCaret = 0;
inline int g_CodexSearchAnchor = -1;

inline bool CodexSearchHasSelection() {
    return g_CodexSearchAnchor >= 0 && g_CodexSearchAnchor != g_CodexSearchCaret;
}
inline int CodexSearchSelectionStart() {
    return CodexSearchHasSelection()
        ? (g_CodexSearchAnchor < g_CodexSearchCaret ? g_CodexSearchAnchor : g_CodexSearchCaret)
        : g_CodexSearchCaret;
}
inline int CodexSearchSelectionEnd() {
    return CodexSearchHasSelection()
        ? (g_CodexSearchAnchor > g_CodexSearchCaret ? g_CodexSearchAnchor : g_CodexSearchCaret)
        : g_CodexSearchCaret;
}
inline void CodexSearchSetComposition(const wchar_t* text, int caret = -1) {
    g_CodexSearchCompositionLen = 0;
    g_CodexSearchComposition[0] = 0;
    g_CodexSearchCompositionCaret = 0;
    if (!text) return;

    const bool replaceSelection = CodexSearchHasSelection();
    const int replaceLen = replaceSelection
        ? CodexSearchSelectionEnd() - CodexSearchSelectionStart() : 0;
    const int maxLen = std::max(0, 63 - (g_CodexSearchLen - replaceLen));
    for (int i = 0; text[i] && g_CodexSearchCompositionLen < maxLen; ++i) {
        const int units = text[i] >= 0xD800 && text[i] <= 0xDBFF &&
                          text[i + 1] >= 0xDC00 && text[i + 1] <= 0xDFFF ? 2 : 1;
        if (text[i] < 32 || text[i] == 127) continue;
        if (g_CodexSearchCompositionLen + units > maxLen) break;
        g_CodexSearchComposition[g_CodexSearchCompositionLen++] = text[i];
        if (units == 2)
            g_CodexSearchComposition[g_CodexSearchCompositionLen++] = text[++i];
    }
    g_CodexSearchComposition[g_CodexSearchCompositionLen] = 0;
    g_CodexSearchCompositionCaret = caret < 0
        ? g_CodexSearchCompositionLen
        : std::clamp(caret, 0, g_CodexSearchCompositionLen);
}
inline void CodexSearchClearComposition() {
    g_CodexSearchComposition[0] = 0;
    g_CodexSearchCompositionLen = 0;
    g_CodexSearchCompositionCaret = 0;
}
inline std::wstring CodexSearchQuery() {
    if (g_CodexSearchCompositionLen <= 0)
        return std::wstring(g_CodexSearch, g_CodexSearch + g_CodexSearchLen);
    const int begin = CodexSearchHasSelection()
        ? CodexSearchSelectionStart() : g_CodexSearchCaret;
    const int end = CodexSearchHasSelection()
        ? CodexSearchSelectionEnd() : g_CodexSearchCaret;
    std::wstring query;
    query.reserve((size_t)g_CodexSearchLen + g_CodexSearchCompositionLen);
    query.append(g_CodexSearch, g_CodexSearch + begin);
    query.append(g_CodexSearchComposition,
                 g_CodexSearchComposition + g_CodexSearchCompositionLen);
    query.append(g_CodexSearch + end, g_CodexSearch + g_CodexSearchLen);
    return query;
}
inline int CodexSearchQueryCaret() {
    if (g_CodexSearchCompositionLen <= 0) return g_CodexSearchCaret;
    const int begin = CodexSearchHasSelection()
        ? CodexSearchSelectionStart() : g_CodexSearchCaret;
    return begin + g_CodexSearchCompositionCaret;
}
inline int CodexSearchCompositionStart() {
    return CodexSearchHasSelection()
        ? CodexSearchSelectionStart() : g_CodexSearchCaret;
}
inline int CodexSearchPreviousBoundary(int pos) {
    if (pos <= 0) return 0;
    --pos;
    if (pos > 0 && g_CodexSearch[pos] >= 0xDC00 && g_CodexSearch[pos] <= 0xDFFF &&
        g_CodexSearch[pos - 1] >= 0xD800 && g_CodexSearch[pos - 1] <= 0xDBFF) --pos;
    return pos;
}
inline int CodexSearchNextBoundary(int pos) {
    if (pos >= g_CodexSearchLen) return g_CodexSearchLen;
    ++pos;
    if (pos < g_CodexSearchLen && g_CodexSearch[pos - 1] >= 0xD800 &&
        g_CodexSearch[pos - 1] <= 0xDBFF && g_CodexSearch[pos] >= 0xDC00 &&
        g_CodexSearch[pos] <= 0xDFFF) ++pos;
    return pos;
}
inline void CodexSearchEraseRange(int begin, int end) {
    if (begin < 0) begin = 0;
    if (end > g_CodexSearchLen) end = g_CodexSearchLen;
    if (begin >= end) return;
    for (int i = end; i <= g_CodexSearchLen; ++i)
        g_CodexSearch[i - (end - begin)] = g_CodexSearch[i];
    g_CodexSearchLen -= end - begin;
    g_CodexSearchCaret = begin;
    g_CodexSearchAnchor = -1;
}
inline void CodexSearchDeleteSelection() {
    if (CodexSearchHasSelection())
        CodexSearchEraseRange(CodexSearchSelectionStart(), CodexSearchSelectionEnd());
}
inline void CodexSearchClear() {
    g_CodexSearch[0] = 0;
    g_CodexSearchLen = 0;
    g_CodexSearchCaret = 0;
    g_CodexSearchAnchor = -1;
    CodexSearchClearComposition();
}
inline void CodexSearchMoveCaret(int target, bool extend) {
    if (extend) {
        if (g_CodexSearchAnchor < 0) g_CodexSearchAnchor = g_CodexSearchCaret;
        g_CodexSearchCaret = target;
        if (g_CodexSearchCaret == g_CodexSearchAnchor) g_CodexSearchAnchor = -1;
    } else {
        g_CodexSearchCaret = target;
        g_CodexSearchAnchor = -1;
    }
}
inline void CodexSearchMoveLeft(bool extend) {
    if (!extend && CodexSearchHasSelection())
        CodexSearchMoveCaret(CodexSearchSelectionStart(), false);
    else CodexSearchMoveCaret(CodexSearchPreviousBoundary(g_CodexSearchCaret), extend);
}
inline void CodexSearchMoveRight(bool extend) {
    if (!extend && CodexSearchHasSelection())
        CodexSearchMoveCaret(CodexSearchSelectionEnd(), false);
    else CodexSearchMoveCaret(CodexSearchNextBoundary(g_CodexSearchCaret), extend);
}
inline void CodexSearchMoveHome(bool extend) { CodexSearchMoveCaret(0, extend); }
inline void CodexSearchMoveEnd(bool extend) { CodexSearchMoveCaret(g_CodexSearchLen, extend); }
inline void CodexSearchSelectAll() {
    g_CodexSearchAnchor = 0;
    g_CodexSearchCaret = g_CodexSearchLen;
    if (g_CodexSearchLen == 0) g_CodexSearchAnchor = -1;
}
inline std::wstring CodexSearchSelectedText() {
    if (!CodexSearchHasSelection()) return L"";
    return std::wstring(g_CodexSearch + CodexSearchSelectionStart(),
                        g_CodexSearch + CodexSearchSelectionEnd());
}
inline void CodexSearchInsertText(const wchar_t* text) {
    if (!text) return;
    CodexSearchClearComposition();
    CodexSearchDeleteSelection();
    for (int i = 0; text[i] && g_CodexSearchLen < 63; ++i) {
        const wchar_t ch = text[i];
        if (ch < 32 || ch == 127) continue;
        const int units = (ch >= 0xD800 && ch <= 0xDBFF &&
                           text[i + 1] >= 0xDC00 && text[i + 1] <= 0xDFFF) ? 2 : 1;
        if (g_CodexSearchLen + units > 63) break;
        for (int j = g_CodexSearchLen; j >= g_CodexSearchCaret; --j)
            g_CodexSearch[j + units] = g_CodexSearch[j];
        g_CodexSearch[g_CodexSearchCaret++] = ch;
        ++g_CodexSearchLen;
        if (units == 2) {
            g_CodexSearch[g_CodexSearchCaret++] = text[++i];
            ++g_CodexSearchLen;
        }
    }
    g_CodexSearchAnchor = -1;
}
inline void CodexSearchInsertCodepoint(unsigned int cp) {
    if (cp < 32 || cp == 127 || cp > 0x10FFFF) return;
    wchar_t chars[3] = {};
    if (cp <= 0xFFFF) chars[0] = (wchar_t)cp;
    else {
        cp -= 0x10000;
        chars[0] = (wchar_t)(0xD800 + (cp >> 10));
        chars[1] = (wchar_t)(0xDC00 + (cp & 0x3FF));
    }
    CodexSearchInsertText(chars);
}
inline void CodexSearchBackspace() {
    if (CodexSearchHasSelection()) CodexSearchDeleteSelection();
    else if (g_CodexSearchCaret > 0)
        CodexSearchEraseRange(CodexSearchPreviousBoundary(g_CodexSearchCaret), g_CodexSearchCaret);
}
inline void CodexSearchDeleteForward() {
    if (CodexSearchHasSelection()) CodexSearchDeleteSelection();
    else if (g_CodexSearchCaret < g_CodexSearchLen)
        CodexSearchEraseRange(g_CodexSearchCaret, CodexSearchNextBoundary(g_CodexSearchCaret));
}

inline bool CodexMatch(const wchar_t* name) {
    const std::wstring query = CodexSearchQuery();
    if (query.empty()) return true;
    if (!name || !name[0]) return false;
    std::wstring a(name);
    auto lc = [](std::wstring s) {
        for (auto& c : s) if (c < 128) c = (wchar_t)towlower(c);
        return s;
    };
    return lc(a).find(lc(query)) != std::wstring::npos;
}

inline bool CodexMatchAny(const wchar_t* const* names, int count) {
    if (CodexSearchQuery().empty()) return true;
    if (!names || count <= 0) return false;
    for (int i = 0; i < count; ++i)
        if (CodexMatch(names[i])) return true;
    return false;
}

inline bool CodexSearchMatchesAny(const wchar_t* const* names, int count) {
    return CodexMatchAny(names, count);
}

// ── 증강 발견 (ALL_AUGS 인덱스 기준) ──
inline bool g_AugSeen[AUG_TOTAL] = { false };
inline bool g_CodexDirty   = false;

inline void MarkAugSeen(int augIdx) {
    if (augIdx < 0 || augIdx >= AUG_TOTAL) return;
    if (!g_AugSeen[augIdx]) { g_AugSeen[augIdx] = true; g_CodexDirty = true; }
}

inline bool CodexAugSeen(int augIdx) {
    if (CodexFullReveal() && augIdx >= 0 && augIdx < AUG_TOTAL) return true;
    if (augIdx < 0 || augIdx >= AUG_TOTAL) return false;
    return g_AugSeen[augIdx];
}

// ── 새 적은 enum, kRosterOrder, CODEX_MOB_PROFILES에 추가하면 두 UI에 반영된다. ──
enum CodexMobId {
    CM_ROTOR,
    CM_GENESIS,
    CM_SCOPE,
    CM_SWARM,
    CM_GRAVIS,
    CM_QUASAR,
    CM_GIMBAL,
    CM_REGULUS, // Reuse the reserved slot to preserve saved Codex indices.
    CM_RETIRED_08, // Reserved to preserve saved Codex indices after Sagitta removal.
    CM_ANTARES,
    CM_MAGNETAR,
    CM_COUNT
};


struct CodexMobProfile {
    const wchar_t* name[3];
    const wchar_t* description[3];
    const wchar_t* threat;
    int rewardTier; // Kill milestone tier; separate from appearance tier.
    int spawnTierHalfSteps; // Tier × 2, so 1.5 is stored as 3.
    int baseHp;
    const wchar_t* damage;
    const wchar_t* speed;
    const wchar_t* attack[3];
    const wchar_t* form[3];
};

inline const CodexMobProfile CODEX_MOB_PROFILES[CM_COUNT] = {
    { { L"로터", L"ROTOR", L"ローター" },
      { L"단일 회전 프레임으로 플레이어를 집요하게 추적하는 기본 신호", L"Basic signal that relentlessly tracks the player with a single rotating frame", L"単一の回転フレームでプレイヤーを追跡する基本シグナル" },
      L"LOW", 1, 3, 90, L"5 / sec (contact)", L"120-180 px/s",
      { L"플레이어 추적", L"Pursues the player", L"プレイヤーを追跡" },
      { L"네 개의 신호점이 회전하는 사각 프레임의 꼭짓점을 이룹니다.", L"Four signal nodes sit at the corners of a rotating square frame.", L"4つの信号点が回転する四角いフレームの頂点に配置されています。" } },
    { { L"제네시스", L"GENESIS", L"ジェネシス" },
      { L"코어에서 SWARM을 생산해 플레이어에게 던지는 발사대", L"Launcher that forges SWARMs in its core and hurls them at the player", L"コアでSWARMを生産しプレイヤーへ投げつける発射台" },
      L"HIGH", 2, 8, 630, L"10 / thrown Swarm, 6 / landing blast, 5 / sec (contact)", L"42-63 px/s",
      { L"껍질 3개가 모두 차면 SWARM 3마리 투척 (비행 중 격추 가능)", L"Throws 3 Swarms once all three shells are charged (can be shot down mid-flight)", L"殻が3つとも満ちるとSWARMを3体投擲（飛行中に撃墜可能）" },
      { L"작은 코어를 감싼 껍질 3개가 돌며 하나씩 SWARM을 품고, 셋이 차면 틈이 플레이어를 향해 멈춥니다.", L"Three turning shells around a small core each cradle a Swarm; when all are full, a gap stops facing the player.", L"小さなコアを囲む3つの殻が回りながらSWARMを一体ずつ抱え、満ちると隙間がプレイヤーへ向いて止まります。" } },
    { { L"스코프", L"SCOPE", L"スコープ" },
      { L"중앙 코어와 조준선으로 원거리에서 공격하는 감시 신호", L"Ranged surveillance signal that attacks from afar with a central core and aim lanes", L"中央コアと照準線で遠距離攻撃する監視シグナル" },
      L"MODERATE", 2, 5, 360, L"10 / shot (3-shot burst)", L"Variable",
      { L"유도탄 3발 연속 발사", L"Fires 3 homing shots per burst", L"誘導弾を3発連続発射" },
      { L"중앙 렌즈를 원형 고리와 네 개의 관측점이 둘러싸고 있습니다.", L"A central lens is surrounded by circular rings and four observation nodes.", L"中央のレンズを円形のリングと4つの観測点が囲んでいます。" } },
    { { L"스웜", L"SWARM", L"スウォーム" },
      { L"작은 신호 조각이 무리를 이루어 압박하는 물량형 신호", L"Swarm signal made of small shards that pressure the arena in large numbers", L"小さな信号片の群れで戦場を圧迫する物量型シグナル" },
      L"MODERATE", 1, 2, 32, L"5 / sec (contact)", L"126-189 px/s",
      { L"무리를 이루어 접근", L"Pressures the player in groups", L"群れでプレイヤーに接近" },
      { L"세 개의 작은 신호점이 삼각형을 이루는 소형 형상입니다.", L"A small triangular form made from three signal nodes.", L"3つの小さな信号点で三角形を形作る小型の存在です。" } },
    { { L"그라비스", L"GRAVIS", L"グラビス" },
      { L"중력장으로 이동과 탄도 궤적을 왜곡하는 고위험 신호", L"High-threat signal that bends movement and bullet trajectories with gravity", L"重力場で移動と弾道を歪める高脅威シグナル" },
      L"HIGH", 3, 4, 720, L"6 / sec (contact)", L"29-43 px/s",
      { L"중력장으로 이동과 탄도 왜곡", L"Distorts movement and projectiles", L"重力場で移動と弾道を歪める" },
      { L"중심 코어에서 여섯 신호점이 뻗으며 넓은 원형 장이 둘러쌉니다.", L"Six nodes extend from a central core, surrounded by a broad circular field.", L"中心コアから6つの信号点が伸び、広い円形の場が周囲を囲みます。" } },
    { { L"퀘이사", L"QUASAR", L"クエーサー" },
      { L"장거리 조준선을 교차시켜 전장을 통제하는 희귀 천체", L"Rare long-range signal that controls the arena with crossing aim lanes", L"交差する照準線で戦場を制御する希少シグナル" },
      L"HIGH", 2, 8, 576, L"34 / sec (beam), 4 / sec (contact)", L"62-94 px/s",
      { L"플레이어를 조준해 0.7초 뒤 빔 2.5초, 발사 중 플레이어 쪽으로 천천히 회전 (재사용 3.2초)", L"Locks onto the player, then fires a 2.5s beam after 0.7s that slowly turns toward the player (3.2s cooldown)", L"プレイヤーを照準し、0.7秒後にビーム2.5秒。発射中はプレイヤー側へゆっくり回転（再使用3.2秒）" },
      { L"쉬지 않고 도는 자이로 띠 두 개가 감긴 구체 안에서, 구속된 불안정한 코어가 빔 축을 겨눕니다. 적도 플랜지는 따로 자전합니다.", L"Inside an orb wrapped by two ceaselessly turning gyro bands, a restrained, unstable core holds the beam axis while the equatorial flange spins on its own.", L"絶えず回る2本のジャイロ帯に巻かれた球体の中で、拘束された不安定なコアがビーム軸を狙います。赤道フランジは独自に自転します。" } },
    { { L"짐벌", L"Gimbal", L"ジンバル" },
      { L"약 700px 거리를 유지하며 렌즈를 뒤집어 충전한 뒤 느린 비유도탄 한 발을 쏘는 밝은 라임색 신호", L"A bright lime signal that holds about 700px distance, flips its lens to charge, and fires one slow unguided shot", L"約700pxの距離を保ち、レンズを反転させて充填した後、遅い非誘導弾を一発撃つ明るいライム色のシグナル" },
      L"MODERATE", 2, 3, 149, L"8 / shot, 4 / sec (contact)", L"70-105 px/s",
      { L"렌즈가 점점 빠르게 뒤집히며 충전한 뒤 비유도탄 1발 발사", L"Flips its lens faster and faster to charge, then fires one unguided shot", L"レンズを加速しながら反転させて充填後、非誘導弾を1発発射" },
      { L"마주 보는 두 브래킷이 축 핀으로 마름모 렌즈를 물고 있고, 렌즈가 그 축을 따라 뒤집힙니다.", L"Two facing brackets hold a diamond lens on a pin axis, and the lens flips about that axis.", L"向かい合う2つのブラケットが軸ピンでひし形のレンズを挟み、レンズはその軸に沿って反転します。" } },
    { { L"레굴루스", L"REGULUS", L"レグルス" },
      { L"주변 신호에게 보호막 탄을 쏘아 독립 보호막을 부여하는 후방 지원 코어",
        L"Rear support core that fires shield bolts to grant nearby signals independent shields",
        L"周囲の信号にシールド弾を撃ち、独立したシールドを付与する後方支援コア" },
      L"HIGH", 3, 6, 360, L"None (support)", L"50-76 px/s (retreats)",
      { L"1초마다 500px 안 무작위 신호에게 보호막 탄 · 보호막 체력 216",
        L"Shield bolt to a random signal within 500px every 1s · shield HP 216",
        L"1秒ごとに500px内のランダムな信号へシールド弾 · シールドHP 216" },
      { L"끝이 평평한 방패판 넷이 십자로 모인 문장을 곡면 판 네 장이 감쌉니다. 보호막을 받은 신호는 모서리 괄호로 표시됩니다.",
        L"A crest of four flat-tipped shield plates sits inside four curved plates; shielded signals are framed by corner brackets.",
        L"先端の平らな盾板4枚が十字に集まった紋章を、曲面板4枚が囲みます。シールドを受けた信号は角括弧で示されます。" } },
    { { L"", L"", L"" }, { L"", L"", L"" }, L"UNKNOWN", 0, 0, 0,
      L"?", L"?", { L"?", L"?", L"?" }, { L"", L"", L"" } },
    { { L"안타레스", L"ANTARES", L"アンタレス" },
      { L"쫓아올수록 빨라지지만, 빨라질수록 방향을 못 틀어 플레이어를 지나쳐 버리는 투우형 추적 신호",
        L"Bull-like hunter that keeps speeding up as it chases, but turns worse the faster it goes and overshoots",
        L"追うほど加速するが、速くなるほど曲がれずプレイヤーを通り過ぎる闘牛型追跡シグナル" },
      L"HIGH", 2, 6, 600, L"Up to 12 / ram, 5 / sec (contact)", L"60-700 px/s",
      { L"가속하며 추적 · 들이받으면 속도만큼 밀쳐내고 잠시 기절 · 지나치면 미끄러진 뒤 휴식",
        L"Accelerates while chasing · a ram knocks you back by its speed and stuns it · a miss skids, then rests",
        L"加速しながら追跡 · 衝突すると速度に応じて吹き飛ばし一時気絶 · 外すと滑って休止" },
      { L"꼭짓점마다 틈이 있는 큰 진홍색 오각 프레임이 빨라질수록 잔상을 남깁니다.",
        L"A large crimson pentagon frame, split at each vertex, leaves afterimages as it speeds up.",
        L"頂点ごとに隙間のある大きな深紅の五角フレームが、加速するほど残像を残します。" } },
    { { L"마그네타", L"MAGNETAR", L"マグネター" },
      { L"플레이어를 추격하며 격자 결계로 주변에 들어온 탄환을 막아내는 수호형 신호",
        L"Chasing warden signal whose lattice ward stops every shot that enters around it",
        L"プレイヤーを追跡し、格子の結界で周囲に入った弾をすべて防ぐ守護型シグナル" },
      L"HIGH", 3, 7, 450, L"5 / sec (contact)", L"94-140 px/s",
      { L"격자 20면이 각각 결계 · 한 면이 탄 4발을 막으면 깨져 그쪽으로는 탄이 통과 · 깨진 면은 8초 뒤 복구",
        L"Each of the 20 lattice faces is its own ward · a face breaks after stopping 4 shots and lets shots through on that side · broken faces restore after 8s",
        L"格子の20面がそれぞれ結界 · 1面が弾4発を防ぐと割れ、その方向は弾が通過 · 割れた面は8秒後に復旧" },
      { L"천천히 도는 정이십면체 격자가 곧 결계이며, 안쪽의 작은 결정 씨앗이 연결선으로 격자를 붙잡고 있습니다.",
        L"A slowly turning icosahedral lattice is the ward itself, held by tethers from a small crystal seed inside.",
        L"ゆっくり回る正二十面体の格子そのものが結界で、内側の小さな結晶の種が連結線で格子を支えています。" } },
};

inline const CodexMobProfile* CodexMobProfileFor(int id) {
    if (id == CM_RETIRED_08) return nullptr;
    return id >= 0 && id < CM_COUNT ? &CODEX_MOB_PROFILES[id] : nullptr;
}

inline const wchar_t* CodexMobProfileText(const wchar_t* const* localized) {
    int li = (int)g_Language;
    if (li < 0 || li >= LANG_COUNT) li = 0;
    return localized[li];
}

inline int CodexMobRewardTier(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->rewardTier : 0;
}

inline int CodexMobSpawnTierHalfSteps(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->spawnTierHalfSteps : 0;
}

inline int CodexMobBaseHp(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->baseHp : 0;
}

inline const wchar_t* CodexMobDamageValue(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->damage : L"?";
}

inline const wchar_t* CodexMobBaseSpeed(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->speed : L"?";
}

inline const wchar_t* CodexMobAttackPattern(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? CodexMobProfileText(profile->attack) : L"?";
}

inline const wchar_t* CodexMobFormDescription(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? CodexMobProfileText(profile->form) : L"";
}


inline bool g_MobSeen[CM_COUNT] = { false };
inline long long g_MobKillCounts[CM_COUNT] = {};
inline long long g_RunMobKillCounts[CM_COUNT] = {};
inline bool g_SuppressMobSeen = false;
inline void MarkMobSeenId(CodexMobId id);

inline CodexMobId CodexMobIdForKind(MobKind k) {
    switch (k) {
    case MobKind::ROTOR:   return CM_ROTOR;
    case MobKind::GENESIS: return CM_GENESIS;
    case MobKind::SWARM:   return CM_SWARM;
    case MobKind::GRAVIS:  return CM_GRAVIS;
    case MobKind::QUASAR:  return CM_QUASAR;
    case MobKind::GIMBAL:  return CM_GIMBAL;
    case MobKind::REGULUS: return CM_REGULUS;
    case MobKind::ANTARES: return CM_ANTARES;
    case MobKind::MAGNETAR: return CM_MAGNETAR;
    default:               return CM_COUNT;
    }
}

template <typename Fn>
inline void ForEachCodexMobEntry(Fn&& fn) {
    static constexpr CodexMobId kRosterOrder[] = {
        CM_ROTOR, CM_ANTARES, CM_SCOPE, CM_REGULUS, CM_GIMBAL, CM_SWARM,
        CM_GENESIS, CM_GRAVIS, CM_QUASAR, CM_MAGNETAR
    };
    for (CodexMobId id : kRosterOrder) fn((int)id);
}

inline void MarkMobSeen(MobKind k) {
    if (g_SuppressMobSeen) return;
    MarkMobSeenId(CodexMobIdForKind(k));
}
inline void MarkMobSeenId(CodexMobId id) {
    if (id == CM_RETIRED_08) return;
    if (id < 0 || id >= CM_COUNT) return;
    if (!g_MobSeen[id]) { g_MobSeen[id] = true; g_CodexDirty = true; }
}
inline void RegisterCodexMobKill(CodexMobId id) {
    if (id == CM_RETIRED_08) return;
    if (g_CreativeMode || id < 0 || id >= CM_COUNT) return;
    ++g_RunMobKillCounts[id];
}
inline void RegisterCodexMobKill(MobKind kind) {
    RegisterCodexMobKill(CodexMobIdForKind(kind));
}

inline bool CodexMobSeen(int id) {
    if (id == CM_RETIRED_08) return false;
    if (CodexFullReveal() && id >= 0 && id < CM_COUNT) return true;
    if (id < 0 || id >= CM_COUNT) return false;
    return g_MobSeen[id];
}

inline long long CodexMobKillCount(int id) {
    if (id == CM_RETIRED_08) return 0;
    if (id < 0 || id >= CM_COUNT) return 0;
    return g_MobKillCounts[id] + g_RunMobKillCounts[id];
}

inline long long CodexMobKillThreshold(int id, int milestone) {
    const int tier = CodexMobRewardTier(id);
    if (tier < 1 || milestone < 0 || milestone > 2) return 0;
    static const long long kThresholds[3][3] = {
        { 1, 500, 2000 },
        { 1, 50, 400 },
        { 1, 20, 100 },
    };
    // Reward milestones have three bands; tier-4 enemies use the top band.
    return kThresholds[std::min(tier, 3) - 1][milestone];
}

// 0=overview locked, 1=overview, 2=combat stats, 3=full profile.
inline int CodexMobDataStage(int id) {
    if (id == CM_RETIRED_08) return 0;
    if (id < 0 || id >= CM_COUNT) return 0;
    const long long kills = CodexMobKillCount(id);
    if (CodexFullReveal()) return 3;
    if (kills >= CodexMobKillThreshold(id, 2)) return 3;
    if (kills >= CodexMobKillThreshold(id, 1)) return 2;
    if (kills >= CodexMobKillThreshold(id, 0)) return 1;
    return 0;
}

inline const wchar_t* CodexLocalizedText(const wchar_t* kr, const wchar_t* en,
                                         const wchar_t* jp) {
    int li = (int)g_Language;
    if (li < 0 || li >= LANG_COUNT) li = 0;
    const wchar_t* text[3] = { kr, en, jp };
    return text[li];
}

inline const wchar_t* const* MobLocalizedNames(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->name : nullptr;
}
inline const wchar_t* MobName(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    const wchar_t* const* names = MobLocalizedNames(id);
    return names ? names[li] : L"???";
}
inline bool CodexMobNameUnlocked(int id) {
    return CodexMobDataStage(id) >= 1;
}
inline const wchar_t* CodexMobListLabel(int id) {
    if (CodexMobNameUnlocked(id)) return MobName(id);
    if (!CodexMobSeen(id))
        return CodexLocalizedText(L"미발견", L"UNDISCOVERED", L"未発見");
    return L"???";
}
inline const wchar_t* MobDesc(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    if (id < 0 || id >= CM_COUNT) return L"???";
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->description[li] : L"";
}


inline const wchar_t* MobThreatLabel(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->threat : L"UNKNOWN";
}

inline const wchar_t* MobTierLabel(int id) {
    switch (CodexMobSpawnTierHalfSteps(id)) {
    case 2: return L"1";
    case 3: return L"1.5";
    case 4: return L"2";
    case 5: return L"2.5";
    case 6: return L"3";
    case 7: return L"3.5";
    case 8: return L"4";
    default: return L"?";
    }
}

inline MobKind CodexMobKind(int id) {
    switch (id) {
    case CM_ROTOR:   return MobKind::ROTOR;
    case CM_GENESIS: return MobKind::GENESIS;
    case CM_SWARM:   return MobKind::SWARM;
    case CM_GRAVIS:  return MobKind::GRAVIS;
    case CM_QUASAR:  return MobKind::QUASAR;
    case CM_GIMBAL:  return MobKind::GIMBAL;
    case CM_REGULUS: return MobKind::REGULUS;
    case CM_ANTARES: return MobKind::ANTARES;
    case CM_MAGNETAR: return MobKind::MAGNETAR;
    default:         return MobKind::ROTOR;
    }
}
