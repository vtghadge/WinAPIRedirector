#include "pch.h"

RUNNING_PROCESS_TABLE g_runningProcessTable;

LONG g_lIsProcessMonitoringEnabled = FALSE;
LONG g_lIsRunningProcessTableInitialized = FALSE;

NTSTATUS RegisterProcessCreateCallbacks()
{
	NTSTATUS ntStatus;
	// 
	// Init running process tree
	//
	ntStatus = InitRunningProcessTable();
	if (!NT_SUCCESS(ntStatus))
	{
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Set the global variable that the running process tree is initialized
	//
	InterlockedExchange(&g_lIsRunningProcessTableInitialized, TRUE);

	//
	// Set process create/terminate callback
	//
	ntStatus = PsSetCreateProcessNotifyRoutineEx(
		ProcessCreateCallback,
		FALSE
	);
	if (!NT_SUCCESS(ntStatus))
	{
		DeInitRunningProcessTable();
		return ntStatus;
	}

	//
	// Set the global variable that the process monitoring is enabled
	//
	InterlockedExchange(&g_lIsProcessMonitoringEnabled, TRUE);

	return STATUS_SUCCESS;
}

void UnRegisterProcessCreateCallbacks()
{
	NTSTATUS ntStatus;

	//
	// Check if the process monitoring is enabled or not
	//
	LONG lIsEnabled = InterlockedCompareExchange(&g_lIsProcessMonitoringEnabled, 0, 0);
	if (TRUE == lIsEnabled)
	{
		//
		// Remove process create/terminate callback
		//
		ntStatus = PsSetCreateProcessNotifyRoutineEx(
			ProcessCreateCallback,
			TRUE
		);
		if (!NT_SUCCESS(ntStatus))
		{
#if DBG
			DbgPrint("StopProcessMonitoring() Unregistering the process create/terminate callback failed\n");
#endif
		}

		//
		// Reset process monitoring state
		//
		InterlockedExchange(&g_lIsProcessMonitoringEnabled, FALSE);
	}

	//
	// Check if the running process table is initialized or not
	//
	lIsEnabled = InterlockedCompareExchange(&g_lIsRunningProcessTableInitialized, 0, 0);
	if (TRUE == lIsEnabled)
	{
		//
		// Deinit running process table
		//
		ntStatus = DeInitRunningProcessTable();
		if (!NT_SUCCESS(ntStatus))
		{
#if DBG
			DbgPrint("StopProcessMonitoring() Deinitialization of the process table failed\n");
#endif
		}

		//
		// Reset process monitoring state
		//
		InterlockedExchange(&g_lIsRunningProcessTableInitialized, FALSE);
	}

	return;
}

void ProcessCreateCallback(
	PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
	if (CreateInfo)
	{
		OnProcessCreated(Process, ProcessId, CreateInfo);
	}
	else
	{
		OnProcessTerminated(Process, ProcessId);
	}

	return;
}

void OnProcessCreated(
	PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO pCreateInfo
)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	RUNNING_PROCESS_INFO *pProcessInfo = NULL;
	BOOLEAN bRet;

	UNREFERENCED_PARAMETER(Process);

	if (!g_globalData.bIsMonitoringEnabled)
	{
		return;
	}

	pProcessInfo = (RUNNING_PROCESS_INFO*)ExAllocateFromNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList);
	if (NULL == pProcessInfo)
	{
		goto EXIT_LABLE;
	}
	RtlZeroMemory(pProcessInfo, sizeof(RUNNING_PROCESS_INFO));

	pProcessInfo->dwProcessId = (DWORD)(ULONG_PTR)ProcessId;
	pProcessInfo->dwParentProcessId = (DWORD)(ULONG_PTR)pCreateInfo->ParentProcessId;

	//
	//	Init process path.
	//
	RtlInitEmptyUnicodeString(
		&pProcessInfo->usProcessPath,
		pProcessInfo->wszProcessPath,
		sizeof(pProcessInfo->wszProcessPath) - sizeof(WCHAR)
	);
	ntStatus = CopyUnicodeString((PUNICODE_STRING)pCreateInfo->ImageFileName, &pProcessInfo->usProcessPath, FALSE);
	if (!NT_SUCCESS(ntStatus))
	{
		goto EXIT_LABLE;
	}

	//
	// Add process event to the running process table
	//
	ntStatus = AddElementInRunningProcessTable(pProcessInfo);
	if (!NT_SUCCESS(ntStatus))
	{
		goto EXIT_LABLE;
	}

	bRet = SendProcessEvent(pProcessInfo);
	
EXIT_LABLE:

	if (NULL != pProcessInfo)
	{
		ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pProcessInfo);
	}

	return;
}

void OnProcessTerminated(
	PEPROCESS Process,
	HANDLE ProcessId
)
{
	NTSTATUS ntStatus;

	UNREFERENCED_PARAMETER(Process);

	if (!g_globalData.bIsMonitoringEnabled)
	{
		return;
	}

	ntStatus = RemoveElementFromRunningProcessTable(ProcessId);
	if (!NT_SUCCESS(ntStatus))
	{
	}

	return;
}

NTSTATUS InitRunningProcessTable()
{
	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Initialize tree lock
	//
	ExInitializeResourceLite(&g_runningProcessTable.RunningProcessTableLock);

	//
	// Initialize process table
	//
	RtlInitializeGenericTable(
		&g_runningProcessTable.RunningProcessTable,
		RunningProcessCompareRoutine,
		RunningProcessAllocateRoutine,
		RunningProcessFreeRoutine,
		NULL
	);

	//
	// Initialize lookaside list for running process info
	//
	ExInitializeNPagedLookasideList(
		&g_runningProcessTable.RunningProcessInfoList,
		NULL,
		NULL,
		0,
		sizeof(RUNNING_PROCESS_INFO),
		MINIFILTER_RUNNING_PROCESS_INFO_TAG,
		0
	);

	g_runningProcessTable.dwEntryCount = 0;
	g_runningProcessTable.bIsTableInitialized = TRUE;

	return STATUS_SUCCESS;
}

NTSTATUS DeInitRunningProcessTable()
{
	//
	// Remove all entries form the tree
	// 
	RemoveAllEntriesFromRunningProcessTable();

	//
	// Delete lookaside list for running process info
	//
	ExDeleteNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList);

	//
	// Delete resource
	// 
	ExDeleteResourceLite(&g_runningProcessTable.RunningProcessTableLock);

	return STATUS_SUCCESS;
}

RTL_GENERIC_COMPARE_RESULTS
RunningProcessCompareRoutine(
	_In_ RTL_GENERIC_TABLE  *Table,
	_In_ PVOID  FirstStruct,
	_In_ PVOID  SecondStruct
)
{
	RUNNING_PROCESS_INFO *pFirstRunningProcInfo = NULL;
	RUNNING_PROCESS_INFO *pSecondRunningProcInfo = NULL;

	UNREFERENCED_PARAMETER(Table);

	pFirstRunningProcInfo = (RUNNING_PROCESS_INFO *)FirstStruct;
	pSecondRunningProcInfo = (RUNNING_PROCESS_INFO *)SecondStruct;

	if (pFirstRunningProcInfo->dwProcessId > pSecondRunningProcInfo->dwProcessId)
	{
		return GenericGreaterThan;
	}

	if (pFirstRunningProcInfo->dwProcessId < pSecondRunningProcInfo->dwProcessId)
	{
		return GenericLessThan;
	}

	return GenericEqual;
}

PVOID
RunningProcessAllocateRoutine(
	_In_ RTL_GENERIC_TABLE  *Table,
	_In_ CLONG  ByteSize
)
{
	PVOID pRunningProcessInfo = NULL;

	UNREFERENCED_PARAMETER(Table);

	pRunningProcessInfo = (PVOID)ExAllocatePoolWithTag(NonPagedPool, ByteSize, MINIFILTER_RUNNING_PROCESS_INFO_TAG);
	if (NULL == pRunningProcessInfo)
	{
#if DBG
		DbgPrint("RunningProcessAllocateRoutine:: pRunningProcessInfo:: Not enough memory\n");
#endif
	}
	RtlZeroMemory(pRunningProcessInfo, ByteSize);

	return pRunningProcessInfo;
}

VOID
RunningProcessFreeRoutine(
	_In_ RTL_GENERIC_TABLE  *Table,
	_In_ PVOID  Buffer
)
{
	UNREFERENCED_PARAMETER(Table);

	if (NULL != Buffer)
	{
		ExFreePoolWithTag(Buffer, MINIFILTER_RUNNING_PROCESS_INFO_TAG);
	}
}

NTSTATUS AddElementInRunningProcessTable(PRUNNING_PROCESS_INFO pProcessInfo)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	BOOLEAN bIsNewElementInserted = FALSE;
	RUNNING_PROCESS_INFO *pRunningProcessInfo = NULL;
	RUNNING_PROCESS_INFO *pNewInsertedElement = NULL;

	if (NULL == pProcessInfo)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	pRunningProcessInfo = (RUNNING_PROCESS_INFO *)ExAllocateFromNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList);
	if (NULL == pRunningProcessInfo)
	{
		goto EXIT_LABLE;
	}
	RtlZeroMemory(pRunningProcessInfo, sizeof(RUNNING_PROCESS_INFO));

	pRunningProcessInfo->dwProcessId = pProcessInfo->dwProcessId;
	pRunningProcessInfo->dwProcessFlags = pProcessInfo->dwProcessFlags;
	pRunningProcessInfo->dwParentProcessId = pProcessInfo->dwParentProcessId;
	
	//
	//	Init process path.
	//
	RtlInitEmptyUnicodeString(
		&pRunningProcessInfo->usProcessPath,
		pRunningProcessInfo->wszProcessPath,
		sizeof(pRunningProcessInfo->wszProcessPath) - sizeof(WCHAR)
	);
	ntStatus = CopyUnicodeString(&pProcessInfo->usProcessPath, &pRunningProcessInfo->usProcessPath, FALSE);
	if (!NT_SUCCESS(ntStatus))
	{
		goto EXIT_LABLE;
	}
	
	FltAcquireResourceExclusive(&g_runningProcessTable.RunningProcessTableLock);

	pNewInsertedElement = (RUNNING_PROCESS_INFO *)RtlInsertElementGenericTable(
		&g_runningProcessTable.RunningProcessTable,
		pRunningProcessInfo,
		sizeof(RUNNING_PROCESS_INFO),
		&bIsNewElementInserted
	);
	if (NULL == pNewInsertedElement)
	{
		FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

		ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);

		return STATUS_UNSUCCESSFUL;
	}

	g_runningProcessTable.dwEntryCount++;

	FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

	ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);

EXIT_LABLE:

	if (!NT_SUCCESS(ntStatus))
	{
		ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);
		
		return ntStatus;
	}

	return STATUS_SUCCESS;
}

NTSTATUS RemoveElementFromRunningProcessTable(HANDLE hProcessId)
{
	BOOLEAN bIsElementRemoved = FALSE;
	RUNNING_PROCESS_INFO *pRunningProcessInfo = NULL;

	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Allocate memory for running process info from lookaside list
	//
	pRunningProcessInfo = (RUNNING_PROCESS_INFO *)ExAllocateFromNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList);
	if (NULL == pRunningProcessInfo)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(pRunningProcessInfo, sizeof(RUNNING_PROCESS_INFO));

	//
	// Fill running process info
	//
	pRunningProcessInfo->dwProcessId = (DWORD)(ULONG_PTR)hProcessId;

	//
	// Acquire lock
	//
	FltAcquireResourceExclusive(&g_runningProcessTable.RunningProcessTableLock);

	//
	// delete element from the running process table
	//
	bIsElementRemoved = RtlDeleteElementGenericTable(
		&g_runningProcessTable.RunningProcessTable,
		pRunningProcessInfo
	);
	if (FALSE == bIsElementRemoved)
	{
		//
		// Release lock
		//
		FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

		//
		// Free running process info memory
		//
		ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);

		return STATUS_UNSUCCESSFUL;
	}

	//
	// Decrement entry count
	//
	--g_runningProcessTable.dwEntryCount;

	//
	// Release lock
	//
	FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

	//
	// Free running process info memory
	//
	ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);

	return STATUS_SUCCESS;
}

BOOLEAN CheckIfEntryPresentInRunningProcessTable(HANDLE hProcessId)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PRUNNING_PROCESS_INFO pRunningProcessInfo = NULL;
	PRUNNING_PROCESS_INFO pReturnedRunningProcessInfo = NULL;

	if (NULL == hProcessId)
	{
		return FALSE;
	}

	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return FALSE;
	}

	pRunningProcessInfo = (PRUNNING_PROCESS_INFO)ExAllocateFromNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList);
	EXIT_ON_ALLOC_FAILURE(pRunningProcessInfo, ntStatus);

	RtlZeroMemory(pRunningProcessInfo, sizeof(RUNNING_PROCESS_INFO));

	pRunningProcessInfo->dwProcessId = (DWORD)(ULONG_PTR)hProcessId;

	FltAcquireResourceShared(&g_runningProcessTable.RunningProcessTableLock);

	pReturnedRunningProcessInfo = (RUNNING_PROCESS_INFO *)RtlLookupElementGenericTable(
		&g_runningProcessTable.RunningProcessTable,
		pRunningProcessInfo
	);
	if (NULL == pReturnedRunningProcessInfo)
	{
		FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

		if (NULL != pRunningProcessInfo)
		{
			ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);
			pRunningProcessInfo = NULL;
		}

		return FALSE;
	}

	FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

	ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);
	pReturnedRunningProcessInfo = NULL;

EXIT_LABLE:

	if (!NT_SUCCESS(ntStatus))
	{
		if (NULL != pRunningProcessInfo)
		{
			ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);
			pRunningProcessInfo = NULL;
		}

		return FALSE;
	}

	return TRUE;
}

BOOLEAN GetProcessPathFromRunningProcessTable(HANDLE hProcessId, PUNICODE_STRING pusDosProcessPath)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PRUNNING_PROCESS_INFO pRunningProcessInfo = NULL;
	PRUNNING_PROCESS_INFO pReturnedRunningProcessInfo = NULL;

	if (NULL == hProcessId)
	{
		return FALSE;
	}

	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return FALSE;
	}

	pRunningProcessInfo = (PRUNNING_PROCESS_INFO)ExAllocateFromNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList);
	EXIT_ON_ALLOC_FAILURE(pRunningProcessInfo, ntStatus);

	RtlZeroMemory(pRunningProcessInfo, sizeof(RUNNING_PROCESS_INFO));

	pRunningProcessInfo->dwProcessId = (DWORD)(ULONG_PTR)hProcessId;

	FltAcquireResourceShared(&g_runningProcessTable.RunningProcessTableLock);

	pReturnedRunningProcessInfo = (RUNNING_PROCESS_INFO*)RtlLookupElementGenericTable(
		&g_runningProcessTable.RunningProcessTable,
		pRunningProcessInfo
	);
	if (NULL == pReturnedRunningProcessInfo)
	{
		FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

		if (NULL != pRunningProcessInfo)
		{
			ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);
			pRunningProcessInfo = NULL;
		}

		return FALSE;
	}

	ntStatus = CopyUnicodeStringWchar(pReturnedRunningProcessInfo->wszProcessPath, pusDosProcessPath, TRUE);
	if (!NT_SUCCESS(ntStatus))
	{
		FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

		ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);
		pReturnedRunningProcessInfo = NULL;

		return FALSE;
	}

	FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

	ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);
	pReturnedRunningProcessInfo = NULL;

EXIT_LABLE:

	if (!NT_SUCCESS(ntStatus))
	{
		if (NULL != pRunningProcessInfo)
		{
			ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);
			pRunningProcessInfo = NULL;
		}

		return FALSE;
	}

	return TRUE;
}

BOOLEAN GetProcessInfoFromRunningProcessTable(HANDLE hProcessId, RUNNING_PROCESS_INFO *pRunningProcessInfo)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PRUNNING_PROCESS_INFO pTempRunningProcessInfo = NULL;
	PRUNNING_PROCESS_INFO pReturnedRunningProcessInfo = NULL;

	if (NULL == hProcessId || NULL == pRunningProcessInfo)
	{
		return FALSE;
	}

	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return FALSE;
	}

	pTempRunningProcessInfo = (PRUNNING_PROCESS_INFO)ExAllocateFromNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList);
	EXIT_ON_ALLOC_FAILURE(pTempRunningProcessInfo, ntStatus);

	RtlZeroMemory(pTempRunningProcessInfo, sizeof(RUNNING_PROCESS_INFO));

	pTempRunningProcessInfo->dwProcessId = (DWORD)(ULONG_PTR)hProcessId;

	FltAcquireResourceShared(&g_runningProcessTable.RunningProcessTableLock);

	pReturnedRunningProcessInfo = (RUNNING_PROCESS_INFO*)RtlLookupElementGenericTable(
		&g_runningProcessTable.RunningProcessTable,
		pTempRunningProcessInfo
	);
	if (NULL == pReturnedRunningProcessInfo)
	{
		FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

		if (NULL != pTempRunningProcessInfo)
		{
			ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pTempRunningProcessInfo);
			pTempRunningProcessInfo = NULL;
		}

		return FALSE;
	}
	RtlInitUnicodeString(&pReturnedRunningProcessInfo->usProcessPath, pReturnedRunningProcessInfo->wszProcessPath);

	pRunningProcessInfo->dwParentProcessId = pReturnedRunningProcessInfo->dwParentProcessId;
	pRunningProcessInfo->dwProcessId = pReturnedRunningProcessInfo->dwProcessId;
	pRunningProcessInfo->dwProcessFlags = pReturnedRunningProcessInfo->dwProcessFlags;

	//
	//	Init process path.
	//
	RtlInitEmptyUnicodeString(
		&pRunningProcessInfo->usProcessPath,
		pRunningProcessInfo->wszProcessPath,
		sizeof(pRunningProcessInfo->wszProcessPath) - sizeof(WCHAR)
	);
	ntStatus = CopyUnicodeString(&pReturnedRunningProcessInfo->usProcessPath, &pRunningProcessInfo->usProcessPath, FALSE);
	if (!NT_SUCCESS(ntStatus))
	{
		FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

		ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pTempRunningProcessInfo);
		pReturnedRunningProcessInfo = NULL;

		return FALSE;
	}

	FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

	ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pTempRunningProcessInfo);
	pReturnedRunningProcessInfo = NULL;

EXIT_LABLE:

	if (!NT_SUCCESS(ntStatus))
	{
		if (NULL != pTempRunningProcessInfo)
		{
			ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pTempRunningProcessInfo);
			pTempRunningProcessInfo = NULL;
		}

		return FALSE;
	}

	return TRUE;
}

NTSTATUS RemoveAllEntriesFromRunningProcessTable()
{
	RUNNING_PROCESS_INFO *pRunningProcessInfo = NULL;

	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Acquire lock
	//
	FltAcquireResourceExclusive(&g_runningProcessTable.RunningProcessTableLock);

	// 
	// Remove all elements
	//
	pRunningProcessInfo = (RUNNING_PROCESS_INFO *)RtlGetElementGenericTable(&g_runningProcessTable.RunningProcessTable, 0);
	while (NULL != pRunningProcessInfo)
	{
		//
		// Delete one element
		//
		RtlDeleteElementGenericTable(&g_runningProcessTable.RunningProcessTable, pRunningProcessInfo);

		//
		// Get next element
		//
		pRunningProcessInfo = (RUNNING_PROCESS_INFO*)RtlGetElementGenericTable(&g_runningProcessTable.RunningProcessTable, 0);

		//
		// Decrement count
		//
		--g_runningProcessTable.dwEntryCount;
	}

	//
	// Release lock
	//
	FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);

	return STATUS_SUCCESS;
}

NTSTATUS GetProcessFlags(HANDLE hPID, DWORD* pdwProcessFlags)
{
	RUNNING_PROCESS_INFO *pRunningProcessInfo = NULL;
	PRUNNING_PROCESS_INFO pReturnedRunningProcessInfo = NULL;

	if (NULL == hPID || NULL == pdwProcessFlags)
	{
		return STATUS_INVALID_PARAMETER;
	}
	*pdwProcessFlags = PROCESS_FLAG_NONE;

	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	pRunningProcessInfo = (RUNNING_PROCESS_INFO*)ExAllocateFromNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList);
	if (NULL == pRunningProcessInfo)
	{
		return STATUS_UNSUCCESSFUL;
	}
	RtlZeroMemory(pRunningProcessInfo, sizeof(RUNNING_PROCESS_INFO));

	pRunningProcessInfo->dwProcessId = (DWORD)(ULONG_PTR)hPID;

	FltAcquireResourceShared(&g_runningProcessTable.RunningProcessTableLock);

	pReturnedRunningProcessInfo = (RUNNING_PROCESS_INFO*)RtlLookupElementGenericTable(
		&g_runningProcessTable.RunningProcessTable,
		pRunningProcessInfo
	);
	if (NULL == pReturnedRunningProcessInfo)
	{
		FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);
		ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);
		return STATUS_SUCCESS;
	}

	*pdwProcessFlags = pReturnedRunningProcessInfo->dwProcessFlags;

	FltReleaseResource(&g_runningProcessTable.RunningProcessTableLock);
	ExFreeToNPagedLookasideList(&g_runningProcessTable.RunningProcessInfoList, pRunningProcessInfo);

	return STATUS_SUCCESS;
}

NTSTATUS AllocateUnicodeString(_In_ POOL_TYPE PoolType, _In_ ULONG ulStringSize, _Out_ PUNICODE_STRING pUniStr)
{
	pUniStr->Buffer = ExAllocatePoolWithTag(PoolType, (USHORT)ulStringSize, MINIFILTER_UNICODE_STRING_MEM_TAG);
	if (NULL == pUniStr->Buffer)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(pUniStr->Buffer, ulStringSize);

	pUniStr->Length = 0;
	pUniStr->MaximumLength = (USHORT)ulStringSize;

	return STATUS_SUCCESS;
}

void FreeUnicodeString(_Pre_notnull_ PUNICODE_STRING pUniStr)
{
	if (NULL == pUniStr)
	{
		return;
	}

	if (NULL != pUniStr->Buffer && 0 != pUniStr->MaximumLength)
	{
		ExFreePoolWithTag(pUniStr->Buffer, MINIFILTER_UNICODE_STRING_MEM_TAG);

		pUniStr->Buffer = NULL;
		pUniStr->Length = 0;
		pUniStr->MaximumLength = 0;
	}

	return;
}

NTSTATUS CopyUnicodeString(
	PUNICODE_STRING pusSourceString,
	PUNICODE_STRING pusDestinationString,
	BOOLEAN bAllocateDestString
)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (
		NULL == pusSourceString ||
		NULL == pusSourceString->Buffer ||
		0 == pusSourceString->Length
		)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (FALSE == bAllocateDestString)
	{
		if (
			NULL == pusDestinationString ||
			NULL == pusDestinationString->Buffer
			)
		{
			return STATUS_INVALID_PARAMETER;
		}
	}

	if (TRUE == bAllocateDestString)
	{
		ntStatus = AllocateUnicodeString(NonPagedPool, pusSourceString->Length + sizeof(WCHAR), pusDestinationString);
		EXIT_ON_NTSTATUS_FAILURE(ntStatus);
	}

	if (pusSourceString->Length > pusDestinationString->MaximumLength)
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	RtlCopyUnicodeString(pusDestinationString, pusSourceString);

	pusDestinationString->Buffer[pusDestinationString->Length / sizeof(WCHAR)] = L'\0';

EXIT_LABLE:

	if (!NT_SUCCESS(ntStatus))
	{
		if (TRUE == bAllocateDestString)
		{
			FreeUnicodeString(pusDestinationString);
		}

		return ntStatus;
	}

	return STATUS_SUCCESS;
}

NTSTATUS CopyUnicodeStringWchar(
	PWCHAR  pwszSourceString,
	PUNICODE_STRING pusDestinationString,
	BOOLEAN bAllocateDestString
)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	UNICODE_STRING usSourceString = EMPTY_UNICODE_STRING;

	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (NULL == pwszSourceString)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (FALSE == bAllocateDestString)
	{
		if (
			NULL == pusDestinationString ||
			NULL == pusDestinationString->Buffer
			)
		{
			return STATUS_INVALID_PARAMETER;
		}
	}

	RtlInitUnicodeString(&usSourceString, pwszSourceString);

	if (TRUE == bAllocateDestString)
	{
		ntStatus = AllocateUnicodeString(NonPagedPool, usSourceString.Length + sizeof(WCHAR), pusDestinationString);
		EXIT_ON_NTSTATUS_FAILURE(ntStatus);
	}

	if (usSourceString.Length > pusDestinationString->MaximumLength)
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	RtlCopyUnicodeString(pusDestinationString, &usSourceString);

	pusDestinationString->Buffer[pusDestinationString->Length / sizeof(WCHAR)] = L'\0';

EXIT_LABLE:

	if (!NT_SUCCESS(ntStatus))
	{
		if (TRUE == bAllocateDestString)
		{
			FreeUnicodeString(pusDestinationString);
		}

		return ntStatus;
	}

	return STATUS_SUCCESS;
}
