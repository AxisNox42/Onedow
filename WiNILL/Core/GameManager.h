#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include "MonsterManager.h"
#include "Bullet.h"
#include "Augment.h"
#include "Experience.h"
#include "LevelUpTransition.h"
#include "WaveSystem.h"

enum class GameState {
    MAIN_MENU,         // 시작 메뉴 (시작/설정/종료)
    CREATIVE_CONFIG,   // 크리에이티브 설정 (시작점수/시작증강)
    CODEX,             // 도감 (적/증강 발견 목록)
    TUTORIAL,          // 플레이 가이드 (페이지형)
    SETTINGS,          // 설정 화면
    READY, RUNNING, PAUSED, GAMEOVER, AUG_SELECT, AUG_REPLACE, DEBUFF_SELECT, DYING
};

class GameManager {
public:
    GameState currentState = GameState::MAIN_MENU;
    GameState lastState    = GameState::MAIN_MENU;
    GameState pauseResumeState = GameState::RUNNING;

    float     playerHP   = 100.0f;
    float     maxHP      = 100.0f;   // mirrors PlayerStats::maxHP for HUD
    int       currentWave = 1;
    int       waveTransitionFrom = 0;
    float     waveTransitionTimer = 0.0f;
    int       lastWaveDebuff = -1;
    float     waveDebuffToast = 0.0f;
    // Debuff rolled ahead for the wave about to start (-1 = not rolled yet).
    int       nextWaveDebuff = -1;
    // Debuff each wave brought, by the wave it entered; index + 1, 0 = none.
    int       waveDebuffLog[64] = {};
    // Debug console switches (not reset with a run, not saved).
    bool      showDebuffPreview = true;   // announce the next wave's debuff early
    bool      showDebuffLog = true;       // per-wave debuff list on pause/result
    bool      waveVictory = false;
    long long score      = 0;
    float     scoreAccum = 0.0f;

    XpAmount  xp           = 0;        // 현재 레벨 안의 EXP (0.001 XP 단위)
    int       playerLevel  = 1;        // 레벨 (시작 1)
    int       augChoices[3] = {-1,-1,-1}; // indices into ALL_AUGS for current pick
    int       augChoiceCount = 0;
    int       augmentSelectionSerial = 0;
    bool      augmentKeyboardFocus = false;
    int       conversionAug = -1;      // 변환 4번째 카드 (-1=없음)
    int       hoveredCard  = -1;       // AUG/DEBUFF_SELECT 호버 인덱스 (-1=none, 3=변환 카드)
    int       pendingAugIdx = -1;      // AUG_REPLACE: 장착 대기 중인 신규 증강
    int       replaceChoices[32] = {}; // AUG_REPLACE: 교체 후보 (ALL_AUGS 인덱스)
    int       replaceChoiceCount = 0;
    int       pendingAugmentRewards = 0;
    bool      augmentRewardActive = false;
    LevelUpTransition levelUpTransition;  // slow-mo before the reward opens
    bool      takenOnce[AUG_TOTAL] = {}; // EPIC/LEGENDARY 한 번만

    bool spaceReleased = true;
    bool escReleased   = true;

    int screenW = 0, screenH = 0;

    GameManager() = default;
    ~GameManager();

    void Init(int sw, int sh);
    void ShutdownGlResources();
    void HandleInput(GLFWwindow* window);
    void UpdateStateSystem(MonsterManager& mm, std::vector<Bullet>& bullets);
    void ResetRunProgress(MonsterManager& mm, std::vector<Bullet>& bullets,
                          float initialHp = 100.0f,
                          long long initialScore = 0);
    void AddScore(float amount);
    // 등급 가중치 + takenOnce + 고유 카테고리 잠금 적용한 버프 3장 픽
    void PickAugChoices(bool sizeTaken = false);
    // n장만 픽 (RANDOM_AUG·PANDORA·CHAOS용)
    int  PickRandomAugIndices(int* outArr, int n,
                              bool sizeTaken = false,
                              bool allowUnique = true, bool allowSpecial = false,
                              bool allowDebuff = false);
    // 디버프만 n개 픽 (PANDORA / CHAOS 의 디버프 슬롯용)
    int  PickRandomDebuffIndices(int* outArr, int n);
    void QueueAugmentReward();
    bool ActivateNextAugmentReward();
    void ClearAugmentRewardQueue();
    bool ShouldUpdate()   const {
        return currentState == GameState::RUNNING ||
               currentState == GameState::DYING;
    }
    GameState GetState()  const { return currentState; }
    void Render();

private:
    unsigned int shaderProgram = 0;
    int projectionLoc = -1;
    int colorLoc = -1;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int overlayVAO = 0, overlayVBO = 0;

    void initShaders();
    void initRenderData();
    void drawQuad(float x, float y, float w, float h, float r, float g, float b, float a);
    void setOrtho(int projLoc);
};
