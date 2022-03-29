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
    HANDLE hFile;
    std::wstring path = lpFileName;
    std::wstring redirectedPath;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(path, dwDesiredAccess, dwCreationDisposition, dwFlagsAndAttributes, redirectedPath);
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
    HANDLE hFile;
    std::string pathA = lpFileName;
    std::wstring pathW = ConvertStringToWstring(pathA);
    std::wstring redirectedPathW;

    bool boInterested = WinAPIRedirector::GetInstance()->IsOperationInterested(pathW, dwDesiredAccess, dwCreationDisposition, dwFlagsAndAttributes, redirectedPathW);
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
        //s_winAPIRedirector = std::make_unique<WinAPIRedirector>(srcDirPath, redirectDirPath);
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

bool WinAPIRedirector::IsOperationInterested(std::wstring& path, DWORD dwDesiredAccess, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, std::wstring &redirectedPath)
{
    if (false == CheckIfSourcePathLocation(path, redirectedPath))
    {
        return false;
    }

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
