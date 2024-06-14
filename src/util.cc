#include <string>
#include <windows.h>

LPCWSTR str2wstr(const std::string& s)
{
    std::wstring stemp = std::wstring(s.begin(), s.end());
    return stemp.c_str();
}