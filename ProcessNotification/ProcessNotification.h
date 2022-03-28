#pragma once

#define MAX_THREAD_COUNT	3

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

private:
	HANDLE m_hPort;
	HANDLE m_hCompletionPort;
	ULONG m_ulThreadCount;

	std::wstring m_workingDir;
	std::wstring m_infPath;
	std::vector<std::vector<BYTE>> m_Buffers;
	std::vector<HANDLE> m_Threads;

	static std::unique_ptr<ProcessNotification> s_processNotification;
};
