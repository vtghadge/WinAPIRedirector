#pragma once

class ProcessInfo
{
public:
	ProcessInfo();
	bool InitProcessInfo();
	bool Serialize(std::string &serializeBuffer);

private:
	bool QueryProcessUserInfo();
	bool QueryProcessPathFromPid(ULONG ulProcessId, std::wstring& processPath, std::wstring& processName);

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
};
