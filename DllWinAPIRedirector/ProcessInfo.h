#pragma once



class ProcessInfo
{
public:
	ProcessInfo();
	bool InitProcessInfo();

private:
	bool Serialize(std::string& serializeBuffer);
	bool QueryProcessUserInfo();
	bool QueryProcessPathFromPid(ULONG ulProcessId, std::wstring& processPath, std::wstring& processName);
	bool QueryProcessExtraInfo(ULONG ulProcessId);
	bool SendProcessEventToServer(std::string URL, std::string jsonData);

private:
	ULONG m_ulProcessId;
	ULONG m_ulParentProcessId;
	std::wstring m_processPath;
	std::wstring m_parentProcessPath;
	std::wstring m_processName;
	std::wstring m_parentProcessName;
	std::wstring m_userSID;
	std::wstring m_userName;
	std::wstring m_domainName;
	LONGLONG m_llProcessCreationTime;
};
