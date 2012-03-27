#include "stdafx.h"
#include "RTT4ECCameraMonitor.h"
#include "RTT4ECController.h"
#include "RTT4ECAccessor.h"
#include "Miscellaneous.h"

static const int BUFFER_SIZE = 256;
static const int MONITORING_INTERVAL = 1000;

static void RTT4ECFilter(LPSTR lpszCommand, RTT4ECContext* pRtt4ecContext);
static void RTT4ECParseCommand(LPCSTR lpszCommand, RTT4ECContext* pRtt4ecContext);

unsigned int __stdcall RTT4ECCameraMonitorThreadProc(void *pParam)
{
	RTTContext *pContext = (RTTContext*)pParam;
	CHAR szRecvBuffer[BUFFER_SIZE] = {0};
	TCHAR szError[BUFFER_SIZE];

	while(pContext->bAlive){
		Sleep(MONITORING_INTERVAL);

		//if(pContext->bIsFreshly){
		//	pContext->bIsFreshly = false;
			if (!pContext->pAccessor->RTT4ECRecv(pContext->pRtt4ecContext, szRecvBuffer, sizeof(szRecvBuffer)) ) {	// ƒJƒƒ‰î•ñ‚ðŽóM‚·‚é
				_stprintf_s(szError, _countof(szError), _T("[ERROR] recv() is failed. Error No. %d"), GetLastError());
				LogDebugMessage(Log_Error, szError);
			} else {
				RTT4ECFilter((LPSTR)szRecvBuffer, pContext->pRtt4ecContext);
			}
		//}
	}
	return TRUE;
}

void RTT4ECFilter(LPSTR lpszCommand, RTT4ECContext* pRtt4ecContext)
{
	if (memcmp("EVENT", lpszCommand, 5) == 0) {
		RTT4ECParseCommand((LPCSTR)lpszCommand, pRtt4ecContext);
	}
}

// EVENT CAMERA_POSITION -423.29 -709.225 739.015 CAMERA_DIRECTION 0.390573 0.681535 -0.61884 CAMERA_PHR 38.2315 150.184 -6.52136e-06
static void RTT4ECParseCommand(LPCSTR lpszCommand, RTT4ECContext* pRtt4ecContext) {
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
			case 2:
				pRtt4ecContext->x = (int)atof(tempChar);
				break;
			case 3:
				pRtt4ecContext->y = (int)atof(tempChar);
				break;
			case 4:
				pRtt4ecContext->z = (int)atof(tempChar);
				break;
			case 10:
				pRtt4ecContext->p = (int)atof(tempChar);
				break;
			case 11:
				pRtt4ecContext->h = (int)atof(tempChar);
				break;
			case 12:
				pRtt4ecContext->r = (int)atof(tempChar);
				break;
			}
			i++;
		}
	}
}