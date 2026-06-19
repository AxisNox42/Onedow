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
//   맵 '전체'가 체스판(타일). 킹(본체)은 공격하지 않고 보드 중앙에 앉아만 있다.
//   2초보다 빠르게 기물 소환: 폰 50% / 나이트·비숍·룩 각 16.6%.
//     폰 HP=커널 프로세스(288), 나이트/비숍/룩 HP=288*1.2.
//   기물은 체스 규칙대로 '이동 범위'를 1.5초 예고 후 플레이어 쪽으로 이동.
//   ■ 기물 가치 합(폰1/나이트·비숍3/룩5) 이 VALUE_CAP(100) 이상이면 소환 정지.
//   ■ 킹 보호막: 평소 무적. 파괴한 기물 가치가 DEAL_THRESHOLD(150) 누적될 때마다
//      보호막 해제 → 3~5초 '딜 타임'(취약). 이후 재생성, 반복.
// ─────────────────────────────────────────────────────────────
class TrojanKingBoss {
public:
    float worldX, worldY;          // 킹 위치(보드 중앙 셀)
    float hp, maxHp;
    bool  alive = true, exploded = false;
    int   screenW, screenH;
    bool  shakePulse = false;

    static constexpr float SPAWN_INT = 1.3f;    // 소환 주기(빠르게)
    static constexpr float TELE_T   = 1.5f;     // 범위 예고 → 이동
    static constexpr float MOVE_T   = 0.30f;
    static constexpr float SETTLE_T = 0.55f;
    static constexpr float VALUE_CAP = 100.0f;  // 보드 위 기물 가치 상한
    static constexpr float DEAL_THRESHOLD = 150.0f;  // 누적 파괴 가치 → 딜 타임
    static constexpr float MINI_HP  = 288.0f;   // 커널 프로세스

    enum Ptype { PAWN = 0, KNIGHT, BISHOP, ROOK };
    static int pval(int t) { return (t == PAWN) ? 1 : (t == ROOK) ? 5 : 3; }

    struct Piece {
        int   type; float hp; bool alive;
        int   col, row, tcol, trow;
        int   phase; float t;
        float wx, wy;
    };
    std::vector<Piece> pieces;
    float spawnTimer = 0.0f;
    float boardAnim  = 0.0f;

    int   cols, rows;              // 맵 전체를 덮는 타일 수
    float cell, boardX, boardY;

    // 킹 보호막 / 딜 타임
    float breakValue = 0.0f;       // 누적 파괴 가치
    float dealTime   = 0.0f;       // >0 이면 취약(딜 타임)

    TrojanKingBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        // 기존 타일(min*0.82/8)의 1/2 크기 + 맵 전체를 덮는 격자
        cell = (float)std::min(sw, sh) * 0.82f / 8.0f * 0.5f;
        cols = (int)((float)sw / cell);
        rows = (int)((float)sh / cell);
        if (cols < 4) cols = 4; if (rows < 4) rows = 4;
        boardX = (sw - cols * cell) * 0.5f;
        boardY = (sh - rows * cell) * 0.5f;
        glm::vec2 kc = cellCenter(kcol(), krow());
        worldX = kc.x; worldY = kc.y;
    }

    glm::vec2 cellCenter(int c, int r) const {
        return glm::vec2(boardX + ((float)c + 0.5f) * cell, boardY + ((float)r + 0.5f) * cell);
    }
    int kcol() const { return cols / 2; }
    int krow() const { return rows / 2; }
    int totalValue() const { int v = 0; for (auto& p : pieces) if (p.alive) v += pval(p.type); return v; }
    bool occupied(int c, int r) const {
        if (c == kcol() && r == krow()) return true;
        for (auto& p : pieces) if (p.alive && p.col == c && p.row == r) return true;
        return false;
    }
    bool kingVulnerable() const { return dealTime > 0.0f; }

    // 파괴된 기물 가치 누적 → 임계 넘으면 딜 타임
    void onPieceKilled(int type) {
        breakValue += (float)pval(type);
        if (breakValue >= DEAL_THRESHOLD) {
            breakValue -= DEAL_THRESHOLD;
            dealTime = 3.0f + (float)(rand() % 200) * 0.01f;   // 3~5초 취약
            shakePulse = true;
        }
    }

    void reachCells(const Piece& p, std::vector<glm::ivec2>& out) const {
        out.clear();
        int md = std::max(cols, rows);
        auto add = [&](int c, int r) { if (c >= 0 && c < cols && r >= 0 && r < rows) out.push_back(glm::ivec2(c, r)); };
        if (p.type == PAWN) {
            for (int dc = -1; dc <= 1; dc++) for (int dr = -1; dr <= 1; dr++)
                if (dc || dr) add(p.col + dc, p.row + dr);
        } else if (p.type == KNIGHT) {
            int dc[8] = { 1,2,2,1,-1,-2,-2,-1 }, dr[8] = { 2,1,-1,-2,-2,-1,1,2 };
            for (int i = 0; i < 8; i++) add(p.col + dc[i], p.row + dr[i]);
        } else if (p.type == BISHOP) {
            for (int d = 1; d < md; d++) { add(p.col+d,p.row+d); add(p.col+d,p.row-d); add(p.col-d,p.row+d); add(p.col-d,p.row-d); }
        } else { // ROOK
            for (int d = 1; d < md; d++) { add(p.col+d,p.row); add(p.col-d,p.row); add(p.col,p.row+d); add(p.col,p.row-d); }
        }
    }
    void pickTarget(Piece& p, float px, float py) {
        std::vector<glm::ivec2> rg; reachCells(p, rg);
        float best = 1e18f; p.tcol = p.col; p.trow = p.row;
        for (auto& rc : rg) {
            if (rc.x == kcol() && rc.y == krow()) continue;
            glm::vec2 cc = cellCenter(rc.x, rc.y);
            float d = (cc.x - px) * (cc.x - px) + (cc.y - py) * (cc.y - py);
            if (d < best) { best = d; p.tcol = rc.x; p.trow = rc.y; }
        }
    }

    void spawnPiece(float px, float py) {
        int roll = rand() % 6;
        int type = (roll < 3) ? PAWN : (roll == 3) ? KNIGHT : (roll == 4) ? BISHOP : ROOK;
        int c = 0, r = 0;
        for (int tryN = 0; tryN < 40; tryN++) {
            if (rand() % 2) { c = (rand() % 2) ? 0 : cols - 1; r = rand() % rows; }
            else            { r = (rand() % 2) ? 0 : rows - 1; c = rand() % cols; }
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
        if (dealTime > 0.0f) { dealTime -= dt; if (dealTime < 0.0f) dealTime = 0.0f; }

        spawnTimer += dt;
        if (spawnTimer >= SPAWN_INT && (float)totalValue() < VALUE_CAP && boardAnim >= 1.0f) {
            spawnTimer = 0.0f; spawnPiece(px, py); shakePulse = true;
        }

        for (auto& p : pieces) {
            if (!p.alive) continue;
            p.t += dt;
            if (p.phase == 0) {
                if (p.t >= TELE_T) { p.phase = 1; p.t = 0.0f; }
            } else if (p.phase == 1) {
                float a = p.t / MOVE_T; if (a > 1.0f) a = 1.0f;
                glm::vec2 f = cellCenter(p.col, p.row), to = cellCenter(p.tcol, p.trow);
                p.wx = f.x + (to.x - f.x) * a; p.wy = f.y + (to.y - f.y) * a;
                if (p.t >= MOVE_T) { p.col = p.tcol; p.row = p.trow; p.phase = 2; p.t = 0.0f; }
            } else {
                if (p.t >= SETTLE_T) { p.phase = 0; p.t = 0.0f; pickTarget(p, px, py); }
            }
            float ddx = px - p.wx, ddy = py - p.wy;
            if (ddx*ddx + ddy*ddy < (cell*0.42f)*(cell*0.42f)) playerHP -= 12.0f * dt;
        }
        pieces.erase(std::remove_if(pieces.begin(), pieces.end(),
            [](const Piece& p) { return !p.alive; }), pieces.end());
    }

    static void typeColor(int t, float& r, float& g, float& b) {
        switch (t) {
        case PAWN:   r = 0.95f; g = 0.4f;  b = 0.4f;  break;
        case KNIGHT: r = 0.4f;  g = 0.85f; b = 1.0f;  break;
        case BISHOP: r = 0.6f;  g = 1.0f;  b = 0.5f;  break;
        default:     r = 1.0f;  g = 0.7f;  b = 0.3f;  break;
        }
    }
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
        default:     drawRect(p.wx - s*0.8f, p.wy - s*0.8f, s*1.6f, s*1.6f, r, g, b, 1.0f); break;
        }
        drawCircle(p.wx, p.wy, s*0.28f, 1.0f, 1.0f, 1.0f, 0.9f);
    }

    void render(float t) const {
        BindMainShader();
        // 맵 전체 체스판 — 등장 애니메이션(대각선 스윕)
        for (int c = 0; c < cols; c++) for (int rr = 0; rr < rows; rr++) {
            float delay = (float)(c + rr) / (float)(cols + rows);
            float a = (boardAnim - delay) / 0.25f; if (a < 0) a = 0; if (a > 1) a = 1;
            if (a <= 0.001f) continue;
            if (((c + rr) & 1) == 0) continue;                  // 어두운 칸만 채워 체스무늬
            float cx = boardX + c * cell, cy = boardY + rr * cell;
            drawRect(cx, cy, cell, cell, 0.12f, 0.09f, 0.16f, 0.55f * a);
        }
        if (boardAnim > 0.05f)
            drawNeonBorder(boardX, boardY, cols * cell, rows * cell, 0.8f, 0.4f, 0.95f);

        // 기물 이동 예고
        for (const auto& p : pieces) {
            if (!p.alive || p.phase != 0) continue;
            float blink = 0.5f + 0.5f * sinf(t * 8.0f);
            std::vector<glm::ivec2> rg; reachCells(p, rg);
            for (auto& rc : rg) {
                float cx = boardX + rc.x * cell, cy = boardY + rc.y * cell;
                drawRect(cx + 2, cy + 2, cell - 4, cell - 4, 1.0f, 0.85f, 0.3f, 0.09f);
            }
            float tx = boardX + p.tcol * cell, ty = boardY + p.trow * cell;
            drawRect(tx + 2, ty + 2, cell - 4, cell - 4, 1.0f, 0.3f, 0.25f, 0.16f + 0.22f * blink);
        }

        // 킹 — 보호막(무적) / 딜 타임(취약) 표시
        float ks = cell * 0.7f;
        bool vuln = kingVulnerable();
        if (!vuln) {                                            // 보호막 — 청록 헥스 링
            float pul = 0.5f + 0.5f * sinf(t * 3.0f);
            drawCircle(worldX, worldY, ks * 1.9f, 0.2f, 0.7f, 1.0f, 0.10f + 0.08f * pul);
            for (int i = 0; i < 6; i++) {
                float ang = t * 0.8f + (float)i * 1.0471976f;
                drawDiamond(worldX + cosf(ang)*ks*1.7f, worldY + sinf(ang)*ks*1.7f, ks*0.3f, 0.3f, 0.85f, 1.0f, 0.85f);
            }
        } else {                                                // 딜 타임 — 빨강 취약 오라
            float pul = 0.5f + 0.5f * sinf(t * 12.0f);
            drawCircle(worldX, worldY, ks * 1.8f, 1.0f, 0.3f, 0.25f, 0.16f + 0.18f * pul);
        }
        drawDiamond(worldX, worldY, ks * 2.0f, 0.30f, 0.22f, 0.05f, 1.0f);   // 받침
        drawDiamond(worldX, worldY, ks * 1.4f, 1.0f, 0.82f, 0.25f, 1.0f);    // 금 본체
        for (int i = -1; i <= 1; i++)
            drawDiamond(worldX + (float)i * ks * 0.55f, worldY - ks * 0.85f, ks * 0.45f, 1.0f, 0.9f, 0.4f, 1.0f);
        drawRect(worldX - ks*0.1f,  worldY - ks*1.45f, ks*0.2f, ks*0.65f, 1.0f, 0.95f, 0.6f, 1.0f);
        drawRect(worldX - ks*0.28f, worldY - ks*1.2f,  ks*0.56f, ks*0.2f, 1.0f, 0.95f, 0.6f, 1.0f);

        for (const auto& p : pieces) if (p.alive) drawPiece(p);
        BatchFlush();
    }
};
