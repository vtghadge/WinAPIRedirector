// TestInjection.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <Windows.h>
#include <iostream>
#include <strsafe.h>

#define INJECTOR_DLL_NAME	L"DllWinAPIRedirector.dll"

BOOL GetModuleFolderPath(wchar_t* pwszFolderPath, DWORD dwCchSize, bool bIncludeLastBackslash)
{
	DWORD dwError;
	wchar_t* pwszTemp = NULL;

	if (NULL == pwszFolderPath || 0 == dwCchSize)
	{
		return FALSE;
	}

	dwError = GetModuleFileName(GetModuleHandle(nullptr), pwszFolderPath, dwCchSize);
	if (0 == dwError)
	{
		return FALSE;
	}
	if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
	{
		return FALSE;
	}

	pwszTemp = wcsrchr(pwszFolderPath, L'\\');
	if (NULL == pwszTemp)
	{
		return FALSE;
	}

	if (true == bIncludeLastBackslash)
	{
		pwszTemp++;
		*pwszTemp = L'\0';
	}
	else
	{
		*pwszTemp = L'\0';
	}

	return TRUE;
}

bool InjectDLL(ULONG ulProcessId)
{
	WCHAR wszFolderPath[MAX_PATH];
	BOOL boRet = GetModuleFolderPath(wszFolderPath, ARRAYSIZE(wszFolderPath), true);
	if (FALSE == boRet)
	{
		wprintf(L"InjectDLL: GetModuleFolderPath() failed with error(%u)", GetLastError());
		return false;
	}

	HRESULT hResult = StringCchCatW(wszFolderPath, ARRAYSIZE(wszFolderPath), INJECTOR_DLL_NAME);
	if (FAILED(hResult))
	{
		wprintf(L"InjectDLL: StringCchCatW() failed with error(%u)", HRESULT_CODE(hResult));
		return false;
	}

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ulProcessId);
    if (NULL == hProcess)
    {
        wprintf(L"InjectDLL: OpenProcess() failed for pid(%u) with error(%u)", ulProcessId, GetLastError());
        return false;
    }

	size_t stMemorySize = wcslen(wszFolderPath) * sizeof(WCHAR) + sizeof(WCHAR);

	LPVOID lpInjectorDLLPath;
	lpInjectorDLLPath = VirtualAllocEx(hProcess, NULL, stMemorySize, MEM_COMMIT, PAGE_READWRITE);
	if (NULL == lpInjectorDLLPath)
	{
		wprintf(L"InjectDLL: VirtualAllocEx() failed with error(%u)", GetLastError());
		CloseHandle(hProcess);
		return false;
	}

	SIZE_T stBytesWrite;
	boRet = WriteProcessMemory(hProcess, lpInjectorDLLPath, wszFolderPath, stMemorySize, &stBytesWrite);
	if (FALSE == boRet)
	{
		wprintf(L"InjectDLL: WriteProcessMemory() failed with error(%u)", GetLastError());
		VirtualFreeEx(hProcess, lpInjectorDLLPath, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	HMODULE hModule = GetModuleHandleW(L"Kernel32");
	if (NULL == hModule)
	{
		wprintf(L"InjectDLL: GetModuleHandleW() failed with error(%u)", GetLastError());
		VirtualFreeEx(hProcess, lpInjectorDLLPath, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	PVOID pvLoadLib = GetProcAddress(hModule, "LoadLibraryW");
	if (NULL == pvLoadLib)
	{
		wprintf(L"InjectDLL: GetProcAddress() failed ");
		VirtualFreeEx(hProcess, lpInjectorDLLPath, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pvLoadLib, lpInjectorDLLPath, 0, NULL);
	if (NULL == hRemoteThread)
	{
		wprintf(L"InjectDLL: CreateRemoteThread() failed with error(%u)", GetLastError());
		VirtualFreeEx(hProcess, lpInjectorDLLPath, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	WaitForSingleObject(hRemoteThread, INFINITE);

	CloseHandle(hRemoteThread);
	VirtualFreeEx(hProcess, lpInjectorDLLPath, 0, MEM_RELEASE);
	CloseHandle(hProcess);

    return true;
}

int wmain(int argc, WCHAR *argv[])
{
    if (argc != 2)
    {
        wprintf(L"Please enter valid inputs: ");
        wprintf(L"Help : TestInjection.exe <pid>");
        return 0;
    }

	ULONG ulPid = _wtol(argv[1]);

	InjectDLL(ulPid);
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
