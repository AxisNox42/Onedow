#pragma once
// ─────────────────────────────────────────────────────────────
// 텍스트 렌더러 (OpenGL 텍스처 방식)
//   Windows : GDI 로 문자열 단위 래스터화 (기존 검증된 경로) + 캐시 상한
//   그 외(macOS/Linux) : stb_truetype 글리프 아틀라스 (글자 단위 1회 캐싱)
//                        + 다중 폰트 폴백 체인
//
//   stb 경로가 글자 단위 캐싱이라 점수처럼 매 프레임 바뀌는 문자열도
//   추가 비용 0 (글리프는 이미 캐싱됨) → 누수/렉 없음.
// ─────────────────────────────────────────────────────────────

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cstdint>
#include <utility>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <cstdio>
  #include <cmath>
  #include "stb_truetype.h"   // 선언부. 구현은 stb_impl.cpp 에서 STB_TRUETYPE_IMPLEMENTATION
#endif

class TextRenderer {
public:
#ifdef _WIN32
    bool Init(const char* fontPath, const char* faceName, int ptSize,
              int screenW, int screenH);
    bool InitWithFace(const char* faceName, int ptSize,
                      int screenW, int screenH);
#endif
    // 크로스플랫폼: TTF 파일 폴백 체인 (앞쪽 우선, 없는 글리프는 다음 폰트)
    bool InitFromFiles(const char* const* ttfPaths, int nPaths, int ptSize,
                       int screenW, int screenH);

    void Cleanup();
    ~TextRenderer() { Cleanup(); }

    float Width(const wchar_t* text, float scale = 1.0f);
    float Height(const wchar_t* text, float scale = 1.0f);

    void Draw(const wchar_t* text, float x, float y, float scale,
              float r, float g, float b, float a = 1.0f);
    void Draw(const char* utf8, float x, float y, float scale,
              float r, float g, float b, float a = 1.0f);

private:
    int   screenW_ = 0, screenH_ = 0;
    GLuint prog_ = 0, VAO_ = 0, VBO_ = 0;
    GLint  uProj_ = -1, uCol_ = -1;

#ifdef _WIN32
    struct Entry { GLuint tex; int w, h; };
    HFONT hFont_ = nullptr;
    char  fontPath_[MAX_PATH] = {};
    std::unordered_map<std::wstring, Entry> cache_;
    Entry&  Cache(const std::wstring& ws);
    void    Issue(const Entry& e, float x, float y, float scale,
                  float r, float g, float b, float a);
#else
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
    float emPx_        = 24.0f;   // em → 픽셀 매핑 크기
    float ascentPx_    = 0.0f;    // 주 폰트 ascent (픽셀)
    float lineHeightPx_= 0.0f;    // 줄 높이 (픽셀)
    int    FaceForCodepoint(int cp) const;
    Glyph& GetGlyph(int cp);
#endif

    GLuint  Compile(GLenum type, const char* src);
    bool    InitGLPipeline();
};

// ═══════════════════════ 공통 GL 파이프라인 ════════════════════════════════════

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

// ═══════════════════════ Windows (GDI) ═════════════════════════════════════════
#ifdef _WIN32

inline void TextRenderer::Issue(const Entry& e,
                                float x, float y, float scale,
                                float r, float g, float b, float a)
{
    float w = e.w * scale, h = e.h * scale;
    float P[16] = {
         2.0f / screenW_,  0,               0, 0,
         0,               -2.0f / screenH_, 0, 0,
         0,                0,              -1, 0,
        -1,                1,               0, 1
    };
    float v[24] = {
        x,   y,   0, 0,   x+w, y,   1, 0,   x+w, y+h, 1, 1,
        x,   y,   0, 0,   x+w, y+h, 1, 1,   x,   y+h, 0, 1,
    };
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(prog_);
    glUniformMatrix4fv(uProj_, 1, GL_FALSE, P);
    glUniform4f(uCol_, r, g, b, a);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e.tex);
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

inline float TextRenderer::Width(const wchar_t* text, float scale)  { return Cache(std::wstring(text)).w * scale; }
inline float TextRenderer::Height(const wchar_t* text, float scale) { return Cache(std::wstring(text)).h * scale; }
inline void  TextRenderer::Draw(const wchar_t* text, float x, float y, float scale,
                                float r, float g, float b, float a)
{ Issue(Cache(std::wstring(text)), x, y, scale, r, g, b, a); }

inline bool TextRenderer::InitWithFace(const char* faceName, int ptSize, int sw, int sh)
{
    screenW_ = sw;  screenH_ = sh;
    fontPath_[0] = '\0';
    HDC hdc = GetDC(nullptr);
    LOGFONTA lf = {};
    lf.lfHeight  = -MulDiv(ptSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    lf.lfWeight  = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = ANTIALIASED_QUALITY;
    strncpy_s(lf.lfFaceName, faceName, LF_FACESIZE - 1);
    ReleaseDC(nullptr, hdc);
    hFont_ = CreateFontIndirectA(&lf);
    if (!hFont_) return false;
    return InitGLPipeline();
}

inline bool TextRenderer::Init(const char* fontPath, const char* faceName,
                               int ptSize, int sw, int sh)
{
    screenW_ = sw;  screenH_ = sh;
    strncpy_s(fontPath_, fontPath, MAX_PATH - 1);
    if (AddFontResourceExA(fontPath, FR_PRIVATE, nullptr) == 0) {
        char abs[MAX_PATH];
        GetFullPathNameA(fontPath, MAX_PATH, abs, nullptr);
        AddFontResourceExA(abs, FR_PRIVATE, nullptr);
        strncpy_s(fontPath_, abs, MAX_PATH - 1);
    }
    HDC hdc = GetDC(nullptr);
    LOGFONTA lf = {};
    lf.lfHeight  = -MulDiv(ptSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    lf.lfWeight  = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = ANTIALIASED_QUALITY;
    strncpy_s(lf.lfFaceName, faceName, LF_FACESIZE - 1);
    ReleaseDC(nullptr, hdc);
    hFont_ = CreateFontIndirectA(&lf);
    if (!hFont_) return false;
    return InitGLPipeline();
}

inline void TextRenderer::Cleanup() {
    for (auto& p : cache_) glDeleteTextures(1, &p.second.tex);
    cache_.clear();
    if (VAO_)  { glDeleteVertexArrays(1, &VAO_); VAO_ = 0; }
    if (VBO_)  { glDeleteBuffers(1, &VBO_);       VBO_ = 0; }
    if (prog_) { glDeleteProgram(prog_);           prog_ = 0; }
    if (hFont_){ DeleteObject(hFont_);             hFont_ = nullptr; }
    if (fontPath_[0] != '\0')
        RemoveFontResourceExA(fontPath_, FR_PRIVATE, nullptr);
}

inline TextRenderer::Entry& TextRenderer::Cache(const std::wstring& ws)
{
    auto it = cache_.find(ws);
    if (it != cache_.end()) return it->second;

    // 캐시 상한 — 점수처럼 매번 바뀌는 문자열의 무한 누적 방지
    if (cache_.size() > 512) {
        for (auto& p : cache_) glDeleteTextures(1, &p.second.tex);
        cache_.clear();
    }

    HDC dc = CreateCompatibleDC(nullptr);
    SelectObject(dc, hFont_);
    SetMapMode(dc, MM_TEXT);
    SIZE sz = {};
    GetTextExtentPoint32W(dc, ws.c_str(), (int)ws.size(), &sz);
    int w = (sz.cx > 0) ? sz.cx : 1;
    int h = (sz.cy > 0) ? sz.cy : 1;
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void*   bits = nullptr;
    HBITMAP bmp  = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    SelectObject(dc, bmp);
    SelectObject(dc, hFont_);
    SetBkMode(dc,   OPAQUE);
    SetBkColor(dc,  RGB(0,   0,   0));
    SetTextColor(dc, RGB(255, 255, 255));
    TextOutW(dc, 0, 0, ws.c_str(), (int)ws.size());
    GdiFlush();
    std::vector<uint8_t> red(w * h);
    auto* src = reinterpret_cast<uint8_t*>(bits);
    for (int i = 0; i < w * h; i++) red[i] = src[i * 4 + 2];
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, red.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    DeleteObject(bmp);
    DeleteDC(dc);
    return cache_[ws] = Entry{tex, w, h};
}

inline void TextRenderer::Draw(const char* utf8, float x, float y, float scale,
                               float r, float g, float b, float a)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    std::wstring ws(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, ws.data(), n);
    if (!ws.empty() && ws.back() == L'\0') ws.pop_back();
    Issue(Cache(ws), x, y, scale, r, g, b, a);
}

// Windows 빌드에서는 미사용 (InitWithFace 사용)
inline bool TextRenderer::InitFromFiles(const char* const*, int, int, int sw, int sh)
{ screenW_ = sw; screenH_ = sh; return InitGLPipeline(); }

// ═══════════════════════ macOS / Linux (stb 글리프 아틀라스) ════════════════════
#else

inline bool TextRenderer::InitFromFiles(const char* const* ttfPaths, int nPaths,
                                        int ptSize, int sw, int sh)
{
    screenW_ = sw;  screenH_ = sh;
    // GDI 의 점→픽셀 변환(pt × 96/72)에 맞춰 em 매핑 크기 보정 (한글 크기 일치)
    emPx_ = (float)ptSize * (96.0f / 72.0f);

    faces_.reserve(nPaths);
    for (int i = 0; i < nPaths; i++) {
        FontFace face;
        FILE* fp = std::fopen(ttfPaths[i], "rb");
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
    if (faces_.empty()) return false;

    // 주 폰트 기준 베이스라인/줄높이
    float s0 = stbtt_ScaleForMappingEmToPixels(&faces_[0].info, emPx_);
    int asc = 0, desc = 0, gap = 0;
    stbtt_GetFontVMetrics(&faces_[0].info, &asc, &desc, &gap);
    ascentPx_     = asc * s0;
    lineHeightPx_ = (asc - desc) * s0;
    return InitGLPipeline();
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
    float baseline = y + ascentPx_ * scale;   // y = 텍스트 상단 (GDI 와 동일 기준)
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

#endif // _WIN32
