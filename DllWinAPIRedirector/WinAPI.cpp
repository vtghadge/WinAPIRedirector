#include "pch.h"

int g_iDetach = 0;
#define ATTACH(x)       DetAttach(&(PVOID&)Real_##x,Mine_##x,#x)
#define DETACH(x)       DetDetach(&(PVOID&)Real_##x,Mine_##x,#x)

HANDLE
(WINAPI *Real_CreateFileW)(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
) = CreateFileW;

HANDLE
(WINAPI *Real_CreateFileA)(
    _In_ LPCSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
) = CreateFileA;

BOOL
(WINAPI *Real_CloseHandle)(
    _In_ _Post_ptr_invalid_ HANDLE hObject
) = CloseHandle;

BOOL
(WINAPI *Real_CopyFileA)(
    _In_ LPCSTR lpExistingFileName,
    _In_ LPCSTR lpNewFileName,
    _In_ BOOL bFailIfExists
)= CopyFileA;

BOOL
(WINAPI *Real_CopyFileW)(
    _In_ LPCWSTR lpExistingFileName,
    _In_ LPCWSTR lpNewFileName,
    _In_ BOOL bFailIfExists
)= CopyFileW;

BOOL
(WINAPI *Real_CopyFileExA)(
    _In_        LPCSTR lpExistingFileName,
    _In_        LPCSTR lpNewFileName,
    _In_opt_    LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_    LPVOID lpData,
    _When_(pbCancel != NULL, _Pre_satisfies_(*pbCancel == FALSE))
    _Inout_opt_ LPBOOL pbCancel,
    _In_        DWORD dwCopyFlags
)= CopyFileExA;

BOOL
(WINAPI *Real_CopyFileExW)(
    _In_        LPCWSTR lpExistingFileName,
    _In_        LPCWSTR lpNewFileName,
    _In_opt_    LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_    LPVOID lpData,
    _When_(pbCancel != NULL, _Pre_satisfies_(*pbCancel == FALSE))
    _Inout_opt_ LPBOOL pbCancel,
    _In_        DWORD dwCopyFlags
)= CopyFileExW;

BOOL
(WINAPI *Real_DeleteFileA)(
    _In_ LPCSTR lpFileName
)= DeleteFileA;

BOOL
(WINAPI *Real_DeleteFileW)(
    _In_ LPCWSTR lpFileName
)= DeleteFileW;

BOOL
(WINAPI *Real_CreateDirectoryA)(
    _In_ LPCSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
)= CreateDirectoryA;

BOOL
(WINAPI *Real_CreateDirectoryW)(
    _In_ LPCWSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
)= CreateDirectoryW;


BOOL
(WINAPI *Real_RemoveDirectoryA)(
    _In_ LPCSTR lpPathName
)= RemoveDirectoryA;

BOOL
(WINAPI *Real_RemoveDirectoryW)(
    _In_ LPCWSTR lpPathName
)= RemoveDirectoryW;

BOOL
(WINAPI *Real_MoveFileA)(
    _In_ LPCSTR lpExistingFileName,
    _In_ LPCSTR lpNewFileName
)= MoveFileA;

BOOL
(WINAPI *Real_MoveFileW)(
    _In_ LPCWSTR lpExistingFileName,
    _In_ LPCWSTR lpNewFileName
)= MoveFileW;

BOOL
(WINAPI *Real_MoveFileExA)(
    _In_     LPCSTR lpExistingFileName,
    _In_opt_ LPCSTR lpNewFileName,
    _In_     DWORD    dwFlags
)= MoveFileExA;

BOOL
(WINAPI *Real_MoveFileExW)(
    _In_     LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_     DWORD    dwFlags
)= MoveFileExW;

HANDLE
(WINAPI *Real_FindFirstFileA)(
    _In_ LPCSTR lpFileName,
    _Out_ LPWIN32_FIND_DATAA lpFindFileData
)= FindFirstFileA;

HANDLE
(WINAPI *Real_FindFirstFileW)(
    _In_ LPCWSTR lpFileName,
    _Out_ LPWIN32_FIND_DATAW lpFindFileData
)= FindFirstFileW;

HANDLE
(WINAPI *Real_FindFirstFileExA)(
    _In_ LPCSTR lpFileName,
    _In_ FINDEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FIND_DATAA)) LPVOID lpFindFileData,
    _In_ FINDEX_SEARCH_OPS fSearchOp,
    _Reserved_ LPVOID lpSearchFilter,
    _In_ DWORD dwAdditionalFlags
)= FindFirstFileExA;

HANDLE
(WINAPI *Real_FindFirstFileExW)(
    _In_ LPCWSTR lpFileName,
    _In_ FINDEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FIND_DATAW)) LPVOID lpFindFileData,
    _In_ FINDEX_SEARCH_OPS fSearchOp,
    _Reserved_ LPVOID lpSearchFilter,
    _In_ DWORD dwAdditionalFlags
)= FindFirstFileExW;

//===========================================================================================

HANDLE
WINAPI
Mine_CreateFileW(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
)
{
    if (NULL == lpFileName)
    {
        return Real_CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    HANDLE hFile;
    std::wstring path = lpFileName;
    std::wstring redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(path, redirectedPath);
    if (!boInterested)
    {
        return Real_CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    //MessageBoxW(NULL, redirectedPath.c_str(), L"Mine_CreateFileW", MB_OK | MB_SYSTEMMODAL);
    hFile = Real_CreateFileW(redirectedPath.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return hFile;
    }

    WinAPIRedirector::GetInstance()->OnHandleCreation(hFile, path, redirectedPath);
    return hFile;
}

HANDLE
WINAPI
Mine_CreateFileA(
    _In_ LPCSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
)
{
    if (NULL == lpFileName)
    {
        return Real_CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    HANDLE hFile;
    std::string pathA = lpFileName;
    std::wstring pathW = ConvertStringToWstring(pathA);
    std::wstring redirectedPathW;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(pathW, redirectedPathW);
    if (!boInterested)
    {
        return Real_CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }
    std::string redirectedPathA = ConvertWstringToString(redirectedPathW);

    //MessageBoxA(NULL, redirectedPathA.c_str(), "Mine_CreateFileA", MB_OK | MB_SYSTEMMODAL);

    hFile = Real_CreateFileA(redirectedPathA.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return hFile;
    }

    WinAPIRedirector::GetInstance()->OnHandleCreation(hFile, pathW, redirectedPathW);
    return hFile;
}

BOOL
WINAPI
Mine_CloseHandle(
    _In_ _Post_ptr_invalid_ HANDLE hObject
)
{
    if (g_iDetach)
    {
        return Real_CloseHandle(hObject);
    }

    WinAPIRedirector::GetInstance()->OnHandleClose(hObject);

    return Real_CloseHandle(hObject);
}


BOOL
WINAPI
Mine_CopyFileA(
    _In_ LPCSTR lpExistingFileName,
    _In_ LPCSTR lpNewFileName,
    _In_ BOOL bFailIfExists
)
{
    if (NULL == lpExistingFileName || NULL == lpNewFileName)
    {
        return Real_CopyFileA(lpExistingFileName, lpNewFileName, bFailIfExists);
    }

    std::string existingPath = lpExistingFileName;
    std::string newPath = lpNewFileName;
    std::string redirectedPathExisting;
    std::string redirectedPathNew;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(existingPath, redirectedPathExisting);
    if (!boInterested)
    {
        redirectedPathExisting = lpExistingFileName;
    }

    boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(newPath, redirectedPathNew);
    if (!boInterested)
    {
        redirectedPathNew = lpNewFileName;
    }

    MessageBoxA(NULL, redirectedPathExisting.c_str(), "Mine_CopyFileA", MB_OK | MB_SYSTEMMODAL);
    return Real_CopyFileA(redirectedPathExisting.c_str(), redirectedPathNew.c_str(), bFailIfExists);
}


BOOL
WINAPI
Mine_CopyFileW(
    _In_ LPCWSTR lpExistingFileName,
    _In_ LPCWSTR lpNewFileName,
    _In_ BOOL bFailIfExists
)
{
    if (NULL == lpExistingFileName || NULL == lpNewFileName)
    {
        return Real_CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
    }

    std::wstring existingPath = lpExistingFileName;
    std::wstring newPath = lpNewFileName;
    std::wstring redirectedPathExisting;
    std::wstring redirectedPathNew;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(existingPath, redirectedPathExisting);
    if (!boInterested)
    {
        redirectedPathExisting = lpExistingFileName;
    }

    boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(newPath, redirectedPathNew);
    if (!boInterested)
    {
        redirectedPathNew = lpNewFileName;
    }

    MessageBoxW(NULL, redirectedPathExisting.c_str(), L"Mine_CopyFileExW", MB_OK | MB_SYSTEMMODAL);
    return Real_CopyFileW(redirectedPathExisting.c_str(), redirectedPathNew.c_str(), bFailIfExists);
}


BOOL
WINAPI
Mine_CopyFileExA(
    _In_        LPCSTR lpExistingFileName,
    _In_        LPCSTR lpNewFileName,
    _In_opt_    LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_    LPVOID lpData,
    _When_(pbCancel != NULL, _Pre_satisfies_(*pbCancel == FALSE))
    _Inout_opt_ LPBOOL pbCancel,
    _In_        DWORD dwCopyFlags
)
{
    if (NULL == lpExistingFileName || NULL == lpNewFileName)
    {
        return Real_CopyFileExA(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
    }

    std::string existingPath = lpExistingFileName;
    std::string newPath = lpNewFileName;
    std::string redirectedPathExisting;
    std::string redirectedPathNew;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(existingPath, redirectedPathExisting);
    if (!boInterested)
    {
        redirectedPathExisting = lpExistingFileName;
    }

    boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(newPath, redirectedPathNew);
    if (!boInterested)
    {
        redirectedPathNew = lpNewFileName;
    }

    MessageBoxA(NULL, redirectedPathExisting.c_str(), "Mine_CopyFileExA", MB_OK | MB_SYSTEMMODAL);
    return Real_CopyFileExA(redirectedPathExisting.c_str(), redirectedPathNew.c_str(), lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
}

BOOL
WINAPI
Mine_CopyFileExW(
    _In_        LPCWSTR lpExistingFileName,
    _In_        LPCWSTR lpNewFileName,
    _In_opt_    LPPROGRESS_ROUTINE lpProgressRoutine,
    _In_opt_    LPVOID lpData,
    _When_(pbCancel != NULL, _Pre_satisfies_(*pbCancel == FALSE))
    _Inout_opt_ LPBOOL pbCancel,
    _In_        DWORD dwCopyFlags
)
{
    if (NULL == lpExistingFileName || NULL == lpNewFileName)
    {
        return Real_CopyFileExW(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
    }

    std::wstring existingPath = lpExistingFileName;
    std::wstring newPath = lpNewFileName;
    std::wstring redirectedPathExisting;
    std::wstring redirectedPathNew;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(existingPath, redirectedPathExisting);
    if (!boInterested)
    {
        redirectedPathExisting = lpExistingFileName;
    }

    boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(newPath, redirectedPathNew);
    if (!boInterested)
    {
        redirectedPathNew = lpNewFileName;
    }

    MessageBoxW(NULL, redirectedPathExisting.c_str(), L"Mine_CopyFileExW", MB_OK | MB_SYSTEMMODAL);
    return Real_CopyFileExW(redirectedPathExisting.c_str(), redirectedPathNew.c_str(), lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
}

BOOL
WINAPI
Mine_DeleteFileA(
    _In_ LPCSTR lpFileName
)
{
    if (NULL == lpFileName)
    {
        return Real_DeleteFileA(lpFileName);
    }

    std::string path = lpFileName;
    std::string redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(path, redirectedPath);
    if (!boInterested)
    {
        redirectedPath = lpFileName;
    }

    MessageBoxA(NULL, redirectedPath.c_str(), "Mine_DeleteFileA", MB_OK | MB_SYSTEMMODAL);
    return Real_DeleteFileA(redirectedPath.c_str());
}


BOOL
WINAPI
Mine_DeleteFileW(
    _In_ LPCWSTR lpFileName
)
{
    if (NULL == lpFileName)
    {
        return Real_DeleteFileW(lpFileName);
    }

    std::wstring path = lpFileName;
    std::wstring redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(path, redirectedPath);
    if (!boInterested)
    {
        redirectedPath = lpFileName;
    }

    MessageBoxW(NULL, redirectedPath.c_str(), L"Mine_DeleteFileW", MB_OK | MB_SYSTEMMODAL);
    return Real_DeleteFileW(redirectedPath.c_str());
}


BOOL
WINAPI
Mine_CreateDirectoryA(
    _In_ LPCSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
)
{
    if (NULL == lpPathName)
    {
        return Real_CreateDirectoryA(lpPathName, lpSecurityAttributes);
    }

    std::string path = lpPathName;
    std::string redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(path, redirectedPath);
    if (!boInterested)
    {
        redirectedPath = lpPathName;
    }
    //MessageBoxA(NULL, redirectedPath.c_str(), "Mine_CreateDirectoryA", MB_OK | MB_SYSTEMMODAL);


    return Real_CreateDirectoryA(redirectedPath.c_str(), lpSecurityAttributes);
}


BOOL
WINAPI
Mine_CreateDirectoryW(
    _In_ LPCWSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
)
{
    if (NULL == lpPathName)
    {
        return Real_CreateDirectoryW(lpPathName, lpSecurityAttributes);
    }

    std::wstring path = lpPathName;
    std::wstring redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(path, redirectedPath);
    if (!boInterested)
    {
        redirectedPath = lpPathName;
    }
    //MessageBoxW(NULL, redirectedPath.c_str(), L"Mine_CreateDirectoryW", MB_OK | MB_SYSTEMMODAL);

    return Real_CreateDirectoryW(redirectedPath.c_str(), lpSecurityAttributes);
}


BOOL
WINAPI
Mine_RemoveDirectoryA(
    _In_ LPCSTR lpPathName
)
{
    if (NULL == lpPathName)
    {
        return Real_RemoveDirectoryA(lpPathName);
    }

    std::string path = lpPathName;
    std::string redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(path, redirectedPath);
    if (!boInterested)
    {
        redirectedPath = lpPathName;
    }
    MessageBoxA(NULL, redirectedPath.c_str(), "Mine_RemoveDirectoryW", MB_OK | MB_SYSTEMMODAL);

    return Real_RemoveDirectoryA(redirectedPath.c_str());
}

BOOL
WINAPI
Mine_RemoveDirectoryW(
    _In_ LPCWSTR lpPathName
)
{
    if (NULL == lpPathName)
    {
        return Real_RemoveDirectoryW(lpPathName);
    }

    std::wstring path = lpPathName;
    std::wstring redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(path, redirectedPath);
    if (!boInterested)
    {
        redirectedPath = lpPathName;
    }
    MessageBoxW(NULL, redirectedPath.c_str(), L"Mine_RemoveDirectoryW", MB_OK | MB_SYSTEMMODAL);

    return Real_RemoveDirectoryW(redirectedPath.c_str());
}


BOOL
WINAPI
Mine_MoveFileA(
    _In_ LPCSTR lpExistingFileName,
    _In_ LPCSTR lpNewFileName
)
{
    if (NULL == lpExistingFileName || NULL == lpNewFileName)
    {
        return Real_MoveFileA(lpExistingFileName, lpNewFileName);
    }

    std::string existingPath = lpExistingFileName;
    std::string newPath = lpNewFileName;
    std::string redirectedPathExisting;
    std::string redirectedPathNew;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(existingPath, redirectedPathExisting);
    if (!boInterested)
    {
        redirectedPathExisting = lpExistingFileName;
    }

    boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(newPath, redirectedPathNew);
    if (!boInterested)
    {
        redirectedPathNew = lpNewFileName;
    }

    MessageBoxA(NULL, redirectedPathExisting.c_str(), "Mine_MoveFileA", MB_OK | MB_SYSTEMMODAL);
    return Real_MoveFileA(redirectedPathExisting.c_str(), redirectedPathNew.c_str());
}

BOOL
WINAPI
Mine_MoveFileW(
    _In_ LPCWSTR lpExistingFileName,
    _In_ LPCWSTR lpNewFileName
)
{
    if (NULL == lpExistingFileName || NULL == lpNewFileName)
    {
        return Real_MoveFileW(lpExistingFileName, lpNewFileName);
    }

    std::wstring existingPath = lpExistingFileName;
    std::wstring newPath = lpNewFileName;
    std::wstring redirectedPathExisting;
    std::wstring redirectedPathNew;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(existingPath, redirectedPathExisting);
    if (!boInterested)
    {
        redirectedPathExisting = lpExistingFileName;
    }

    boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(newPath, redirectedPathNew);
    if (!boInterested)
    {
        redirectedPathNew = lpNewFileName;
    }

    MessageBoxW(NULL, redirectedPathExisting.c_str(), L"Mine_MoveFileW", MB_OK | MB_SYSTEMMODAL);
    return Real_MoveFileW(redirectedPathExisting.c_str(), redirectedPathNew.c_str());
}


BOOL
WINAPI
Mine_MoveFileExA(
    _In_     LPCSTR lpExistingFileName,
    _In_opt_ LPCSTR lpNewFileName,
    _In_     DWORD    dwFlags
)
{
    if (NULL == lpExistingFileName)
    {
        return Real_MoveFileExA(lpExistingFileName, lpNewFileName, dwFlags);
    }

    std::string existingPath = lpExistingFileName;
    std::string redirectedPathExisting;
    std::string newPath;
    std::string redirectedPathNew;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(existingPath, redirectedPathExisting);
    if (!boInterested)
    {
        redirectedPathExisting = lpExistingFileName;
    }

    if (NULL != lpNewFileName)
    {
        newPath = lpNewFileName;
        boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(newPath, redirectedPathNew);
        if (!boInterested)
        {
            redirectedPathNew = lpNewFileName;
        }

        return Real_MoveFileExA(redirectedPathExisting.c_str(), redirectedPathNew.c_str(), dwFlags);
    }

    MessageBoxA(NULL, redirectedPathExisting.c_str(), "Mine_MoveFileExA", MB_OK | MB_SYSTEMMODAL);
    return Real_MoveFileExA(redirectedPathExisting.c_str(), lpNewFileName, dwFlags);
}

BOOL
WINAPI
Mine_MoveFileExW(
    _In_     LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_     DWORD    dwFlags
)
{
    if (NULL == lpExistingFileName)
    {
        return Real_MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
    }

    std::wstring existingPath = lpExistingFileName;
    std::wstring redirectedPathExisting;
    std::wstring newPath;
    std::wstring redirectedPathNew;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(existingPath, redirectedPathExisting);
    if (!boInterested)
    {
        redirectedPathExisting = lpExistingFileName;
    }

    if (NULL != lpNewFileName)
    {
        newPath = lpNewFileName;
        boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(newPath, redirectedPathNew);
        if (!boInterested)
        {
            redirectedPathNew = lpNewFileName;
        }

        return Real_MoveFileExW(redirectedPathExisting.c_str(), redirectedPathNew.c_str(), dwFlags);
    }

    MessageBoxW(NULL, redirectedPathExisting.c_str(), L"Mine_MoveFileExW", MB_OK | MB_SYSTEMMODAL);
    return Real_MoveFileExW(redirectedPathExisting.c_str(), lpNewFileName, dwFlags);
}


HANDLE
WINAPI
Mine_FindFirstFileA(
    _In_ LPCSTR lpFileName,
    _Out_ LPWIN32_FIND_DATAA lpFindFileData
)
{
    if (NULL == lpFileName)
    {
        return Real_FindFirstFileA(lpFileName, lpFindFileData);
    }

    std::string path = lpFileName;
    std::string redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(path, redirectedPath);
    if (!boInterested)
    {
        redirectedPath = lpFileName;
    }

    //MessageBoxA(NULL, redirectedPath.c_str(), "Mine_FindFirstFileA", MB_OK | MB_SYSTEMMODAL);
    return Real_FindFirstFileA(redirectedPath.c_str(), lpFindFileData);
}


HANDLE
WINAPI
Mine_FindFirstFileW(
    _In_ LPCWSTR lpFileName,
    _Out_ LPWIN32_FIND_DATAW lpFindFileData
)
{
    if (NULL == lpFileName)
    {
        return Real_FindFirstFileW(lpFileName, lpFindFileData);
    }

    std::wstring path = lpFileName;
    std::wstring redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(path, redirectedPath);
    if (!boInterested)
    {
        redirectedPath = lpFileName;
    }

    //MessageBoxW(NULL, redirectedPath.c_str(), L"Mine_FindFirstFileW", MB_OK | MB_SYSTEMMODAL);
    return Real_FindFirstFileW(redirectedPath.c_str(), lpFindFileData);
}

HANDLE
WINAPI
Mine_FindFirstFileExA(
    _In_ LPCSTR lpFileName,
    _In_ FINDEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FIND_DATAA)) LPVOID lpFindFileData,
    _In_ FINDEX_SEARCH_OPS fSearchOp,
    _Reserved_ LPVOID lpSearchFilter,
    _In_ DWORD dwAdditionalFlags
)
{
    if (NULL == lpFileName)
    {
        return Real_FindFirstFileExA(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    }

    std::string path = lpFileName;
    std::string redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterestedA(path, redirectedPath);
    if (!boInterested)
    {
        redirectedPath = lpFileName;
    }

    MessageBoxA(NULL, redirectedPath.c_str(), "Mine_FindFirstFileExA", MB_OK | MB_SYSTEMMODAL);
    return Real_FindFirstFileExA(redirectedPath.c_str(), fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
}

HANDLE
WINAPI
Mine_FindFirstFileExW(
    _In_ LPCWSTR lpFileName,
    _In_ FINDEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FIND_DATAW)) LPVOID lpFindFileData,
    _In_ FINDEX_SEARCH_OPS fSearchOp,
    _Reserved_ LPVOID lpSearchFilter,
    _In_ DWORD dwAdditionalFlags
)
{
    if (NULL == lpFileName)
    {
        return Real_FindFirstFileExW(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    }

    std::wstring path = lpFileName;
    std::wstring redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(path, redirectedPath);
    if (!boInterested)
    {
        redirectedPath = lpFileName;
    }

    MessageBoxW(NULL, redirectedPath.c_str(), L"Mine_FindFirstFileExW", MB_OK | MB_SYSTEMMODAL);
    return Real_FindFirstFileExW(redirectedPath.c_str(), fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
}

VOID DetAttach(PVOID* ppvReal, PVOID pvMine, const char* psz)
{
    PVOID pvReal = NULL;
    if (ppvReal == NULL) {
        ppvReal = &pvReal;
    }

    LONG l = DetourAttach(ppvReal, pvMine);
    if (l != 0) {
        DbgViewf(L"Attach failed: error %d\n", l);
    }
}

VOID DetDetach(PVOID* ppvReal, PVOID pvMine, const char* psz)
{
    DetourDetach(ppvReal, pvMine);
}

LONG AttachDetours(VOID)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    // For this many APIs, we'll ignore one or two can't be detoured.
    DetourSetIgnoreTooSmall(TRUE);

    ATTACH(CreateFileW);
    ATTACH(CreateFileA);
    ATTACH(CloseHandle);
    ATTACH(CopyFileA);
    ATTACH(CopyFileW);
    ATTACH(CopyFileExA);
    ATTACH(CopyFileExW);
    ATTACH(DeleteFileA);
    ATTACH(DeleteFileW);
    ATTACH(CreateDirectoryA);
    ATTACH(CreateDirectoryW);
    ATTACH(RemoveDirectoryA);
    ATTACH(RemoveDirectoryW);
    ATTACH(MoveFileA);
    ATTACH(MoveFileW);
    ATTACH(MoveFileExA);
    ATTACH(MoveFileExW);
    ATTACH(FindFirstFileA);
    ATTACH(FindFirstFileW);
    //ATTACH(FindFirstFileExA);
    //ATTACH(FindFirstFileExW);

    PVOID* ppbFailedPointer = NULL;
    LONG error = DetourTransactionCommitEx(&ppbFailedPointer);
    if (error != 0) {
        DbgViewf(L"traceapi.dll: Attach transaction failed to commit. Error %ld (%p/%p)",
            error, ppbFailedPointer, *ppbFailedPointer);
        return error;
    }
    return 0;
}

LONG DetachDetours(VOID)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    // For this many APIs, we'll ignore one or two can't be detoured.
    DetourSetIgnoreTooSmall(TRUE);

    DETACH(CreateFileW);

    if (DetourTransactionCommit() != 0) {
        PVOID* ppbFailedPointer = NULL;
        LONG error = DetourTransactionCommitEx(&ppbFailedPointer);

        DbgViewf(L"traceapi.dll: Detach transaction failed to commit. Error %ld (%p/%p)",
            error, ppbFailedPointer, *ppbFailedPointer);
        return error;
    }
    return 0;
}

bool WinAPIRedirector::Init()
{
    std::wstring srcDirPath;
    std::wstring redirectDirPath;

    bool boRet = InitRedirectedPathFromConfig(srcDirPath, redirectDirPath);
    if (false == boRet)
    {
        MessageBoxW(NULL, L"Source and Redirected directories are not properly configure in config.ini !!!", L"ERROR", MB_OK | MB_SYSTEMMODAL);
        DbgViewf(L"Source and Redirected directories are not properly configure!!!\n");
        return false;
    }

    //
    //	Check source directory existance.
    //
    if (!FileExists(srcDirPath))
    {
        WCHAR szError[MAX_PATH];
        StringCchPrintf(szError, ARRAYSIZE(szError), L"Source directory path(%s) not found", srcDirPath.c_str());
        MessageBoxW(NULL, szError, L"ERROR", MB_OK | MB_SYSTEMMODAL);
        DbgViewf(L"(%s)\n", szError);
        return false;
    }

    //
    //	Check redirected directory existance.
    //
    if (!FileExists(redirectDirPath))
    {
        WCHAR szError[MAX_PATH];
        StringCchPrintf(szError, ARRAYSIZE(szError), L"Redirected directory path(%s) not found", redirectDirPath.c_str());
        MessageBoxW(NULL, szError, L"ERROR", MB_OK | MB_SYSTEMMODAL);
        DbgViewf(L"(%s)\n", szError);
        return false;
    }

    LONG error = AttachDetours();
    if (error != NO_ERROR)
    {
        WCHAR szError[MAX_PATH];
        StringCchPrintf(szError, ARRAYSIZE(szError), L"Error attaching detours: %d", errno);
        MessageBoxW(NULL, szError, L"ERROR", MB_OK | MB_SYSTEMMODAL);
        DbgViewf(L"Error attaching detours: %d\n", error);
        return false;
    }

    if (nullptr == s_winAPIRedirector)
    {
        s_winAPIRedirector.reset(new WinAPIRedirector(srcDirPath, redirectDirPath));
    }

    WinAPIRedirector::GetInstance()->GetProcessInfo().InitProcessInfo();

    return true;
}

void WinAPIRedirector::Release()
{
    LONG error = DetachDetours();
    if (error != NO_ERROR)
    {
        WCHAR szError[MAX_PATH];
        StringCchPrintf(szError, ARRAYSIZE(szError), L"Error detaching detours: %d", errno);
        MessageBoxW(NULL, szError, L"ERROR", MB_OK | MB_SYSTEMMODAL);
        DbgViewf(L"Error detaching detours: %d\n", error);
    }

    if (s_winAPIRedirector)
    {
        s_winAPIRedirector.reset(nullptr);
    }
}

WinAPIRedirector* WinAPIRedirector::GetInstance()
{
    return s_winAPIRedirector.get();
}

WinAPIRedirector::WinAPIRedirector(std::wstring srcDirPath, std::wstring redirectedDirPath):m_srcDirPath(srcDirPath), m_redirectedDirPath(redirectedDirPath)
{
    ToLowerCase(m_srcDirPath);
    ToLowerCase(redirectedDirPath);
    InitializeCriticalSection(&m_handleInfoLock);
}

WinAPIRedirector::~WinAPIRedirector()
{
    DeleteCriticalSection(&m_handleInfoLock);
}

ProcessInfo& WinAPIRedirector::GetProcessInfo()
{
    return m_processInfo;
}

WinAPIRedirector::HandleInfo::HandleInfo()
{
}

WinAPIRedirector::HandleInfo::HandleInfo(std::wstring originalPath, std::wstring redirectedPath) :m_originalPath(originalPath), m_redirectedPath(redirectedPath)
{
}

bool WinAPIRedirector::IsOperationInterested(std::wstring& path, std::wstring &redirectedPath)
{
    if (false == CheckIfSourcePathLocation(path, redirectedPath))
    {
        return false;
    }

    return true;
}

bool WinAPIRedirector::IsOperationInterestedA(std::string& path, std::string& redirectedPath)
{
    std::wstring pathW = ConvertStringToWstring(path);
    std::wstring redirectedPathW;

    if (false == CheckIfSourcePathLocation(pathW, redirectedPathW))
    {
        return false;
    }

    redirectedPath = ConvertWstringToString(redirectedPathW);
    return true;
}

bool WinAPIRedirector::OnHandleCreation(HANDLE hfile, std::wstring& originalPath, std::wstring& redirectedPath)
{
    if (IsRedirectedHandle(hfile))
    {
        //  Handle already present.
        return false;
    }

    Lock();
    m_handleInfoMap[hfile] = HandleInfo(originalPath, redirectedPath);
    Unlock();

    return true;
}

bool WinAPIRedirector::OnHandleClose(HANDLE hfile)
{
    Lock();
    auto it = m_handleInfoMap.find(hfile);
    if (it == m_handleInfoMap.end())
    {
        Unlock();
        return false;
    }
    m_handleInfoMap.erase(hfile);
    Unlock();
    return true;
}

bool WinAPIRedirector::IsRedirectedHandle(HANDLE hFile)
{
    Lock();
    auto it = m_handleInfoMap.find(hFile);
    if (it != m_handleInfoMap.end())
    {
        Unlock();
        return true;
    }

    Unlock();
    return false;
}

void WinAPIRedirector::Lock()
{
    EnterCriticalSection(&m_handleInfoLock);
}

void WinAPIRedirector::Unlock()
{
    LeaveCriticalSection(&m_handleInfoLock);
}

bool WinAPIRedirector::CheckIfSourcePathLocation(std::wstring& path, std::wstring &redirectedPath)
{
    if (path.length() < m_srcDirPath.length())
    {
        return false;
    }

    std::wstring tempPath = path;
    ToLowerCase(tempPath);

    size_t pos = tempPath.find(m_srcDirPath);
    if (0 != pos)
    {
        return false;
    }

    if (tempPath.length() == m_srcDirPath.length())
    {
        //  Root location is accessed.
        redirectedPath = m_redirectedDirPath;
        return true;
    }

    if ('\\' != tempPath[m_srcDirPath.length()])
    {
        return false;
    }

    redirectedPath = m_redirectedDirPath;
    size_t relativePathLen = tempPath.length() - m_srcDirPath.length();
    std::wstring relativePath(tempPath.substr(m_srcDirPath.length(), relativePathLen));
    redirectedPath += relativePath;

    return true;
}
