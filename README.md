# WinAPIRedirector
Redirect Windows API 

Modules:
1. FsFilter: Minifilter driver to receive create process notifications, it will send process creation notification to user mode module ProcessNotification.

2. ProcessNotification: User mode module which will communicate with driver and will get process creation notifications.
	On process creation notification we will check if process is configure for injection, if yes then we will inject our hooking dll DllWinAPIRedirector.dll into that process.
	
3. DllWinAPIRedirector: This is our Injection DLL which will hook multiple win32 API's. 
	- On init it will query process information and convert it into a json format.
	- Read API redirection directory paths from config.ini i.e) source directory path and redirection directory path.
	- if someone tries to perform any operation on source directory we will redirect that operation to redirected directory by hooking win32 API's.
	
4. Detours: Detours is a software package for monitoring and instrumenting API calls on Windows. DllWinAPIRedirector dll is using this library for win32 API hooking.

5. TestInjection: sample tool to try DLL injection into process.

Config.ini: This is config file and must be present to product directory. DllWinAPIRedirector and ProcessNotification modules use this file.

Format:

[ProcessName]
Filetest.exe
Notepad.exe

[RedirectionPath]
C:\Users\lenovo\Documents\Important=C:\Users\Public\Documents\Important

ProcessNotification module will read process name([ProcessName]) values from this config file and then inject DLL into these processes only.
note: It should only contain process names not full path.

DllWinAPIRedirector module will read redirection paths([RedirectionPath]) values from this config file, value format must be like <source dir path>=<redirect dir path>.
Note: <source dir path> and <redirect dir path> must be present in the system otherwise we will not monitor any process.

Usage:

- Enable test signing mode of system using command : bcdedit /set testsigning on
- Launch ProcessNotification.exe 
	- process monitoring will start by launching this process.
- Launch any process configure in ini file for DLL injection.
- perform operation from newly launch process and observe results.
- to stop process monitoring:
	just enter ctrl+c on console.
	
Testcases:
1. Add FileTest.exe in monitor list and perform different I/O operation on source directory using this tool.
2. Add notepad.txt in monitor list and perform operation from this process.
3. launch dbgview to see json format monitor process information. 


	
 
	

