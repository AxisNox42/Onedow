#include "GameFonts.h"

#include "../Entity/Augment.h"
#include "../System/Translations.h"
#include "../System/EmbeddedResource.h"
#include "TextRenderer.h"
#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <string>

extern TextRenderer g_TextL;
extern TextRenderer g_TextS;
extern TextRenderer g_TextXL;

void InitializeGameFonts(int screenWidth, int screenHeight) {
    auto fileExists = [](const char* path) -> bool {
#ifdef _MSC_VER
        FILE* fp = nullptr;
        if (fopen_s(&fp, path, "rb") != 0 || !fp) return false;
#else
        FILE* fp = std::fopen(path, "rb");
        if (!fp) return false;
#endif
        std::fclose(fp);
        return true;
    };
    auto pickFont = [&](const char* a, const char* b, const char* c,
                        const char* d = nullptr) -> const char* {
        if (a && fileExists(a)) return a;
        if (b && fileExists(b)) return b;
        if (c && fileExists(c)) return c;
        if (d && fileExists(d)) return d;
        return nullptr;
    };

    // Windows releases use the exact fonts compiled into the executable.
    // This prevents a stale Resource/Font folder from changing the UI
    // after a user downloads only WiNILL.exe from GitHub.
    const char* embeddedFontNames[] = {
        "FONT_CHAKRA", "FONT_ORBIT", "FONT_JUA", "FONT_KOSUGI"
    };
    const unsigned char* memoryFonts[4] = {};
    int memoryFontSizes[4] = {};
    int memoryFontCount = 0;
    for (const char* name : embeddedFontNames) {
        EmbeddedResourceView view;
        if (LoadEmbeddedResource(name, view)) {
            memoryFonts[memoryFontCount] = view.data;
            memoryFontSizes[memoryFontCount] = view.size;
            ++memoryFontCount;
        }
    }

    bool fontsInitialized = false;
    if (memoryFontCount > 0) {
        fontsInitialized =
            g_TextL.InitFromMemory(memoryFonts, memoryFontSizes,
                                   memoryFontCount, 36,
                                   screenWidth, screenHeight) &&
            g_TextS.InitFromMemory(memoryFonts, memoryFontSizes,
                                   memoryFontCount, 22,
                                   screenWidth, screenHeight) &&
            g_TextXL.InitFromMemory(memoryFonts, memoryFontSizes,
                                    memoryFontCount, 100,
                                    screenWidth, screenHeight);
    }

    if (!fontsInitialized) {
        const char* chain[6] = {};
        int nFonts = 0;
        auto addFont = [&](const char* path) {
            if (path && nFonts < (int)(sizeof(chain) / sizeof(chain[0])))
                chain[nFonts++] = path;
        };

        addFont(pickFont("Resource/Font/ChakraPetch-Regular.ttf",
                         "../Resource/Font/ChakraPetch-Regular.ttf",
                         "../../Resource/Font/ChakraPetch-Regular.ttf"));
        addFont(pickFont("Resource/Font/Orbit-Regular.ttf",
                         "../Resource/Font/Orbit-Regular.ttf",
                         "../../Resource/Font/Orbit-Regular.ttf"));
        addFont(pickFont("Resource/Font/Jua-Regular.ttf",
                         "../Resource/Font/Jua-Regular.ttf",
                         "../../Resource/Font/Jua-Regular.ttf",
                         "C:/Windows/Fonts/malgun.ttf"));
        addFont(pickFont("Resource/Font/KosugiMaru-Regular.ttf",
                         "../Resource/Font/KosugiMaru-Regular.ttf",
                         "../../Resource/Font/KosugiMaru-Regular.ttf",
                         "C:/Windows/Fonts/meiryo.ttc"));
        if (nFonts == 0) {
            addFont(pickFont("C:/Windows/Fonts/malgun.ttf",
                             "C:/Windows/Fonts/arial.ttf",
                             "C:/Windows/Fonts/meiryo.ttc"));
        }

        g_TextL.InitFromFiles(chain, nFonts, 36, screenWidth, screenHeight);
        g_TextS.InitFromFiles(chain, nFonts, 22, screenWidth, screenHeight);
        g_TextXL.InitFromFiles(chain, nFonts, 100, screenWidth, screenHeight);
    }
    g_TextS.SetMinScale(0.58f);

    // Warm only characters used by the game. The renderer caches glyphs
    // lazily, so this removes first-use stalls without baking all Hangul.
    for (int si = 0; si < (int)StrId::_COUNT; ++si) {
        for (int li = 0; li < LANG_COUNT; ++li) {
            g_TextL.PreloadText(kStrings[si][li]);
            g_TextS.PreloadText(kStrings[si][li]);
        }
    }
    for (int ai = 0; ai < AUG_TOTAL; ++ai) {
        for (int li = 0; li < LANG_COUNT; ++li) {
            g_TextL.PreloadText(ALL_AUGS[ai].locName[li]);
            g_TextL.PreloadText(ALL_AUGS[ai].locDesc[li]);
            g_TextS.PreloadText(ALL_AUGS[ai].locName[li]);
            g_TextS.PreloadText(ALL_AUGS[ai].locDesc[li]);
        }
    }
}

void UpdateGameTextHalo(int screenHeight, bool backdropBlurEnabled) {
    // A blurred desktop already removes competing detail, so the halo only
    // needs to lift contrast; a sharp desktop needs a fuller outline.
    const float strength = backdropBlurEnabled ? 0.55f : 0.90f;
    const float radiusPx = std::max(1.5f, 2.0f * (float)screenHeight / 1600.0f);
    g_TextL.SetHalo(strength, radiusPx);
    g_TextS.SetHalo(strength, radiusPx);
    g_TextXL.SetHalo(strength, radiusPx);
}
