#include "pch.h"

NTSTATUS MinifilterCommportMessage(
	_In_ PVOID pConnectionCookie,
	_In_reads_bytes_opt_(ulcbInputBufferSize) PVOID pInputBuffer,
	_In_ ULONG ulcbInputBufferSize,
	_Out_writes_bytes_to_opt_(ulcbOutBufferSize, *pReturnOutputBufferLen) PVOID pOutBuffer,
	_In_ ULONG ulcbOutBufferSize,
	_Out_ ULONG *pReturnOutputBufferLen
)
{
	UNREFERENCED_PARAMETER(pOutBuffer);
	UNREFERENCED_PARAMETER(ulcbOutBufferSize);
	UNREFERENCED_PARAMETER(pReturnOutputBufferLen);

	PVOID pvData;
	ULONG ulcbData;
	NTSTATUS NTStatus = STATUS_NOT_IMPLEMENTED;
	FILTER_COMMAND_MESSAGE* pFilterCommandMessage;
	PORT_CONTEXT* pPortContext;

	if (NULL == pConnectionCookie)
	{
		return STATUS_INVALID_PARAMETER;
	}

	pPortContext = (PORT_CONTEXT*)pConnectionCookie;

	//
	//	Check for input buffer length.
	//
	if (ulcbInputBufferSize < sizeof(FILTER_COMMAND_MESSAGE))
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	//
	//	Typecast input parameters.
	//
	pFilterCommandMessage = (P_FILTER_COMMAND_MESSAGE)pInputBuffer;	//	TBD: We will remove pFilterCommandMessage is not required.

	//
	//	Take out the pointer to actual data and its size, it will be common for all functions.
	//
	pvData = pFilterCommandMessage->Data;
	ulcbData = (ULONG)(ulcbInputBufferSize - FIELD_OFFSET(FILTER_COMMAND_MESSAGE, Data));

	if (pFilterCommandMessage->MsgId >= MsgId_Max)
	{
		return STATUS_INVALID_PARAMETER;
	}

	switch (pFilterCommandMessage->MsgId)
	{
	case MsgId_EnableSystem:

		g_globalData.bIsMonitoringEnabled = TRUE;
		NTStatus = STATUS_SUCCESS;
		break;

	case MsgId_DisableSystem:

		g_globalData.bIsMonitoringEnabled = FALSE;
		NTStatus = STATUS_SUCCESS;
		break;

	default:
		break;
	}

	return NTStatus;
}

NTSTATUS MinifilterCommportConnect(
	_In_ PFLT_PORT pClientPort,
	_In_ PVOID pServerPortCookie,
	_In_reads_bytes_(ulcbContextSize) PVOID pConnectionContext,
	_In_ ULONG ulcbContextSize,
	_Flt_ConnectionCookie_Outptr_ PVOID *pConnectionCookie
)
{
	UNREFERENCED_PARAMETER(pServerPortCookie);
	UNREFERENCED_PARAMETER(pConnectionContext);
	UNREFERENCED_PARAMETER(ulcbContextSize);

	PORT_CONTEXT* pPortContext;

	pPortContext = (PORT_CONTEXT*)ExAllocatePoolWithTag(PagedPool, sizeof(PORT_CONTEXT), PORT_CONTEXT_TAG);
	if (NULL == pPortContext)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	g_globalData.pServicePort = pClientPort;
	pPortContext->hClientPort = pClientPort;
	pPortContext->hProcessId = PsGetCurrentProcessId();
	pPortContext->ulFlags = 0;

	*pConnectionCookie = pPortContext;

	return STATUS_SUCCESS;
}

VOID MinifilterCommportDisconnect(
	_In_opt_ PVOID pConnectionCookie
)
{
	PORT_CONTEXT* pPortContext;

	if (NULL == pConnectionCookie)
	{
		return;
	}

	pPortContext = (PORT_CONTEXT*)pConnectionCookie;

	//
	//	Close client port if not already closed.
	//

	if (g_globalData.pServicePort == pPortContext->hClientPort)
	{
		FltCloseClientPort(g_globalData.pFltFilterHandle, &g_globalData.pServicePort);
		g_globalData.pServicePort = NULL;
		pPortContext->hClientPort = NULL;
	}
	else
	{
		FltCloseClientPort(g_globalData.pFltFilterHandle, &pPortContext->hClientPort);
		pPortContext->hClientPort = NULL;
	}

	ExFreePoolWithTag(pPortContext, PORT_CONTEXT_TAG);
}


NTSTATUS
SendControlMessage(
	PVOID senderBuffer,
	ULONG senderBufferLength,
	PVOID replyBuffer,
	PULONG replyBufferLength,
	PLARGE_INTEGER timeout
)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (NULL == g_globalData.pServicePort)
	{
		return STATUS_INVALID_PORT_HANDLE;
	}

	__try
	{
		Status = FltSendMessage(g_globalData.pFltFilterHandle, &g_globalData.pServicePort, senderBuffer, senderBufferLength, replyBuffer, replyBufferLength, timeout);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return STATUS_INVALID_PORT_HANDLE;
	}

	return Status;
}

NTSTATUS
SendDriverEvent(
	ULONG eventId,
	const char* rawData,
	ULONG rawDataSize,
	PVOID pReplyBuffer,
	ULONG replyLength
)
{
	ULONG msgDataSize;
	ULONG msgSize;
	PUCHAR buffer;
	DRV_MESSAGE_HEADER* msgHeader;
	DRIVER_MESSAGE* msgData;
	NTSTATUS NTStatus;
	LARGE_INTEGER TimeOut;

	msgDataSize = sizeof(DRIVER_MESSAGE) + rawDataSize;

	if (msgDataSize > COMM_PORT_MAX_MESSAGE_BODY_SIZE)
	{
		return STATUS_BUFFER_OVERFLOW;
	}

	msgSize = msgDataSize + sizeof(DRV_MESSAGE_HEADER);

	buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, msgSize, DRV_MESSAGE_TAG);
	if (NULL == buffer)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	msgHeader = (DRV_MESSAGE_HEADER*)buffer;

	msgHeader->messageId = eventId;
	msgHeader->messageDataSize = msgDataSize;

	msgData = (DRIVER_MESSAGE*)msgHeader->messageData;

	msgData->rawDataSize = rawDataSize;

	RtlCopyMemory(msgData->rawData, rawData, msgData->rawDataSize);

	TimeOut.QuadPart = COMM_PORT_TIMEOUT;

	NTStatus = SendControlMessage(buffer, msgSize, pReplyBuffer, &replyLength, &TimeOut);

	ExFreePoolWithTag(buffer, DRV_MESSAGE_TAG);

	return NTStatus;
}

BOOLEAN SendProcessEvent(RUNNING_PROCESS_INFO *pProcessInfo)
{
	PROCESS_EVENT_INFO EventInfo = { 0 };
	UNICODE_STRING usPath;
	NTSTATUS ntStatus;

	if (NULL == pProcessInfo)
	{
		return FALSE;
	}

	//
	//	Init process path.
	//
	RtlInitEmptyUnicodeString(
		&usPath,
		EventInfo.wszProcessPath,
		sizeof(EventInfo.wszProcessPath) - sizeof(WCHAR)
	);

	ntStatus = CopyUnicodeString(&pProcessInfo->usProcessPath, &usPath, FALSE);
	if (!NT_SUCCESS(ntStatus))
	{
		return FALSE;
	}

	EventInfo.dwProcessId = pProcessInfo->dwProcessId;
	EventInfo.dwParentProcessId = pProcessInfo->dwParentProcessId;
	EventInfo.dwProcessFlags = pProcessInfo->dwProcessFlags;

	ntStatus = SendDriverEvent(DRIVER_PROCESS_EVENT, (const char*)&EventInfo, sizeof(EventInfo), NULL, 0);
	if (!NT_SUCCESS(ntStatus))
	{
		return FALSE;
	}

	return TRUE;
}
