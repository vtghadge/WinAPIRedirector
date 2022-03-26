#include "pch.h"
#include "detours.h"
#include "Common.h"
#include "WinAPI.h"


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
    MessageBoxW(NULL, lpFileName, L"Mine_CreateFileW", MB_OK | MB_SYSTEMMODAL);
    return Real_CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
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
    MessageBoxA(NULL, lpFileName, "Mine_CreateFileA", MB_OK | MB_SYSTEMMODAL);
    return Real_CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
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

bool WinAPIRedirector::Init(std::wstring srcDirPath, std::wstring redirectDirPath)
{
    //
    //	Check source directory existance.
    //
    if (-1 == _waccess(srcDirPath.c_str(), 00))
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
    if (-1 == _waccess(redirectDirPath.c_str(), 00))
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
        //s_winAPIRedirector = std::make_unique<WinAPIRedirector>(srcDirPath, redirectDirPath);
        s_winAPIRedirector.reset(new WinAPIRedirector(srcDirPath, redirectDirPath));
    }

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
    InitializeCriticalSection(&m_handleInfoLock);
}

WinAPIRedirector::~WinAPIRedirector()
{
    DeleteCriticalSection(&m_handleInfoLock);
}

void WinAPIRedirector::Lock()
{
    EnterCriticalSection(&m_handleInfoLock);
}

void WinAPIRedirector::Unlock()
{
    LeaveCriticalSection(&m_handleInfoLock);
}
