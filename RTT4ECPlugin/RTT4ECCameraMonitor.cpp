#include "stdafx.h"
#include "RTT4ECCameraMonitor.h"
#include "RTT4ECController.h"
#include "RTT4ECAccessor.h"
#include "Miscellaneous.h"

static const int BUFFER_SIZE = 256;
static const int MONITORING_INTERVAL = 100;

static void RTT4ECParseCommand(LPSTR lpszCommand, RTT4ECContext* pRtt4ecContext);

unsigned int __stdcall RTT4ECCameraMonitorThreadProc(void *pParam)
{
	RTTContext *pContext = (RTTContext*)pParam;
	CHAR szRecvBuffer[BUFFER_SIZE] = {0};
	CHAR szCameraInfomation[BUFFER_SIZE] = {0};
	TCHAR szError[BUFFER_SIZE];

	// ソケットにタイムアウトオプションを付け、タイムアウトしたときにのみ
	// CAMERA情報をRepackする
	timeval socketTimeout;
	socketTimeout.tv_sec	= MONITORING_INTERVAL;
	socketTimeout.tv_usec	= 0;
	setsockopt(pContext->pRtt4ecContext->socketHandler, SOL_SOCKET, SO_RCVTIMEO, (const char*)&socketTimeout, sizeof(socketTimeout));

	while(pContext->bAlive){
		if (!pContext->pAccessor->RTT4ECRecv(pContext->pRtt4ecContext, szRecvBuffer, sizeof(szRecvBuffer)) ) {	// カメラ情報を受信する
			if (GetLastError() ==  WSAETIMEDOUT && szCameraInfomation) {
				RTT4ECParseCommand(szCameraInfomation, pContext->pRtt4ecContext);
				continue;
			}
			_stprintf_s(szError, _countof(szError), _T("[ERROR] recv() is failed. Error No. %d"), GetLastError());
			LogDebugMessage(Log_Error, szError);
		} else {
			char* p = strstr(szRecvBuffer, "EVENT");
			if (p != NULL && strchr(p, '\0')) {
				strcpy_s(szCameraInfomation, _countof(szCameraInfomation), p);
			}
		}
	}
	return TRUE;
}

// EVENT CAMERA_POSITION -423.29 -709.225 739.015 CAMERA_DIRECTION 0.390573 0.681535 -0.61884 CAMERA_PHR 38.2315 150.184 -6.52136e-06
static void RTT4ECParseCommand(LPSTR lpszCommand, RTT4ECContext* pRtt4ecContext) {
	char	*tempChar = NULL;
	int		i = 0;
	char	*ctx = NULL;
	char	buffer[BUFFER_SIZE] = {0};

	memcpy(buffer, lpszCommand, _countof(buffer));

	tempChar = strtok_s( buffer, " ", &ctx);
	while ( tempChar != NULL ) {
		tempChar = strtok_s( NULL," ", &ctx);
		if ( tempChar != NULL ){
			switch(i){
			case 1:
				pRtt4ecContext->x = atof(tempChar);
				break;
			case 2:
				pRtt4ecContext->y = atof(tempChar);
				break;
			case 3:
				pRtt4ecContext->z = atof(tempChar);
				break;
			case 9:
				pRtt4ecContext->p = atof(tempChar);
				break;
			case 10:
				pRtt4ecContext->h = atof(tempChar);
				break;
			case 11:
				pRtt4ecContext->r = atof(tempChar);
				break;
			}
			i++;
		}
	}
}