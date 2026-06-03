// stb_truetype 구현부 — 단일 번역 단위에서만 정의 (다중 정의 방지)
//   모든 플랫폼에서 stb 글리프 아틀라스 텍스트 렌더러를 사용한다.
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// stb_image 구현부 — 아이콘(PNG) 로딩용
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
