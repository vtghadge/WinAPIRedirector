#pragma once

VOID DetAttach(PVOID* ppvReal, PVOID pvMine, const char* psz);

VOID DetDetach(PVOID* ppvReal, PVOID pvMine, const char* psz);

LONG AttachDetours(VOID);

LONG DetachDetours(VOID);

class WinAPIRedirector
{
public:
	static void Init();
	static void Release();
	static WinAPIRedirector* GetInstance();

private:
	struct HandleInfo
	{
		std::wstring originalPath;
		std::wstring redirectedPath;
	};
};
