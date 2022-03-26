#include"pch.h"
#include "Common.h"

void DbgViewf(LPCTSTR format, ...)
{
    TCHAR buffer[512];
    TCHAR* p = buffer;
    va_list args;

    va_start(args, format);
    if (_vsntprintf_s(p, 512, _TRUNCATE, format, args) == -1)
    {
        int size = _vsctprintf(format, args) + 1;
        p = (TCHAR*)malloc(sizeof(TCHAR) * size);
        if (p != NULL)
            _vsntprintf_s(p, size, _TRUNCATE, format, args);
    }
    va_end(args);

    if (p != NULL)
    {
        OutputDebugString(p);
        if (p != buffer) free(p);
    }
}


bool WstringCompareNoCase(const std::wstring& a, const std::wstring& b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
        [](wchar_t a, wchar_t b)
        {
            return tolower(a) == tolower(b);
        });
}

bool StringCompareNoCase(const std::string& a, const std::string& b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
        [](char a, char b)
        {
            return tolower(a) == tolower(b);
        });
}
