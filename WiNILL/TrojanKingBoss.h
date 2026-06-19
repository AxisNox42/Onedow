#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"

// ─────────────────────────────────────────────────────────────
// Trojan_king.vir — 체스 보스
//   킹(본체) 은 공격하지 않는 'DPS 체크' — 보드 중앙에 앉아만 있다.
//   등장 시 맵에 체스판이 애니메이션으로 깔리고, 2초마다 기물 소환:
//     폰 50% / 나이트·비숍·룩 각 16.6%.
//     폰 HP = 커널 프로세스(288), 나이트/비숍/룩 HP = 288*1.2.
//   기물은 체스 규칙대로 '이동 범위'를 1.5초 예고한 뒤 플레이어 쪽으로 이동.
//   기물 가치 합(폰1/나이트·비숍3/룩5) 이 30 이상이면 소환 정지.
// ─────────────────────────────────────────────────────────────
class TrojanKingBoss {
public:
    float worldX, worldY;          // 킹 위치(보드 중앙)
    float hp, maxHp;
    bool  alive = true, exploded = false;
    int   screenW, screenH;
    bool  shakePulse = false;

    static constexpr int   N        = 8;        // 8x8 보드
    static constexpr float SPAWN_INT = 2.0f;
    static constexpr float TELE_T   = 1.5f;     // 범위 예고 → 이동까지
    static constexpr float MOVE_T   = 0.32f;    // 이동 보간
    static constexpr float SETTLE_T = 0.6f;     // 이동 후 대기
    static constexpr int   VALUE_CAP = 30;
    static constexpr float MINI_HP  = 288.0f;   // 커널 프로세스

    enum Ptype { PAWN = 0, KNIGHT, BISHOP, ROOK };
    static int pval(int t) { return (t == PAWN) ? 1 : (t == ROOK) ? 5 : 3; }

    struct Piece {
        int   type; float hp; bool alive;
        int   col, row, tcol, trow;  // 현재/목표 셀
        int   phase;                 // 0 예고 / 1 이동 / 2 정착
        float t;                     // 페이즈 경과
        float wx, wy;                // 월드 위치(렌더/충돌)
    };
    std::vector<Piece> pieces;
    float spawnTimer = 0.0f;
    float boardAnim  = 0.0f;         // 0→1 등장 애니메이션

    float cell, boardX, boardY, boardSize;

    TrojanKingBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        boardSize = (float)std::min(sw, sh) * 0.82f;
        cell   = boardSize / (float)N;
        boardX = sw * 0.5f - boardSize * 0.5f;
        boardY = sh * 0.5f - boardSize * 0.5f;
        worldX = boardX + boardSize * 0.5f;
        worldY = boardY + boardSize * 0.5f;   // 킹 = 보드 중앙
    }

    glm::vec2 cellCenter(int c, int r) const {
        return glm::vec2(boardX + ((float)c + 0.5f) * cell, boardY + ((float)r + 0.5f) * cell);
    }
    int kc() const { return N / 2; }
    int kr() const { return N / 2; }
    int totalValue() const { int v = 0; for (auto& p : pieces) if (p.alive) v += pval(p.type); return v; }
    bool occupied(int c, int r) const {
        if (c == kc() && r == kr()) return true;
        for (auto& p : pieces) if (p.alive && p.col == c && p.row == r) return true;
        return false;
    }

    // 기물 이동 가능 셀(보드 내). 블로킹은 단순화(무시).
    void reachCells(const Piece& p, std::vector<glm::ivec2>& out) const {
        out.clear();
        auto add = [&](int c, int r) { if (c >= 0 && c < N && r >= 0 && r < N) out.push_back(glm::ivec2(c, r)); };
        if (p.type == PAWN) {
            for (int dc = -1; dc <= 1; dc++) for (int dr = -1; dr <= 1; dr++)
                if (dc || dr) add(p.col + dc, p.row + dr);         // 한 칸(8방)
        } else if (p.type == KNIGHT) {
            int dc[8] = { 1,2,2,1,-1,-2,-2,-1 }, dr[8] = { 2,1,-1,-2,-2,-1,1,2 };
            for (int i = 0; i < 8; i++) add(p.col + dc[i], p.row + dr[i]);
        } else if (p.type == BISHOP) {
            for (int d = 1; d < N; d++) { add(p.col+d,p.row+d); add(p.col+d,p.row-d); add(p.col-d,p.row+d); add(p.col-d,p.row-d); }
        } else { // ROOK
            for (int d = 1; d < N; d++) { add(p.col+d,p.row); add(p.col-d,p.row); add(p.col,p.row+d); add(p.col,p.row-d); }
        }
    }
    // 플레이어에 가장 가까운 합법 셀을 목표로
    void pickTarget(Piece& p, float px, float py) {
        std::vector<glm::ivec2> rg; reachCells(p, rg);
        float best = 1e18f; p.tcol = p.col; p.trow = p.row;
        for (auto& rc : rg) {
            if (rc.x == kc() && rc.y == kr()) continue;           // 킹 칸은 피함
            glm::vec2 cc = cellCenter(rc.x, rc.y);
            float d = (cc.x - px) * (cc.x - px) + (cc.y - py) * (cc.y - py);
            if (d < best) { best = d; p.tcol = rc.x; p.trow = rc.y; }
        }
    }

    void spawnPiece(float px, float py) {
        int roll = rand() % 6;     // 0-2 폰(50%) / 3 나이트 / 4 비숍 / 5 룩
        int type = (roll < 3) ? PAWN : (roll == 3) ? KNIGHT : (roll == 4) ? BISHOP : ROOK;
        int c = 0, r = 0;
        for (int tryN = 0; tryN < 30; tryN++) {
            // 가장자리 위주로 빈 셀
            if (rand() % 2) { c = (rand() % 2) ? 0 : N - 1; r = rand() % N; }
            else            { r = (rand() % 2) ? 0 : N - 1; c = rand() % N; }
            if (!occupied(c, r)) break;
        }
        Piece p; p.type = type; p.hp = (type == PAWN) ? MINI_HP : MINI_HP * 1.2f;
        p.alive = true; p.col = c; p.row = r; p.phase = 0; p.t = 0.0f;
        glm::vec2 cc = cellCenter(c, r); p.wx = cc.x; p.wy = cc.y;
        pickTarget(p, px, py);
        pieces.push_back(p);
    }

    void Update(float px, float py, float dt, float& playerHP) {
        if (!alive) return;
        if (boardAnim < 1.0f) { boardAnim += dt / 0.9f; if (boardAnim > 1.0f) boardAnim = 1.0f; }

        // 기물 소환 — 가치 총합 30 미만일 때만
        spawnTimer += dt;
        if (spawnTimer >= SPAWN_INT && totalValue() < VALUE_CAP && boardAnim >= 1.0f) {
            spawnTimer = 0.0f; spawnPiece(px, py); shakePulse = true;
        }

        for (auto& p : pieces) {
            if (!p.alive) continue;
            p.t += dt;
            if (p.phase == 0) {                  // 예고(범위 표시)
                if (p.t >= TELE_T) { p.phase = 1; p.t = 0.0f; }
            } else if (p.phase == 1) {           // 이동(보간)
                float a = p.t / MOVE_T; if (a > 1.0f) a = 1.0f;
                glm::vec2 f = cellCenter(p.col, p.row), to = cellCenter(p.tcol, p.trow);
                p.wx = f.x + (to.x - f.x) * a; p.wy = f.y + (to.y - f.y) * a;
                if (p.t >= MOVE_T) { p.col = p.tcol; p.row = p.trow; p.phase = 2; p.t = 0.0f; }
            } else {                             // 정착 → 다음 예고(타겟 재계산)
                if (p.t >= SETTLE_T) { p.phase = 0; p.t = 0.0f; pickTarget(p, px, py); }
            }
            // 접촉 데미지(기물 본체)
            float ddx = px - p.wx, ddy = py - p.wy;
            if (ddx*ddx + ddy*ddy < (cell*0.42f)*(cell*0.42f)) playerHP -= 12.0f * dt;
        }
        // 죽은 기물 제거
        pieces.erase(std::remove_if(pieces.begin(), pieces.end(),
            [](const Piece& p) { return !p.alive; }), pieces.end());
        // 킹은 공격하지 않음 — 접촉 데미지 없음
    }

    static void typeColor(int t, float& r, float& g, float& b) {
        switch (t) {
        case PAWN:   r = 0.95f; g = 0.4f;  b = 0.4f;  break;   // 적
        case KNIGHT: r = 0.4f;  g = 0.85f; b = 1.0f;  break;   // 청
        case BISHOP: r = 0.6f;  g = 1.0f;  b = 0.5f;  break;   // 녹
        default:     r = 1.0f;  g = 0.7f;  b = 0.3f;  break;   // 룩=주황
        }
    }
    // 기물 글리프
    void drawPiece(const Piece& p) const {
        float r, g, b; typeColor(p.type, r, g, b);
        float s = cell * 0.34f;
        switch (p.type) {
        case PAWN:
            drawCircle(p.wx, p.wy - s*0.2f, s*0.55f, r, g, b, 1.0f);
            drawRect(p.wx - s*0.5f, p.wy + s*0.2f, s, s*0.6f, r, g, b, 1.0f);
            break;
        case KNIGHT: drawPentagon(p.wx, p.wy, s*2.0f, r, g, b, 1.0f); break;
        case BISHOP: drawDiamond(p.wx, p.wy, s*1.9f, r, g, b, 1.0f); break;
        default: // ROOK
            drawRect(p.wx - s*0.8f, p.wy - s*0.8f, s*1.6f, s*1.6f, r, g, b, 1.0f);
            break;
        }
        drawCircle(p.wx, p.wy, s*0.28f, 1.0f, 1.0f, 1.0f, 0.9f);   // 코어 점
    }

    void render(float t) const {
        BindMainShader();
        // 체스판 — 등장 애니메이션(대각선 스윕으로 셀이 차례로 나타남)
        for (int c = 0; c < N; c++) for (int rr = 0; rr < N; rr++) {
            float delay = (float)(c + rr) / (float)(2 * N);       // 0~1
            float a = (boardAnim - delay) / 0.25f; if (a < 0) a = 0; if (a > 1) a = 1;
            if (a <= 0.001f) continue;
            float cx = boardX + c * cell, cy = boardY + rr * cell;
            bool dark = ((c + rr) & 1) != 0;
            float v = dark ? 0.10f : 0.20f;
            drawRect(cx, cy, cell, cell, v, v*0.7f, v*1.1f, 0.82f * a);
        }
        if (boardAnim > 0.05f)
            drawNeonBorder(boardX, boardY, boardSize, boardSize, 0.8f, 0.4f, 0.95f);

        // 기물 이동 예고(범위 + 목표 셀)
        for (const auto& p : pieces) {
            if (!p.alive || p.phase != 0) continue;
            float blink = 0.5f + 0.5f * sinf(t * 8.0f);
            std::vector<glm::ivec2> rg; reachCells(p, rg);
            for (auto& rc : rg) {
                float cx = boardX + rc.x * cell, cy = boardY + rc.y * cell;
                drawRect(cx + 2, cy + 2, cell - 4, cell - 4, 1.0f, 0.85f, 0.3f, 0.10f);  // 범위(옅게)
            }
            float tx = boardX + p.tcol * cell, ty = boardY + p.trow * cell;             // 목표(강조)
            drawRect(tx + 2, ty + 2, cell - 4, cell - 4, 1.0f, 0.3f, 0.25f, 0.18f + 0.22f * blink);
        }

        // 킹 — 보드 중앙(공격 X). 금색 왕관.
        float ks = cell * 0.42f;
        drawDiamond(worldX, worldY, ks * 2.2f, 0.30f, 0.22f, 0.05f, 1.0f);   // 받침(어둠)
        drawDiamond(worldX, worldY, ks * 1.5f, 1.0f, 0.82f, 0.25f, 1.0f);    // 본체(금)
        for (int i = -1; i <= 1; i++)                                         // 왕관 뿔 3개
            drawDiamond(worldX + (float)i * ks * 0.6f, worldY - ks * 0.9f, ks * 0.5f, 1.0f, 0.9f, 0.4f, 1.0f);
        drawRect(worldX - ks*0.12f, worldY - ks*1.5f, ks*0.24f, ks*0.7f, 1.0f, 0.95f, 0.6f, 1.0f); // 십자(세로)
        drawRect(worldX - ks*0.3f,  worldY - ks*1.25f, ks*0.6f, ks*0.22f, 1.0f, 0.95f, 0.6f, 1.0f); // 십자(가로)

        // 기물 본체
        for (const auto& p : pieces) if (p.alive) drawPiece(p);
        BatchFlush();
    }
};
