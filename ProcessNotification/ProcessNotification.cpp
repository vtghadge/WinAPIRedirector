// ProcessNotification.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma pack()
typedef struct tagFILTER_MESSAGE_IMP
{
	FILTER_MESSAGE			Message;
	OVERLAPPED				Ovlp;

}	FILTER_MESSAGE_IMP, * P_FILTER_MESSAGE_IMP;

std::unique_ptr<ProcessNotification> ProcessNotification::s_processNotification = nullptr;

ProcessNotification* ProcessNotification::GetInstance()
{
    return s_processNotification.get();
}

void ProcessNotification::Create()
{
    if (nullptr == s_processNotification)
    {
        s_processNotification.reset(new ProcessNotification(MAX_THREAD_COUNT));
    }
}

void ProcessNotification::Release()
{
    s_processNotification.reset(nullptr);
}

ProcessNotification::ProcessNotification(ULONG ulThreadCount) :m_ulThreadCount(ulThreadCount)
{
	m_hPort = NULL;
	m_hCompletionPort = NULL;

	GetWorkingDirPathW(m_workingDir, true);
	m_infPath = m_workingDir;
	m_infPath += INF_NAME;
}

ProcessNotification::~ProcessNotification()
{
}

BOOL __stdcall ProcessNotification::CtrlCHandler(DWORD fdwCtrlType)
{
	if (fdwCtrlType == CTRL_C_EVENT || fdwCtrlType == CTRL_CLOSE_EVENT)
	{
		wprintf(L"CtrlCHandler: Quit console window event received\n");
		ProcessNotification::GetInstance()->DeinitSystem();
	}

	return FALSE;
}

bool ProcessNotification::InitSystem()
{
	DWORD dwError;
	bool boRet;

	BOOLEAN bRetVal = SetPrivilege(L"SeLoadDriverPrivilege", TRUE, &dwError);
	if (FALSE == bRetVal)
	{
		//	do not return.
	}

	//	Install driver.
	Install(m_infPath);

	//	Start driver.
	boRet = StartDriver();
	if (false == boRet)
	{
		Uninstall(m_infPath);
		return false;
	}

	boRet = InitThreadPool();
	if (false == boRet)
	{
		wprintf(L"InitSystem: InitThreadPool failed.\n");
		StopDriver();
		Uninstall(m_infPath);
		return false;
	}

	wprintf(L"InitSystem: success...\n");

	return true;
}

bool ProcessNotification::DeinitSystem()
{
	DeinitThreadPool();
	StopDriver();
	Uninstall(m_infPath);
	return false;
}

DWORD __stdcall ProcessNotification::WorkerThread(void* parameter)
{
	UNREFERENCED_PARAMETER(parameter);

	ProcessNotification::GetInstance()->MessageLoop();
	return 0;
}

bool ProcessNotification::MessageLoop()
{
	BOOL boQueued;
	ULONG_PTR key;
	DWORD dwError;
	DWORD dwOutSize;
	HRESULT hResult;
	LPOVERLAPPED pOvlp;
	ULONG ulMessageSize;
	FILTER_MESSAGE* pMessage;
	DRIVER_MESSAGE* pDriverMessage;
	FILTER_MESSAGE_IMP* pMessageInternal;
	PROCESS_EVENT_INFO* pProcessInfo;

	while (true)
	{
		boQueued = GetQueuedCompletionStatus(m_hCompletionPort, &dwOutSize, &key, &pOvlp, INFINITE);
		if (FALSE == boQueued)
		{
			dwError = GetLastError();
			wprintf(L"GetQueuedCompletionStatus failed(%u).", dwError);
			return false;
		}

		if (dwOutSize == 0 && key == NULL)
		{
			wprintf(L"MessageLoop: exiting....");
			break;
		}

		pMessageInternal = CONTAINING_RECORD(pOvlp, FILTER_MESSAGE_IMP, Ovlp);
		if (NULL == pMessageInternal)
		{
			wprintf(L"pMessageInternal is NULL");
			break;
		}
		pMessage = &pMessageInternal->Message;

		DRV_MESSAGE_HEADER* pDrvMsgHeader = (DRV_MESSAGE_HEADER*)pMessageInternal->Message.rawData;
		pDriverMessage = (DRIVER_MESSAGE*)pDrvMsgHeader->messageData;

		ulMessageSize = sizeof(pMessageInternal->Message.MessageHeader) + pDrvMsgHeader->messageDataSize;

		if (DRIVER_PROCESS_EVENT == pDrvMsgHeader->messageId)
		{
			pProcessInfo = (PROCESS_EVENT_INFO*)pDriverMessage->rawData;
			if (NULL != pProcessInfo)
			{
				wprintf(L"Process (%s) created\n", pProcessInfo->wszProcessPath);
			}
		}

		memset(&pMessageInternal->Ovlp, 0, sizeof(OVERLAPPED));

		hResult = FilterGetMessage(m_hPort, &pMessageInternal->Message.MessageHeader, FIELD_OFFSET(FILTER_MESSAGE_IMP, Ovlp), &pMessageInternal->Ovlp);
		if (hResult != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
		{
			break;
		}
	}

	return true;
}

bool ProcessNotification::InitThreadPool()
{
	HRESULT hResult = FilterConnectCommunicationPort(SERVER_COMM_PORT, 0, NULL, 0, NULL, &m_hPort);
	if (FAILED(hResult))
	{
		wprintf(L"InitThreadPool: FilterConnectCommunicationPort failed with error(%u)", HRESULT_CODE(hResult));
		return false;
	}

	m_hCompletionPort = CreateIoCompletionPort(m_hPort, NULL, 0, m_ulThreadCount);
	if (NULL == m_hCompletionPort)
	{
		wprintf(L"InitThreadPool: CreateIoCompletionPort failed with error(%u)", GetLastError());
		CloseHandle(m_hPort);
		m_hPort = NULL;
		return false;
	}

	std::vector<std::vector<BYTE>> buffers;

	for (ULONG iIndex = 0; iIndex < m_ulThreadCount; iIndex++)
	{
		HANDLE hThread = CreateThread(NULL, 0, this->WorkerThread, NULL, 0, NULL);
		if (NULL == hThread)
		{
			hResult = HRESULT_FROM_WIN32(GetLastError());
			break;
		}

		m_Threads.push_back(hThread);

		buffers.push_back(std::vector<BYTE>(sizeof(FILTER_MESSAGE_IMP), 0));
		FILTER_MESSAGE_IMP* pMessageInternal = reinterpret_cast<FILTER_MESSAGE_IMP*>(&(*buffers.rbegin())[0]);

		hResult = FilterGetMessage(m_hPort, &pMessageInternal->Message.MessageHeader, FIELD_OFFSET(FILTER_MESSAGE_IMP, Ovlp), &pMessageInternal->Ovlp);
		if (hResult != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
		{
			break;
		}
		else
		{
			hResult = S_OK;
		}
	}
	if (FAILED(hResult))
	{
		DeinitThreadPool();
		return false;
	}

	m_Buffers.swap(buffers);

	return true;
}

bool ProcessNotification::DeinitThreadPool()
{
	if (NULL == m_hCompletionPort)
	{
		return false;
	}

	PostQueuedCompletionStatus(m_hCompletionPort, 0, (DWORD)NULL, NULL);

	CloseHandle(m_hCompletionPort);

	if (m_Threads.size() > 0)
	{
		WaitForMultipleObjects((DWORD)m_Threads.size(), &m_Threads[0], TRUE, INFINITE);
		for (ULONG iIndex = 0; iIndex < m_Threads.size(); iIndex++)
		{
			CloseHandle(m_Threads[iIndex]);
		}
		m_Threads.clear();
		m_Buffers.clear();
	}

	m_hCompletionPort = NULL;

	CloseHandle(m_hPort);
	m_hPort = NULL;

	return true;
}

int wmain()
{
	ProcessNotification::Create();

	bool boRet = ProcessNotification::GetInstance()->InitSystem();
	if (false == boRet)
	{
		wprintf(L"InitThreadPool: InitSystem failed.");
		ProcessNotification::Release();
		return 0;
	}

	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ProcessNotification::GetInstance()->CtrlCHandler, TRUE))
	{
		Sleep(INFINITE);
		wprintf(L"Main thread out of sleep.");
	}

	ProcessNotification::Release();

	return 0;
}
