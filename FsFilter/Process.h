#pragma once

#define MINIFILTER_RUNNING_PROCESS_INFO_TAG						'iprg'

#define PROCESS_FLAG_NONE						0x0000
#define PROCESS_FLAG_SIGNED						0x0001

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

typedef struct _RUNNING_PROCESS_TABLE
{
	RTL_GENERIC_TABLE		RunningProcessTable;
	ERESOURCE				RunningProcessTableLock;
	DWORD					dwEntryCount;
	NPAGED_LOOKASIDE_LIST	RunningProcessInfoList;
	BOOLEAN					bIsTableInitialized;

}RUNNING_PROCESS_TABLE, *PRUNNING_PROCESS_TABLE;

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

NTSTATUS InitRunningProcessTable();

NTSTATUS DeInitRunningProcessTable();

RTL_GENERIC_COMPARE_RESULTS
RunningProcessCompareRoutine(
	_In_ RTL_GENERIC_TABLE  *Table,
	_In_ PVOID  FirstStruct,
	_In_ PVOID  SecondStruct
);

PVOID
RunningProcessAllocateRoutine(
	_In_ RTL_GENERIC_TABLE  *Table,
	_In_ CLONG  ByteSize
);

VOID
RunningProcessFreeRoutine(
	_In_ RTL_GENERIC_TABLE  *Table,
	_In_ PVOID  Buffer
);

NTSTATUS AddElementInRunningProcessTable(PRUNNING_PROCESS_INFO pProcessInfo);

NTSTATUS RemoveElementFromRunningProcessTable(HANDLE hProcessId);

BOOLEAN CheckIfEntryPresentInRunningProcessTable(HANDLE hProcessId);

BOOLEAN GetProcessPathFromRunningProcessTable(HANDLE hProcessId, PUNICODE_STRING pusDosProcessPath);

BOOLEAN GetProcessInfoFromRunningProcessTable(HANDLE hProcessId, RUNNING_PROCESS_INFO* pRunningProcessInfo);

NTSTATUS RemoveAllEntriesFromRunningProcessTable();

NTSTATUS GetProcessFlags(HANDLE hPID, DWORD *pdwProcessFlags);

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
