#include "stdafx.h"
#include "RTT4ECCameraMonitor.h"
#include "Miscellaneous.h"

static const int BUFFER_SIZE = 256;
static const int MONITORING_INTERVAL = 1000;

unsigned int __stdcall RTT4ECCameraMonitorThreadProc(void *pParam)
{
	//RTTContext *pContext = (RTTContext*)pParam;
	//CHAR szSendBuffer[BUFFER_SIZE] = "SUBSCRIBE CAMERA";	// "SUBSCRIBE CAMERA\0"
	//CHAR szRecvBuffer[BUFFER_SIZE] = {0};
	//TCHAR szError[BUFFER_SIZE];

	//// �\�P�b�g�ʐM�̊m�����Ɉ�x��������
	//if (!RBSend(context, szSendBuffer)) {	// ��΍��W���擾����R�}���h�𑗐M
	//	_stprintf_s(szError, _countof(szError), _T("[ERROR] send() is failed. Error No. %d"), GetLastError());
	//	LogDebugMessage(Log_Error, szError);
	//}

	//while(context->bIsAlived){
	//	Sleep(MONITORING_INTERVAL);

	//	if(context->bIsFreshly){
	//		context->bIsFreshly = false;
	//		if (!RBRecv(context, szRecvBuffer, sizeof(szRecvBuffer)) ) {	// �J����������M����
	//			_stprintf_s(szError, _countof(szError), _T("[ERROR] recv() is failed. Error No. %d"), GetLastError());
	//			LogDebugMessage(Log_Error, szError);
	//		} else {
	//			RTT4ECFilter((LPSTR)szRecvBuffer, context);
	//		}
	//	}
	//}
	return TRUE;
}

void RTT4ECFilter(LPSTR lpszCommand, RTTContext* pContext)
{

}
