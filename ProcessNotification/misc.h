#pragma once

BOOLEAN SetPrivilege(const TCHAR* pcszPrivilegeStr, BOOL bEnablePrivilege, DWORD* pdwError);

BOOL GetWorkingDirPathW(std::wstring& folderPath, bool bIncludeLastBackslash);
void Install(std::wstring infFilename);
void Uninstall(std::wstring infFilename);
bool StartDriver();
bool StopDriver();
