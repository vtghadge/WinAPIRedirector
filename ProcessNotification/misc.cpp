
#include "pch.h"

BOOLEAN SetPrivilege(const TCHAR* pcszPrivilegeStr, BOOL bEnablePrivilege, DWORD* pdwError)
{
	BOOL bRet;
	LUID luid;
	HANDLE hToken;
	HANDLE hProcess;
	TOKEN_PRIVILEGES tokenPrivilege;

	if (NULL == pcszPrivilegeStr || NULL == pdwError)
	{
		return FALSE;
	}

	hProcess = GetCurrentProcess();

	bRet = OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	if (FALSE == bRet)
	{
		*pdwError = GetLastError();
		return FALSE;
	}

	bRet = LookupPrivilegeValue(NULL, pcszPrivilegeStr, &luid);
	if (FALSE == bRet)
	{
		*pdwError = GetLastError();
		CloseHandle(hToken);
		return FALSE;
	}

	ZeroMemory(&tokenPrivilege, sizeof(tokenPrivilege));
	tokenPrivilege.PrivilegeCount = 1;
	tokenPrivilege.Privileges[0].Luid = luid;
	if (TRUE == bEnablePrivilege)
	{
		tokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	}
	else
	{
		tokenPrivilege.Privileges[0].Attributes = 0;
	}

	//
	//	Adjust Token privileges.
	//
	bRet = AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tokenPrivilege,
		sizeof(TOKEN_PRIVILEGES),
		NULL,
		NULL
	);
	if (FALSE == bRet)
	{
		*pdwError = GetLastError();
		CloseHandle(hToken);

		return FALSE;
	}

	*pdwError = GetLastError();

	if (ERROR_NOT_ALL_ASSIGNED == *pdwError)
	{
		CloseHandle(hToken);
		return FALSE;
	}

	CloseHandle(hToken);
	return TRUE;
}

BOOL GetWorkingDirPathW(std::wstring &folderPath, bool bIncludeLastBackslash)
{
	DWORD dwError;
	wchar_t* pwszTemp = NULL;
	WCHAR wszPath[MAX_PATH];

	dwError = GetModuleFileNameW(GetModuleHandle(nullptr), wszPath, ARRAYSIZE(wszPath));
	if (0 == dwError)
	{
		return FALSE;
	}
	if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
	{
		return FALSE;
	}

	pwszTemp = wcsrchr(wszPath, L'\\');
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

	folderPath = wszPath;

	return TRUE;
}

void Install(std::wstring infFilename)
{
	std::wstring CmdLine(L"DefaultInstall 128 ");
	CmdLine.append(infFilename);

	InstallHinfSection(NULL, NULL, CmdLine.c_str(), 0);
}

void Uninstall(std::wstring infFilename)
{
	std::wstring CmdLine(L"DefaultUninstall 128 ");
	CmdLine.append(infFilename);

	InstallHinfSection(NULL, NULL, CmdLine.c_str(), 0);
}

bool StartDriver()
{
	HRESULT hResult;

	hResult = FilterLoad(FILTER_NAME);

	if (FAILED(hResult))
	{
		if (
			HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hResult ||
			HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING) == hResult
			)
		{
			//
			//	Not an error condition.
			//
		}
		else
		{
			//
			//	Unable to load driver.
			//
			wprintf(L"StartDriver: FilterLoad failed with error(%u).\n", HRESULT_CODE(hResult));
			return false;
		}
	}

	return true;
}


bool StopDriver()
{
	HRESULT hResult;
	DWORD dwError;

	BOOLEAN bRetVal = SetPrivilege(_T("SeLoadDriverPrivilege"), TRUE, &dwError);
	if (FALSE == bRetVal)
	{
		//
		//	Do not return FALSE. TODO - Need to be explored.
		//
	}

	hResult = FilterUnload(FILTER_NAME);
	if (FAILED(hResult))
	{
		return false;
	}

	return true;
}

void ToLowerCase(std::wstring& path)
{
	std::transform(path.begin(), path.end(), path.begin(), [](wchar_t c) { return std::tolower(c); });
}

void ExtractProcessName(std::wstring& processPath)
{
	size_t pos = processPath.find_last_of(L"\\");
	if (pos != std::wstring::npos)
	{
		processPath.erase(0, pos + 1);
	}
}

bool SendProcessEventToServer(std::string URL, std::string header, std::string jsonData)
{
	CURLcode CurlError;
	CURL* pCurlHandle = NULL;
	char chCurlErrorBuffer[CURL_ERROR_SIZE + 1];

	CurlError = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (CURLE_OK != CurlError)
	{
		std::string strLog = curl_easy_strerror(CurlError);
		return false;
	}

	pCurlHandle = curl_easy_init();
	if (NULL == pCurlHandle)
	{
		curl_global_cleanup();
		return false;
	}

	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_ERRORBUFFER, chCurlErrorBuffer))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}

	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_VERBOSE, 0L))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}

	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_CONNECTTIMEOUT, 30))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}

	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_URL, URL.c_str()))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}

	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_SSL_VERIFYPEER, true))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}
	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_SSL_VERIFYHOST, 2L))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}
	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_POST, 1L))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}
	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_POSTFIELDS, jsonData.c_str()))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}

	int iRetVal = curl_easy_perform(pCurlHandle);
	if (0 != iRetVal)
	{
		std::string strLog = curl_easy_strerror((CURLcode)iRetVal);
		strLog = "curl_easy_perform() Failed : " + strLog + "";
		strLog = std::string(chCurlErrorBuffer) + "";

		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}

	return true;
}
