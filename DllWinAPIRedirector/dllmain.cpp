// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "detours.h"
#include "WinAPI.h"
#include "Common.h"

WCHAR g_szExePath[MAX_PATH];
WCHAR g_szDllPath[MAX_PATH];
HMODULE g_hModule = NULL;

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

        GetModuleFileName(NULL, g_szExePath, MAX_PATH);
        DbgViewf(L"%s, %s\n", __FUNCTIONW__, g_szExePath);
        DbgViewf(L"Hook DLL has been loaded to [%s]", g_szExePath);

        GetModuleFileName(hModule, g_szDllPath, MAX_PATH);

        //if (NULL != wcsistr(g_szExePath, L"TestCase"))
        {
            MessageBoxW(NULL, g_szExePath, L"start", MB_OK | MB_SYSTEMMODAL);
        }

        DetourRestoreAfterWith();
        BOOL boRet = ProcessAttach(hModule);
        return boRet;
    }
    case DLL_PROCESS_DETACH:
    {
        MessageBoxW(NULL, g_szExePath, L"Stop", MB_OK | MB_SYSTEMMODAL);

        BOOL boRet = ProcessDetach(hModule);
        return boRet;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

