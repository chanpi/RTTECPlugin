#include "stdafx.h"
#include "RTT4ECCameraMonitor.h"
#include "RTT4ECController.h"
#include "RTT4ECAccessor.h"
#include "I4C3DCommon.h"
#include "DataNotifier.h"
#include "Miscellaneous.h"

static const int BUFFER_SIZE = 256;
static const int MONITORING_INTERVAL = 100;

static void RTT4ECParseCommand(LPSTR lpszCommand, RTTContext* pContext);

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
				RTT4ECParseCommand(szCameraInfomation, pContext);
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
static void RTT4ECParseCommand(LPSTR lpszCommand, RTTContext* pContext) {
	char	*tempChar = NULL;
	int		i = 0;
	char	*ctx = NULL;
	char	buffer[BUFFER_SIZE] = {0};

	if (!pContext->bAlive) {
		return;
	}

	memcpy(buffer, lpszCommand, _countof(buffer));


	EnterCriticalSection(&pContext->lockObject);
	tempChar = strtok_s( buffer, " ", &ctx);
	while ( tempChar != NULL ) {
		tempChar = strtok_s( NULL," ", &ctx);
		if ( tempChar != NULL ){
			switch(i){
			case 1:
				pContext->pRtt4ecContext->x = (float)atof(tempChar);
				break;
			case 2:
				pContext->pRtt4ecContext->y = (float)atof(tempChar);
				break;
			case 3:
				pContext->pRtt4ecContext->z = (float)atof(tempChar);
				{
					// 通知するためにデータを詰める
					int height = (int)pContext->pRtt4ecContext->z + 15;
					pContext->notifyData.bodyHeight[0] = height & 0xFF;
					pContext->notifyData.bodyHeight[1] = (height >> 8) & 0xFF;
					pContext->notifyData.bodyHeight[2] = (height >> 16) & 0xFF;
					pContext->notifyData.bodyHeight[3] = (height >> 24) & 0xFF;
					pContext->pNotifier->Notify(&pContext->notifyData);

					TCHAR szBuffer[32];
					_stprintf_s(szBuffer, 32, _T("height = %d\n"), height);
					OutputDebugString(szBuffer);
				}
				break;
			case 9:
				pContext->pRtt4ecContext->p = (float)atof(tempChar);
				break;
			case 10:
				pContext->pRtt4ecContext->h = (float)atof(tempChar);
				break;
			case 11:
				pContext->pRtt4ecContext->r = (float)atof(tempChar);
				break;
			}
			i++;
			//if (11 < ++i) {
			//	break;
			//}
		}
	}
	LeaveCriticalSection(&pContext->lockObject);
}