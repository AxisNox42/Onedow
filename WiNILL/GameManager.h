#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include "MonsterManager.h"
#include "Bullet.h"
#include "Augment.h"

enum class GameState {
    MAIN_MENU,         // 시작 메뉴 (시작/설정/종료)
    DIFFICULTY_SELECT, // 난이도 선택 (쉬움/보통/어려움)
    WEAPON_SELECT,     // 시작 무기 선택 (랜덤 3개)
    SETTINGS,          // 설정 화면
    READY, RUNNING, PAUSED, GAMEOVER, AUG_SELECT, DEBUFF_SELECT, DYING
};

class GameManager {
public:
    GameState currentState = GameState::MAIN_MENU;
    GameState lastState    = GameState::MAIN_MENU;

    float     playerHP   = 100.0f;
    float     maxHP      = 100.0f;   // mirrors PlayerStats::maxHP for HUD
    long long score      = 0;
    float     scoreAccum = 0.0f;

    long long xp           = 0;        // 현재 레벨 안에서 누적된 EXP
    int       playerLevel  = 1;        // 레벨 (시작 1)
    int       augChoices[3] = {0,0,0}; // indices into ALL_AUGS for current pick
    int       conversionAug = -1;      // 변환 4번째 카드 (-1=없음)
    int       hoveredCard  = -1;       // AUG/DEBUFF_SELECT 호버 인덱스 (-1=none, 3=변환 카드)
    bool      takenOnce[AUG_TOTAL] = {}; // EPIC/LEGENDARY 한 번만

    bool spaceReleased = true;
    bool escReleased   = true;

    int screenW = 0, screenH = 0;

    GameManager() = default;
    ~GameManager();

    void Init(int sw, int sh);
    void HandleInput(GLFWwindow* window);
    void UpdateStateSystem(MonsterManager& mm, std::vector<Bullet>& bullets);
    void AddScore(float amount);
    // 등급 가중치 + takenOnce + 고유 카테고리 잠금 적용한 3장 픽 (디버프 제외)
    void PickAugChoices(bool sizeTaken = false, bool distTaken = false);
    // 디버프 카드 3장 픽 (DEBUFF 등급만)
    void PickDebuffChoices();
    // n장만 픽 (RANDOM_AUG·PANDORA·CHAOS용)
    void PickRandomAugIndices(int* outArr, int n,
                              bool sizeTaken = false, bool distTaken = false,
                              bool allowUnique = true, bool allowSpecial = false,
                              bool allowDebuff = false);
    // 디버프만 n개 픽 (PANDORA / CHAOS 의 디버프 슬롯용)
    void PickRandomDebuffIndices(int* outArr, int n);
    bool ShouldUpdate()   const { return currentState == GameState::RUNNING ||
                                         currentState == GameState::DYING; }
    GameState GetState()  const { return currentState; }
    void UpdateTitle(GLFWwindow* window);
    void Render();

private:
    unsigned int shaderProgram = 0;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int overlayVAO = 0, overlayVBO = 0;

    void initShaders();
    void initRenderData();
    void drawQuad(float x, float y, float w, float h, float r, float g, float b, float a);
    void setOrtho(int projLoc);
};
