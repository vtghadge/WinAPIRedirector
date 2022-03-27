// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "detours.h"
#include "WinAPI.h"
#include "Common.h"

WCHAR g_szExePath[MAX_PATH];
WCHAR g_szDllPath[MAX_PATH];
HMODULE g_hModule = NULL;

std::unique_ptr<WinAPIRedirector> WinAPIRedirector::s_winAPIRedirector = nullptr;

#define SOURCE_DIR_PATH L"C:\\Users\\lenovo\\Documents\\Important"
#define REDIRECT_DIR_PATH L"C:\\Users\\Public\\Documents\\Important"

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

        {
        //    MessageBoxW(NULL, g_szExePath, L"start", MB_OK | MB_SYSTEMMODAL);
        }
        DetourRestoreAfterWith();
        BOOL boRet = WinAPIRedirector::Init(SOURCE_DIR_PATH, REDIRECT_DIR_PATH);
        return boRet;
    }
    case DLL_PROCESS_DETACH:
    {
        g_iDetach = 1;
        //MessageBoxW(NULL, g_szExePath, L"Stop", MB_OK | MB_SYSTEMMODAL);

        WinAPIRedirector::Release();

        return true;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

