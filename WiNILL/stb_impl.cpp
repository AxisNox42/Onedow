// stb_truetype 구현부 — 단일 번역 단위에서만 정의 (다중 정의 방지)
//   Windows 빌드에서는 GDI 텍스트 경로를 쓰므로 비워둔다.
#ifndef _WIN32
  #define STB_TRUETYPE_IMPLEMENTATION
  #include "stb_truetype.h"
#endif
