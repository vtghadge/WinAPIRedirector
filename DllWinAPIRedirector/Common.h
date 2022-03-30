#pragma once

void DbgViewf(LPCTSTR format, ...);
void DbgViewfA(LPCSTR format, ...);

bool FileExists(const std::wstring& path);
void ToLowerCase(std::wstring& path);
std::wstring ConvertStringToWstring(std::string& string);
std::string ConvertWstringToString(std::wstring& wstring);
void ExtractProcessName(std::wstring& processPath);
bool InitRedirectedPathFromConfig(std::wstring& srcPath, std::wstring& rediredtedPath);
bool GetRedirectionPathData(WCHAR** ppwszData);
void ReleaseRedirectionPathData(WCHAR* pwszData);
