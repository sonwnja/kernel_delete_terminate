/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2015, portions (C) Mark Russinovich, FileMon
*
*  TITLE:       INSTDRV.C
*
*  VERSION:     1.10
*
*  DATE:        10 Mar 2015
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/
#include "global.h"

/*
* scmInstallDriver
*
* Purpose:
*
* Create SCM service entry describing kernel driver.
*
*/
BOOL scmInstallDriver(
	_In_ SC_HANDLE SchSCManager,
	_In_ LPCTSTR DriverName,
	_In_opt_ LPCTSTR ServiceExe
	)
{
	SC_HANDLE  schService;

	schService = CreateService(SchSCManager, // SCManager database
		DriverName,           // name of service
		DriverName,           // name to display
		SERVICE_ALL_ACCESS,    // desired access
		SERVICE_KERNEL_DRIVER, // service type
		SERVICE_DEMAND_START,  // start type
		SERVICE_ERROR_NORMAL,  // error control type
		ServiceExe,            // service's binary
		NULL,                  // no load ordering group
		NULL,                  // no tag identifier
		NULL,                  // no dependencies
		NULL,                  // LocalSystem account
		NULL                   // no password
		);
	if (schService == NULL) 
	{
		OutputDebugStringA("CreateSevice error!");
		return FALSE;
	}

	CloseServiceHandle(schService);
	return TRUE;
}

/*
* scmStartDriver
*
* Purpose:
*
* Start service, resulting in SCM drvier load.
*
*/
BOOL scmStartDriver(
	_In_ SC_HANDLE SchSCManager,
	_In_ LPCTSTR DriverName
	)
{
	SC_HANDLE  schService;
	BOOL       ret;

	schService = OpenService(SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS
		);
	if (schService == NULL)
	{
		OutputDebugStringA("OpenService error!");
		return FALSE;
	}

	ret = StartService(schService, 0, NULL)
		|| GetLastError() == ERROR_SERVICE_ALREADY_RUNNING;

	CloseServiceHandle(schService);

	return ret;
}

/*
* scmOpenDevice
*
* Purpose:
*
* Open driver device by symbolic link.
*
*/
BOOL scmOpenDevice(
	_In_ LPCTSTR DriverName,
	_Inout_opt_ PHANDLE lphDevice
	)
{
	TCHAR    completeDeviceName[64];
	HANDLE   hDevice;

	RtlSecureZeroMemory(completeDeviceName, sizeof(completeDeviceName));
	wsprintf(completeDeviceName, TEXT("\\\\.\\%s"), DriverName);

	hDevice = CreateFile(completeDeviceName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		OutputDebugStringA("scmOpenDevice error!");
		return FALSE;
	}

	if (lphDevice) {
		*lphDevice = hDevice;
	}
	else {
		CloseHandle(hDevice);
	}

	return TRUE;
}

/*
* scmStopDriver
*
* Purpose:
*
* Command SCM to stop service, resulting in driver unload.
*
*/
BOOL scmStopDriver(
	_In_ SC_HANDLE SchSCManager,
	_In_ LPCTSTR DriverName
	)
{
	SC_HANDLE       schService;
	BOOL            ret;
	SERVICE_STATUS  serviceStatus;

	schService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS);
	if (schService == NULL) {
		return FALSE;
	}

	ret = ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
	CloseServiceHandle(schService);

	return ret;
}

/*
* scmRemoveDriver
*
* Purpose:
*
* Remove service entry from SCM database.
*
*/
BOOL scmRemoveDriver(
	_In_ SC_HANDLE SchSCManager,
	_In_ LPCTSTR DriverName
	)
{
	SC_HANDLE  schService;
	BOOL       bResult = FALSE;

	schService = OpenService(SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS
		);

	if (schService == NULL) {
		return bResult;
	}

	bResult = DeleteService(schService);

	CloseServiceHandle(schService);

	return bResult;
}

/*
* scmUnloadDeviceDriver
*
* Purpose:
*
* Combines scmStopDriver and scmRemoveDriver.
*
*/
BOOL scmUnloadDeviceDriver(
	_In_ LPCTSTR Name
	)
{
	SC_HANDLE	schSCManager;
	BOOL		bResult = FALSE;

	if (Name == NULL) {
		return bResult;
	}

	schSCManager = OpenSCManager(NULL,
		NULL,
		SC_MANAGER_ALL_ACCESS
		);
	if (schSCManager) {
		scmStopDriver(schSCManager, Name);
		bResult = scmRemoveDriver(schSCManager, Name);
		CloseServiceHandle(schSCManager);
	}
	return bResult;
}

/*
* scmLoadDeviceDriver
*
* Purpose:
*
* Unload if already exists, Create, Load and Open driver instance.
*
*/
BOOL scmLoadDeviceDriver(
	_In_		LPCTSTR Name,
	_In_opt_	LPCTSTR Path,
	_Inout_		PHANDLE lphDevice
	)
{
	SC_HANDLE	schSCManager;
	BOOL		bResult = FALSE;

	if (Name == NULL) {
		return bResult;
	}

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager) {
		scmRemoveDriver(schSCManager, Name);
		if (!scmInstallDriver(schSCManager, Name, Path))
			OutputDebugStringA("scmInstallDriver error!");
		if (!scmStartDriver(schSCManager, Name))
			OutputDebugStringA("scmStartDriver error!");
		bResult = scmOpenDevice(Name, lphDevice);
		CloseServiceHandle(schSCManager);
	}
	return bResult;
}
