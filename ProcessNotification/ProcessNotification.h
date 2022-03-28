#pragma once

#define MAX_THREAD_COUNT	1
#define INJECTOR_DLL_NAME	L"DllWinAPIRedirector.dll"

#define FILE_NAME_CONFIG_W          L"Config.ini"
#define	MAX_INI_SECTION_CHARS		32767
#define CONFIG_SECTION_NAME_W       L"ProcessName"

class ProcessNotification
{
public:
	static ProcessNotification* GetInstance();
	static void Create();
	static void Release();

	~ProcessNotification();

	static BOOL WINAPI CtrlCHandler(DWORD fdwCtrlType);

	bool InitSystem();
	bool DeinitSystem();

	static DWORD WINAPI WorkerThread(void* parameter);
	bool MessageLoop();

private:
	ProcessNotification(ULONG ulThreadCount);

	bool InitThreadPool();
	bool DeinitThreadPool();

	bool InjectInProcess(std::wstring path, ULONG ulProcessId);
	bool IsProcessPresentInInjectionList(std::wstring path);
	bool InjectDLL(ULONG ulProcessId);

	//	Get process names from config.ini file.
	bool InitProcessInjectionList();
	bool GetProcessNameData(WCHAR** ppwszProcessNameData);
	void ReleaseProcessNameData(WCHAR* pwszProcessNameData);

private:
	HANDLE m_hPort;
	HANDLE m_hCompletionPort;
	ULONG m_ulThreadCount;

	std::wstring m_workingDir;
	std::wstring m_infPath;
	std::vector<std::vector<BYTE>> m_Buffers;
	std::vector<HANDLE> m_Threads;

	std::list<std::wstring> m_processInjectionList;

	static std::unique_ptr<ProcessNotification> s_processNotification;
};
