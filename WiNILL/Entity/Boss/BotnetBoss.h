#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "PlayerStats.h"

extern TextRenderer g_TextS;

// ─────────────────────────────────────────────────────────────
// C2_RELAY.sys — Command & Control (리메이크)
//   · 코어(터미널)는 방화벽 보호: 살아있는 호스트 1기당 피해 28% 감소
//   · 코어가 주기적으로 호스트에 "명령 신호" 송신 (링크 위를 이동하는 펄스)
//   · 수신~실행 중인 호스트를 파괴하면 릴레이 과부하 → 코어 노출 (피해 1.6배)
//   · 호스트 3기 정예: TURRET(포격) / SPAWNER(좀비) / WARDEN(브로드캐스트 핑)
//   · 시그니처: DDoS 웨이브 — 예고 후 틈이 있는 패킷 벽이 밀려옴
//   · 호스트 전멸 시 방화벽 0% + 코어 패닉 (호스트는 시간이 지나면 재구축)
// ─────────────────────────────────────────────────────────────
class BotnetBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    bool  shakePulse = false;

    bool  phase2 = false;
    float orbitRot = 0.0f;
    float driftT = 0.0f;
    float logScroll = 0.0f;
    float hostHpBase = 0.0f;
    float lastPx = 0.0f, lastPy = 0.0f;

    enum class HostRole { Turret, Spawner, Warden };

    struct Host {
        HostRole role = HostRole::Turret;
        float slotAngle = 0.0f;
        float x = 0.0f, y = 0.0f;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = true;
        float fireCd = 0.0f;
        float rebuildCd = 0.0f;
    };
    Host hosts[3];

    enum class MinionKind { Rush, Heavy, Pulse };

    struct Minion {
        MinionKind kind = MinionKind::Rush;
        float x = 0.0f, y = 0.0f;
        float vx = 0.0f, vy = 0.0f;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = false;
        float fireCd = 0.0f;
        float pulseAng = 0.0f;
    };
    std::vector<Minion> minions;

    // ── 명령 사이클 ──
    enum class CmdPhase { Idle, Travel, Execute };
    CmdPhase cmdPhase = CmdPhase::Idle;
    int   cmdHost = -1;
    float cmdT = 0.0f;
    float cmdCd = 4.0f;

    // ── 과부하 / 노출 ──
    float exposedT = 0.0f;
    float stunT = 0.0f;

    // ── DDoS 웨이브 ──
    float ddosCd = 8.5f;
    float ddosTele = 0.0f;
    float ddosDirX = 1.0f, ddosDirY = 0.0f;
    float panicCd = 2.5f;

    static constexpr int   NHOST      = 3;
    static constexpr int   MAX_MINION = 8;
    static constexpr float BODY       = 62.0f;
    static constexpr float TERM_W     = 148.0f;
    static constexpr float TERM_H     = 108.0f;
    static constexpr float ORBIT_R    = 236.0f;
    static constexpr float ORBIT_SPD  = 0.26f;
    static constexpr float HOST_W     = 46.0f;
    static constexpr float HOST_H     = 38.0f;
    static constexpr float HOST_HIT   = 27.0f;
    static constexpr float PKT_SPD    = 440.0f;
    static constexpr float PKT_SPD2   = 540.0f;

    static constexpr float FW_PER_HOST   = 0.28f;   // 호스트 1기당 방화벽 28%
    static constexpr float EXPOSED_MULT  = 1.6f;    // 노출 시 받는 피해 배율
    static constexpr float EXPOSED_DUR   = 4.0f;
    static constexpr float EXPOSED_DUR2  = 3.2f;
    static constexpr float CMD_TRAVEL    = 0.9f;    // 신호 이동 시간
    static constexpr float CMD_EXECUTE   = 1.0f;    // 호스트 실행(충전) 시간
    static constexpr float CMD_GAP       = 5.5f;
    static constexpr float CMD_GAP2      = 4.0f;
    static constexpr float REBUILD_T     = 12.0f;
    static constexpr float REBUILD_T2    = 9.0f;
    static constexpr float DDOS_GAP      = 9.5f;
    static constexpr float DDOS_GAP2     = 7.0f;
    static constexpr float DDOS_TELE_DUR = 1.1f;
    static constexpr float DDOS_SPAN     = 460.0f;  // 벽 반경 (중심 기준 ±)
    static constexpr float DDOS_STEP     = 40.0f;

    BotnetBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.45f;
        hostHpBase = hpInit * 0.2f;
        if (hostHpBase < 220.0f) hostHpBase = 220.0f;
        static const HostRole roles[3] = { HostRole::Turret, HostRole::Spawner, HostRole::Warden };
        for (int i = 0; i < NHOST; i++) {
            hosts[i].role = roles[i];
            hosts[i].slotAngle = (float)i * (6.2831853f / (float)NHOST);
            hosts[i].hp = hosts[i].maxHp = hostHpBase;
            hosts[i].fireCd = 1.0f + (float)(rand() % 80) * 0.02f;
            hosts[i].rebuildCd = 0.0f;
        }
        minions.reserve(MAX_MINION);
    }

    int aliveHosts() const {
        int n = 0;
        for (int i = 0; i < NHOST; i++) if (hosts[i].alive) ++n;
        return n;
    }

    int aliveMinions() const {
        int n = 0;
        for (auto& m : minions) if (m.alive) ++n;
        return n;
    }

    bool exposed() const { return exposedT > 0.0f; }

    float bodyDamageTakenMult() const {
        if (exposedT > 0.0f) return EXPOSED_MULT;
        float red = FW_PER_HOST * (float)aliveHosts();
        if (red > 0.9f) red = 0.9f;
        return 1.0f - red;
    }

    float hostShieldPercent() const {
        if (exposedT > 0.0f) return 0.0f;
        return (1.0f - bodyDamageTakenMult()) * 100.0f;
    }

    static float minionHit(MinionKind k) {
        switch (k) {
        case MinionKind::Heavy: return 22.0f;
        case MinionKind::Pulse: return 16.0f;
        default:                return 13.0f;
        }
    }

    static const wchar_t* roleTag(HostRole r) {
        switch (r) {
        case HostRole::Turret:  return L"GUN";
        case HostRole::Spawner: return L"FAB";
        default:                return L"PING";
        }
    }

    bool addMinion(const Minion& src) {
        if (aliveMinions() >= MAX_MINION) return false;
        for (auto& slot : minions) {
            if (!slot.alive) { slot = src; slot.alive = true; return true; }
        }
        minions.push_back(src);
        minions.back().alive = true;
        return true;
    }

    Minion spawnRush(float sx, float sy, float tx, float ty) {
        Minion m;
        m.kind = MinionKind::Rush;
        m.x = sx; m.y = sy;
        float dx = tx - sx, dy = ty - sy;
        float len = std::sqrt(dx * dx + dy * dy) + 1e-3f;
        float spd = phase2 ? 230.0f : 185.0f;
        m.vx = dx / len * spd;
        m.vy = dy / len * spd;
        m.maxHp = m.hp = hostHpBase * 0.12f;
        if (m.maxHp < 40.0f) m.maxHp = m.hp = 40.0f;
        return m;
    }

    void deployRushFromHosts(int count, float tx, float ty) {
        for (int n = 0; n < count; n++) {
            for (int i = 0; i < NHOST; i++) {
                int idx = (n + i) % NHOST;
                if (!hosts[idx].alive) continue;
                addMinion(spawnRush(hosts[idx].x, hosts[idx].y, tx, ty));
                break;
            }
        }
    }

    void deployMapRush(float tx, float ty, int count) {
        for (int i = 0; i < count; i++) {
            int edge = rand() % 4;
            float sx = 0, sy = 0;
            float pad = 60.0f;
            switch (edge) {
            case 0: sx = pad + (float)(rand() % (screenW - (int)pad * 2)); sy = -30.0f; break;
            case 1: sx = (float)screenW + 30.0f; sy = pad + (float)(rand() % (screenH - (int)pad * 2)); break;
            case 2: sx = pad + (float)(rand() % (screenW - (int)pad * 2)); sy = (float)screenH + 30.0f; break;
            default: sx = -30.0f; sy = pad + (float)(rand() % (screenH - (int)pad * 2)); break;
            }
            addMinion(spawnRush(sx, sy, tx, ty));
        }
    }

    void firePacket(std::vector<Bullet>& bullets, float fx, float fy,
                    float tx, float ty, float spd, glm::vec3 col, float edmg) {
        Bullet bb(fx, fy, tx, ty);
        bb.isEnemy = true;
        bb.speed = spd;
        bb.enemyDmg = edmg;
        bb.color = col;
        bullets.push_back(bb);
    }

    void syncHostPositions() {
        for (int i = 0; i < NHOST; i++) {
            Host& h = hosts[i];
            if (!h.alive) continue;
            float a = orbitRot + h.slotAngle;
            h.x = worldX + cosf(a) * ORBIT_R;
            h.y = worldY + sinf(a) * ORBIT_R;
        }
    }

    // ── 명령 실행: 역할별 액션 ──
    void executeCommand(int idx, float px, float py, std::vector<Bullet>& bullets) {
        if (idx < 0 || idx >= NHOST || !hosts[idx].alive) return;
        Host& h = hosts[idx];
        glm::vec3 hot(1.0f, 0.5f, 0.2f);
        glm::vec3 green(0.25f, 0.95f, 0.55f);
        glm::vec3 cyan(0.3f, 0.75f, 1.0f);

        switch (h.role) {
        case HostRole::Turret: {
            int n = phase2 ? 9 : 7;
            float base = atan2f(py - h.y, px - h.x);
            float spread = 0.5f;
            for (int i = 0; i < n; i++) {
                float u = (n > 1) ? (float)i / (float)(n - 1) : 0.5f;
                float a = base - spread * 0.5f + spread * u;
                firePacket(bullets, h.x, h.y, h.x + cosf(a) * 200.0f, h.y + sinf(a) * 200.0f,
                           PKT_SPD2, hot, phase2 ? 10.0f : 8.5f);
            }
        } break;
        case HostRole::Spawner: {
            int n = phase2 ? 4 : 3;
            for (int i = 0; i < n; i++)
                addMinion(spawnRush(h.x, h.y, px + (float)(rand() % 120 - 60),
                                    py + (float)(rand() % 120 - 60)));
        } break;
        case HostRole::Warden: {
            // 브로드캐스트 핑 — 전 호스트 동시 링 파동
            int n = phase2 ? 18 : 14;
            for (int hi = 0; hi < NHOST; hi++) {
                if (!hosts[hi].alive) continue;
                float off = (float)(rand() % 628) * 0.01f;
                for (int i = 0; i < n; i++) {
                    float a = off + (float)i / (float)n * 6.2831853f;
                    firePacket(bullets, hosts[hi].x, hosts[hi].y,
                               hosts[hi].x + cosf(a) * 200.0f, hosts[hi].y + sinf(a) * 200.0f,
                               330.0f, cyan, 8.0f);
                }
            }
        } break;
        }
        (void)green;
    }

    void triggerOverload() {
        exposedT = phase2 ? EXPOSED_DUR2 : EXPOSED_DUR;
        stunT = 1.6f;
        shakePulse = true;
        cmdPhase = CmdPhase::Idle;
        cmdHost = -1;
        cmdCd = (phase2 ? CMD_GAP2 : CMD_GAP) + 1.5f;
    }

    void updateCommand(float px, float py, float dt, std::vector<Bullet>& bullets) {
        // 명령 대상 호스트가 (외부 총알 등으로) 죽었으면 → 과부하
        if (cmdPhase != CmdPhase::Idle) {
            if (cmdHost < 0 || !hosts[cmdHost].alive) {
                triggerOverload();
                return;
            }
        }

        switch (cmdPhase) {
        case CmdPhase::Idle:
            cmdCd -= dt;
            if (cmdCd <= 0.0f && aliveHosts() > 0) {
                // 살아있는 호스트 중 무작위 선택
                int tries = 0;
                do { cmdHost = rand() % NHOST; tries++; }
                while (!hosts[cmdHost].alive && tries < 12);
                if (!hosts[cmdHost].alive) { cmdHost = -1; cmdCd = 1.0f; break; }
                cmdPhase = CmdPhase::Travel;
                cmdT = 0.0f;
            }
            break;
        case CmdPhase::Travel:
            cmdT += dt;
            if (cmdT >= CMD_TRAVEL) {
                cmdPhase = CmdPhase::Execute;
                cmdT = 0.0f;
            }
            break;
        case CmdPhase::Execute:
            cmdT += dt;
            if (cmdT >= CMD_EXECUTE) {
                executeCommand(cmdHost, px, py, bullets);
                cmdPhase = CmdPhase::Idle;
                cmdHost = -1;
                cmdCd = phase2 ? CMD_GAP2 : CMD_GAP;
                cmdCd += (float)(rand() % 20) * 0.05f;
            }
            break;
        }
    }

    void updateRebuild(float dt) {
        for (int i = 0; i < NHOST; i++) {
            Host& h = hosts[i];
            if (h.alive) continue;
            if (h.rebuildCd <= 0.0f)
                h.rebuildCd = phase2 ? REBUILD_T2 : REBUILD_T;
            h.rebuildCd -= dt;
            if (h.rebuildCd <= 0.0f) {
                h.alive = true;
                h.hp = h.maxHp = hostHpBase * 0.5f;
                h.fireCd = 1.2f;
                h.rebuildCd = 0.0f;
            }
        }
    }

    void updateHosts(float px, float py, float dt, float& playerHP,
                     std::vector<Bullet>& bullets) {
        glm::vec3 pktCol = phase2 ? glm::vec3(1.0f, 0.4f, 0.25f)
                                  : glm::vec3(0.25f, 0.95f, 0.55f);
        for (int i = 0; i < NHOST; i++) {
            Host& h = hosts[i];
            if (!h.alive) continue;

            // 명령 수신/실행 중인 호스트는 패시브 사격 정지 (충전 모션)
            bool busy = (cmdHost == i && cmdPhase != CmdPhase::Idle);

            if (!busy && stunT <= 0.0f && h.role == HostRole::Turret) {
                h.fireCd -= dt;
                if (h.fireCd <= 0.0f) {
                    h.fireCd = (phase2 ? 1.5f : 2.4f) + (float)(rand() % 20) * 0.02f;
                    firePacket(bullets, h.x, h.y, px, py,
                               phase2 ? PKT_SPD2 : PKT_SPD, pktCol, phase2 ? 9.0f : 7.5f);
                }
            }
            if (!busy && stunT <= 0.0f && h.role == HostRole::Spawner) {
                h.fireCd -= dt;
                if (h.fireCd <= 0.0f) {
                    h.fireCd = phase2 ? 5.0f : 7.0f;
                    if (aliveMinions() < MAX_MINION)
                        addMinion(spawnRush(h.x, h.y, px, py));
                }
            }

            float hdx = px - h.x, hdy = py - h.y;
            if (hdx * hdx + hdy * hdy < HOST_HIT * HOST_HIT)
                HurtPlayer(playerHP, (phase2 ? 10.0f : 7.0f) * dt);
        }
    }

    void updateMinions(float px, float py, float dt, float& playerHP) {
        for (auto& m : minions) {
            if (!m.alive) continue;
            float dx = px - m.x, dy = py - m.y;
            float len = std::sqrt(dx * dx + dy * dy) + 1e-3f;
            float spd = phase2 ? 230.0f : 185.0f;
            float steer = std::min(1.0f, dt * 2.2f);
            m.vx += (dx / len * spd - m.vx) * steer;
            m.vy += (dy / len * spd - m.vy) * steer;
            m.x += m.vx * dt;
            m.y += m.vy * dt;

            float hit = minionHit(m.kind);
            if (dx * dx + dy * dy < (hit + 8.0f) * (hit + 8.0f))
                HurtPlayer(playerHP, (phase2 ? 12.5f : 11.0f) * dt);
        }
    }

    void fireDdosWave(std::vector<Bullet>& bullets) {
        float dx = ddosDirX, dy = ddosDirY;
        float perpX = -dy, perpY = dx;
        float ox = worldX - dx * (DDOS_SPAN + 20.0f);
        float oy = worldY - dy * (DDOS_SPAN + 20.0f);

        // 틈 2개 (서로 200 이상 떨어지게)
        float gap1 = (float)(rand() % (int)(DDOS_SPAN * 1.6f)) - DDOS_SPAN * 0.8f;
        float gap2 = gap1;
        for (int tries = 0; tries < 8 && fabsf(gap2 - gap1) < 200.0f; tries++)
            gap2 = (float)(rand() % (int)(DDOS_SPAN * 1.6f)) - DDOS_SPAN * 0.8f;
        float gapHalf = phase2 ? 48.0f : 60.0f;

        glm::vec3 col(1.0f, 0.62f, 0.15f);
        float spd = phase2 ? 470.0f : 400.0f;
        for (float s = -DDOS_SPAN; s <= DDOS_SPAN; s += DDOS_STEP) {
            if (fabsf(s - gap1) < gapHalf || fabsf(s - gap2) < gapHalf) continue;
            float bx = ox + perpX * s;
            float by = oy + perpY * s;
            firePacket(bullets, bx, by, bx + dx * 200.0f, by + dy * 200.0f,
                       spd, col, phase2 ? 10.0f : 9.0f);
        }
    }

    void updateDdos(float dt, std::vector<Bullet>& bullets) {
        if (ddosTele > 0.0f) {
            ddosTele -= dt;
            if (ddosTele <= 0.0f)
                fireDdosWave(bullets);
            return;
        }
        ddosCd -= dt;
        if (ddosCd <= 0.0f) {
            ddosCd = (phase2 ? DDOS_GAP2 : DDOS_GAP) + (float)(rand() % 20) * 0.05f;
            int dir = rand() % 4;
            switch (dir) {
            case 0:  ddosDirX = 1.0f;  ddosDirY = 0.0f;  break;
            case 1:  ddosDirX = -1.0f; ddosDirY = 0.0f;  break;
            case 2:  ddosDirX = 0.0f;  ddosDirY = 1.0f;  break;
            default: ddosDirX = 0.0f;  ddosDirY = -1.0f; break;
            }
            ddosTele = DDOS_TELE_DUR;
        }
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& bullets) {
        if (!alive) return;
        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;
        lastPx = px; lastPy = py;

        if (exposedT > 0.0f) exposedT -= dt;
        if (stunT > 0.0f) stunT -= dt;

        driftT += dt;
        logScroll += dt * 32.0f;
        float drift = (stunT > 0.0f) ? 0.0f : (phase2 ? 52.0f : 68.0f);
        worldX += cosf(driftT * 0.22f) * drift * dt;
        worldY += sinf(driftT * 0.31f) * drift * dt;
        float m = 80.0f;
        if (worldX < m) worldX = m;
        if (worldX > screenW - m) worldX = screenW - m;
        if (worldY < m) worldY = m;
        if (worldY > screenH - m) worldY = screenH - m;

        if (stunT <= 0.0f)
            orbitRot += ORBIT_SPD * dt;
        syncHostPositions();
        updateRebuild(dt);

        if (stunT <= 0.0f)
            updateCommand(px, py, dt, bullets);
        else if (cmdPhase != CmdPhase::Idle) {
            // 스턴 중 명령 취소
            cmdPhase = CmdPhase::Idle;
            cmdHost = -1;
        }

        updateHosts(px, py, dt, playerHP, bullets);
        updateMinions(px, py, dt, playerHP);

        if (stunT <= 0.0f)
            updateDdos(dt, bullets);

        // 호스트 전멸 → 코어 패닉: 방화벽 0% + 발악 방사탄
        if (aliveHosts() == 0 && stunT <= 0.0f) {
            panicCd -= dt;
            if (panicCd <= 0.0f) {
                panicCd = phase2 ? 2.0f : 2.6f;
                int n = 8;
                float off = (float)(rand() % 628) * 0.01f;
                for (int i = 0; i < n; i++) {
                    float a = off + (float)i / (float)n * 6.2831853f;
                    firePacket(bullets, worldX, worldY,
                               worldX + cosf(a) * 200.0f, worldY + sinf(a) * 200.0f,
                               PKT_SPD, glm::vec3(1.0f, 0.35f, 0.25f), 8.5f);
                }
            }
        }

        float bdx = px - worldX, bdy = py - worldY;
        if (bdx * bdx + bdy * bdy < BODY * BODY)
            HurtPlayer(playerHP, (phase2 ? 13.0f : 8.0f) * dt);
    }

    // ── 렌더 헬퍼 ──
    static void wireSeg(float x0, float y0, float x1, float y1, float thick,
                        float r, float g, float b, float a) {
        float dx = x1 - x0, dy = y1 - y0;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.5f) return;
        float nx = -dy / len * thick * 0.5f;
        float ny =  dx / len * thick * 0.5f;
        BatchTri(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, r, g, b, a);
        BatchTri(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, r, g, b, a);
    }

    static void hollowRing(float cx, float cy, float rad, int seg, float thick,
                           float r, float g, float b, float a) {
        float px = cx + rad, py = cy;
        for (int s = 1; s <= seg; s++) {
            float th = (float)s / (float)seg * 6.2831853f;
            float nx = cx + rad * cosf(th), ny = cy + rad * sinf(th);
            wireSeg(px, py, nx, ny, thick, r, g, b, a);
            px = nx; py = ny;
        }
    }

    void renderCore(float time) {
        BindMainShader();
        float pulse = 0.5f + 0.5f * sinf(time * 4.0f);
        const float GR = 0.22f, GG = 0.92f, GB = 0.48f;
        bool exp = exposed();

        // ── 링크 (코어 ↔ 호스트, 점선) ──
        for (int i = 0; i < NHOST; i++) {
            const Host& h = hosts[i];
            if (!h.alive) continue;
            float lx = h.x - worldX, ly = h.y - worldY;
            for (int s = 0; s <= 16; s++) {
                float u = (float)s / 16.0f;
                float blink = (sinf(time * 14.0f - u * 10.0f) > 0.0f) ? 1.0f : 0.25f;
                drawCircle(worldX + lx * u, worldY + ly * u, 2.2f, GR, GG, GB, 0.24f * blink);
            }
        }

        // ── 명령 신호 펄스 (Travel: 링크 위 이동) ──
        if (cmdPhase == CmdPhase::Travel && cmdHost >= 0 && hosts[cmdHost].alive) {
            const Host& h = hosts[cmdHost];
            float u = cmdT / CMD_TRAVEL;
            float sx = worldX + (h.x - worldX) * u;
            float sy = worldY + (h.y - worldY) * u;
            drawCircle(sx, sy, 9.0f + pulse * 3.0f, 1.0f, 0.85f, 0.25f, 0.85f);
            for (int tr = 1; tr <= 3; tr++) {
                float tu = u - (float)tr * 0.06f;
                if (tu < 0.0f) break;
                drawCircle(worldX + (h.x - worldX) * tu, worldY + (h.y - worldY) * tu,
                           6.0f - (float)tr * 1.4f, 1.0f, 0.8f, 0.25f, 0.4f - (float)tr * 0.1f);
            }
        }

        // ── 실행 충전 (Execute: 요격 찬스 링) ──
        if (cmdPhase == CmdPhase::Execute && cmdHost >= 0 && hosts[cmdHost].alive) {
            const Host& h = hosts[cmdHost];
            float remain = 1.0f - cmdT / CMD_EXECUTE;
            float warn = 0.5f + 0.5f * sinf(time * 18.0f);
            hollowRing(h.x, h.y, HOST_HIT * 1.5f + remain * 26.0f, 14, 2.4f,
                       1.0f, 0.75f, 0.2f, 0.4f + 0.4f * warn);
            wchar_t ex[] = L"EXEC";
            float ew = g_TextS.Width(ex, 0.36f * g_ViewZoom);
            g_TextS.Draw(ex, W2SX(h.x) - ew * 0.5f, W2SY(h.y - HOST_H - 20.0f),
                         0.36f * g_ViewZoom, 1.0f, 0.8f, 0.2f, 0.9f);
        }

        // ── DDoS 예고 ──
        if (ddosTele > 0.0f) {
            float warn = 0.25f + 0.3f * sinf(time * 16.0f);
            float dx = ddosDirX, dy = ddosDirY;
            float perpX = -dy, perpY = dx;
            float ox = worldX - dx * (DDOS_SPAN + 20.0f);
            float oy = worldY - dy * (DDOS_SPAN + 20.0f);
            for (float s = -DDOS_SPAN; s <= DDOS_SPAN; s += DDOS_STEP * 1.5f) {
                float bx = ox + perpX * s, by = oy + perpY * s;
                drawCircle(bx, by, 4.0f, 1.0f, 0.55f, 0.15f, warn);
                // 진행 방향 화살 점
                float march = fmodf(time * 140.0f, 60.0f);
                drawCircle(bx + dx * march, by + dy * march, 2.4f, 1.0f, 0.5f, 0.15f, warn * 0.7f);
            }
        }

        // ── 호스트 (역할별 형태) ──
        for (int i = 0; i < NHOST; i++) {
            const Host& h = hosts[i];
            if (!h.alive) continue;
            float hw = HOST_W, hh = HOST_H;
            float hx = h.x - hw * 0.5f, hy = h.y - hh * 0.5f;

            switch (h.role) {
            case HostRole::Turret: {
                drawRect(hx, hy, hw, hh, 0.14f, 0.1f, 0.08f, 1.0f);
                drawRect(hx + 4.0f, hy + 4.0f, hw - 8.0f, hh - 8.0f, 0.3f, 0.16f, 0.08f, 1.0f);
                float ba = atan2f(lastPy - h.y, lastPx - h.x);
                wireSeg(h.x, h.y, h.x + cosf(ba) * (HOST_W * 0.75f),
                        h.y + sinf(ba) * (HOST_W * 0.75f), 5.0f, 1.0f, 0.55f, 0.25f, 0.95f);
                drawCircle(h.x, h.y, 7.0f, 1.0f, 0.6f, 0.25f, 0.9f);
            } break;
            case HostRole::Spawner: {
                drawRect(hx, hy, hw, hh, 0.08f, 0.16f, 0.1f, 1.0f);
                drawRect(hx + 4.0f, hy + 4.0f, hw - 8.0f, hh - 8.0f, 0.1f, 0.3f, 0.16f, 1.0f);
                float hatch = 0.5f + 0.5f * sinf(time * 5.0f + (float)i);
                drawRect(h.x - 9.0f, h.y - 5.0f, 18.0f, 10.0f, GR, GG, GB, 0.4f + 0.5f * hatch);
            } break;
            case HostRole::Warden: {
                drawRect(hx, hy, hw, hh, 0.08f, 0.12f, 0.2f, 1.0f);
                drawRect(hx + 4.0f, hy + 4.0f, hw - 8.0f, hh - 8.0f, 0.1f, 0.2f, 0.36f, 1.0f);
                drawDiamond(h.x, h.y, 16.0f + pulse * 3.0f, 0.3f, 0.75f, 1.0f, 0.85f);
                hollowRing(h.x, h.y, HOST_HIT * 1.15f, 12, 1.4f, 0.3f, 0.75f, 1.0f, 0.3f + 0.2f * pulse);
            } break;
            }

            // 역할 태그
            {
                const wchar_t* tag = roleTag(h.role);
                float ts = 0.3f * g_ViewZoom;
                float tw = g_TextS.Width(tag, ts);
                g_TextS.Draw(tag, W2SX(h.x) - tw * 0.5f, W2SY(hy + hh + 4.0f), ts,
                             0.85f, 0.9f, 0.95f, 0.7f);
                BindMainShader();
            }

            if (h.hp < h.maxHp) {
                float hf = h.hp / h.maxHp;
                if (hf < 0.0f) hf = 0.0f;
                drawRect(hx, hy - 7.0f, hw * hf, 4.0f, GR, GG, GB, 0.9f);
            }
        }

        // ── 코어 터미널 ──
        float tx = worldX - TERM_W * 0.5f, ty = worldY - TERM_H * 0.5f;
        drawRect(tx - 6.0f, ty - 6.0f, TERM_W + 12.0f, TERM_H + 12.0f, 0.04f, 0.06f, 0.05f, 0.96f);
        drawRect(tx, ty, TERM_W, TERM_H, 0.02f, 0.04f, 0.03f, 1.0f);
        float hdR = exp ? 0.7f : (phase2 ? 0.55f : 0.1f);
        float hdG = exp ? 0.1f : (phase2 ? 0.14f : 0.26f);
        float hdB = exp ? 0.08f : (phase2 ? 0.08f : 0.12f);
        drawRect(tx, ty, TERM_W, 14.0f, hdR, hdG, hdB, 1.0f);

        const wchar_t* title = exp    ? L"C2_RELAY.sys  !! EXPOSED !!"
                             : (stunT > 0.0f) ? L"C2_RELAY.sys  ** OVERLOAD **"
                             : phase2 ? L"C2_RELAY.sys  !!! DEFCON 2 !!!"
                                      : L"C2_RELAY.sys  :: relay active";
        float tScale = 0.38f * g_ViewZoom;
        g_TextS.Draw(title, W2SX(tx + 8.0f), W2SY(ty + 2.0f), tScale, 1.0f, 0.95f, 0.9f, 1.0f);

        wchar_t stat[80];
        if (exp)
            swprintf_s(stat, L"HOST %d/%d  FW --- BREACH  PKT %d",
                       aliveHosts(), NHOST, aliveMinions());
        else
            swprintf_s(stat, L"HOST %d/%d  FW %d%%  PKT %d",
                       aliveHosts(), NHOST, (int)(hostShieldPercent() + 0.5f), aliveMinions());
        g_TextS.Draw(stat, W2SX(tx + 8.0f), W2SY(ty + TERM_H - 22.0f),
                     0.30f * g_ViewZoom, exp ? 1.0f : GR, exp ? 0.35f : GG, exp ? 0.25f : GB, 0.85f);

        float lineY = ty + 18.0f;
        for (int ln = 0; ln < 6; ln++) {
            float ly = lineY + ln * 12.0f;
            float wob = fmodf(logScroll + ln * 41.0f, 140.0f);
            drawRect(tx + 8.0f, ly, 22.0f + wob, 2.0f, GR * 0.7f, GG * 0.7f, GB * 0.7f, 0.55f);
        }
        float curBlink = (sinf(time * 8.0f) > 0.0f) ? 1.0f : 0.0f;
        drawRect(tx + 10.0f + fmodf(logScroll, 60.0f), ty + TERM_H - 28.0f,
                 7.0f, 10.0f, GR, GG, GB, curBlink);

        if (exp) {
            float fl = 0.5f + 0.5f * sinf(time * 20.0f);
            drawNeonBorder(tx - 3.0f, ty - 3.0f, TERM_W + 6.0f, TERM_H + 6.0f,
                           1.0f, 0.15f + 0.2f * fl, 0.1f);
            // 노출 잔여 시간 바
            float ef = exposedT / (phase2 ? EXPOSED_DUR2 : EXPOSED_DUR);
            if (ef < 0.0f) ef = 0.0f;
            drawRect(tx, ty - 14.0f, TERM_W * ef, 5.0f, 1.0f, 0.3f, 0.15f, 0.95f);
        } else {
            drawNeonBorder(tx - 3.0f, ty - 3.0f, TERM_W + 6.0f, TERM_H + 6.0f, GR, GG * pulse, GB);
        }

        if (hp < maxHp) {
            float hf = hp / maxHp;
            if (hf < 0.0f) hf = 0.0f;
            drawRect(tx, ty + TERM_H + 8.0f, TERM_W * hf, 5.0f,
                     phase2 ? 1.0f : GR, phase2 ? 0.4f : GG, phase2 ? 0.2f : GB, 0.95f);
        }
    }

    void renderMinions(float time) {
        BindMainShader();
        const float GR = 0.22f, GG = 0.92f, GB = 0.48f;
        for (auto& m : minions) {
            if (!m.alive) continue;
            float wob = sinf(time * 9.0f + m.pulseAng + m.x * 0.01f) * 2.0f;
            drawCircle(m.x, m.y, 11.0f + wob, GR, GG, GB, 0.16f);
            drawRect(m.x - 8.0f, m.y - 3.0f + wob * 0.4f, 16.0f, 6.0f, GR, GG, GB, 0.9f);
            if (m.hp < m.maxHp) {
                float hf = m.hp / m.maxHp;
                if (hf < 0.0f) hf = 0.0f;
                float w = minionHit(m.kind) * 1.5f;
                drawRect(m.x - w * 0.5f, m.y - minionHit(m.kind) - 7.0f, w * hf, 3.0f,
                         GR, GG, GB, 0.85f);
            }
        }
    }
};
