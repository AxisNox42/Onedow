#pragma once
// ─────────────────────────────────────────────────────────────
// 텍스트 렌더러 (OpenGL 텍스처 방식)
//   Windows : GDI 로 글리프 래스터화 (기존 검증된 경로)
//   그 외(macOS/Linux) : stb_truetype 로 래스터화 + 다중 폰트 폴백 체인
//
//   공통 부분(GL 파이프라인 / 캐시 / Draw)은 플랫폼 무관하게 공유하고
//   글리프 래스터화(Cache) 와 폰트 핸들만 #ifdef 로 분기한다.
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
    // fontPath: 실행 파일 기준 상대 경로, faceName: TTF 내부 패밀리 이름
    bool Init(const char* fontPath, const char* faceName, int ptSize,
              int screenW, int screenH);
    // 외부에서 이미 등록된 폰트(AddFontMemResourceEx 등)를 face 이름으로 init
    bool InitWithFace(const char* faceName, int ptSize,
                      int screenW, int screenH);
#endif
    // 크로스플랫폼: TTF 파일 폴백 체인으로 init (앞쪽 폰트 우선, 없는 글리프는 다음 폰트)
    //   ttfPaths[0] 가 주 폰트, 이후는 폴백. (macOS/Linux 경로)
    bool InitFromFiles(const char* const* ttfPaths, int nPaths, int ptSize,
                       int screenW, int screenH);

    void Cleanup();
    ~TextRenderer() { Cleanup(); }

    // 너비/높이(픽셀) — 중앙 정렬용
    float Width(const wchar_t* text, float scale = 1.0f);
    float Height(const wchar_t* text, float scale = 1.0f);

    void Draw(const wchar_t* text, float x, float y, float scale,
              float r, float g, float b, float a = 1.0f);
    void Draw(const char* utf8, float x, float y, float scale,
              float r, float g, float b, float a = 1.0f);

private:
    struct Entry { GLuint tex; int w, h; };

    int   screenW_ = 0, screenH_ = 0;
    GLuint prog_ = 0, VAO_ = 0, VBO_ = 0;
    GLint  uProj_ = -1, uCol_ = -1;
    std::unordered_map<std::wstring, Entry> cache_;

#ifdef _WIN32
    HFONT hFont_ = nullptr;
    char  fontPath_[MAX_PATH] = {};
#else
    int   ptSize_ = 24;
    struct FontFace {
        std::vector<unsigned char> data;
        stbtt_fontinfo info;
        bool ok = false;
    };
    std::vector<FontFace> faces_;
    // 코드포인트를 가진 첫 폰트 인덱스 (-1=없음)
    int  FaceForCodepoint(int cp) const;
#endif

    Entry&  Cache(const std::wstring& ws);
    void    Issue(const Entry& e, float x, float y, float scale,
                  float r, float g, float b, float a);
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
        x,   y,   0, 0,
        x+w, y,   1, 0,
        x+w, y+h, 1, 1,
        x,   y,   0, 0,
        x+w, y+h, 1, 1,
        x,   y+h, 0, 1,
    };

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);
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

inline float TextRenderer::Width(const wchar_t* text, float scale)
{
    return Cache(std::wstring(text)).w * scale;
}
inline float TextRenderer::Height(const wchar_t* text, float scale)
{
    return Cache(std::wstring(text)).h * scale;
}
inline void TextRenderer::Draw(const wchar_t* text, float x, float y, float scale,
                               float r, float g, float b, float a)
{
    Issue(Cache(std::wstring(text)), x, y, scale, r, g, b, a);
}

// ═══════════════════════ Windows (GDI) ═════════════════════════════════════════
#ifdef _WIN32

inline bool TextRenderer::InitWithFace(const char* faceName, int ptSize,
                                       int sw, int sh)
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
    for (int i = 0; i < w * h; i++)
        red[i] = src[i * 4 + 2];

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0,
                 GL_RED, GL_UNSIGNED_BYTE, red.data());
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

// InitFromFiles 는 Windows 빌드에서는 사용하지 않음 (InitWithFace 사용)
inline bool TextRenderer::InitFromFiles(const char* const*, int, int,
                                        int sw, int sh)
{
    screenW_ = sw;  screenH_ = sh;
    return InitGLPipeline();
}

// ═══════════════════════ macOS / Linux (stb_truetype) ══════════════════════════
#else

inline bool TextRenderer::InitFromFiles(const char* const* ttfPaths, int nPaths,
                                        int ptSize, int sw, int sh)
{
    screenW_ = sw;  screenH_ = sh;
    ptSize_  = ptSize;

    faces_.reserve(nPaths);  // 재할당 방지 (info.data 포인터 안정성)
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
    return InitGLPipeline();
}

inline int TextRenderer::FaceForCodepoint(int cp) const
{
    for (int i = 0; i < (int)faces_.size(); i++) {
        if (!faces_[i].ok) continue;
        if (stbtt_FindGlyphIndex(&faces_[i].info, cp) != 0)
            return i;
    }
    return faces_.empty() ? -1 : 0;  // 못 찾으면 주 폰트(.notdef)
}

inline void TextRenderer::Cleanup() {
    for (auto& p : cache_) glDeleteTextures(1, &p.second.tex);
    cache_.clear();
    if (VAO_)  { glDeleteVertexArrays(1, &VAO_); VAO_ = 0; }
    if (VBO_)  { glDeleteBuffers(1, &VBO_);       VBO_ = 0; }
    if (prog_) { glDeleteProgram(prog_);           prog_ = 0; }
    faces_.clear();
}

inline TextRenderer::Entry& TextRenderer::Cache(const std::wstring& ws)
{
    auto it = cache_.find(ws);
    if (it != cache_.end()) return it->second;

    const int   N    = (int)ws.size();
    const float fpx  = (float)ptSize_;

    // ── 1패스: 너비/상하 범위 측정 (베이스라인 기준) ──
    float penX  = 0.0f;
    int   minY0 =  100000, maxY1 = -100000;
    bool  anyGlyph = false;

    for (int i = 0; i < N; i++) {
        int cp = (int)ws[i];
        int fi = FaceForCodepoint(cp);
        if (fi < 0) continue;
        const stbtt_fontinfo* fn = &faces_[fi].info;
        float s = stbtt_ScaleForPixelHeight(fn, fpx);

        int adv = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(fn, cp, &adv, &lsb);

        int ix0, iy0, ix1, iy1;
        stbtt_GetCodepointBitmapBox(fn, cp, s, s, &ix0, &iy0, &ix1, &iy1);
        if (iy1 > iy0) {
            if (iy0 < minY0) minY0 = iy0;
            if (iy1 > maxY1) maxY1 = iy1;
            anyGlyph = true;
        }
        penX += adv * s;
    }

    if (!anyGlyph) { minY0 = -(int)(fpx * 0.8f); maxY1 = (int)(fpx * 0.2f); }
    int ascent = -minY0;
    if (ascent < 1) ascent = (int)(fpx * 0.8f);
    int W = (int)(penX + 0.5f);  if (W < 1) W = 1;
    int H = ascent + maxY1;      if (H < 1) H = 1;
    // 안티앨리어싱 여유 1px 패딩
    W += 2; H += 2;

    std::vector<unsigned char> bitmap((size_t)W * H, 0);

    // ── 2패스: 글리프를 베이스라인에 맞춰 합성 ──
    float fx = 1.0f;  // 펜 X (서브픽셀 누적)
    int   baseline = ascent + 1;  // +1 패딩
    for (int i = 0; i < N; i++) {
        int cp = (int)ws[i];
        int fi = FaceForCodepoint(cp);
        if (fi < 0) continue;
        const stbtt_fontinfo* fn = &faces_[fi].info;
        float s = stbtt_ScaleForPixelHeight(fn, fpx);

        int adv = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(fn, cp, &adv, &lsb);

        int ix0, iy0, ix1, iy1;
        stbtt_GetCodepointBitmapBox(fn, cp, s, s, &ix0, &iy0, &ix1, &iy1);
        int gw = ix1 - ix0, gh = iy1 - iy0;
        if (gw > 0 && gh > 0) {
            int gx = (int)(fx + 0.5f) + ix0;
            int gy = baseline + iy0;
            if (gx < 0) gx = 0;
            if (gy < 0) gy = 0;
            if (gx + gw <= W && gy + gh <= H) {
                stbtt_MakeCodepointBitmap(
                    fn, &bitmap[(size_t)gy * W + gx],
                    gw, gh, W, s, s, cp);
            }
        }
        fx += adv * s;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, W, H, 0,
                 GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return cache_[ws] = Entry{tex, W, H};
}

// UTF-8 → wstring(UTF-32) 디코드 후 Draw
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
    Issue(Cache(ws), x, y, scale, r, g, b, a);
}

#endif // _WIN32
