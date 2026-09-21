// Real OpenGL regression: run with scripts/test_blur_windows.ps1.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "DrawPrim.h"
#include "BlurShader.h"
#include <cstdio>
#include <cstdlib>
float g_BaseOrtho[16] = {};
GLuint CompileGlShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) std::exit(2);
    return shader;
}
static void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static void TestCapture(int size, int samples, GLuint unrelatedRead) {
    GLuint target = 0, color = 0;
    if (samples) {
        glGenFramebuffers(1, &target);
        glBindFramebuffer(GL_FRAMEBUFFER, target);
        glGenRenderbuffers(1, &color);
        glBindRenderbuffer(GL_RENDERBUFFER, color);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, size, size);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
        Check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "MSAA target");
    } else glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, size, size);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(size / 2, 0, size / 2, size);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    // Capture must use the DRAW target even when READ is another framebuffer.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, unrelatedRead);
    InitBlurSystem(size, size);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT_AND_BACK);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    CaptureBackdrop();
    GLint binding = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &binding);
    Check(binding == (GLint)unrelatedRead, "restore read framebuffer");
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &binding);
    Check(binding == (GLint)target, "restore draw framebuffer");
    Check(glIsEnabled(GL_CULL_FACE), "restore culling");
    GLboolean mask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, mask);
    Check(!mask[0] && !mask[3], "restore color mask");
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    g_BaseOrtho[0] = 2.f / size; g_BaseOrtho[5] = -2.f / size;
    g_BaseOrtho[10] = -1; g_BaseOrtho[12] = -1;
    g_BaseOrtho[13] = 1; g_BaseOrtho[15] = 1;
    DrawBlurPanel(0, 0, (float)size, (float)size, 1, 0, 0, 0);
    unsigned char center[4], left[4], right[4];
    glReadBuffer(GL_BACK);
    glReadPixels(size/2, size/2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, center);
    glReadPixels(1, size/2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, left);
    glReadPixels(size-2, size/2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, right);
    Check(center[0] > 30 && center[0] < 90, "edge must be blurred");
    Check(left[0] < center[0] && right[0] > center[0], "capture correct target and UVs");
    Check(center[0] == center[1] && center[1] == center[2], "no stale/invalid texture");
    Check(glGetError() == GL_NO_ERROR, "OpenGL error-free");
    std::printf("PASS size=%d samples=%d pixels=%u/%u/%u\n", size, samples, left[0], center[0], right[0]);
    glDeleteRenderbuffers(1, &color);
    glDeleteFramebuffers(1, &target);
}
int main() {
    Check(glfwInit() != 0, "GLFW init");
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(256, 256, "Blur regression", nullptr, nullptr);
    Check(window != nullptr, "OpenGL context");
    glfwMakeContextCurrent(window);
    Check(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) != 0, "GL loader");
    std::printf("Renderer: %s\n", glGetString(GL_RENDERER));
    glGenVertexArrays(1, &g_MainVAO); glBindVertexArray(g_MainVAO);
    glGenBuffers(1, &g_VBO); glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, 65536 * sizeof(float), nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    GLuint other = 0, texture = 0;
    glGenFramebuffers(1, &other); glBindFramebuffer(GL_FRAMEBUFFER, other);
    glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glClearColor(1, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    TestCapture(128, 0, other);
    TestCapture(256, 0, other);
    TestCapture(128, 4, other);
    TestCapture(128, 0, other);
    glfwDestroyWindow(window); glfwTerminate();
}
