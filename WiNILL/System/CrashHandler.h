#pragma once
// ─────────────────────────────────────────────────────────────
// 크래시 핸들러 — 강제종료(강종/E23) 원인 추적 인프라.
//   처리되지 않은 예외(SEH) 발생 시:
//     1) onedow_crash.log 에 시각/예외코드/주소/모듈 오프셋 + 마지막 브레드크럼
//     2) onedow_crash_<시각>.dmp 미니덤프(전역 데이터 포함)
//     3) 사용자에게 안내 메시지박스
//   재현이 안 되는 크래시도 "다음에 터질 때" 위치/상태를 남겨 원인 특정 가능.
//   브레드크럼: main 이 매 프레임 현재 상태(씬/점수/활성보스/엔티티수)를 갱신 →
//   덤프 없이 로그만 봐도 어디서 죽었는지 대략 좁혀짐.
// ─────────────────────────────────────────────────────────────
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#pragma comment(lib, "Dbghelp.lib")

namespace CrashHandler {

// main 이 매 프레임 갱신하는 상태 문자열 (크래시 로그에 찍힘)
inline char g_Breadcrumb[256] = "startup";
inline void SetBreadcrumb(const char* s) {
    if (!s) return;
    size_t i = 0;
    for (; i < sizeof(g_Breadcrumb) - 1 && s[i]; ++i) g_Breadcrumb[i] = s[i];
    g_Breadcrumb[i] = '\0';
}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)   // fopen / localtime
#endif

inline void writeLog(EXCEPTION_POINTERS* ep) {
    FILE* f = std::fopen("onedow_crash.log", "a");
    if (!f) return;
    time_t t = std::time(nullptr);
    struct tm tmv; localtime_s(&tmv, &t);
    char ts[64]; std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv);
    std::fprintf(f, "==== CRASH %s ====\n", ts);
    std::fprintf(f, "state: %s\n", g_Breadcrumb);
    if (ep && ep->ExceptionRecord) {
        auto* er = ep->ExceptionRecord;
        std::fprintf(f, "code=0x%08lX addr=%p\n",
                     (unsigned long)er->ExceptionCode, er->ExceptionAddress);
        HMODULE mod = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)er->ExceptionAddress, &mod) && mod) {
            char modPath[MAX_PATH] = { 0 };
            GetModuleFileNameA(mod, modPath, MAX_PATH);
            std::fprintf(f, "module=%s base=%p offset=0x%llX\n", modPath, (void*)mod,
                         (unsigned long long)((char*)er->ExceptionAddress - (char*)mod));
        }
    }
    std::fprintf(f, "\n");
    std::fclose(f);
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

inline void writeDump(EXCEPTION_POINTERS* ep) {
    time_t t = std::time(nullptr);
    struct tm tmv; localtime_s(&tmv, &t);
    char name[128];
    std::strftime(name, sizeof(name), "onedow_crash_%Y%m%d_%H%M%S.dmp", &tmv);
    HANDLE hFile = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;
    MINIDUMP_EXCEPTION_INFORMATION mei;
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = ep;
    mei.ClientPointers = FALSE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                      (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory),
                      ep ? &mei : NULL, NULL, NULL);
    CloseHandle(hFile);
}

inline LONG WINAPI Filter(EXCEPTION_POINTERS* ep) {
    writeLog(ep);
    writeDump(ep);
    MessageBoxA(NULL,
        "Onedow has crashed.\n\n"
        "A crash log (onedow_crash.log) and a dump file were saved\n"
        "next to the game. Please send them to the developer.",
        "Onedow - Crash", MB_OK | MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;   // 프로세스 정상 종료(무한 루프/재진입 방지)
}

inline void Install() {
    SetUnhandledExceptionFilter(Filter);
}

}  // namespace CrashHandler
#else
namespace CrashHandler {
inline void SetBreadcrumb(const char*) {}
inline void Install() {}
}
#endif
