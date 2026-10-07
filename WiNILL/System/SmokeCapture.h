#pragma once
// ─────────────────────────────────────────────────────────────
// 스모크 캡처 — UI QA 검증용 자동 장면 캡처.
//   ONEDOW_SMOKE      = "STEP;STEP;..."   (필수, 없으면 비활성)
//   ONEDOW_SMOKE_LANG = KR | EN | JP      (선택)
//   ONEDOW_SMOKE_TAG  = 파일명 접두어      (선택, 기본 smoke)
//   ONEDOW_SMOKE_SIZE = WxH               (선택, 창 크기 강제)
//   ONEDOW_SMOKE_BLUR = 0 | 1             (선택, 배경 블러 강제)
// STEP 형식: NAME[@fx,fy][!][~sec]
//   @fx,fy  마우스 위치(화면 비율 0~1). 없으면 화면 밖으로 치움.
//   !       유지 시간 절반 시점에 한 번 클릭.
//   ~sec    캡처 전 유지 시간(기본 1.6초).
// 각 단계는 Screenshots/<TAG>_<LANG>_<NN>_<NAME>.png 로 저장되고,
// 마지막 단계 뒤 창을 닫는다. 세션 동안 SaveGame()은 건너뛴다.
// ─────────────────────────────────────────────────────────────
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include "DebugToolkit.h"
#include "Settings.h"

inline std::string SmokeEnv(const char* name) {
#if defined(_MSC_VER)
    char* value = nullptr;
    size_t len = 0;
    std::string out;
    if (_dupenv_s(&value, &len, name) == 0 && value) out = value;
    std::free(value);
    return out;
#else
    const char* value = std::getenv(name);
    return value ? value : "";
#endif
}

class SmokeCapture {
public:
    struct Step {
        std::string name;
        bool hasMouse = false;
        float fx = 0.0f, fy = 0.0f;
        bool click = false;
        float hold = 1.6f;
    };

    // LoadGame() 이후에 호출. 세이브에서 읽은 언어/디버그 설정을 덮어쓴다.
    bool Init() {
        const std::string spec = SmokeEnv("ONEDOW_SMOKE");
        if (spec.empty()) return false;
        ParseSteps(spec);
        if (steps_.empty()) return false;

        const std::string lang = SmokeEnv("ONEDOW_SMOKE_LANG");
        if (lang == "EN") g_Language = Language::EN;
        else if (lang == "JP") g_Language = Language::JP;
        else if (lang == "KR") g_Language = Language::KR;
        const std::string tag = SmokeEnv("ONEDOW_SMOKE_TAG");
        if (!tag.empty()) tag_ = tag;
        const std::string size = SmokeEnv("ONEDOW_SMOKE_SIZE");
        const size_t x = size.find('x');
        if (x != std::string::npos) {
            width_ = std::atoi(size.c_str());
            height_ = std::atoi(size.c_str() + x + 1);
        }
        const std::string blur = SmokeEnv("ONEDOW_SMOKE_BLUR");
        if (blur == "0") g_BackdropBlurEnabled = false;
        else if (blur == "1") g_BackdropBlurEnabled = true;
        g_SmokeCapture = true;
        g_DebugMode = true;   // 캡처 경로가 디버그 모드에 묶여 있음 (저장 안 됨)
        return true;
    }

    bool Active() const { return g_SmokeCapture; }
    // Forced window size for resolution checks; 0 keeps the monitor size.
    int Width() const { return width_; }
    int Height() const { return height_; }

    // 매 프레임 입력 처리 직후 호출. 단계 진입 시 applyStep(name)을 부르고,
    // 마우스/클릭을 스크립트 값으로 덮어쓴다.
    void Frame(GLFWwindow* window, int sw, int sh,
               double& mx, double& my, bool& lmb,
               const std::function<void(const std::string&)>& applyStep) {
        if (!Active()) return;
        if (index_ >= steps_.size()) {
            if (!g_DebugToolkit.SuppressOverlayForCapture())
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            mx = my = -10000.0;
            lmb = false;
            return;
        }
        const Step& step = steps_[index_];
        const double now = glfwGetTime();
        if (!started_) {
            applyStep(step.name);
            stepStart_ = now;
            started_ = true;
            clicked_ = false;
            requested_ = false;
        }
        const float t = (float)(now - stepStart_);
        if (step.hasMouse) {
            mx = step.fx * (float)sw;
            my = step.fy * (float)sh;
            glfwSetCursorPos(window, mx, my);
        } else {
            mx = my = -10000.0;
        }
        lmb = false;
        if (step.click && !clicked_ && t >= step.hold * 0.5f) {
            lmb = true;
            clicked_ = true;
        }
        if (!requested_ && t >= step.hold) {
            g_DebugToolkit.RequestNamedScreenshot(FileName(step));
            requested_ = true;
        } else if (requested_ && !g_DebugToolkit.SuppressOverlayForCapture()) {
            ++index_;
            started_ = false;
        }
    }

private:
    std::vector<Step> steps_;
    std::string tag_ = "smoke";
    int width_ = 0;
    int height_ = 0;
    size_t index_ = 0;
    bool started_ = false;
    bool clicked_ = false;
    bool requested_ = false;
    double stepStart_ = 0.0;

    void ParseSteps(const std::string& spec) {
        size_t pos = 0;
        while (pos <= spec.size()) {
            size_t end = spec.find(';', pos);
            if (end == std::string::npos) end = spec.size();
            std::string token = spec.substr(pos, end - pos);
            pos = end + 1;
            if (token.empty()) continue;

            Step step;
            const size_t tilde = token.find('~');
            if (tilde != std::string::npos) {
                step.hold = (float)std::atof(token.c_str() + tilde + 1);
                token.resize(tilde);
            }
            if (!token.empty() && token.back() == '!') {
                step.click = true;
                token.pop_back();
            }
            const size_t at = token.find('@');
            if (at != std::string::npos) {
                const std::string coords = token.substr(at + 1);
                const size_t comma = coords.find(',');
                if (comma != std::string::npos) {
                    step.hasMouse = true;
                    step.fx = (float)std::atof(coords.c_str());
                    step.fy = (float)std::atof(coords.c_str() + comma + 1);
                }
                token.resize(at);
            }
            step.name = token;
            steps_.push_back(step);
        }
    }

    std::wstring FileName(const Step& step) const {
        static const char* kLang[] = { "KR", "EN", "JP" };
        char buf[160];
        std::snprintf(buf, sizeof(buf), "%s_%s_%02d_%s", tag_.c_str(),
                      kLang[LangIndex()], (int)index_ + 1, step.name.c_str());
        return std::wstring(buf, buf + std::strlen(buf));
    }
};

inline SmokeCapture g_SmokeRun;
