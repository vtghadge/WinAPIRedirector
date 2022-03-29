// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"
#include "..\common\rapidjson\rapidjson.h"
#include "..\common\rapidjson\document.h"
#include "..\common\rapidjson\stringbuffer.h"
#include "..\common\rapidjson\prettywriter.h"
#include "..\common\curl\curl.h"
#include "detours.h"
#include "Common.h"
#include "WinAPI.h"
#include "ProcessInfo.h"

extern WCHAR g_szExePath[MAX_PATH];
extern WCHAR g_szDllPath[MAX_PATH];
extern HMODULE g_hModule;

#endif //PCH_H
