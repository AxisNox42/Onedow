#include "Platform.h"
#include <cassert>
#include <cwchar>

int main() {
    wchar_t array[16] = {};
    assert(PortableWideFormat::swprintf_s(array, L"%ls %d", L"HP", 12) == 5);
    assert(std::wcscmp(array, L"HP 12") == 0);

    wchar_t storage[16] = {};
    wchar_t* pointer = storage;
    assert(PortableWideFormat::swprintf_s(pointer, 16, L"%d", 123) == 3);
    assert(std::wcscmp(storage, L"123") == 0);

    wchar_t shortBuffer[4] = {};
    assert(PortableWideFormat::swprintf_s(shortBuffer, L"%ls", L"long") == -1);
    assert(shortBuffer[3] == L'\0');
}
