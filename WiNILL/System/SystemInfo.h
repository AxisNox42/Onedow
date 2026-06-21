#pragma once
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cwchar>
#endif

// 베타 테스터 피드백용 — 현재 PC 사양 요약 (설정 화면 표시)
inline std::wstring GetSystemSpecLine1() {
#ifdef _WIN32
    wchar_t cpu[256] = L"CPU: (unknown)";
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD sz = (DWORD)sizeof(cpu);
        RegQueryValueExW(hKey, L"ProcessorNameString", nullptr, nullptr,
                         (LPBYTE)cpu, &sz);
        RegCloseKey(hKey);
        while (cpu[0] == L' ') {
            size_t n = wcslen(cpu);
            for (size_t i = 0; i + 1 < n; ++i) cpu[i] = cpu[i + 1];
            cpu[n - 1] = 0;
        }
    }
    SYSTEM_INFO si = {};
    GetNativeSystemInfo(&si);
    MEMORYSTATUSEX mem = {};
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    double ramGb = (double)mem.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
    wchar_t buf[512];
    swprintf_s(buf, L"%ls  |  %u cores  |  RAM %.1f GB",
               cpu, (unsigned)si.dwNumberOfProcessors, ramGb);
    return buf;
#else
    return L"CPU: n/a";
#endif
}

inline std::wstring GetSystemSpecLine2(int screenW, int screenH) {
#ifdef _WIN32
    OSVERSIONINFOEXW vi = {};
    vi.dwOSVersionInfoSize = sizeof(vi);
    std::wstring os = L"Windows";
    typedef LONG (WINAPI *RtlGetVersionFn)(PRTL_OSVERSIONINFOW);
    if (HMODULE ntd = GetModuleHandleW(L"ntdll.dll")) {
        auto fn = (RtlGetVersionFn)GetProcAddress(ntd, "RtlGetVersion");
        if (fn && fn((PRTL_OSVERSIONINFOW)&vi) == 0)
            os = L"Win " + std::to_wstring(vi.dwMajorVersion) + L"."
               + std::to_wstring(vi.dwMinorVersion) + L" build "
               + std::to_wstring(vi.dwBuildNumber);
    }
    wchar_t buf[256];
    swprintf_s(buf, L"%ls  |  Display %dx%d", os.c_str(), screenW, screenH);
    return buf;
#else
    (void)screenW; (void)screenH;
    return L"OS: n/a";
#endif
}
