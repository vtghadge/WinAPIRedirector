
Utility: 	Redirect Windows API
Author:		Vishal Ghadge
Email:		vtghadge@gmail.com
==============================

Building Redirect Windows API:
==============================
1. Unpack the whole archive as-is to an empty directory
2. Open the workspace WinAPIRedirector.sln 
3. Select build->Batch build
4. Uncheck platform ARM, ARM64 of FsFilter(somehow it is not getting removed from configuration manager).
5. Click on Rebuild

Modules:
========

1. FsFilter: Minifilter driver to receive create process notifications, it will send process creation notification to user mode module ProcessNotification.

2. ProcessNotification: User mode module which will communicate with the driver and will get process creation notifications. On process creation notification we will check if the process is configured for injection, if yes then we will inject our hooking Dll DllWinAPIRedirector.dll into that process.

3. DllWinAPIRedirector: This is our Injection DLL which will hook multiple win32 APIs.
	-On init, it will query process information and convert it into a JSON format.
	-Read API redirection directory paths from config.ini i.e) source directory path and redirection directory path.
	-if someone tries to perform any operation on the source directory we will redirect that operation to the redirected directory by hooking win32 APIs.
4. Detours: Detours is a software package for monitoring and instrumenting API calls on Windows. DllWinAPIRedirector Dll is using this library for win32 API hooking.

5. TestInjection: sample tool to try DLL injection into the process.


Config.ini Format:
===========
Config.ini: This is a config file and must be present in the product directory. DllWinAPIRedirector and ProcessNotification modules use this file.

[ProcessName]
Filetest.exe
Notepad.exe

[RedirectionPath]
C:\Users\lenovo\Documents\Important=C:\Users\Public\Documents\Important

ProcessNotification module will read process name([ProcessName]) values from this config file and then inject DLL into these processes only. note: It should only contain process names, not full path.

DllWinAPIRedirector module will read redirection paths([RedirectionPath]) values from this config file, value format must be like =. Note: and must be present in the system otherwise we will not monitor any process.

Usage:
=======
1. Enable test signing mode of the system using command: bcdedit /set testsigning on
2. Launch ProcessNotification.exe
	- process monitoring will start by launching this process.
3. Launch any process configure in ini file for DLL injection.
4. perform operation from newly launch process and observe results.
5. to stop process monitoring: just enter ctrl+c on console.

Testcases:
==========
1. Add FileTest.exe in monitor list and perform different I/O operation on source directory using this tool.
2. Add notepad.txt in monitor list and perform operation from this process.
3. launch dbgview to see json format monitor process information.


	
 
	

