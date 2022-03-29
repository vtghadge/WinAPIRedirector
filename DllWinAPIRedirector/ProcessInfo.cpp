#include "pch.h"
#include <sddl.h>

ProcessInfo::ProcessInfo()
{
	m_ulProcessId = GetCurrentProcessId();
	m_ulParentProcessId = 0;
}

bool ProcessInfo::InitProcessInfo()
{
	bool boRet = QueryProcessPathFromPid(m_ulProcessId, m_processPath, m_processName);
	if (false == boRet)
	{
		return false;
	}

	boRet = QueryProcessUserInfo();
	if (false == boRet)
	{
		return false;
	}

	std::string serializeProcessData;
	Serialize(serializeProcessData);

	return true;
}

bool ProcessInfo::Serialize(std::string &serializeBuffer)
{
	rapidjson::StringBuffer stringBuffer;
	rapidjson::Writer<rapidjson::StringBuffer> JsonWriter(stringBuffer);

	std::string tempStr;
	JsonWriter.StartObject();

	JsonWriter.String("ProcessId");
	JsonWriter.Uint64(m_ulProcessId);

	JsonWriter.String("ParentProcessId");
	JsonWriter.Uint64(m_ulParentProcessId);

	JsonWriter.String("ProcessPath");
	tempStr = ConvertWstringToString(m_processPath);
	JsonWriter.String(tempStr.c_str());

	JsonWriter.String("ProcessName");
	tempStr = ConvertWstringToString(m_processPath);
	JsonWriter.String(tempStr.c_str());

	JsonWriter.String("SID");
	tempStr = ConvertWstringToString(m_userSID);
	JsonWriter.String(tempStr.c_str());

	JsonWriter.EndObject();

	serializeBuffer = stringBuffer.GetString();
	SendProcessEventToServer("https://webhook.site/cc73f40a-f803-4894-9bc7-5d53af4aad17", serializeBuffer);

	return true;
}

bool ProcessInfo::QueryProcessUserInfo()
{
	BOOL			boRetVal;
	DWORD			dwRetLength;
	HANDLE			hProcess;
	HANDLE			hToken;
	SID_NAME_USE	sidNameUse;
	WCHAR			wszUserName_l[MAX_PATH];
	WCHAR			wszDomainName[MAX_PATH];
	DWORD			dwRetUserNameLength = MAX_PATH;
	DWORD			dwRetDomainNameLength = MAX_PATH;
	PTOKEN_USER		pTokenUser = NULL;
	LPTSTR pszSIDString = NULL;

	if (0 == m_ulProcessId)
	{
		return false;
	}

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_ulProcessId);
	if (NULL == hProcess)
	{
		return false;
	}

	boRetVal = OpenProcessToken(hProcess, TOKEN_READ, &hToken);
	if (FALSE == boRetVal)
	{
		CloseHandle(hProcess);
		return false;
	}

	//	Get token length.
	boRetVal = GetTokenInformation(hToken, TokenUser, NULL, 0, &dwRetLength);

	pTokenUser = (PTOKEN_USER)malloc(dwRetLength);
	if (NULL == pTokenUser)
	{
		CloseHandle(hToken);
		CloseHandle(hProcess);
		return false;
	}

	boRetVal = GetTokenInformation(hToken, TokenUser, pTokenUser, dwRetLength, &dwRetLength);
	if (FALSE == boRetVal)
	{
		free(pTokenUser);
		CloseHandle(hToken);
		CloseHandle(hProcess);
		return false;
	}

	if (!IsValidSid(pTokenUser->User.Sid))
	{
		free(pTokenUser);
		CloseHandle(hToken);
		CloseHandle(hProcess);
		return false;
	}

	boRetVal = LookupAccountSid(NULL, pTokenUser->User.Sid, wszUserName_l, &dwRetUserNameLength, wszDomainName, &dwRetDomainNameLength, &sidNameUse);
	if (FALSE == boRetVal)
	{
		free(pTokenUser);
		CloseHandle(hToken);
		CloseHandle(hProcess);
		return false;
	}

	m_userName = std::wstring(wszUserName_l);
	m_domainName = std::wstring(wszDomainName);

	boRetVal = ConvertSidToStringSidW(pTokenUser->User.Sid, &pszSIDString);
	if (FALSE == boRetVal)
	{
		free(pTokenUser);
		CloseHandle(hToken);
		CloseHandle(hProcess);
		return false;
	}

	m_userSID = pszSIDString;

	free(pTokenUser);
	CloseHandle(hToken);
	CloseHandle(hProcess);

	return true;
}

bool ProcessInfo::QueryProcessPathFromPid(ULONG ulProcessId, std::wstring& processPath, std::wstring& processName)
{
	DWORD dwRetVal;
	HANDLE hProcess;
	WCHAR procPath[MAX_PATH];

	if (0 == ulProcessId)
	{
		return false;
	}

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ulProcessId);
	if (NULL == hProcess)
	{
		return false;
	}

	dwRetVal = GetModuleFileNameEx(hProcess, NULL, procPath, MAX_PATH);
	if (0 == dwRetVal)
	{
		CloseHandle(hProcess);
		return false;
	}

	processPath.assign(procPath);

	processName = processPath;

	ExtractProcessName(processName);

	CloseHandle(hProcess);
	return true;
}

bool ProcessInfo::SendProcessEventToServer(std::string URL, std::string jsonData)
{
	UNREFERENCED_PARAMETER(URL);
	UNREFERENCED_PARAMETER(jsonData);
	/*
		CURLcode CurlError;
	CURL* pCurlHandle = NULL;
	struct curl_slist* HeaderData = NULL;
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

	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_URL, URL))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}

	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_SSL_VERIFYPEER, false))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}
	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_SSL_VERIFYHOST, 0L))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}
	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_NOSIGNAL, 1))
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
	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_HEADERDATA, (void*)NULL))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}
	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_LOW_SPEED_LIMIT, 10))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}
	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_LOW_SPEED_TIME, 10))
	{
		curl_easy_cleanup(pCurlHandle);
		curl_global_cleanup();
		return false;
	}
	if (0 != curl_easy_setopt(pCurlHandle, CURLOPT_HTTPHEADER, HeaderData))
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

*/
return true;
}
