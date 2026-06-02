#pragma once
// ─────────────────────────────────────────────────────────────
// 텍스트 렌더러 — stb_truetype 글리프 아틀라스 (글자 단위 1회 캐싱)
//   + 다중 폰트 폴백 체인 (라틴 / 한글 / 일본어 등 항상 동시 로드)
//   글자 단위 캐싱이라 점수처럼 매 프레임 바뀌는 문자열도 추가 비용 0.
//   Windows 는 임베디드 RCDATA(InitFromMemory), macOS/Linux 는 디스크
//   TTF(InitFromFiles) — 양쪽 다 동일한 글리프 폴백 렌더링.
// ─────────────────────────────────────────────────────────────

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include "stb_truetype.h"   // 선언부. 구현은 stb_impl.cpp 의 STB_TRUETYPE_IMPLEMENTATION

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

    void Draw(const wchar_t* text, float x, float y, float scale,
              float r, float g, float b, float a = 1.0f);
    void Draw(const char* utf8, float x, float y, float scale,
              float r, float g, float b, float a = 1.0f);

private:
    int    screenW_ = 0, screenH_ = 0;
    GLuint prog_ = 0, VAO_ = 0, VBO_ = 0;
    GLint  uProj_ = -1, uCol_ = -1;

    struct FontFace {
        std::vector<unsigned char> data;
        stbtt_fontinfo info;
        bool ok = false;
    };
    struct Glyph {
        GLuint tex = 0;
        int    w = 0, h = 0, xoff = 0, yoff = 0;
        float  advance = 0.0f;
    };
    std::vector<FontFace> faces_;
    std::unordered_map<int, Glyph> glyphs_;   // 코드포인트 → 글리프 (영구 캐싱)
    float  emPx_        = 24.0f;
    float  ascentPx_    = 0.0f;
    float  lineHeightPx_= 0.0f;

    int    FaceForCodepoint(int cp) const;
    Glyph& GetGlyph(int cp);
    bool   FinishInit(int ptSize, int sw, int sh);
    GLuint Compile(GLenum type, const char* src);
    bool   InitGLPipeline();
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
        "out vec4 fragColor;\n"
        "void main(){\n"
        "  float a = texture(tex,uv).r;\n"
        "  fragColor = vec4(col.rgb, col.a*a);\n"
        "}\n";

    GLuint v = Compile(GL_VERTEX_SHADER,   VS);
    GLuint f = Compile(GL_FRAGMENT_SHADER, FS);
    prog_ = glCreateProgram();
    glAttachShader(prog_, v); glAttachShader(prog_, f);
    glLinkProgram(prog_);
    glDeleteShader(v); glDeleteShader(f);

    uProj_ = glGetUniformLocation(prog_, "proj");
    uCol_  = glGetUniformLocation(prog_, "col");
    glUseProgram(prog_);
    glUniform1i(glGetUniformLocation(prog_, "tex"), 0);
    glUseProgram(0);

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
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
        if (stbtt_FindGlyphIndex(&faces_[i].info, cp) != 0) return i;
    }
    return faces_.empty() ? -1 : 0;
}

inline TextRenderer::Glyph& TextRenderer::GetGlyph(int cp)
{
    auto it = glyphs_.find(cp);
    if (it != glyphs_.end()) return it->second;

    Glyph g;
    int fi = FaceForCodepoint(cp);
    if (fi >= 0) {
        const stbtt_fontinfo* fn = &faces_[fi].info;
        float s = stbtt_ScaleForMappingEmToPixels(fn, emPx_);
        int adv = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(fn, cp, &adv, &lsb);
        g.advance = adv * s;
        int ix0, iy0, ix1, iy1;
        stbtt_GetCodepointBitmapBox(fn, cp, s, s, &ix0, &iy0, &ix1, &iy1);
        int w = ix1 - ix0, h = iy1 - iy0;
        if (w > 0 && h > 0) {
            std::vector<unsigned char> bmp((size_t)w * h);
            stbtt_MakeCodepointBitmap(fn, bmp.data(), w, h, w, s, s, cp);
            GLuint tex = 0;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, bmp.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            g.tex = tex; g.w = w; g.h = h; g.xoff = ix0; g.yoff = iy0;
        }
    }
    return glyphs_[cp] = g;
}

inline void TextRenderer::Cleanup() {
    for (auto& p : glyphs_) if (p.second.tex) glDeleteTextures(1, &p.second.tex);
    glyphs_.clear();
    if (VAO_)  { glDeleteVertexArrays(1, &VAO_); VAO_ = 0; }
    if (VBO_)  { glDeleteBuffers(1, &VBO_);       VBO_ = 0; }
    if (prog_) { glDeleteProgram(prog_);           prog_ = 0; }
    faces_.clear();
}

inline float TextRenderer::Width(const wchar_t* text, float scale)
{
    float w = 0.0f;
    for (const wchar_t* p = text; *p; ++p) w += GetGlyph((int)*p).advance;
    return w * scale;
}
inline float TextRenderer::Height(const wchar_t* /*text*/, float scale)
{
    return lineHeightPx_ * scale;
}

inline void TextRenderer::Draw(const wchar_t* text, float x, float y, float scale,
                               float r, float g, float b, float a)
{
    float P[16] = {
         2.0f / screenW_,  0,               0, 0,
         0,               -2.0f / screenH_, 0, 0,
         0,                0,              -1, 0,
        -1,                1,               0, 1
    };
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(prog_);
    glUniformMatrix4fv(uProj_, 1, GL_FALSE, P);
    glUniform4f(uCol_, r, g, b, a);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);

    float penX     = x;
    float baseline = y + ascentPx_ * scale;   // y = 텍스트 상단
    for (const wchar_t* p = text; *p; ++p) {
        Glyph& gph = GetGlyph((int)*p);
        if (gph.tex) {
            float gx = penX + gph.xoff * scale;
            float gy = baseline + gph.yoff * scale;
            float gw = gph.w * scale, gh = gph.h * scale;
            float v[24] = {
                gx,    gy,    0, 0,   gx+gw, gy,    1, 0,   gx+gw, gy+gh, 1, 1,
                gx,    gy,    0, 0,   gx+gw, gy+gh, 1, 1,   gx,    gy+gh, 0, 1,
            };
            glBindTexture(GL_TEXTURE_2D, gph.tex);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        penX += gph.advance * scale;
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
