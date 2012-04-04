#include "stdafx.h"
#include "RTTECCameraMonitor.h"
#include "RTTECController.h"
#include "RTTECAccessor.h"
#include "I4C3DCommon.h"
#include "DataNotifier.h"
#include "Misc.h"
#include "SharedConstants.h"

#if UNICODE || _UNICODE
static LPCTSTR g_FILE = __FILEW__;
#else
static LPCTSTR g_FILE = __FILE__;
#endif

static const int BUFFER_SIZE = 256;
static const int MONITORING_INTERVAL = 80;

static void RTTECParseCommand(LPSTR lpszCommand, RTTContext* pContext);

unsigned int __stdcall RTTECCameraMonitorThreadProc(void *pParam)
{
	RTTContext *pContext = (RTTContext*)pParam;
	CHAR szRecvBuffer[BUFFER_SIZE] = {0};
	CHAR szCameraInfomation[BUFFER_SIZE] = {0};

	// �\�P�b�g�Ƀ^�C���A�E�g�I�v�V������t���A�^�C���A�E�g�����Ƃ��ɂ̂�
	// CAMERA����Repack����
	timeval socketTimeout;
	socketTimeout.tv_sec	= MONITORING_INTERVAL;
	socketTimeout.tv_usec	= 0;
	setsockopt(pContext->pRttecContext->socketHandler, SOL_SOCKET, SO_RCVTIMEO, (const char*)&socketTimeout, sizeof(socketTimeout));

	while(pContext->bAlive){
		if (!pContext->pAccessor->RTTECRecv(pContext->pRttecContext, szRecvBuffer, sizeof(szRecvBuffer)) ) {	// �J����������M����
			if (GetLastError() ==  WSAETIMEDOUT && szCameraInfomation) {
				RTTECParseCommand(szCameraInfomation, pContext);
				continue;
			}			
			LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_RECV), GetLastError(), g_FILE, __LINE__);
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
static void RTTECParseCommand(LPSTR lpszCommand, RTTContext* pContext) {
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
				pContext->pRttecContext->x = (float)atof(tempChar);
				break;
			case 2:
				pContext->pRttecContext->y = (float)atof(tempChar);
				break;
			case 3:
				pContext->pRttecContext->z = (float)atof(tempChar);
				{
					// �ʒm���邽�߂Ƀf�[�^���l�߂�
					pContext->pNotifier->Int2Byte((int)pContext->pRttecContext->z, pContext->notifyData.bodyHeight);
					pContext->pNotifier->Notify(&pContext->notifyData);

					//TCHAR szBuffer[32];
					//_stprintf_s(szBuffer, 32, _T("height = %d\n"), (int)pContext->pRttecContext->z);
					//OutputDebugString(szBuffer);
				}
				break;
			case 9:
				pContext->pRttecContext->p = (float)atof(tempChar);
				break;
			case 10:
				pContext->pRttecContext->h = (float)atof(tempChar);
				break;
			case 11:
				pContext->pRttecContext->r = (float)atof(tempChar);
				break;
			}
			if (11 < ++i) {
				break;
			}
		}
	}
	LeaveCriticalSection(&pContext->lockObject);
}