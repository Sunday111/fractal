#include "os.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#undef APIENTRY
#include "Windows.h"

namespace klgl::os
{
std::filesystem::path GetExecutableDir()
{
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    const size_t index = std::wstring_view(path).find_last_of(L"\\/");
    path[index] = L'\0';
    return path;
}
}  // namespace klgl::os

#endif