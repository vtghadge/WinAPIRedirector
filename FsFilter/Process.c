#include "pch.h"

LONG g_lIsProcessMonitoringEnabled = FALSE;

NTSTATUS RegisterProcessCreateCallbacks()
{
	NTSTATUS ntStatus;

	//
	// Set process create/terminate callback
	//
	ntStatus = PsSetCreateProcessNotifyRoutineEx(
		ProcessCreateCallback,
		FALSE
	);
	if (!NT_SUCCESS(ntStatus))
	{
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

	pProcessInfo = (RUNNING_PROCESS_INFO*)ExAllocatePoolWithTag(PagedPool, sizeof(RUNNING_PROCESS_INFO), MINIFILTER_RUNNING_PROCESS_INFO_TAG);
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

	bRet = SendProcessEvent(pProcessInfo);
	
EXIT_LABLE:

	if (NULL != pProcessInfo)
	{
		ExFreePoolWithTag(pProcessInfo, MINIFILTER_RUNNING_PROCESS_INFO_TAG);
	}

	return;
}

void OnProcessTerminated(
	PEPROCESS Process,
	HANDLE ProcessId
)
{
	UNREFERENCED_PARAMETER(Process);
	UNREFERENCED_PARAMETER(ProcessId);

	return;
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
