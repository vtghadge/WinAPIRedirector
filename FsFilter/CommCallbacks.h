#pragma once

typedef struct tagPORT_CONTEXT
{
	PFLT_PORT								hClientPort;
	ULONG									ulFlags;		//	See PORT_FLAGS_XXX.
	HANDLE									hProcessId;

}	PORT_CONTEXT, * P_PORT_CONTEXT;

#define PORT_CONTEXT_TAG	'gtCP'
#define DRV_MESSAGE_TAG		'gtMD'

NTSTATUS MinifilterCommportConnect(
	_In_ PFLT_PORT pClientPort,
	_In_ PVOID pServerPortCookie,
	_In_reads_bytes_(ulcbContextSize) PVOID pConnectionContext,
	_In_ ULONG ulcbContextSize,
	_Flt_ConnectionCookie_Outptr_ PVOID *pConnectionCookie
);

VOID MinifilterCommportDisconnect(
	_In_opt_ PVOID pConnectionCookie
);

NTSTATUS MinifilterCommportMessage(
	_In_ PVOID pConnectionCookie,
	_In_reads_bytes_opt_(ulcbInputBufferSize) PVOID pInputBuffer,
	_In_ ULONG ulcbInputBufferSize,
	_Out_writes_bytes_to_opt_(ulcbOutBufferSize, *pReturnOutputBufferLen) PVOID pOutBuffer,
	_In_ ULONG ulcbOutBufferSize,
	_Out_ ULONG *pReturnOutputBufferLen
);


NTSTATUS
SendControlMessage(
	PVOID senderBuffer,
	ULONG senderBufferLength,
	PVOID replyBuffer,
	PULONG replyBufferLength,
	PLARGE_INTEGER timeout
);

NTSTATUS
SendDriverEvent(
	ULONG eventId,
	const char* rawData,
	ULONG rawDataSize,
	PVOID pReplyBuffer,
	ULONG replyLength
);

BOOLEAN SendProcessEvent(RUNNING_PROCESS_INFO* pProcessInfo);
