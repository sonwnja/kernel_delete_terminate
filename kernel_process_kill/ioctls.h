#ifndef IOCTLS_H
#define IOCTLS_H

#ifndef CTL_CODE
#pragma message("CTL_CODE undefined. Include winioctl.h or wdm.h")
#endif

#define TERMINATE_PROCESS CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x800, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)

#define DELETE_FILE CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x801, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)

#define WIPE_FILE CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x802, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)

#endif
