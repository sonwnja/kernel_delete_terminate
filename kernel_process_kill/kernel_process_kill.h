#include <ntifs.h>
#include <ntstrsafe.h>
#include "ioctls.h"

typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT pDevObj;
	UNICODE_STRING usDevName;
	UNICODE_STRING usSymbolName;
	PUCHAR buffer;
	ULONG file_length;
}DEVICE_EXTENSION, * PDEVICE_EXTENSION;

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))
#define MAX_FILE_LENGTH 1024
#define DEVICE_NAME	L"\\Device\\ProcessTerminateDevice"
#define SYM_LINK_NAME L"\\??\\ProcessTerminateDevice"

VOID Unload(IN PDRIVER_OBJECT pDriverObject);						// 卸载例程
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject);				// 封装设备创建函数
NTSTATUS DispatchRoutin(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp);	// 通用派遣函数
NTSTATUS DeviceIOControl(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp);	// IO控制函数（与ring3通信）
NTSTATUS TerminateProcess(IN ULONG ProcessID);						// 进程结束函数
NTSTATUS WipeFile(IN wchar_t* path);								// 文件擦除函数 