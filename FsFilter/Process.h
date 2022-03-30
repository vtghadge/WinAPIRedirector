#pragma once

#define MINIFILTER_RUNNING_PROCESS_INFO_TAG						'iprg'

#define PROCESS_INFO_BUFFER_SIZE						65536
#define MINIFILTER_MAX_PATH				1024
#define MINIFILTER_UNICODE_STRING_MEM_TAG						'msun'

typedef struct _RUNNING_PROCESS_INFO
{
	DWORD		dwProcessId;
	DWORD		dwParentProcessId;
	DWORD		dwProcessFlags;

	UNICODE_STRING	usProcessPath;
	WCHAR wszProcessPath[MINIFILTER_MAX_PATH];

}RUNNING_PROCESS_INFO, *PRUNNING_PROCESS_INFO;

//
// Routines
//
NTSTATUS RegisterProcessCreateCallbacks();

void UnRegisterProcessCreateCallbacks();

void ProcessCreateCallback(
	PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo
);

void OnProcessCreated(
	PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO pCreateInfo
);

void OnProcessTerminated(
	PEPROCESS Process,
	HANDLE ProcessId
);

NTSTATUS AllocateUnicodeString(_In_ POOL_TYPE PoolType, _In_ ULONG ulStringSize, _Out_ PUNICODE_STRING pUniStr);
void FreeUnicodeString(_Pre_notnull_ PUNICODE_STRING pUniStr);

NTSTATUS CopyUnicodeString(
	PUNICODE_STRING pusSourceString,
	PUNICODE_STRING pusDestinationString,
	BOOLEAN bAllocateDestString
);

NTSTATUS CopyUnicodeStringWchar(
	PWCHAR  pwszSourceString,
	PUNICODE_STRING pusDestinationString,
	BOOLEAN bAllocateDestString
);
