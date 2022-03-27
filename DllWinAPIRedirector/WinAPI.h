#pragma once
#include "ProcessInfo.h"

VOID DetAttach(PVOID* ppvReal, PVOID pvMine, const char* psz);

VOID DetDetach(PVOID* ppvReal, PVOID pvMine, const char* psz);

LONG AttachDetours(VOID);

LONG DetachDetours(VOID);

extern int g_iDetach;

class WinAPIRedirector
{
public:
	static bool Init(std::wstring srcDirPath, std::wstring redirectDirPath);
	static void Release();
	static WinAPIRedirector* GetInstance();

	~WinAPIRedirector();

	bool IsOperationInterested(std::wstring& path, DWORD dwDesiredAccess, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, std::wstring& redirectedPath);

	bool OnHandleCreation(HANDLE hfile, std::wstring& originalPath, std::wstring& redirectedPath);
	bool OnHandleClose(HANDLE hfile);
	bool IsRedirectedHandle(HANDLE hFile);

	ProcessInfo& GetProcessInfo();

private:
	WinAPIRedirector(std::wstring srcDirPath, std::wstring redirectedDirPath);

	std::wstring m_srcDirPath;
	std::wstring m_redirectedDirPath;

	struct HandleInfo
	{
		std::wstring m_originalPath;
		std::wstring m_redirectedPath;

		HandleInfo();
		HandleInfo(std::wstring originalPath, std::wstring redirectedPath);
	};

	std::map<HANDLE, HandleInfo> m_handleInfoMap;
	CRITICAL_SECTION m_handleInfoLock;

	static std::unique_ptr<WinAPIRedirector> s_winAPIRedirector;
	ProcessInfo m_processInfo;

private:

	void Lock();
	void Unlock();

	bool CheckIfSourcePathLocation(std::wstring& path, std::wstring& redirectedPath);

};
