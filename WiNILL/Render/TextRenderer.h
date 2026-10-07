#pragma once
// ─────────────────────────────────────────────────────────────
// 텍스트 렌더러 — stb_truetype 글리프 아틀라스 (글자 단위 1회 캐싱)
//   + 다중 폰트 폴백 체인 (라틴 / 한글 / 일본어 등 항상 동시 로드)
//   글자 단위 캐싱이라 점수처럼 매 프레임 바뀌는 문자열도 추가 비용 0.
//   Windows 는 임베디드 RCDATA(InitFromMemory), macOS/Linux 는 디스크
//   TTF(InitFromFiles) — 양쪽 다 동일한 글리프 폴백 렌더링.
// ─────────────────────────────────────────────────────────────

#include <glad/glad.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include "stb_truetype.h"   // 선언부. 구현은 stb_impl.cpp 의 STB_TRUETYPE_IMPLEMENTATION
#include "DrawPrim.h"        // BatchFlush — 텍스트 그리기 전 메인 배치를 비움

class TextRenderer {
public:
    // 디스크 TTF 폴백 체인 (앞쪽 우선, 없는 글리프는 다음 폰트)
    bool InitFromFiles(const char* const* ttfPaths, int nPaths, int ptSize,
                       int screenW, int screenH);
    // 메모리 폰트 폴백 체인 (Windows 임베디드 RCDATA 등)
    bool InitFromMemory(const unsigned char* const* datas, const int* sizes,
                        int nFonts, int ptSize, int screenW, int screenH);

    void Cleanup();
    ~TextRenderer() { Cleanup(); }

    float Width(const wchar_t* text, float scale = 1.0f);
    float Height(const wchar_t* text, float scale = 1.0f);
    float LineHeightPixels() const { return lineHeightPx_; }
    // Distance from the Draw() y coordinate to the text baseline.
    float BaselineOffset(float scale) const {
        return ascentPx_ * EffectiveScale(scale);
    }
    // Warm glyph textures before the first interactive frame.
    void PreloadText(const wchar_t* text);
    void SetMinScale(float scale) { minScale_ = scale; }
    // Soft dark halo hugging each glyph so text reads over any wallpaper
    // without a plate behind it. strength 0 disables; radiusPx is the
    // on-screen halo width (clamped to the baked glyph padding).
    void SetHalo(float strength, float radiusPx = 2.0f) {
        haloStrength_ = strength;
        haloRadiusPx_ = radiusPx;
    }

    void Draw(const wchar_t* text, float x, float y, float scale,
              float r, float g, float b, float a = 1.0f);
    void DrawRotated(const wchar_t* text, float x, float y, float scale, float angle,
                     float r, float g, float b, float a = 1.0f);
    void Draw(const char* utf8, float x, float y, float scale,
              float r, float g, float b, float a = 1.0f);

private:
    int    screenW_ = 0, screenH_ = 0;
    GLuint prog_ = 0, VAO_ = 0, VBO_ = 0;
    GLint  uProj_ = -1, uCol_ = -1, uHalo_ = -1, uHaloR_ = -1;
    float  haloStrength_ = 0.0f;
    float  haloRadiusPx_ = 2.0f;
    // Empty texels baked around every glyph so the halo can fall off
    // inside the glyph's own atlas cell.
    static constexpr int kGlyphPad = 4;
    void   SetHaloUniforms(float scale);

    struct FontFace {
        std::vector<unsigned char> data;
        stbtt_fontinfo info;
        bool ok = false;
    };
    struct Glyph {
        int    atlasPage = -1;
        int    w = 0, h = 0, xoff = 0, yoff = 0;
        float  u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
        float  advance = 0.0f;
    };
    struct GlyphAtlasPage {
        GLuint tex = 0;
        int cursorX = 1, cursorY = 1, rowHeight = 0;
    };
    std::vector<FontFace> faces_;
    std::vector<GlyphAtlasPage> atlasPages_;
    std::vector<float> drawVertices_;
    static constexpr int kGlyphAtlasSize = 1024;
    std::unordered_map<int, Glyph> glyphs_;   // 코드포인트 → 글리프 (영구 캐싱)
    float  emPx_        = 24.0f;
    float  ascentPx_    = 0.0f;
    float  lineHeightPx_= 0.0f;
    float  minScale_    = 0.0f;

    int    FaceForCodepoint(int cp) const;
    Glyph& GetGlyph(int cp);
    int    AllocateAtlasRect(int w, int h, int& x, int& y);
    void   CreateAtlasPage();
    bool   FinishInit(int ptSize, int sw, int sh);
    GLuint Compile(GLenum type, const char* src);
    bool   InitGLPipeline();
    float  EffectiveScale(float scale) const {
        return (scale > 0.0f && minScale_ > 0.0f && scale < minScale_) ? minScale_ : scale;
    }
};

// ═══════════════════════ GL 파이프라인 ════════════════════════════════════
inline bool TextRenderer::InitGLPipeline()
{
    static const char* VS =
        "#version 330 core\n"
        "layout(location=0) in vec2 aPos;\n"
        "layout(location=1) in vec2 aUV;\n"
        "uniform mat4 proj;\n"
        "out vec2 uv;\n"
        "void main(){ gl_Position=proj*vec4(aPos,0,1); uv=aUV; }\n";
    static const char* FS =
        "#version 330 core\n"
        "in vec2 uv;\n"
        "uniform sampler2D tex;\n"
        "uniform vec4 col;\n"
        "uniform vec4 halo;\n"      // r = share of text colour kept, a = strength
        "uniform float haloR;\n"    // halo radius in atlas texels
        "out vec4 fragColor;\n"
        "void main(){\n"
        "  float a = texture(tex,uv).r;\n"
        "  float h = 0.0;\n"
        "  if (halo.a > 0.0 && haloR > 0.0) {\n"
        "    vec2 t = haloR / vec2(textureSize(tex,0));\n"
        "    for (int i = 0; i < 12; ++i) {\n"
        "      float ang = 6.2831853 * float(i) / 12.0;\n"
        "      vec2 d = vec2(cos(ang), sin(ang)) * t;\n"
        "      h = max(h, texture(tex, uv + d).r * 0.55);\n"
        "      h = max(h, texture(tex, uv + d * 0.5).r);\n"
        "    }\n"
        "    h *= halo.a;\n"
        "  }\n"
        "  float fa = col.a * a;\n"
        "  float ha = col.a * h * (1.0 - a);\n"
        "  float oa = fa + ha;\n"
        "  if (oa < 0.002) discard;\n"
        "  vec3 hc = col.rgb * halo.r;\n"
        "  fragColor = vec4((col.rgb * fa + hc * ha) / oa, oa);\n"
        "}\n";

    GLuint v = Compile(GL_VERTEX_SHADER,   VS);
    GLuint f = Compile(GL_FRAGMENT_SHADER, FS);
    prog_ = glCreateProgram();
    glAttachShader(prog_, v); glAttachShader(prog_, f);
    glLinkProgram(prog_);
    glDeleteShader(v); glDeleteShader(f);

    uProj_ = glGetUniformLocation(prog_, "proj");
    uCol_  = glGetUniformLocation(prog_, "col");
    uHalo_ = glGetUniformLocation(prog_, "halo");
    uHaloR_ = glGetUniformLocation(prog_, "haloR");
    glUseProgram(prog_);
    glUniform1i(glGetUniformLocation(prog_, "tex"), 0);
    glUseProgram(0);

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return true;
}

inline GLuint TextRenderer::Compile(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

// ═══════════════════════ 초기화 ════════════════════════════════════════════
inline bool TextRenderer::FinishInit(int ptSize, int sw, int sh)
{
    screenW_ = sw;  screenH_ = sh;
    // GDI 의 점→픽셀 변환(pt × 96/72)에 맞춰 em 매핑 크기 보정
    emPx_ = (float)ptSize * (96.0f / 72.0f);
    if (faces_.empty()) return false;
    float s0 = stbtt_ScaleForMappingEmToPixels(&faces_[0].info, emPx_);
    int asc = 0, desc = 0, gap = 0;
    stbtt_GetFontVMetrics(&faces_[0].info, &asc, &desc, &gap);
    ascentPx_     = asc * s0;
    lineHeightPx_ = (asc - desc) * s0;
    return InitGLPipeline();
}

inline bool TextRenderer::InitFromFiles(const char* const* ttfPaths, int nPaths,
                                        int ptSize, int sw, int sh)
{
    faces_.clear();
    faces_.reserve(nPaths);
    for (int i = 0; i < nPaths; i++) {
        FontFace face;
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)   // fopen (이 경로는 비-Windows 에서만 사용)
#endif
        FILE* fp = std::fopen(ttfPaths[i], "rb");
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
        if (!fp) continue;
        std::fseek(fp, 0, SEEK_END);
        long len = std::ftell(fp);
        std::fseek(fp, 0, SEEK_SET);
        if (len > 0) {
            face.data.resize((size_t)len);
            size_t rd = std::fread(face.data.data(), 1, (size_t)len, fp);
            (void)rd;
        }
        std::fclose(fp);
        if (face.data.empty()) continue;
        int off = stbtt_GetFontOffsetForIndex(face.data.data(), 0);
        if (stbtt_InitFont(&face.info, face.data.data(), off)) {
            face.ok = true;
            faces_.push_back(std::move(face));
        }
    }
    return FinishInit(ptSize, sw, sh);
}

inline bool TextRenderer::InitFromMemory(const unsigned char* const* datas,
                                         const int* sizes, int nFonts,
                                         int ptSize, int sw, int sh)
{
    faces_.clear();
    faces_.reserve(nFonts);
    for (int i = 0; i < nFonts; i++) {
        if (!datas[i] || sizes[i] <= 0) continue;
        FontFace face;
        face.data.assign(datas[i], datas[i] + sizes[i]);   // 복사 (RCDATA 는 읽기전용)
        int off = stbtt_GetFontOffsetForIndex(face.data.data(), 0);
        if (stbtt_InitFont(&face.info, face.data.data(), off)) {
            face.ok = true;
            faces_.push_back(std::move(face));
        }
    }
    return FinishInit(ptSize, sw, sh);
}

inline int TextRenderer::FaceForCodepoint(int cp) const
{
    for (int i = 0; i < (int)faces_.size(); i++) {
        if (!faces_[i].ok) continue;
        // glyph index 0 = .notdef — missing codepoint; try next font in chain
        if (stbtt_FindGlyphIndex(&faces_[i].info, cp) != 0) return i;
    }
    return -1;
}

inline TextRenderer::Glyph& TextRenderer::GetGlyph(int cp)
{
    auto it = glyphs_.find(cp);
    if (it != glyphs_.end()) return it->second;

    Glyph g;
    int fi = FaceForCodepoint(cp);
    if (fi < 0) {
        // 미지원 글자 — ? 대신 공백 폭만 (사용자에게 물음표 노출 방지)
        if (cp != L' ' && cp > 32) return GetGlyph(L' ');
        return glyphs_[cp] = g;
    }
    const stbtt_fontinfo* fn = &faces_[fi].info;
    float s = stbtt_ScaleForMappingEmToPixels(fn, emPx_);
    int adv = 0, lsb = 0;
    stbtt_GetCodepointHMetrics(fn, cp, &adv, &lsb);
    g.advance = adv * s;
    int ix0, iy0, ix1, iy1;
    stbtt_GetCodepointBitmapBox(fn, cp, s, s, &ix0, &iy0, &ix1, &iy1);
    int w = ix1 - ix0, h = iy1 - iy0;
    if (w > 0 && h > 0) {
        const int P = kGlyphPad;
        const int pw = w + 2 * P, ph = h + 2 * P;
        std::vector<unsigned char> bmp((size_t)pw * ph, 0);
        stbtt_MakeCodepointBitmap(fn, bmp.data() + (size_t)P * pw + P,
                                  w, h, pw, s, s, cp);
        w = pw; h = ph;
        ix0 -= P; iy0 -= P;
        int atlasX = 0, atlasY = 0;
        const int atlasPage = AllocateAtlasRect(w, h, atlasX, atlasY);
        if (atlasPage >= 0) {
            glBindTexture(GL_TEXTURE_2D, atlasPages_[atlasPage].tex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, atlasX, atlasY, w, h,
                            GL_RED, GL_UNSIGNED_BYTE, bmp.data());
            g.atlasPage = atlasPage;
            g.u0 = (atlasX + 0.5f) / (float)kGlyphAtlasSize;
            g.v0 = (atlasY + 0.5f) / (float)kGlyphAtlasSize;
            g.u1 = (atlasX + w - 0.5f) / (float)kGlyphAtlasSize;
            g.v1 = (atlasY + h - 0.5f) / (float)kGlyphAtlasSize;
            g.w = w;
            g.h = h;
            g.xoff = ix0;
            g.yoff = iy0;
        }
    }
    return glyphs_[cp] = g;
}

inline void TextRenderer::CreateAtlasPage()
{
    GlyphAtlasPage page;
    glGenTextures(1, &page.tex);
    glBindTexture(GL_TEXTURE_2D, page.tex);
    std::vector<unsigned char> empty((size_t)kGlyphAtlasSize * kGlyphAtlasSize, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kGlyphAtlasSize, kGlyphAtlasSize, 0,
                 GL_RED, GL_UNSIGNED_BYTE, empty.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    atlasPages_.push_back(page);
}

inline int TextRenderer::AllocateAtlasRect(int w, int h, int& x, int& y)
{
    if (w <= 0 || h <= 0 || w + 2 > kGlyphAtlasSize || h + 2 > kGlyphAtlasSize)
        return -1;

    for (;;) {
        for (int i = 0; i < (int)atlasPages_.size(); ++i) {
            GlyphAtlasPage& page = atlasPages_[i];
            int rowX = page.cursorX;
            int rowY = page.cursorY;
            int rowHeight = page.rowHeight;
            if (rowX + w + 2 > kGlyphAtlasSize) {
                rowX = 0;
                rowY += rowHeight;
                rowHeight = 0;
            }
            if (rowY + h + 2 > kGlyphAtlasSize) continue;

            x = rowX + 1;
            y = rowY + 1;
            page.cursorX = rowX + w + 2;
            page.cursorY = rowY;
            page.rowHeight = std::max(rowHeight, h + 2);
            return i;
        }

        CreateAtlasPage();
        if (atlasPages_.empty() || !atlasPages_.back().tex) return -1;
    }
}

inline void TextRenderer::Cleanup() {
    glyphs_.clear();
    for (auto& page : atlasPages_)
        if (page.tex) glDeleteTextures(1, &page.tex);
    atlasPages_.clear();
    drawVertices_.clear();
    if (VAO_)  { glDeleteVertexArrays(1, &VAO_); VAO_ = 0; }
    if (VBO_)  { glDeleteBuffers(1, &VBO_);       VBO_ = 0; }
    if (prog_) { glDeleteProgram(prog_);           prog_ = 0; }
    faces_.clear();
}

inline float TextRenderer::Width(const wchar_t* text, float scale)
{
    scale = EffectiveScale(scale);
    float w = 0.0f;
    for (const wchar_t* p = text; *p; ++p) w += GetGlyph((int)*p).advance;
    return w * scale;
}
inline void TextRenderer::PreloadText(const wchar_t* text)
{
    if (!text) return;
    for (const wchar_t* p = text; *p; ++p)
        (void)GetGlyph((int)*p);
}

inline float TextRenderer::Height(const wchar_t* /*text*/, float scale)
{
    scale = EffectiveScale(scale);
    return lineHeightPx_ * scale;
}

inline void TextRenderer::SetHaloUniforms(float scale)
{
    // The halo scales with the rendered text size (about 7% of the em) up to
    // the configured width, so small labels keep open counters instead of
    // turning into dark blobs. Never sample past the baked glyph padding.
    const float textPx = emPx_ * scale;
    const float radiusPx = std::min(haloRadiusPx_, std::max(0.9f, textPx * 0.07f));
    const float radiusTexels = scale > 0.0f
        ? std::min((float)kGlyphPad - 0.5f, radiusPx / scale) : 0.0f;
    // halo.x = how much of the text colour the halo keeps (a dark shade of
    // the glyph itself rather than a fixed navy outline).
    glUniform4f(uHalo_, 0.18f, 0.0f, 0.0f, haloStrength_);
    glUniform1f(uHaloR_, radiusTexels);
}

inline void TextRenderer::Draw(const wchar_t* text, float x, float y, float scale,
                               float r, float g, float b, float a)
{
    if (!text || !*text) return;
    scale = EffectiveScale(scale);
    if (g_GfxPass != GfxPass::Text) BatchFlush();
    struct GlyphRun { int page; GLsizei first, count; };
    static std::vector<GlyphRun> runs;
    runs.clear();
    drawVertices_.clear();

    float penX = x;
    const float baseline = y + ascentPx_ * scale;
    for (const wchar_t* p = text; *p; ++p) {
        Glyph& glyph = GetGlyph((int)*p);
        if (glyph.atlasPage >= 0) {
            if (runs.empty() || runs.back().page != glyph.atlasPage) {
                runs.push_back({glyph.atlasPage,
                    (GLsizei)(drawVertices_.size() / 4u), 0});
            }
            GlyphRun& run = runs.back();
            const float gx = penX + glyph.xoff * scale;
            const float gy = baseline + glyph.yoff * scale;
            const float gw = glyph.w * scale, gh = glyph.h * scale;
            const float quad[] = {
                gx,      gy,      glyph.u0, glyph.v0,
                gx + gw, gy,      glyph.u1, glyph.v0,
                gx + gw, gy + gh, glyph.u1, glyph.v1,
                gx,      gy,      glyph.u0, glyph.v0,
                gx + gw, gy + gh, glyph.u1, glyph.v1,
                gx,      gy + gh, glyph.u0, glyph.v1,
            };
            drawVertices_.insert(drawVertices_.end(), quad, quad + 24);
            run.count += 6;
        }
        penX += glyph.advance * scale;
    }
    if (drawVertices_.empty()) return;

    g_GfxPass = GfxPass::Text;
    const float P[16] = {
         2.0f / screenW_,  0,               0, 0,
         0,               -2.0f / screenH_, 0, 0,
         0,                0,              -1, 0,
        -1,                1,               0, 1
    };
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(prog_);
    glUniformMatrix4fv(uProj_, 1, GL_FALSE, P);
    glUniform4f(uCol_, r, g, b, a * g_BatchAlpha);
    SetHaloUniforms(scale);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    const GLsizeiptr bytes = (GLsizeiptr)(drawVertices_.size() * sizeof(float));
    glBufferData(GL_ARRAY_BUFFER, bytes, drawVertices_.data(), GL_STREAM_DRAW);
    for (const GlyphRun& run : runs) {
        glBindTexture(GL_TEXTURE_2D, atlasPages_[run.page].tex);
        glDrawArrays(GL_TRIANGLES, run.first, run.count);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}
inline void TextRenderer::DrawRotated(const wchar_t* text, float x, float y,
                                      float scale, float angle,
                                      float r, float g, float b, float a)
{
    if (!text || !*text) return;
    scale = EffectiveScale(scale);
    if (g_GfxPass != GfxPass::Text) BatchFlush();
    struct GlyphRun { int page; GLsizei first, count; };
    static std::vector<GlyphRun> runs;
    runs.clear();
    drawVertices_.clear();

    const float ca = std::cos(angle), sa = std::sin(angle);
    const float baseline = ascentPx_ * scale;
    float pen = 0.0f;
    auto rotatePoint = [&](float lx, float ly, float& ox, float& oy) {
        ox = x + lx * ca - ly * sa;
        oy = y + lx * sa + ly * ca;
    };
    for (const wchar_t* p = text; *p; ++p) {
        Glyph& glyph = GetGlyph((int)*p);
        if (glyph.atlasPage >= 0) {
            if (runs.empty() || runs.back().page != glyph.atlasPage) {
                runs.push_back({glyph.atlasPage,
                    (GLsizei)(drawVertices_.size() / 4u), 0});
            }
            GlyphRun& run = runs.back();
            const float gx = pen + glyph.xoff * scale;
            const float gy = baseline + glyph.yoff * scale;
            const float gw = glyph.w * scale, gh = glyph.h * scale;
            float x0, y0, x1, y1, x2, y2, x3, y3;
            rotatePoint(gx,      gy,      x0, y0);
            rotatePoint(gx + gw, gy,      x1, y1);
            rotatePoint(gx + gw, gy + gh, x2, y2);
            rotatePoint(gx,      gy + gh, x3, y3);
            const float quad[] = {
                x0, y0, glyph.u0, glyph.v0,
                x1, y1, glyph.u1, glyph.v0,
                x2, y2, glyph.u1, glyph.v1,
                x0, y0, glyph.u0, glyph.v0,
                x2, y2, glyph.u1, glyph.v1,
                x3, y3, glyph.u0, glyph.v1,
            };
            drawVertices_.insert(drawVertices_.end(), quad, quad + 24);
            run.count += 6;
        }
        pen += glyph.advance * scale;
    }
    if (drawVertices_.empty()) return;

    g_GfxPass = GfxPass::Text;
    const float P[16] = {
         2.0f / screenW_,  0,               0, 0,
         0,               -2.0f / screenH_, 0, 0,
         0,                0,              -1, 0,
        -1,                1,               0, 1
    };
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(prog_);
    glUniformMatrix4fv(uProj_, 1, GL_FALSE, P);
    glUniform4f(uCol_, r, g, b, a * g_BatchAlpha);
    SetHaloUniforms(scale);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    const GLsizeiptr bytes = (GLsizeiptr)(drawVertices_.size() * sizeof(float));
    glBufferData(GL_ARRAY_BUFFER, bytes, drawVertices_.data(), GL_STREAM_DRAW);
    for (const GlyphRun& run : runs) {
        glBindTexture(GL_TEXTURE_2D, atlasPages_[run.page].tex);
        glDrawArrays(GL_TRIANGLES, run.first, run.count);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}
inline void TextRenderer::Draw(const char* utf8, float x, float y, float scale,
                               float r, float g, float b, float a)
{
    std::wstring ws;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8);
    while (*p) {
        int cp;
        if (*p < 0x80) { cp = *p++; }
        else if ((*p >> 5) == 0x6) {
            cp = (*p++ & 0x1F) << 6;
            if (*p) cp |= (*p++ & 0x3F);
        } else if ((*p >> 4) == 0xE) {
            cp = (*p++ & 0x0F) << 12;
            if (*p) cp |= (*p++ & 0x3F) << 6;
            if (*p) cp |= (*p++ & 0x3F);
        } else if ((*p >> 3) == 0x1E) {
            cp = (*p++ & 0x07) << 18;
            if (*p) cp |= (*p++ & 0x3F) << 12;
            if (*p) cp |= (*p++ & 0x3F) << 6;
            if (*p) cp |= (*p++ & 0x3F);
        } else { p++; continue; }
        ws.push_back((wchar_t)cp);
    }
    Draw(ws.c_str(), x, y, scale, r, g, b, a);
}
