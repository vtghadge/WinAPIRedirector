#pragma once

VOID DetAttach(PVOID* ppvReal, PVOID pvMine, const char* psz);

VOID DetDetach(PVOID* ppvReal, PVOID pvMine, const char* psz);

LONG AttachDetours(VOID);

LONG DetachDetours(VOID);

class WinAPIRedirector
{
public:
	static bool Init(std::wstring srcDirPath, std::wstring redirectDirPath);
	static void Release();
	static WinAPIRedirector* GetInstance();

	~WinAPIRedirector();

private:
	WinAPIRedirector(std::wstring srcDirPath, std::wstring redirectedDirPath);

	std::wstring m_srcDirPath;
	std::wstring m_redirectedDirPath;

	struct HandleInfo
	{
		std::wstring originalPath;
		std::wstring redirectedPath;
	};

	std::map<HANDLE, HandleInfo> m_handleInfoMap;
	CRITICAL_SECTION m_handleInfoLock;

	static std::unique_ptr<WinAPIRedirector> s_winAPIRedirector;

private:

	void Lock();
	void Unlock();

};
