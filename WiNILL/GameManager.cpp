#include "GameManager.h"
#include "Settings.h"
#include <string>
#include <algorithm>   // std::min (등급 가중치 게이팅)

static const char* gm_vert =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform mat4 projection;\n"
    "void main() { gl_Position = projection * vec4(aPos, 0.0, 1.0); }\n";

static const char* gm_frag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 color;\n"
    "void main() { FragColor = color; }\n";

GameManager::~GameManager() {
    if (VAO)         glDeleteVertexArrays(1, &VAO);
    if (VBO)         glDeleteBuffers(1, &VBO);
    if (EBO)         glDeleteBuffers(1, &EBO);
    if (overlayVAO)  glDeleteVertexArrays(1, &overlayVAO);
    if (overlayVBO)  glDeleteBuffers(1, &overlayVBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

void GameManager::Init(int sw, int sh) {
    screenW = sw; screenH = sh;
    initShaders();
    initRenderData();
}

void GameManager::initShaders() {
    unsigned int v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &gm_vert, NULL);
    glCompileShader(v);

    unsigned int f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &gm_frag, NULL);
    glCompileShader(f);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, v);
    glAttachShader(shaderProgram, f);
    glLinkProgram(shaderProgram);
    glDeleteShader(v);
    glDeleteShader(f);
}

void GameManager::initRenderData() {
    // Quad VAO (4 verts, EBO)
    float verts[] = { 0,0, 1,0, 1,1, 0,1 };
    unsigned int idx[] = { 0,1,2, 0,2,3 };
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Fullscreen NDC overlay VAO
    float ov[] = { -1,1, -1,-1, 1,-1, -1,1, 1,-1, 1,1 };
    glGenVertexArrays(1, &overlayVAO); glGenBuffers(1, &overlayVBO);
    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ov), ov, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void GameManager::setOrtho(int projLoc) {
    // Pixel-space ortho: (0,0) top-left, Y goes down
    float l = 0, r = (float)screenW, t = 0, b = (float)screenH;
    float m[16] = {
        2/(r-l), 0,       0,  0,
        0,       2/(t-b), 0,  0,
        0,       0,      -1,  0,
        -(r+l)/(r-l), -(t+b)/(t-b), 0, 1
    };
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, m);
}

void GameManager::drawQuad(float x, float y, float w, float h,
                           float r, float g, float b, float a) {
    float verts[] = { x,y, x+w,y, x+w,y+h, x,y+h };
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glUniform4f(glGetUniformLocation(shaderProgram, "color"), r, g, b, a);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void GameManager::HandleInput(GLFWwindow* window) {
    int sp  = glfwGetKey(window, GLFW_KEY_SPACE);
    int esc = glfwGetKey(window, GLFW_KEY_ESCAPE);

    if (sp == GLFW_PRESS && spaceReleased) {
        if      (currentState == GameState::READY)    currentState = GameState::RUNNING;
        else if (currentState == GameState::RUNNING)  currentState = GameState::PAUSED;
        else if (currentState == GameState::PAUSED)   currentState = GameState::RUNNING;
        // MAIN_MENU/DIFFICULTY_SELECT/SETTINGS 는 마우스 버튼으로만 진행
        spaceReleased = false;
    }
    if (sp == GLFW_RELEASE) spaceReleased = true;

    if (esc == GLFW_PRESS && escReleased) {
        // ESC = 일시정지 토글만. 메뉴/설정 종료는 마우스 클릭 (뒤로/메뉴로 버튼) 으로
        if      (currentState == GameState::RUNNING) currentState = GameState::PAUSED;
        else if (currentState == GameState::PAUSED)  currentState = GameState::RUNNING;
        escReleased = false;
    }
    if (esc == GLFW_RELEASE) escReleased = true;
}

void GameManager::UpdateStateSystem(MonsterManager& mm, std::vector<Bullet>& bullets) {
    if (lastState == GameState::GAMEOVER && currentState == GameState::READY) {
        playerHP    = 100.0f;
        maxHP       = 100.0f;
        score       = 0;
        scoreAccum  = 0.0f;
        xp          = 0;
        playerLevel = 1;
        memset(takenOnce, 0, sizeof(takenOnce));
        bullets.clear();
        mm.Clear();
    }
    lastState = currentState;
}

// 등급별 가중치 — 플레이어 레벨에 비례(진행도 게이팅).
//   초반(저레벨)엔 COMMON/RARE 위주, 에픽 L3+, 전설 L8+ 부터 서서히 열림.
//   → 시작부터 전설 뜨는 극단 운빨 완화.
static int RarityWeight(int rarity, int level) {
    switch ((AugRarity)rarity) {
    case AugRarity::COMMON:    return 50;
    case AugRarity::RARE:      return 25;
    case AugRarity::EPIC:      return level < 3 ? 0 : std::min(12, (level - 2) * 3);  // L3=3 … L6+=12
    case AugRarity::LEGENDARY: return level < 8 ? 0 : std::min(5,  level - 7);        // L8=1 … L12+=5
    case AugRarity::DEBUFF:    return 6;                                              // 샌드박스(allowDebuff)
    case AugRarity::SPECIAL:   return level < 5 ? 0 : 2;                              // 카오스/판도라도 초반 차단
    default:                   return 0;
    }
}

// 후보 한 장 추첨: rarity 가중 → 해당 등급 내 무작위 (takenOnce 및 고유 카테고리 잠금 적용)
// allowDebuff=false 면 DEBUFF 등급 제외 (버프 페이지용)
static int RollOneAug(const bool* takenOnce,
                      bool sizeTaken, bool distTaken,
                      bool allowSpecial, int level,
                      bool allowDebuff = false) {
    for (int attempt = 0; attempt < 200; attempt++) {
        // 등급 추첨 (레벨 비례 가중치)
        int total = 0;
        for (int r = 0; r < 6; r++) {
            if (!allowSpecial && r == (int)AugRarity::SPECIAL) continue;
            if (!allowDebuff  && r == (int)AugRarity::DEBUFF)  continue;
            total += RarityWeight(r, level);
        }
        if (total <= 0) return 0;
        int roll = rand() % total;
        int acc  = 0;
        AugRarity chosen = AugRarity::COMMON;
        for (int r = 0; r < 6; r++) {
            if (!allowSpecial && r == (int)AugRarity::SPECIAL) continue;
            if (!allowDebuff  && r == (int)AugRarity::DEBUFF)  continue;
            acc += RarityWeight(r, level);
            if (roll < acc) { chosen = (AugRarity)r; break; }
        }
        // 보유 여부 체크 헬퍼 — 티어드 증강 선행 조건용
        auto hasOwnedType = [&](AugType wantType) -> bool {
            for (int k = 0; k < AUG_TOTAL; k++) {
                if (ALL_AUGS[k].type == wantType && takenOnce[k]) return true;
            }
            return false;
        };

        // 해당 등급의 후보 수집
        int pool[AUG_TOTAL]; int poolSize = 0;
        for (int i = 0; i < AUG_TOTAL; i++) {
            if (ALL_AUGS[i].rarity != chosen) continue;
            AugType t = ALL_AUGS[i].type;
            // 한 번만 뽑힐 증강 (EPIC/LEG/COMBO·티어드·불리언 플래그·적출현 디버프)
            if (AugOnceOnly(t, ALL_AUGS[i].rarity) && takenOnce[i]) continue;
            // 고유 카테고리 잠금 (SIZE/DISTANCE)
            AugUnique u = ALL_AUGS[i].unique;
            if (u == AugUnique::SIZE     && sizeTaken) continue;
            if (u == AugUnique::DISTANCE && distTaken) continue;
            // 티어드 강화 선행 조건
            if (t == AugType::BULLET_RAIN_2 && !hasOwnedType(AugType::BULLET_RAIN))   continue;
            if (t == AugType::BULLET_RAIN_3 && !hasOwnedType(AugType::BULLET_RAIN_2)) continue;
            if (t == AugType::DRONE_2       && !hasOwnedType(AugType::DRONE))         continue;
            if (t == AugType::CHAKRAM_2     && !hasOwnedType(AugType::CHAKRAM))       continue;
            if (t == AugType::CHAKRAM_3     && !hasOwnedType(AugType::CHAKRAM_2))     continue;
            if (t == AugType::LASER_2       && !hasOwnedType(AugType::LASER))         continue;
            if (t == AugType::PIERCE_2      && !hasOwnedType(AugType::PIERCE))        continue;
            if (t == AugType::TWIN_2        && !hasOwnedType(AugType::TWIN))          continue;
            // 클래스 전용 — 해당 클래스 런에서만 등장
            if ((t == AugType::MELEE_WIDE || t == AugType::BLADE_WIND) && !g_RunMelee) continue;
            if ((t == AugType::POWER_DRAW || t == AugType::MULTISHOT)  && !g_RunBow)   continue;
            // 변환 전용 증강 — 일반 픽 제외 (4번째 변환 카드 슬롯에서만 등장)
            // BAYONET 은 일반 에픽으로 복원
            if (t == AugType::CANNON || t == AugType::SNIPER ||
                t == AugType::SHOTGUN) continue;
            // 경험치 증가 증강 제거 (#104)
            if (t == AugType::XP_UP) continue;
            // 쉬움: 자폭병 관련 증강 제외 (#107)
            if (g_Difficulty == Difficulty::EASY && t == AugType::HACK_BOMBER) continue;
            pool[poolSize++] = i;
        }
        if (poolSize > 0)
            return pool[rand() % poolSize];
    }
    return 0;
}

// 디버프 등급 한 장 추첨 (디버프 등급 내에서만 무작위)
static int RollOneDebuff(const bool* takenOnce = nullptr) {
    int pool[AUG_TOTAL]; int poolSize = 0;
    for (int i = 0; i < AUG_TOTAL; i++) {
        if (ALL_AUGS[i].rarity != AugRarity::DEBUFF) continue;
        AugType t = ALL_AUGS[i].type;
        // 한 번만 뜨는 디버프(적 출현형)는 이미 보유 시 제외
        if (takenOnce && AugOnceOnly(t, AugRarity::DEBUFF) && takenOnce[i]) continue;
        // 쉬움: 자폭병 디버프 제외 (#107)
        if (g_Difficulty == Difficulty::EASY &&
            (t == AugType::D_BOMBER_BLAST ||
             t == AugType::D_BOMBER_BUFF  ||
             t == AugType::D_BOMBER_SPEED)) continue;
        pool[poolSize++] = i;
    }
    if (poolSize == 0) return 0;
    return pool[rand() % poolSize];
}

void GameManager::PickAugChoices(bool sizeTaken, bool distTaken, bool allowDebuff) {
    // 3장 — 같은 카드 안 나오게 중복 방지, SPECIAL 포함
    //   allowDebuff (크리에이티브 샌드박스) 면 디버프도 카드 풀에 섞임
    bool used[AUG_TOTAL] = {};
    for (int i = 0; i < 3; i++) {
        int idx = 0;
        for (int attempt = 0; attempt < 50; attempt++) {
            idx = RollOneAug(takenOnce, sizeTaken, distTaken,
                             /*allowSpecial=*/true, playerLevel, allowDebuff);
            if (!used[idx]) break;
        }
        used[idx] = true;
        augChoices[i] = idx;
    }

    // 조합 증강 주입 — 레시피(모든 재료 보유) 충족 + 미획득이면 한 칸을 조합으로 교체
    for (int c = 0; c < COMBO_COUNT; c++) {
        AugType res = COMBO_DEFS[c].result;
        if (g_TypeOwned[(int)res]) continue;        // 이미 획득
        bool met = true;
        for (int r = 0; r < COMBO_DEFS[c].reqCount; r++)
            if (!g_TypeOwned[(int)COMBO_DEFS[c].reqs[r]]) { met = false; break; }
        if (met) {
            int ridx = AugIndexOfType(res);
            if (ridx >= 0) { augChoices[rand() % 3] = ridx; break; }
        }
    }
}

void GameManager::PickRandomDebuffIndices(int* outArr, int n) {
    // ALL_AUGS 의 모든 DEBUFF 등급 인덱스 수집 (쉬움: 자폭병 디버프 제외)
    int pool[AUG_TOTAL]; int poolSize = 0;
    for (int i = 0; i < AUG_TOTAL; i++) {
        if (ALL_AUGS[i].rarity != AugRarity::DEBUFF) continue;
        AugType t = ALL_AUGS[i].type;
        if (g_Difficulty == Difficulty::EASY &&
            (t == AugType::D_BOMBER_BLAST ||
             t == AugType::D_BOMBER_BUFF  ||
             t == AugType::D_BOMBER_SPEED)) continue;
        pool[poolSize++] = i;
    }
    if (poolSize == 0) {
        for (int i = 0; i < n; i++) outArr[i] = 0;
        return;
    }
    // 중복 없이 n 개 (n > poolSize 면 중복 허용)
    bool used[AUG_TOTAL] = {};
    for (int i = 0; i < n; i++) {
        int idx = 0;
        for (int attempt = 0; attempt < 30; attempt++) {
            idx = pool[rand() % poolSize];
            if (!used[idx]) break;
        }
        used[idx] = true;
        outArr[i] = idx;
    }
}

void GameManager::PickDebuffChoices() {
    bool used[AUG_TOTAL] = {};
    for (int i = 0; i < 3; i++) {
        int idx = 0;
        for (int attempt = 0; attempt < 50; attempt++) {
            idx = RollOneDebuff(takenOnce);
            if (!used[idx]) break;
        }
        used[idx] = true;
        augChoices[i] = idx;
    }
}

void GameManager::PickRandomAugIndices(int* outArr, int n,
                                       bool sizeTaken, bool distTaken,
                                       bool allowUnique, bool allowSpecial,
                                       bool allowDebuff) {
    bool used[AUG_TOTAL] = {};
    for (int i = 0; i < n; i++) {
        int idx = 0;
        for (int attempt = 0; attempt < 80; attempt++) {
            idx = RollOneAug(takenOnce,
                             allowUnique ? sizeTaken : true,
                             allowUnique ? distTaken : true,
                             allowSpecial, playerLevel,
                             allowDebuff);
            // RANDOM_AUG 자기 자신 제외
            if (ALL_AUGS[idx].type == AugType::RANDOM_AUG) continue;
            if (!used[idx]) break;
        }
        used[idx] = true;
        outArr[i] = idx;
    }
}

void GameManager::AddScore(float amount) {
    if (currentState != GameState::RUNNING) return;
    scoreAccum += amount;
    score = (long long)scoreAccum;
}

void GameManager::UpdateTitle(GLFWwindow* window) {
    // AUG/DEBUFF_SELECT: update title every frame with choices
    if (currentState == GameState::AUG_SELECT ||
        currentState == GameState::DEBUFF_SELECT) {
        std::string t = (currentState == GameState::DEBUFF_SELECT)
                        ? "CHOOSE DEBUFF >> [1] " : "CHOOSE AUG >> [1] ";
        t += ALL_AUGS[augChoices[0]].name;
        t += "  [2] ";
        t += ALL_AUGS[augChoices[1]].name;
        t += "  [3] ";
        t += ALL_AUGS[augChoices[2]].name;
        glfwSetWindowTitle(window, t.c_str());
        return;
    }

    static double lastTime = 0.0;
    static int frames = 0, fps = 0;
    double now = glfwGetTime();
    frames++;
    if (now - lastTime >= 1.0) {
        fps = frames; frames = 0; lastTime += 1.0;
        std::string t = "Onedow | FPS:" + std::to_string(fps)
            + " | HP:" + std::to_string((int)playerHP)
            + " | XP:" + std::to_string(xp)
            + " | Score:" + std::to_string(score);
        glfwSetWindowTitle(window, t.c_str());
    }
}

void GameManager::Render() {
    glEnable(GL_BLEND);
    // Alpha 채널도 올바르게 누적 (DWM이 프레임버퍼 알파로 투명도 결정)
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(shaderProgram);

    int projLoc  = glGetUniformLocation(shaderProgram, "projection");
    int colorLoc = glGetUniformLocation(shaderProgram, "color");

    // --- 상태별 전체화면 어두운 오버레이 ---
    //   GAMEOVER 는 main.cpp 가 페이드인 딤을 직접 그림.
    //   MAIN_MENU 는 "진짜 바탕화면"을 비추기 위해 어둡게 덮지 않음.
    if (currentState != GameState::RUNNING && currentState != GameState::DYING &&
        currentState != GameState::GAMEOVER && currentState != GameState::MAIN_MENU) {
        float identity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, identity);

        float oR = 0.0f, oG = 0.0f, oB = 0.0f, overlayA = 0.5f;
        if (currentState == GameState::GAMEOVER)     { overlayA = 0.60f; }
        if (currentState == GameState::AUG_SELECT)   { overlayA = 0.75f; }
        // 디버프 — 기존엔 진한 채도 빨강이라 눈이 아팠음 → 어두운 적갈색으로 완화
        if (currentState == GameState::DEBUFF_SELECT){ oR = 0.14f; oG = 0.035f; oB = 0.045f; overlayA = 0.80f; }
        glUniform4f(colorLoc, oR, oG, oB, overlayA);
        glBindVertexArray(overlayVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // --- AUG_SELECT / DEBUFF_SELECT: 3 또는 4-card picker ----------
    if (currentState == GameState::AUG_SELECT ||
        currentState == GameState::DEBUFF_SELECT) {
        setOrtho(projLoc);

        bool        hasConv = (conversionAug >= 0 &&
                               currentState == GameState::AUG_SELECT);
        int         nCards  = hasConv ? 4 : 3;
        const float CARD_W  = hasConv ? 240.0f : 280.0f;
        const float CARD_H  = 400.0f;
        const float GAP     = hasConv ? 32.0f : 48.0f;
        const float INSET   = 10.0f;
        const float TOTAL_W = (float)nCards * CARD_W + (float)(nCards-1) * GAP;
        float baseX = (screenW - TOTAL_W) * 0.5f;
        float baseY = (screenH - CARD_H)  * 0.4f;

        for (int i = 0; i < nCards; i++) {
            float cx     = baseX + i * (CARD_W + GAP);
            bool  hover  = (hoveredCard == i);

            float cr, cg, cb;
            if (i < 3) {
                AugRarity rar = ALL_AUGS[augChoices[i]].rarity;
                GetRarityColor(rar, cr, cg, cb);
            } else {
                // 변환 카드 — 금색 테마
                cr = 1.00f; cg = 0.78f; cb = 0.10f;
            }

            // 호버 시: 16px 위로 부유 + glow 외곽
            float yOff = hover ? -16.0f : 0.0f;
            if (hover) {
                // 외곽 glow
                drawQuad(cx - 8, baseY + yOff - 8, CARD_W + 16, CARD_H + 16,
                         cr, cg, cb, 0.35f);
            }

            // 카드 배경
            drawQuad(cx, baseY + yOff, CARD_W, CARD_H,
                     cr * 0.45f, cg * 0.45f, cb * 0.45f, 0.95f);
            // 내부 강조 (등급 컬러 반투명)
            drawQuad(cx + INSET, baseY + yOff + INSET,
                     CARD_W - 2*INSET, CARD_H - 2*INSET,
                     cr, cg, cb, hover ? 0.32f : 0.18f);
        }
    }

    // --- 픽셀 좌표계 UI ---
    setOrtho(projLoc);

    // (HP 바는 이제 플레이어 창 하단에 부착됨 — main.cpp 의 (c2) 참고. 여기선 안 그림)

    // (GAMEOVER 표시는 main.cpp [7] 텍스트 블록의 한국어 라벨이 담당)

    glBindVertexArray(0);
    glUseProgram(0);
}
