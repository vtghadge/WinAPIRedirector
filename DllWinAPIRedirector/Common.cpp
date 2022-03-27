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

bool FileExists(const std::wstring& path)
{
    WIN32_FIND_DATA findFileData;
    HANDLE handle = FindFirstFile(path.c_str(), &findFileData);

    bool found = handle != INVALID_HANDLE_VALUE;

    if (found)
    {
        FindClose(handle);
    }

    return found;
}

void ToLowerCase(std::wstring& path)
{
    std::transform(path.begin(), path.end(), path.begin(), [](wchar_t c) { return std::tolower(c); });
}

std::wstring ConvertStringToWstring(std::string& string)
{
    int len;
    int stringLen = (int)string.length() + 1;
    std::wstring convertedString;

    len = MultiByteToWideChar(CP_ACP, 0, string.c_str(), stringLen, 0, 0);
    if (0 == len)
    {
        return std::wstring();
    }

    convertedString.resize(len);

    len = MultiByteToWideChar(CP_ACP, 0, string.c_str(), stringLen, &convertedString[0], len);
    if (0 == len)
    {
        return std::wstring();
    }

    convertedString.erase(convertedString.end() - 1);

    return convertedString;
}

std::string ConvertWstringToString(std::wstring& wstring)
{
    int len;
    int stringLen = (int)wstring.length() + 1;
    std::string convertedString;

    len = WideCharToMultiByte(CP_ACP, 0, wstring.c_str(), stringLen, 0, 0, 0, 0);
    if (0 == len)
    {
        return std::string();
    }

    convertedString.resize((len / sizeof(CHAR)));

    len = WideCharToMultiByte(CP_ACP, 0, wstring.c_str(), stringLen, &convertedString[0], len, 0, 0);
    if (0 == len)
    {
        return std::string();
    }

    convertedString.erase(convertedString.end() - 1);

    return convertedString;
}

void ExtractProcessName(std::wstring &processPath)
{
    size_t pos = processPath.find_last_of(L"\\");
    if (pos != std::wstring::npos)
    {
        processPath.erase(0, pos + 1);
    }
}
