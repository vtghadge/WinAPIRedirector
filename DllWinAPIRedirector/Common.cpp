#include"pch.h"

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

void ExtractFolderPath(std::wstring& processPath, bool boIncludeSlash)
{
    size_t pos = processPath.find_last_of(L"\\");
    if (pos != std::wstring::npos)
    {
        if (boIncludeSlash)
        {
            processPath.erase(pos + 1);
        }
        else
        {
            processPath.erase(pos);
        }
    }
}


bool InitRedirectedPathFromConfig(std::wstring &srcPath, std::wstring &rediredtedPath)
{
    WCHAR* pwszData;
    WCHAR* pwszDataTemp;

    bool boRet = GetRedirectionPathData(&pwszData);
    if (false == boRet)
    {
        return boRet;
    }

    pwszDataTemp = pwszData;
    std::wstring redirectionPathMap;
    bool found = false;
    size_t pos;
    while (*pwszDataTemp != '\0')
    {
        redirectionPathMap = pwszDataTemp;
        ToLowerCase(redirectionPathMap);

        pos = redirectionPathMap.find('=');
        if (std::wstring::npos != pos)
        {
            found = true;
            break;
        }

        pwszDataTemp += redirectionPathMap.length() + 1;
    }

    ReleaseRedirectionPathData(pwszData);

    if (false == found)
    {
        return found;
    }

    srcPath = redirectionPathMap.substr(0, pos);
    rediredtedPath = redirectionPathMap.substr(pos + 1);

    return true;
}

bool
GetRedirectionPathData(
    WCHAR** ppwszData
)
{
    DWORD dwRetVal;
    HRESULT hResult;
    WCHAR* pwszData;
    WCHAR wszFilePath[MAX_PATH];

    if (NULL == ppwszData)
    {
        return false;
    }
    std::wstring dllPath = g_szDllPath;
    ExtractFolderPath(dllPath, true);

    hResult = StringCchCopyW(wszFilePath, ARRAYSIZE(wszFilePath), dllPath.c_str());
    if (FAILED(hResult))
    {
        return false;
    }

    hResult = StringCchCatW(wszFilePath, ARRAYSIZE(wszFilePath), FILE_NAME_CONFIG_W);
    if (FAILED(hResult))
    {
        return false;
    }

    pwszData = (WCHAR*)malloc(MAX_INI_SECTION_CHARS * sizeof(WCHAR));
    if (NULL == pwszData)
    {
        return false;
    }
    ZeroMemory(pwszData, MAX_INI_SECTION_CHARS * sizeof(WCHAR));

    dwRetVal = GetPrivateProfileSectionW(
        CONFIG_SECTION_NAME_W,
        pwszData,
        MAX_INI_SECTION_CHARS,
        wszFilePath
    );
    if (/*0 == dwRetVal || */dwRetVal == (MAX_INI_SECTION_CHARS - 2))	//	If the buffer is not large enough to contain all the key name and value pairs associated with the named section, the return value is equal to nSize minus two.
    {
        wprintf(L"GetPrivateProfileSectionW() Failed.\n");

        free(pwszData);
        return false;
    }

    *ppwszData = pwszData;

    return TRUE;
}

void ReleaseRedirectionPathData(
    WCHAR* pwszData
)
{
    if (NULL == pwszData)
        return;

    free(pwszData);
}
