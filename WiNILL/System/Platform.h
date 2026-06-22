#pragma once
// ─────────────────────────────────────────────────────────────
// 플랫폼 추상화 — 타이머 / 슬립 / 작업 디렉터리
//   배포(LTS): bin/Onedow/{Resource,Font,WindowsOS/Onedow.exe,macOS/Onedow}
//   테스트:    WiNILL/bin/WiNILL.exe
// ─────────────────────────────────────────────────────────────
#include <string>
#include <cstdio>

#ifdef _WIN32
  #include <windows.h>
  #include <timeapi.h>
  #include <direct.h>
#else
  #include <unistd.h>
  #include <libgen.h>
  #include <sys/stat.h>
  #include <thread>
  #include <chrono>
  #include <cwchar>
  #include <ctime>
  #if defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <cstdint>
  #endif

  // MSVC secure CRT 호환 (macOS / Linux)
  #ifndef _TRUNCATE
    #define _TRUNCATE static_cast<size_t>(-1)
  #endif
  #ifndef swprintf_s
    #define swprintf_s(buf, ...) swprintf((buf), sizeof(buf) / sizeof((buf)[0]), __VA_ARGS__)
  #endif
  template <size_t N>
  inline void wcscpy_s(wchar_t (&dest)[N], const wchar_t* src) {
    wcsncpy(dest, src, N - 1);
    dest[N - 1] = L'\0';
  }
  template <size_t N>
  inline void wcsncpy_s(wchar_t (&dest)[N], const wchar_t* src, size_t count) {
    if (count == static_cast<size_t>(-1) || count >= N) {
      wcsncpy(dest, src, N - 1);
      dest[N - 1] = L'\0';
    } else {
      wcsncpy(dest, src, count);
      dest[count] = L'\0';
    }
  }
  inline void localtime_s(struct tm* result, const time_t* timep) {
    localtime_r(timep, result);
  }
#endif

inline void PlatformTimerBegin() {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
}
inline void PlatformTimerEnd() {
#ifdef _WIN32
    timeEndPeriod(1);
#endif
}
inline void PlatformSleepMs(unsigned ms) {
    if (!ms) return;
#ifdef _WIN32
    Sleep(ms);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

inline bool PlatformPathIsDir(const std::string& path) {
#ifdef _WIN32
    DWORD a = GetFileAttributesA(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
#endif
}

inline std::string PlatformExeDirectory() {
#ifdef _WIN32
    char buf[MAX_PATH] = {};
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return {};
    std::string p(buf, n);
    size_t slash = p.find_last_of("\\/");
    if (slash == std::string::npos) return {};
    return p.substr(0, slash);
#elif defined(__APPLE__)
    char buf[4096] = {};
    uint32_t sz = sizeof(buf);
    if (_NSGetExecutablePath(buf, &sz) != 0) return {};
    std::string p(buf);
    size_t slash = p.find_last_of('/');
    if (slash == std::string::npos) return {};
    return p.substr(0, slash);
#else
    char buf[4096] = {};
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return {};
    buf[n] = '\0';
    std::string p(buf);
    size_t slash = p.find_last_of('/');
    if (slash == std::string::npos) return {};
    return p.substr(0, slash);
#endif
}

inline void PlatformChdirToExeDir() {
    std::string exeDir = PlatformExeDirectory();
    if (exeDir.empty()) return;

    auto tryChdir = [](const std::string& dir) -> bool {
        if (dir.empty()) return false;
#ifdef _WIN32
        return _chdir(dir.c_str()) == 0;
#else
        return chdir(dir.c_str()) == 0;
#endif
    };

    std::string parent = exeDir;
    size_t slash = parent.find_last_of("/\\");
    if (slash != std::string::npos)
        parent = parent.substr(0, slash);

    if (PlatformPathIsDir(exeDir + "/Resource"))
        tryChdir(exeDir);
    else if (PlatformPathIsDir(parent + "/Resource"))
        tryChdir(parent);
    else
        tryChdir(exeDir);
}
