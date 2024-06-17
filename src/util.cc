#include <string>
#include <windows.h>

LPCWSTR str2wstr(const std::string &s)
{
    std::wstring stemp = std::wstring(s.begin(), s.end());
    return stemp.c_str();
}

std::string wchar_to_string(const WCHAR* wchar)
{
    std::string str = "";
    int index = 0;
    while(wchar[index] != 0)
    {
        str += static_cast<char>(wchar[index]);
        ++index;
    }
    return str;
}