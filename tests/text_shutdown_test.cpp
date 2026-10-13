// Real OpenGL regression for FIX-02: run with scripts/test_text_shutdown.ps1.
// Every game font (g_TextL/S/XL) must release its GL objects through
// ShutdownGameFonts while the context is current, so the global renderers'
// destructors make no GL calls after glfwTerminate.
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameFonts.h"
#include "TextRenderer.h"
#include <cstdio>
#include <cstdlib>

TextRenderer g_TextL;
TextRenderer g_TextS;
TextRenderer g_TextXL;
float g_BaseOrtho[16] = {};

static void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

// After the context is gone any GL delete is a violation; these count them.
static int s_glCallsWithoutContext = 0;
static void APIENTRY CountDeleteTextures(GLsizei, const GLuint*) { ++s_glCallsWithoutContext; }
static void APIENTRY CountDeleteVertexArrays(GLsizei, const GLuint*) { ++s_glCallsWithoutContext; }
static void APIENTRY CountDeleteBuffers(GLsizei, const GLuint*) { ++s_glCallsWithoutContext; }
static void APIENTRY CountDeleteProgram(GLuint) { ++s_glCallsWithoutContext; }
static void ReplaceGlDeletesWithCounters() {
    glad_glDeleteTextures = CountDeleteTextures;
    glad_glDeleteVertexArrays = CountDeleteVertexArrays;
    glad_glDeleteBuffers = CountDeleteBuffers;
    glad_glDeleteProgram = CountDeleteProgram;
}

int main() {
    Check(glfwInit() == GLFW_TRUE, "GLFW init");
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(256, 256, "Text shutdown regression",
                                          nullptr, nullptr);
    Check(window != nullptr, "window");
    glfwMakeContextCurrent(window);
    Check(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) != 0, "GL loader");

    InitializeGameFonts(1280, 720);
    Check(g_TextL.HasGlResources() && g_TextS.HasGlResources() &&
          g_TextXL.HasGlResources(), "all game fonts hold GL objects after init");

    // Control: a renderer left uncleaned until after glfwTerminate is what
    // the destructor of an unreleased global meets (the old g_TextXL path).
    TextRenderer leaked;
    const char* font[] = { "Resource/Font/Jua-Regular.ttf" };
    Check(leaked.InitFromFiles(font, 1, 22, 1280, 720), "control renderer init");
    leaked.PreloadText(L"FIX-02");

    ShutdownGameFonts();
    Check(!g_TextL.HasGlResources() && !g_TextS.HasGlResources() &&
          !g_TextXL.HasGlResources(), "ShutdownGameFonts releases every game font");
    ShutdownGameFonts();                                   // second pass is safe
    Check(!g_TextXL.HasGlResources(), "repeated shutdown stays released");

    glfwDestroyWindow(window);
    glfwTerminate();
    ReplaceGlDeletesWithCounters();

    // What the global destructors run after main returns: no GL calls.
    g_TextL.Cleanup();
    g_TextS.Cleanup();
    g_TextXL.Cleanup();
    Check(s_glCallsWithoutContext == 0, "released fonts make no GL calls after glfwTerminate");

    // The control must trip the counter, or the check above proves nothing.
    leaked.Cleanup();
    Check(s_glCallsWithoutContext > 0, "detector sees GL calls from an unreleased renderer");
    std::printf("PASS: game fonts released before glfwTerminate; "
                "an unreleased renderer makes %d GL deletes without a context\n",
                s_glCallsWithoutContext);
    return 0;
}
