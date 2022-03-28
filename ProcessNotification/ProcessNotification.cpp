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
        s_processNotification.reset(new ProcessNotification(MAX_THREAD_COUNT, MAX_REQUEST_COUNT));
    }
}

void ProcessNotification::Release()
{
    s_processNotification.reset(nullptr);
}

ProcessNotification::ProcessNotification(ULONG ulThreadCount, ULONG ulRequestCount) :m_ulThreadCount(ulThreadCount), m_ulRequestCount(ulRequestCount)
{
	m_hThreadStopEvent = NULL;
	m_hPort = INVALID_HANDLE_VALUE;
	m_hCompletionPort = INVALID_HANDLE_VALUE;
	m_arrhThreads = new HANDLE[ulThreadCount];

	for (ULONG i = 0; i < m_ulThreadCount; i++)
	{
		m_arrhThreads[i] = NULL;
	}

	GetWorkingDirPathW(m_workingDir, true);
	m_infPath = m_workingDir;
	m_infPath += INF_NAME;
}

ProcessNotification::~ProcessNotification()
{
	for (ULONG i = 0; i < m_ulThreadCount; i++)
	{
		if (NULL != m_arrhThreads[i])
		{
			CloseHandle(m_arrhThreads[i]);
			m_arrhThreads[i] = NULL;
		}
	}

	delete[]m_arrhThreads;
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

	m_hThreadStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
	if (NULL == m_hThreadStopEvent)
	{
		return false;
	}

	//	Install driver.
	Install(m_infPath);

	//	Start driver.
	boRet = StartDriver();
	if (false == boRet)
	{
		Uninstall(m_infPath);
		CloseHandle(m_hThreadStopEvent);
		return false;
	}

	boRet = InitThreadPool();
	if (false == boRet)
	{
		wprintf(L"InitSystem: InitThreadPool failed.\n");
		StopDriver();
		Uninstall(m_infPath);
		CloseHandle(m_hThreadStopEvent);
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
	CloseHandle(m_hThreadStopEvent);
	m_hThreadStopEvent = NULL;
	return false;
}

BOOL __stdcall ProcessNotification::CtrlCHandler(DWORD fdwCtrlType)
{
	if (fdwCtrlType == CTRL_C_EVENT || fdwCtrlType == CTRL_CLOSE_EVENT)
	{
		wprintf(L"CtrlCHandler, quit console window event received");
		ProcessNotification::GetInstance()->DeinitSystem();
	}

	return FALSE;
}

DWORD __stdcall ProcessNotification::WorkerThread(void* parameter)
{
	UNREFERENCED_PARAMETER(parameter);

	ProcessNotification::GetInstance()->MessageLoop();
	return 0;
}

void ProcessNotification::SignalThreadStopEvent()
{
	SetEvent(m_hThreadStopEvent);
}

void ProcessNotification::WaitForThreadStopEvent()
{
	WaitForSingleObject(m_hThreadStopEvent, INFINITE);
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
		m_arrhThreads[iIndex] = CreateThread(NULL, 0, this->WorkerThread, NULL, CREATE_SUSPENDED, NULL);
		if (NULL == m_arrhThreads[iIndex])
		{
			if (iIndex > 0)
			{
				for (int iIttr = iIndex - 1; iIttr >= 0; iIttr--)
				{
					TerminateThread(m_arrhThreads[iIttr], 0);
				}

				DWORD dwRetVal = WaitForMultipleObjects(iIndex, m_arrhThreads, TRUE, INFINITE);
				if (WAIT_FAILED == dwRetVal)
				{
					//	Do not return.
				}

				//	Release thread handles in reverse order.
				for (int iIttr = iIndex - 1; iIttr >= 0; iIttr--)
				{
					CloseHandle(m_arrhThreads[iIttr]);
					m_arrhThreads[iIttr] = NULL;
				}
			}

			CloseHandle(m_hPort);
			m_hPort = NULL;

			CloseHandle(m_hCompletionPort);
			m_hCompletionPort = NULL;
			return false;
		}

		buffers.push_back(std::vector<BYTE>(sizeof(FILTER_MESSAGE_IMP), 0));
		FILTER_MESSAGE_IMP* pMessageInternal = reinterpret_cast<FILTER_MESSAGE_IMP*>(&(*buffers.rbegin())[0]);

		hResult = FilterGetMessage(m_hPort, &pMessageInternal->Message.MessageHeader, FIELD_OFFSET(FILTER_MESSAGE_IMP, Ovlp), &pMessageInternal->Ovlp);
		if (hResult != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
		{
			//	TODO: cleanup and return.
			free(pMessageInternal);
			return false;
		}
	}
	m_buffers.swap(buffers);

	for (ULONG iIndex = 0; iIndex < m_ulThreadCount; iIndex++)
	{
		ResumeThread(m_arrhThreads[iIndex]);
	}

	return true;
}

bool ProcessNotification::DeinitThreadPool()
{
	PostQueuedCompletionStatus(m_hCompletionPort, 0, (DWORD)NULL, NULL);

	CloseHandle(m_hCompletionPort);

	WaitForMultipleObjects(m_ulThreadCount, m_arrhThreads, TRUE, INFINITE);
	for (ULONG iIndex = 0; iIndex < m_ulThreadCount; iIndex++)
	{
		CloseHandle(m_arrhThreads[iIndex]);
		m_arrhThreads[iIndex] = NULL;
	}

	m_buffers.clear();
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
