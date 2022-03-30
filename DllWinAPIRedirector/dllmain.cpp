// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

WCHAR g_szExePath[MAX_PATH];
WCHAR g_szDllPath[MAX_PATH];
HMODULE g_hModule = NULL;

std::unique_ptr<WinAPIRedirector> WinAPIRedirector::s_winAPIRedirector = nullptr;

BOOL ProcessAttach(HMODULE hDll)
{
    LONG error = AttachDetours();
    if (error != NO_ERROR)
    {
        DbgViewf(L"Error attaching detours: %d\n", error);
    }

    return TRUE;
}

BOOL ProcessDetach(HMODULE hDll)
{
    LONG error = DetachDetours();
    if (error != NO_ERROR)
    {
        DbgViewf(L"Error detaching detours: %d\n", error);
    }

    DbgViewf(L"Closing.\n");

    return TRUE;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        g_hModule = hModule;

        DisableThreadLibraryCalls(hModule);

        DWORD dwRet = GetModuleFileNameW(NULL, g_szExePath, MAX_PATH);
        if (0 == dwRet)
        {
            return FALSE;
        }

        dwRet = GetModuleFileNameW(hModule, g_szDllPath, MAX_PATH);
        if (0 == dwRet)
        {
            return FALSE;
        }

        DbgViewf(L"%s, %s\n", __FUNCTIONW__, g_szExePath);
        DbgViewf(L"Hook DLL has been loaded to [%s]", g_szExePath);
        //MessageBoxW(NULL, g_szExePath, L"start", MB_OK | MB_SYSTEMMODAL);
        DetourRestoreAfterWith();
        BOOL boRet = WinAPIRedirector::Init();
        return boRet;
    }
    case DLL_PROCESS_DETACH:
    {
        g_iDetach = 1;
        //MessageBoxW(NULL, g_szExePath, L"Stop", MB_OK | MB_SYSTEMMODAL);

        WinAPIRedirector::Release();

        return TRUE;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

