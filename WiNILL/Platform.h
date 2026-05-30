#pragma once
// ─────────────────────────────────────────────────────────────
// 플랫폼 추상화 — 타이머 해상도 / 슬립 / 작업 디렉터리
//   Windows : timeBeginPeriod 로 Sleep 해상도 1ms + Sleep
//   그 외   : std::this_thread::sleep_for (이미 고해상도)
// ─────────────────────────────────────────────────────────────
#ifdef _WIN32
  #include <windows.h>
  #include <timeapi.h>
  inline void PlatformTimerBegin() { timeBeginPeriod(1); }
  inline void PlatformTimerEnd()   { timeEndPeriod(1); }
  inline void PlatformSleepMs(unsigned ms) { if (ms) Sleep(ms); }
  // Windows 는 폰트를 EXE 임베디드 리소스로 로드하므로 작업 디렉터리 변경 불필요 (no-op)
  inline void PlatformChdirToExeDir() {}
#else
  #include <thread>
  #include <chrono>
  #include <unistd.h>
  #include <libgen.h>
  #include <string>
  inline void PlatformTimerBegin() {}
  inline void PlatformTimerEnd()   {}
  inline void PlatformSleepMs(unsigned ms) {
      if (ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }
  // 실행 파일이 있는 폴더로 작업 디렉터리 변경
  //   → Finder 더블클릭(작업폴더=/)으로 실행해도 "Resource/Font/..." 상대경로가 잡힘
  #if defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <cstdint>
    inline void PlatformChdirToExeDir() {
        char buf[4096]; uint32_t sz = sizeof(buf);
        if (_NSGetExecutablePath(buf, &sz) == 0) {
            std::string path(buf);
            chdir(dirname(&path[0]));
        }
    }
  #else  // Linux
    inline void PlatformChdirToExeDir() {
        char buf[4096];
        ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (n > 0) { buf[n] = '\0'; chdir(dirname(buf)); }
    }
  #endif
#endif
