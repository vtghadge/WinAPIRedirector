#pragma once

#define RTL_USE_AVL_TABLES 0

#include <initguid.h>
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <Ntstrsafe.h>
#include <ntintsafe.h>
#include <limits.h> 

#include "Process.h"
#include "CommCallbacks.h"
#include "..\common\CommonWithUser.h"

#define EMPTY_UNICODE_STRING			{0, 0, NULL}

#define EXIT_ON_NTSTATUS_FAILURE(ntStatus)							\
{																	\
	if(!NT_SUCCESS(ntStatus))										\
	{																\
		goto EXIT_LABLE;											\
	}																\
}

#define EXIT_ON_ALLOC_FAILURE(ptr, ntStatus)						\
{																	\
	if(NULL == ptr)													\
	{                                                               \
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;					\
		goto EXIT_LABLE;											\
	}																\
}

#define EXIT_ON_LOOKASIDE_ALLOC_FAILURE(head, ntStatus)				\
{																	\
	if(FALSE == head.bIsLookAsideListInitialized)					\
	{																\
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;					\
		goto EXIT_LABLE;											\
	}																\
}

#define EXIT_ON_NULL_POINTER(ptr, ntStatus)							\
{																	\
	if(NULL == ptr)													\
	{																\
		ntStatus = STATUS_UNSUCCESSFUL;								\
		goto EXIT_LABLE;											\
	}																\
}

typedef struct _MINIFILTER_GLOBAL_DATA
{
	PFLT_FILTER pFltFilterHandle;

	PFLT_PORT	pServerPort;
	PFLT_PORT	pServicePort;

	BOOLEAN bIsMonitoringEnabled;

	PDRIVER_OBJECT pDriverObject;

}MINIFILTER_GLOBAL_DATA, * PMINIFILTER_GLOBAL_DATA;

extern MINIFILTER_GLOBAL_DATA g_globalData;
