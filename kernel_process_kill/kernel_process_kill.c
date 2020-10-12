#include "kernel_process_kill.h"
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	KdPrint(("[*]enter DriverEntry\n"));
	NTSTATUS status;

	pDriverObject->DriverUnload = Unload;

	for (int i = 0; i < arraysize(pDriverObject->MajorFunction); ++i)
	{
		pDriverObject->MajorFunction[i] = DispatchRoutin;
	}
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIOControl;
	status = CreateDevice(pDriverObject);
	if (NT_SUCCESS(status))
		KdPrint(("[*]success in CreateDevice"));
	else
		KdPrint(("[*]error in CreateDevice with code 0x%X", status));
	return status;
}

VOID Unload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pNextDevice;
	KdPrint(("[*]enter DriverUnload\n"));
	pNextDevice = pDriverObject->DeviceObject;
	while (pNextDevice != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextDevice->DeviceExtension;
		IoDeleteSymbolicLink(&pDevExt->usSymbolName);
		if (pDevExt->buffer)
		{
			ExFreePoolWithTag(pDevExt->buffer, (ULONG)"DIP");
			pDevExt->buffer = NULL;
		}
		pNextDevice = pDevExt->pDevObj;
	}
}

NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("[*]enter CreateDevice"));
	NTSTATUS status;
	UNICODE_STRING devName;
	UNICODE_STRING symbolName;
	PDEVICE_EXTENSION pDevExt;
	PDEVICE_OBJECT pDevObj;

	RtlInitUnicodeString(&devName, DEVICE_NAME);
	RtlInitUnicodeString(&symbolName, SYM_LINK_NAME);

	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0, TRUE,
		&pDevObj);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("[!]error in IoCreateDevice with code 0x%X"));
		return status;
	}
	pDevObj->Flags |= DO_DIRECT_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->usDevName = devName;
	pDevExt->usSymbolName = symbolName;
	pDevExt->buffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool, MAX_FILE_LENGTH, (ULONG)"DIP");
	pDevExt->file_length = 0;
	status = IoCreateSymbolicLink(&symbolName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		KdPrint(("[!]error in IoCreateSymbolicLink with code 0x%X"));
		return status;
	}
	KdPrint(("[*]leave CreateDevice\n"));
	return STATUS_SUCCESS;
}

NTSTATUS DispatchRoutin(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("[*]enter DispatchRoutin"));

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	static char* irpname[] =
	{
		"IRP_MJ_CREATE",
		"IRP_MJ_CREATE_NAMED_PIPE",
		"IRP_MJ_CLOSE",
		"IRP_MJ_READ",
		"IRP_MJ_WRITE",
		"IRP_MJ_QUERY_INFORMATION",
		"IRP_MJ_SET_INFORMATION",
		"IRP_MJ_QUERY_EA",
		"IRP_MJ_SET_EA",
		"IRP_MJ_FLUSH_BUFFERS",
		"IRP_MJ_QUERY_VOLUME_INFORMATION",
		"IRP_MJ_SET_VOLUME_INFORMATION",
		"IRP_MJ_DIRECTORY_CONTROL",
		"IRP_MJ_FILE_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CONTROL",
		"IRP_MJ_INTERNAL_DEVICE_CONTROL",
		"IRP_MJ_SHUTDOWN",
		"IRP_MJ_LOCK_CONTROL",
		"IRP_MJ_CLEANUP",
		"IRP_MJ_CREATE_MAILSLOT",
		"IRP_MJ_QUERY_SECURITY",
		"IRP_MJ_SET_SECURITY",
		"IRP_MJ_POWER",
		"IRP_MJ_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CHANGE",
		"IRP_MJ_QUERY_QUOTA",
		"IRP_MJ_SET_QUOTA",
		"IRP_MJ_PNP",
	};

	UCHAR type = stack->MajorFunction;
	if (type >= arraysize(irpname))
		KdPrint(("[*]unknown IRP, major type %X\n", type));
	else
		KdPrint(("[*]get IRP %s", irpname[type]));

	NTSTATUS status = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("[*]leave DispatchRoutin\n"));
	return status;
}

NTSTATUS DeviceIOControl(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	NTSTATUS Status = STATUS_SUCCESS;
	KdPrint(("[*]enter DeviceIOControl\n"));

	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG ControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
	ULONG Input_Lenght = Stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG Output_Lenght = Stack->Parameters.DeviceIoControl.OutputBufferLength;

	UNICODE_STRING return_str = { 0 };
	ULONG info = 0;
	switch (ControlCode)
	{
	case TERMINATE_PROCESS:
	{
		ULONG* ProcessID = (ULONG*)pIrp->AssociatedIrp.SystemBuffer;
		KdPrint(("[*]recv ControlCode TERMINATE_PROCESS %d\n", *ProcessID));
		Status = TerminateProcess(*ProcessID);
		if (NT_SUCCESS(Status)) {
			KdPrint(("[*]success in TerminateProcess"));
			RtlInitUnicodeString(&return_str, L"TERMINATE SUCCESS");
		}
		else {
			KdPrint(("[!]success in TerminateProcess with code 0x%X", Status));
			RtlInitUnicodeString(&return_str, L"TERMINATE ERROR");
		}
		break;
	}
	case WIPE_FILE:
	{
		WCHAR* FilePath = ExAllocatePoolWithTag(NonPagedPool, Input_Lenght, "HTAP");
		RtlCopyMemory(FilePath, pIrp->AssociatedIrp.SystemBuffer, Input_Lenght);
		KdPrint(("[*]recv ControlCode WIPE_FILE %S\n", FilePath));
		Status = WipeFile(FilePath);		
		if (NT_SUCCESS(Status)) {
			KdPrint(("[*]success in WipeFile"));
			RtlInitUnicodeString(&return_str, L"WIPE SUCCESS");
		}
		else {
			KdPrint(("[!]success in WipeFile with code 0x%X", Status));
			RtlInitUnicodeString(&return_str, L"WIPE ERROR");
		}
		break;
	}
	default:
		Status = STATUS_INVALID_VARIANT;
	}

	// 完成IRP
	RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, return_str.Buffer, return_str.MaximumLength);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = return_str.MaximumLength;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("[*]leave DeviceIOControl"));

	return Status;
}

NTSTATUS TerminateProcess(IN ULONG ProcessID) {
	NTSTATUS Status = STATUS_SUCCESS;
	KdPrint(("[*]enter TerminateProcess\n"));
	HANDLE ProcessHandle; 
	OBJECT_ATTRIBUTES ObjectAttributes;
	CLIENT_ID ClientId;
	memset(&ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
	ClientId.UniqueProcess = ProcessID;
	ClientId.UniqueThread = 0;
	Status = ZwOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &ObjectAttributes, &ClientId); 
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[!]error in ZwOpenProcess with pid %d with code 0x%X", ProcessID, Status));
		ZwClose(ProcessHandle);
		return Status;
	}
	KdPrint(("[*]success in ZwOpenProcess with pid %X", ProcessID));

	Status = ZwTerminateProcess(ProcessHandle, -1); 
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("[!]error in ZwTerminateProcess with code 0x%X", Status));
		ZwClose(ProcessHandle);
		return Status;
	}

	KdPrint(("[*]success in ZwTerminateProcess"));
	ZwClose(ProcessHandle);
	return Status;
}

NTSTATUS WipeFile(IN wchar_t* path) {
	IO_STATUS_BLOCK ioStatusBlock;
	HANDLE fileHandle;
	NTSTATUS Status;
	IO_STATUS_BLOCK ioBlock;
	DEVICE_OBJECT* device_object = NULL;
	void* object = NULL;
	OBJECT_ATTRIBUTES fileObject;
	UNICODE_STRING uPath;
	size_t  cb;
	PUCHAR buffer;
	BOOLEAN hasWriteAccess = TRUE;
	PEPROCESS eproc = PsGetCurrentProcess();
	int fileLength = 0;
	KeAttachProcess(eproc);
	RtlInitUnicodeString(&uPath, path);
	FILE_STANDARD_INFORMATION fsi = { 0 };
	if (KeGetCurrentIrql() != PASSIVE_LEVEL)
	{
		KdPrint(("[!]IRQL levels too high\r\n"));
		return STATUS_INVALID_DEVICE_STATE;
	}

	InitializeObjectAttributes(&fileObject,
		&uPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	Status = IoCreateFileSpecifyDeviceObjectHint(
		&fileHandle,
		GENERIC_ALL,
		&fileObject,
		&ioBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_OVERWRITE,
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
		0,
		0,
		CreateFileTypeNone,
		0,
		IO_IGNORE_SHARE_ACCESS_CHECK,
		device_object);
	if (Status != STATUS_SUCCESS)
	{
		KdPrint(("[!]error in IoCreateFileSpecifyDeviceObjectHint with GENERIC_ALL with 0x%X", Status));
		Status = IoCreateFileSpecifyDeviceObjectHint(
			&fileHandle,
			SYNCHRONIZE | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | FILE_READ_DATA,
			&fileObject,
			&ioBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			0,
			0,
			CreateFileTypeNone,
			0,
			IO_IGNORE_SHARE_ACCESS_CHECK,
			device_object); 
		if (Status != STATUS_SUCCESS)
		{
			KdPrint(("[!]error in IoCreateFileSpecifyDeviceObjectHint with READ_ACCESS with 0x%X", Status));
			goto _End;
		}
		hasWriteAccess = FALSE;
	}
	if(hasWriteAccess)
		KdPrint(("[*]success in IoCreateFileSpecifyDeviceObjectHint with WRITE_ACCESS"));
	else
		KdPrint(("[*]success in IoCreateFileSpecifyDeviceObjectHint with READ_ACCESS"));
	Status = ObReferenceObjectByHandle(fileHandle, 0, 0, 0, &object, 0);
	if (Status != STATUS_SUCCESS)
	{
		KdPrint(("[*]success in ObReferenceObjectByHandle"));
		ZwClose(fileHandle);
		goto _End;
	}

	((FILE_OBJECT*)object)->SectionObjectPointer->ImageSectionObject = 0;
	((FILE_OBJECT*)object)->DeleteAccess = 1;
	((FILE_OBJECT*)object)->WriteAccess = 1; 
	if (hasWriteAccess)
	{
		Status = ZwQueryInformationFile(fileHandle, &ioStatusBlock, &fsi, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
		if (!NT_SUCCESS(Status))
		{
			ZwClose(fileHandle);
			KdPrint(("[!]error in ZwQueryInformationFile with 0x%X", Status));
			goto _End;
		}
		fileLength = fsi.EndOfFile.QuadPart;
		buffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool, MAX_FILE_LENGTH, (ULONG)"OREZ");
		if (buffer == NULL) {
			KdPrint(("[!]error in ExAllocatePoolWithTag"));
			goto _End;
		}
		RtlZeroMemory(buffer, MAX_FILE_LENGTH);
		int fileOffset = 0;
		int i = 0;
		while (fileOffset < fileLength) {
			i++;
			cb = min(MAX_FILE_LENGTH, fileLength - fileOffset);		// 期望写入大小
			Status = ZwWriteFile(fileHandle, NULL, NULL, NULL, &ioStatusBlock, buffer, cb, fileOffset, NULL);
			if (!NT_SUCCESS(Status))
			{
				ZwClose(fileHandle);
				KdPrint(("[!]error in ZwWriteFile with 0x%X", Status));
				goto _End;
			}
			fileOffset += ioStatusBlock.Information;
			KdPrint(("[*]success in ZwWriteFile with %d bytes and %d times", i, ioStatusBlock.Information));
		}
		KdPrint(("[*]finished in ZwWriteFile"));
	}
	Status = ZwDeleteFile(&fileObject);

	ObDereferenceObject(object);
	ZwClose(fileHandle);

	if (Status != STATUS_SUCCESS)
	{
		KdPrint(("[!]error in ZwDeleteFile with 0x%X", Status));
	}

_End:
	KeDetachProcess();
	return Status;
}
