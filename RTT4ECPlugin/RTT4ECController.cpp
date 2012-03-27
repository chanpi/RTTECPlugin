#include "StdAfx.h"
#include "I4C3DCommon.h"
#include "RTT4ECController.h"
#include "RTT4ECAccessor.h"
#include "I4C3DCursor.h"
#include "RTT4ECCameraMonitor.h"
#include "Miscellaneous.h"
#include <math.h>
#include <float.h>
#include <process.h>

namespace {
	extern const int BUFFER_SIZE = 256;
	const PCSTR INITIALIZE_MESSAGE	= "SUBSCRIBE CAMERA ;";
	RTT4ECAccessor g_accessor;
	RTT4ECContext g_rtt4ecContext;

	RTTContext g_context;
};

RTT4ECController::RTT4ECController(void)
{
	m_pCursor		= new I4C3DCursor;
}

RTT4ECController::~RTT4ECController(void)
{
	delete m_pCursor;
	m_pCursor = NULL;
}

/**
 * @brief
 * Controlオブジェクトの初期化を行います。
 * 
 * @returns
 * 初期化に成功した場合にはTRUE、失敗した場合にはFALSEを返します。
 * 
 * Controlオブジェクトの初期化を行います。
 * どのControlオブジェクトかは、引数のI4C3DContextポインタに登録されているControl派生クラスによります。
 * 
 * @remarks
 * InitializeModifierKeys()で修飾キーの設定を行います。
 * 
 * @see
 * InitializeModifierKeys()
 */

BOOL RTT4ECController::Initialize(LPCSTR szBuffer, char* termination, USHORT uRTTPort)
{
	char tmpCommand[BUFFER_SIZE] = {0};
	char szModKeys[BUFFER_SIZE] = {0};
	double tumbleRate, trackRate, dollyRate;

	g_context.bAlive = true;
	sscanf_s(szBuffer, g_initCommandFormat, tmpCommand,	sizeof(tmpCommand), szModKeys, sizeof(szModKeys), &tumbleRate, &trackRate, &dollyRate, termination, sizeof(*termination));
	m_cTermination = *termination;

	// RTTContextの初期化
	g_context.pAccessor = &g_accessor;
	g_context.pRtt4ecContext = &g_rtt4ecContext;

	// TCPソケットの作成
	g_rtt4ecContext.socketHandler = g_accessor.InitializeTCPSocket(&g_rtt4ecContext.address, "127.0.0.1", TRUE, uRTTPort);
	if (g_rtt4ecContext.socketHandler == INVALID_SOCKET) {
		LogDebugMessage(Log_Error, _T("InitializeSocket <RTT4ECController::Initialize>"));
		UnInitialize();
		return FALSE;
	}
	if (!g_accessor.SetConnectingSocket(g_rtt4ecContext.socketHandler, &g_rtt4ecContext.address)) {
		UnInitialize();
		return FALSE;
	}

	// CAMERA情報取得のためのメッセージ送信
	if (!g_accessor.RTT4ECSend(&g_rtt4ecContext, INITIALIZE_MESSAGE)) {
		UnInitialize();
		return FALSE;
	}

	// カメラ情報受信用スレッドを作成
	g_context.hCameraMonitorThread = (HANDLE)_beginthreadex(NULL, 0, RTT4ECCameraMonitorThreadProc, &g_context, CREATE_SUSPENDED, &g_context.uCameraMonitorThreadID);
	if (g_context.hCameraMonitorThread == INVALID_HANDLE_VALUE) {
		UnInitialize();
		return FALSE;
	}
	ResumeThread(g_context.hCameraMonitorThread);

	return TRUE;
}

void RTT4ECController::UnInitialize(void)
{
	g_context.bAlive = false;

	if (g_rtt4ecContext.socketHandler != INVALID_SOCKET) {
		closesocket(g_rtt4ecContext.socketHandler);
		g_rtt4ecContext.socketHandler = INVALID_SOCKET;
	}

	if (g_context.hCameraMonitorThread != NULL) {
		CloseHandle(g_context.hCameraMonitorThread);
		g_context.hCameraMonitorThread = NULL;
	}
}

void RTT4ECController::Execute(HWND /*hWnd*/, LPCSTR szCommand, double /*deltaX*/, double /*deltaY*/)
{
	OriginalCommandExecute(szCommand);
}

void RTT4ECController::OriginalCommandExecute(LPCSTR command)
{
	char termination;
	float x, y, z, p, h, r;
	char message[BUFFER_SIZE] = {0};

	if (sscanf_s(command, g_cameraCommandFormat, &x, &y, &z, &p, &h, &r, &termination, sizeof(termination)) != 7) {
		strcpy_s(message, _countof(message), command);
		char* pos = strchr(message, m_cTermination);
		if (pos != NULL) {
			*pos = '\0';
		}
		g_accessor.RTT4ECSend(&g_rtt4ecContext, message);
		return;
	}

	sprintf_s(message, _countof(message), g_cameraCommandFormat,
		x + g_rtt4ecContext.x, y + g_rtt4ecContext.y, z + g_rtt4ecContext.z,
		p + g_rtt4ecContext.p, h + g_rtt4ecContext.h, r + g_rtt4ecContext.r, '\0');

	g_accessor.RTT4ECSend(&g_rtt4ecContext, message);
}
