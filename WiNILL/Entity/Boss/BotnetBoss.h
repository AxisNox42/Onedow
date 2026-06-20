#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"
#include "TextRenderer.h"

// ?????????????????????????????????????????????????????????????
// C2_RELAY.sys ??Command & Control (?꾨㈃??
//   쨌 肄붿뼱(?곕???HOST) = C2 媛吏?李?/ adds = ?ㅽ듃?뚰겕 ?⑦궥(媛吏쒖갹 ?놁쓬)
//   쨌 Minion ? 留??꾩뿭 ?대룞쨌援먯쟾, DrawAppWindow ?놁씠 ?⑥닚 ?꾪삎留?//   쨌 HOST 1湲곕떦 蹂몄껜 ?쇳빐 ??1% (理쒕? 88% 媛먯냼)
// ?????????????????????????????????????????????????????????????
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
    float floodTimer = 0.0f;
    float logScroll = 0.0f;
    float hostHpBase = 0.0f;
    float ambientCd = 6.0f;

    struct Host {
        float slotAngle = 0.0f;
        float x = 0.0f, y = 0.0f;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = true;
        float fireCd = 0.0f;
        float rebuildCd = 0.0f;
        bool  shellMarked = false;
    };
    Host hosts[8];

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

    enum class Skill {
        None, DeployRush, LanSweep, RemoteShell, PacketStorm, CoordinatedStrike
    };
    Skill activeSkill = Skill::None;
    float skillCd = 4.0f;
    float skillT = 0.0f;
    bool  skillShot = false;
    float sweepAng = 0.0f;
    float sweepTick = 0.0f;
    int   shellHost = -1;
    int   shellPhase = 0;
    float shellT = 0.0f;
    float shellTx = 0.0f, shellTy = 0.0f;
    float beaconRing = 0.0f;
    int   beaconWave = 0;

    static constexpr int   NHOST      = 8;
    static constexpr int   MAX_MINION = 24;
    static constexpr float BODY       = 62.0f;
    static constexpr float TERM_W     = 148.0f;
    static constexpr float TERM_H     = 108.0f;
    static constexpr float ORBIT_R    = 268.0f;
    static constexpr float ORBIT_SPD  = 0.42f;
    static constexpr float HOST_W     = 40.0f;
    static constexpr float HOST_H     = 32.0f;
    static constexpr float HOST_HIT   = 24.0f;
    static constexpr float MAP_PAD    = 52.0f;
    static constexpr float FIRE_INT   = 2.5f;
    static constexpr float P2_FIRE    = 1.15f;
    static constexpr float PKT_SPD      = 440.0f;
    static constexpr float PKT_SPD2     = 560.0f;
    static constexpr float FLOOD_INT    = 5.2f;
    static constexpr float FLOOD_N      = 14;
    static constexpr float SKILL_GAP    = 6.8f;
    static constexpr float P2_SKILL     = 4.5f;
    static constexpr float HOST_DMG_RED = 0.11f;   // HOST 1湲곕떦 蹂몄껜 ?쇳빐 ??1%

    BotnetBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.45f;
        hostHpBase = hpInit * 0.13f;
        if (hostHpBase < 160.0f) hostHpBase = 160.0f;
        for (int i = 0; i < NHOST; i++) {
            hosts[i].slotAngle = (float)i * (6.2831853f / (float)NHOST);
            hosts[i].hp = hosts[i].maxHp = hostHpBase;
            hosts[i].fireCd = (float)(rand() % 80) * 0.02f;
            hosts[i].rebuildCd = 14.0f + (float)(rand() % 60) * 0.1f;
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

    float bodyDamageTakenMult() const {
        int n = aliveHosts();
        if (n <= 0) return 1.0f;
        float red = HOST_DMG_RED * (float)n;
        if (red > 0.88f) red = 0.88f;
        return 1.0f - red;
    }

    float hostShieldPercent() const {
        return (1.0f - bodyDamageTakenMult()) * 100.0f;
    }

    static float minionHit(MinionKind k) {
        switch (k) {
        case MinionKind::Heavy: return 22.0f;
        case MinionKind::Pulse: return 16.0f;
        default:                return 13.0f;
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
        float spd = phase2 ? 400.0f : 330.0f;
        m.vx = dx / len * spd;
        m.vy = dy / len * spd;
        m.maxHp = m.hp = hostHpBase * 0.2f;
        if (m.maxHp < 42.0f) m.maxHp = m.hp = 42.0f;
        return m;
    }

    Minion spawnHeavy(float sx, float sy, float tx, float ty) {
        Minion m;
        m.kind = MinionKind::Heavy;
        m.x = sx; m.y = sy;
        float dx = tx - sx, dy = ty - sy;
        float len = std::sqrt(dx * dx + dy * dy) + 1e-3f;
        m.vx = dx / len * 120.0f;
        m.vy = dy / len * 120.0f;
        m.maxHp = m.hp = hostHpBase * 0.5f;
        m.fireCd = 1.5f;
        return m;
    }

    Minion spawnPulse(float sx, float sy, float ang) {
        Minion m;
        m.kind = MinionKind::Pulse;
        m.x = sx; m.y = sy;
        m.pulseAng = ang;
        m.maxHp = m.hp = hostHpBase * 0.16f;
        if (m.maxHp < 38.0f) m.maxHp = m.hp = 38.0f;
        return m;
    }

    void deployRushFromHosts(int count, float tx, float ty) {
        for (int n = 0; n < count; n++) {
            for (int i = 0; i < NHOST; i++) {
                if (!hosts[i].alive) continue;
                addMinion(spawnRush(hosts[i].x, hosts[i].y, tx, ty));
                break;
            }
        }
    }

    void deployMapRush(float tx, float ty, int count) {
        for (int i = 0; i < count; i++) {
            int edge = rand() % 4;
            float sx = 0, sy = 0;
            switch (edge) {
            case 0: sx = MAP_PAD + (float)(rand() % (screenW - (int)MAP_PAD * 2)); sy = -30.0f; break;
            case 1: sx = (float)screenW + 30.0f; sy = MAP_PAD + (float)(rand() % (screenH - (int)MAP_PAD * 2)); break;
            case 2: sx = MAP_PAD + (float)(rand() % (screenW - (int)MAP_PAD * 2)); sy = (float)screenH + 30.0f; break;
            default: sx = -30.0f; sy = MAP_PAD + (float)(rand() % (screenH - (int)MAP_PAD * 2)); break;
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

    void startSkill(Skill s) {
        activeSkill = s;
        skillT = 0.0f;
        skillShot = false;
        if (s == Skill::DeployRush) { activeSkill = Skill::None; return; }
        if (s == Skill::LanSweep) { sweepAng = orbitRot; sweepTick = 0.0f; }
        if (s == Skill::RemoteShell) {
            shellPhase = 0; shellHost = -1;
            for (int i = 0; i < NHOST; i++)
                if (hosts[i].alive) { shellHost = i; break; }
            if (shellHost < 0) activeSkill = Skill::None;
            else hosts[shellHost].shellMarked = true;
        }
        if (s == Skill::PacketStorm) { beaconRing = 0.0f; beaconWave = 0; }
    }

    void pickSkill() {
        if (activeSkill != Skill::None) return;
        int roll = rand() % 100;
        if (phase2) {
            if      (roll < 24) startSkill(Skill::DeployRush);
            else if (roll < 44) startSkill(Skill::LanSweep);
            else if (roll < 60) startSkill(Skill::RemoteShell);
            else if (roll < 76) startSkill(Skill::PacketStorm);
            else                startSkill(Skill::CoordinatedStrike);
        } else {
            if      (roll < 26) startSkill(Skill::DeployRush);
            else if (roll < 46) startSkill(Skill::LanSweep);
            else if (roll < 62) startSkill(Skill::RemoteShell);
            else if (roll < 78) startSkill(Skill::PacketStorm);
            else                startSkill(Skill::CoordinatedStrike);
        }
    }

    void updateRebuild(float dt) {
        for (int i = 0; i < NHOST; i++) {
            Host& h = hosts[i];
            if (h.alive) continue;
            h.rebuildCd -= dt;
            if (h.rebuildCd <= 0.0f) {
                h.alive = true;
                h.hp = h.maxHp = hostHpBase * 0.42f;
                h.fireCd = 0.5f;
                h.rebuildCd = phase2 ? 16.0f : 22.0f;
            }
        }
    }

    void updateMinions(float px, float py, float dt, float& playerHP,
                       std::vector<Bullet>& bullets) {
        for (auto& m : minions) {
            if (!m.alive) continue;
            switch (m.kind) {
            case MinionKind::Rush: {
                m.x += m.vx * dt;
                m.y += m.vy * dt;
                float dx = px - m.x, dy = py - m.y;
                if (dx * dx + dy * dy < 400.0f) {
                    float len = std::sqrt(dx * dx + dy * dy) + 1e-3f;
                    m.vx = dx / len * 400.0f;
                    m.vy = dy / len * 400.0f;
                }
                if (m.x < -90.0f || m.x > screenW + 90.0f || m.y < -90.0f || m.y > screenH + 90.0f)
                    m.alive = false;
            } break;
            case MinionKind::Heavy: {
                m.x += m.vx * dt;
                m.y += m.vy * dt;
                m.fireCd -= dt;
                if (m.fireCd <= 0.0f) {
                    m.fireCd = phase2 ? 2.0f : 2.8f;
                    firePacket(bullets, m.x, m.y, px, py, PKT_SPD * 0.75f,
                               glm::vec3(0.35f, 0.65f, 1.0f), 8.0f);
                }
            } break;
            case MinionKind::Pulse: {
                m.pulseAng += dt * (phase2 ? 2.2f : 1.6f);
                float rad = 180.0f + sinf(m.pulseAng * 0.7f) * 90.0f;
                m.x = px + cosf(m.pulseAng) * rad;
                m.y = py + sinf(m.pulseAng) * rad * 0.75f;
            } break;
            }
            float hit = minionHit(m.kind);
            float dx = px - m.x, dy = py - m.y;
            if (dx * dx + dy * dy < (hit + 8.0f) * (hit + 8.0f)) {
                float dps = (m.kind == MinionKind::Heavy) ? 14.0f
                          : (m.kind == MinionKind::Pulse) ? 10.0f : 11.0f;
                if (phase2) dps *= 1.15f;
                playerHP -= dps * dt;
            }
        }
    }

    void updateSkill(float px, float py, float dt, std::vector<Bullet>& bullets) {
        if (activeSkill == Skill::None) return;
        skillT += dt;
        glm::vec3 green(0.25f, 0.95f, 0.55f);
        glm::vec3 red(1.0f, 0.35f, 0.25f);

        switch (activeSkill) {
        case Skill::LanSweep:
            if (skillT < 0.85f) break;
            sweepTick += dt;
            sweepAng += (phase2 ? 2.4f : 1.8f) * dt;
            if (sweepTick >= 0.1f) {
                sweepTick = 0.0f;
                for (int k = 0; k < 3; k++) {
                    float ang = sweepAng + (float)k * 0.35f;
                    firePacket(bullets, worldX, worldY,
                               worldX + cosf(ang) * 600.0f, worldY + sinf(ang) * 600.0f,
                               PKT_SPD, green, phase2 ? 10.0f : 7.5f);
                }
            }
            if (skillT >= (phase2 ? 2.8f : 2.2f)) activeSkill = Skill::None;
            break;

        case Skill::RemoteShell:
            if (shellHost < 0 || !hosts[shellHost].alive) { activeSkill = Skill::None; break; }
            {
                Host& sh = hosts[shellHost];
                if (shellPhase == 0) {
                    if (skillT >= 1.2f) {
                        shellPhase = 1; shellT = 0.0f;
                        shellTx = px; shellTy = py;
                    }
                } else if (shellPhase == 1) {
                    shellT += dt;
                    float dx = shellTx - sh.x, dy = shellTy - sh.y;
                    float len = std::sqrt(dx * dx + dy * dy) + 1e-3f;
                    sh.x += dx / len * 720.0f * dt;
                    sh.y += dy / len * 720.0f * dt;
                    if (shellT >= 1.0f || len < 36.0f) {
                        for (int i = 0; i < 10; i++) {
                            float ang = (float)i / 10.0f * 6.2831853f;
                            firePacket(bullets, sh.x, sh.y,
                                       sh.x + cosf(ang) * 160.0f, sh.y + sinf(ang) * 160.0f,
                                       PKT_SPD2, red, 12.0f);
                        }
                        sh.shellMarked = false;
                        shakePulse = true;
                        shellPhase = 2;
                    }
                } else activeSkill = Skill::None;
            }
            break;

        case Skill::PacketStorm:
            if (skillT < 0.5f) break;
            beaconRing += dt * (phase2 ? 220.0f : 180.0f);
            if (beaconRing > 55.0f) {
                beaconRing = 0.0f;
                int n = 12 + beaconWave * 2;
                for (int i = 0; i < n; i++) {
                    float ang = (float)i / (float)n * 6.2831853f;
                    float rad = 120.0f + beaconWave * 80.0f;
                    firePacket(bullets, worldX, worldY,
                               worldX + cosf(ang) * rad, worldY + sinf(ang) * rad,
                               PKT_SPD * 0.8f, green, 8.0f);
                }
                beaconWave++;
                if (beaconWave >= (phase2 ? 4 : 3)) activeSkill = Skill::None;
            }
            break;

        case Skill::CoordinatedStrike:
            if (!skillShot && skillT >= 0.3f) {
                skillShot = true;
                for (int i = 0; i < NHOST; i++) {
                    if (!hosts[i].alive) continue;
                    float base = atan2f(py - hosts[i].y, px - hosts[i].x);
                    for (int b = 0; b < (phase2 ? 3 : 2); b++) {
                        float ang = base + ((float)b - 0.5f) * 0.2f;
                        firePacket(bullets, hosts[i].x, hosts[i].y,
                                   hosts[i].x + cosf(ang) * 260.0f,
                                   hosts[i].y + sinf(ang) * 260.0f,
                                   PKT_SPD2, phase2 ? red : green, phase2 ? 11.0f : 8.0f);
                    }
                }
            }
            if (skillT >= 0.85f) activeSkill = Skill::None;
            break;

        default: break;
        }
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& bullets) {
        if (!alive) return;
        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;

        driftT += dt;
        logScroll += dt * 32.0f;
        worldX += cosf(driftT * 0.22f) * (phase2 ? 52.0f : 68.0f) * dt;
        worldY += sinf(driftT * 0.31f) * (phase2 ? 52.0f : 68.0f) * dt;
        float m = 80.0f;
        if (worldX < m) worldX = m;
        if (worldX > screenW - m) worldX = screenW - m;
        if (worldY < m) worldY = m;
        if (worldY > screenH - m) worldY = screenH - m;

        orbitRot += ORBIT_SPD * dt;
        syncHostPositions();
        updateRebuild(dt);

        ambientCd -= dt;
        if (ambientCd <= 0.0f) {
            ambientCd = phase2 ? 5.0f : 7.0f;
            if (aliveMinions() < MAX_MINION - 2) {
                deployMapRush(px, py, phase2 ? 2 : 1);
                if (phase2 && aliveMinions() < MAX_MINION - 3)
                    addMinion(spawnPulse(worldX, worldY, orbitRot + (float)(rand() % 628) * 0.01f));
            }
        }

        updateMinions(px, py, dt, playerHP, bullets);

        skillCd -= dt;
        if (skillCd <= 0.0f && activeSkill == Skill::None) {
            pickSkill();
            if (activeSkill == Skill::DeployRush) {
                deployRushFromHosts(phase2 ? 4 : 3, px, py);
                deployMapRush(px, py, phase2 ? 4 : 3);
                if (phase2 && aliveMinions() < MAX_MINION - 2)
                    addMinion(spawnHeavy(worldX, worldY, px, py));
                activeSkill = Skill::None;
            }
            skillCd = phase2 ? P2_SKILL : SKILL_GAP;
            skillCd += (float)(rand() % 30) * 0.05f;
        }
        updateSkill(px, py, dt, bullets);

        float bdx = px - worldX, bdy = py - worldY;
        if (bdx * bdx + bdy * bdy < BODY * BODY)
            playerHP -= (phase2 ? 13.0f : 8.0f) * dt;

        float fireInt = phase2 ? P2_FIRE : FIRE_INT;
        glm::vec3 pktCol = phase2 ? glm::vec3(1.0f, 0.35f, 0.25f)
                                  : glm::vec3(0.25f, 0.95f, 0.55f);
        for (int i = 0; i < NHOST; i++) {
            Host& h = hosts[i];
            if (!h.alive || h.shellMarked) continue;
            h.fireCd -= dt;
            if (h.fireCd <= 0.0f && activeSkill != Skill::RemoteShell) {
                h.fireCd = fireInt + (float)(rand() % 20) * 0.02f;
                firePacket(bullets, h.x, h.y, px, py,
                           phase2 ? PKT_SPD2 : PKT_SPD, pktCol, phase2 ? 11.0f : 7.5f);
            }
            float hdx = px - h.x, hdy = py - h.y;
            if (hdx * hdx + hdy * hdy < HOST_HIT * HOST_HIT)
                playerHP -= (phase2 ? 10.0f : 7.0f) * dt;
        }

        if (phase2) {
            floodTimer += dt;
            if (floodTimer >= FLOOD_INT) {
                floodTimer = 0.0f;
                for (int i = 0; i < FLOOD_N; i++) {
                    float ang = (float)i / (float)FLOOD_N * 6.2831853f + orbitRot;
                    firePacket(bullets, worldX, worldY,
                               worldX + cosf(ang) * 450.0f, worldY + sinf(ang) * 450.0f,
                               PKT_SPD2 * 0.85f, glm::vec3(1.0f, 0.55f, 0.2f), 10.0f);
                }
            }
        }
    }

    void renderCore(float time) {
        BindMainShader();
        float pulse = 0.5f + 0.5f * sinf(time * 4.0f);
        const float GR = 0.22f, GG = 0.92f, GB = 0.48f;

        if (activeSkill == Skill::LanSweep && skillT >= 0.3f && skillT < 0.85f) {
            float warn = 0.2f + 0.18f * sinf(time * 14.0f);
            for (int i = 0; i < 5; i++) {
                float ang = sweepAng + (float)i * 0.35f;
                for (int s = 0; s < 14; s++) {
                    float u = (float)s / 14.0f;
                    drawCircle(worldX + cosf(ang) * (80.0f + u * 520.0f),
                               worldY + sinf(ang) * (80.0f + u * 520.0f),
                               2.8f, 1.0f, 0.45f, 0.2f, warn * (1.0f - u * 0.35f));
                }
            }
        }

        for (int i = 0; i < NHOST; i++) {
            const Host& h = hosts[i];
            if (!h.alive) continue;
            float lx = h.x - worldX, ly = h.y - worldY;
            for (int s = 0; s <= 16; s++) {
                float u = (float)s / 16.0f;
                float blink = (sinf(time * 14.0f - u * 10.0f) > 0.0f) ? 1.0f : 0.25f;
                drawCircle(worldX + lx * u, worldY + ly * u, 2.2f, GR, GG, GB, 0.28f * blink);
            }
        }

        for (int i = 0; i < NHOST; i++) {
            const Host& h = hosts[i];
            if (!h.alive) continue;
            float hw = HOST_W, hh = HOST_H;
            float hx = h.x - hw * 0.5f, hy = h.y - hh * 0.5f;
            if (h.shellMarked) {
                float fl = 0.5f + 0.5f * sinf(time * 18.0f);
                drawCircle(h.x, h.y, HOST_HIT * 1.3f, 1.0f, 0.2f, 0.15f, 0.28f * fl);
            }
            drawRect(hx, hy, hw, hh * 0.72f, 0.12f, 0.14f, 0.18f, 1.0f);
            drawRect(hx + 3.0f, hy + 3.0f, hw - 6.0f, hh * 0.72f - 6.0f, 0.08f, 0.22f, 0.14f, 1.0f);
            drawRect(hx + hw * 0.35f, hy + hh * 0.72f, hw * 0.3f, hh * 0.28f, 0.18f, 0.2f, 0.24f, 1.0f);
            if (h.hp < h.maxHp) {
                float hf = h.hp / h.maxHp;
                if (hf < 0.0f) hf = 0.0f;
                drawRect(hx, hy - 6.0f, hw * hf, 4.0f, GR, GG, GB, 0.9f);
            }
        }

        float tx = worldX - TERM_W * 0.5f, ty = worldY - TERM_H * 0.5f;
        drawRect(tx - 6.0f, ty - 6.0f, TERM_W + 12.0f, TERM_H + 12.0f, 0.04f, 0.06f, 0.05f, 0.96f);
        drawRect(tx, ty, TERM_W, TERM_H, 0.02f, 0.04f, 0.03f, 1.0f);
        drawRect(tx, ty, TERM_W, 14.0f, phase2 ? 0.55f : 0.1f, phase2 ? 0.14f : 0.26f,
                 phase2 ? 0.08f : 0.12f, 1.0f);

        extern TextRenderer g_TextS;
        const wchar_t* title = phase2 ? L"C2_RELAY.sys  !!! OVERLOAD !!!"
                                      : L"C2_RELAY.sys  ??relay active";
        g_TextS.Draw(title, tx + 8.0f, ty + 2.0f, 0.38f, 1.0f, 0.95f, 0.9f, 1.0f);

        wchar_t stat[64];
        swprintf_s(stat, L"HOST %d/%d  SHIELD %d%%  PKT %d",
                   aliveHosts(), NHOST, (int)(hostShieldPercent() + 0.5f), aliveMinions());
        g_TextS.Draw(stat, tx + 8.0f, ty + TERM_H - 22.0f, 0.30f, GR, GG, GB, 0.85f);

        float lineY = ty + 18.0f;
        for (int ln = 0; ln < 6; ln++) {
            float ly = lineY + ln * 12.0f;
            float wob = fmodf(logScroll + ln * 41.0f, 140.0f);
            drawRect(tx + 8.0f, ly, 22.0f + wob, 2.0f, GR * 0.7f, GG * 0.7f, GB * 0.7f, 0.55f);
        }
        float curBlink = (sinf(time * 8.0f) > 0.0f) ? 1.0f : 0.0f;
        drawRect(tx + 10.0f + fmodf(logScroll, 60.0f), ty + TERM_H - 28.0f,
                 7.0f, 10.0f, GR, GG, GB, curBlink);
        drawNeonBorder(tx - 3.0f, ty - 3.0f, TERM_W + 6.0f, TERM_H + 6.0f, GR, GG * pulse, GB);

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
            switch (m.kind) {
            case MinionKind::Rush:
                drawCircle(m.x, m.y, 11.0f, GR, GG, GB, 0.18f);
                drawRect(m.x - 8.0f, m.y - 3.0f, 16.0f, 6.0f, GR, GG, GB, 0.9f);
                break;
            case MinionKind::Heavy:
                drawRect(m.x - 14.0f, m.y - 11.0f, 28.0f, 22.0f, 0.15f, 0.25f, 0.45f, 1.0f);
                drawRect(m.x - 9.0f, m.y - 7.0f, 18.0f, 14.0f, 0.3f, 0.5f, 0.85f, 0.95f);
                break;
            case MinionKind::Pulse:
                drawCircle(m.x, m.y, 10.0f + sinf(time * 8.0f + m.pulseAng) * 3.0f,
                           0.9f, 0.85f, 0.25f, 0.35f);
                drawCircle(m.x, m.y, 5.0f, GR, GG, GB, 0.85f);
                break;
            }
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
