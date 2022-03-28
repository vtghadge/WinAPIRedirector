#pragma once

#define MAX_THREAD_COUNT	1
#define MAX_REQUEST_COUNT	1

class ProcessNotification
{
public:
	static ProcessNotification* GetInstance();
	static void Create();
	static void Release();

	~ProcessNotification();

	bool InitSystem();
	bool DeinitSystem();
	bool MessageLoop();

	static BOOL WINAPI CtrlCHandler(DWORD fdwCtrlType);
	static DWORD WINAPI WorkerThread(void* parameter);

	void SignalThreadStopEvent();
	void WaitForThreadStopEvent();

private:
	ProcessNotification(ULONG ulThreadCount, ULONG ulRequestCount);

	bool InitThreadPool();
	bool DeinitThreadPool();


private:
	HANDLE m_hPort;
	HANDLE m_hCompletionPort;
	ULONG m_ulThreadCount;
	ULONG m_ulRequestCount;
	HANDLE *m_arrhThreads;

	HANDLE m_hThreadStopEvent;

	std::wstring m_workingDir;
	std::wstring m_infPath;
	std::vector<std::vector<BYTE>> m_buffers;

	static std::unique_ptr<ProcessNotification> s_processNotification;
};
