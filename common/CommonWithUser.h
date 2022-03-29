#pragma once

#define	FILTER_NAME												L"FSFILTER"
#define	INF_NAME												L"fsfilter.inf"

#define SERVER_COMM_PORT    L"\\ProcessNotificationPort"

#define MAX_FILE_PATH   1024

#define COMM_PORT_MAX_MESSAGE_BODY_SIZE (1024 * 1024) // 1MB

#define COMM_PORT_TIMEOUT -((LONGLONG)(1) * 10 * 1000 * 1000);

#define MINIFILTER_COMMPORT_MAX_CONNECTIONS			3
#define MINIFILTER_MAX_PATH							1024
#define MINIFILTER_MAX_PATH_W						1024 * sizeof(WCHAR)

typedef enum tagFILTER_MSG_ID
{
	MsgId_Undefined = 0,
	MsgId_EnableSystem,
	MsgId_DisableSystem,

	MsgId_Max

}	FILTER_MSG_ID, * P_FILTER_MSG_ID;

enum DRIVER_EVENT_ID
{
	DRIVER_PROCESS_EVENT = 1,
	DRIVER_MAX,
};

#pragma pack(push, 1)
typedef struct _DRV_MESSAGE_HEADER
{
	ULONG messageId;
	ULONG messageDataSize; // size in bytes
	char messageData[1];

} DRV_MESSAGE_HEADER, * P_DRV_MESSAGE_HEADER;

typedef struct tagDRIVER_REPLY
{
	ULONG ulResultFlags;	//	RESULT_FLAGS_XXX

}	DRIVER_REPLY, * P_DRIVER_REPLY;


typedef struct tagFILTER_REPLY_MESSAGE
{
	FILTER_REPLY_HEADER				ReplyHeader;
	DRIVER_REPLY                    Reply;

}	FILTER_REPLY_MESSAGE, * P_FILTER_REPLY_MESSAGE;

typedef struct _DRIVER_MESSAGE
{
	ULONG rawDataSize;
	char rawData[1];

} DRIVER_MESSAGE, * P_DRIVER_MESSAGE;

typedef struct _PROCESS_EVENT_INFO
{
	DWORD		dwProcessId;
	DWORD		dwParentProcessId;
	DWORD		dwProcessFlags;
	WCHAR		wszProcessPath[MAX_FILE_PATH];

} PROCESS_EVENT_INFO, * P_PROCESS_EVENT_INFO;

typedef struct tagFILTER_COMMAND_MESSAGE
{
	FILTER_MSG_ID			MsgId;					//	Command.
	ULONG					ulReserved1;
	ULONG					ulReserved2;
	UCHAR					Data[1];

}	FILTER_COMMAND_MESSAGE, * P_FILTER_COMMAND_MESSAGE;


typedef struct tagFILTER_MESSAGE
{
	FILTER_MESSAGE_HEADER			MessageHeader;
	char rawData[COMM_PORT_MAX_MESSAGE_BODY_SIZE];

}	FILTER_MESSAGE, * P_FILTER_MESSAGE;
#pragma pack(pop)
