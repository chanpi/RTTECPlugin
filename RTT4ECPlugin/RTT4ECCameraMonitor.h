#ifndef __RTT4EC_CAMERA_MONITOR__
#define __RTT4EC_CAMERA_MONITOR__

#include "stdafx.h"

typedef struct RTTContext
{
	SOCKET socketHandler;
	struct sockaddr_in address;

	int move;
	int angle;
	int pitch;
	int height;
	int x;
	int y;
	int z;
	int p;
	int h;
	int r;
	int speed;
} RTTContext;

#ifdef __cplusplus
extern "C" {
#endif

	unsigned int __stdcall RTT4ECCameraMonitorThreadProc(void *pParam);
	void RTT4ECFilter(LPSTR lpszCommand, void* context);

#ifdef __cplusplus
}
#endif

#endif /* __RTT4EC_CAMERA_MONITOR__ */