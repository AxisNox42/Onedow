#pragma once
#include <glad/glad.h>

// 메인 배치 셰이더 컴파일·링크 + CRT(uFx) 유니폼 위치
GLuint CompileGlShader(GLenum type, const char* src);
void   InitMainShaderPipeline(int screenW, int screenH);

// 줌/흔들림 전 기준 ortho (매 프레임 UI 패스에서 g_MainOrtho 로 복원)
extern float g_BaseOrtho[16];
extern GLint g_MainFxLoc;
extern GLint g_MainResLoc;

// 메인 VAO/VBO + g_BaseOrtho / g_MainOrtho 초기화
void InitMainBatchGeometry(int screenW, int screenH);
