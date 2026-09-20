#pragma once

#include <cstddef>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

// Windows release builds keep runtime assets in the PE resource section.
// The view is valid for the lifetime of the module; callers must not free it.
struct EmbeddedResourceView {
    const unsigned char* data = nullptr;
    int size = 0;

    bool valid() const { return data != nullptr && size > 0; }
};

inline bool LoadEmbeddedResource(const char* name, EmbeddedResourceView& out) {
    out = {};
#ifdef _WIN32
    if (!name || !name[0]) return false;

    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC resource = FindResourceA(module, name, MAKEINTRESOURCEA(10));
    if (!resource) return false;

    HGLOBAL loaded = LoadResource(module, resource);
    const void* bytes = loaded ? LockResource(loaded) : nullptr;
    DWORD byteCount = loaded ? SizeofResource(module, resource) : 0;
    if (!bytes || byteCount == 0) return false;

    out.data = static_cast<const unsigned char*>(bytes);
    out.size = static_cast<int>(byteCount);
    return true;
#else
    (void)name;
    return false;
#endif
}
