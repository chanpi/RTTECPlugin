#include "StdAfx.h"
#include "I4C3DCommon.h"
#include "RTT4ECController.h"
#include "RTT4ECAccessor.h"
#include "I4C3DCursor.h"
#include "RTT4ECCameraMonitor.h"
#include "DataNotifier.h"
#include "Miscellaneous.h"
#include <math.h>
#include <float.h>
#include <process.h>

namespace {
	extern const int BUFFER_SIZE = 256;
	const float M_PI = 3.14159;
	const PCSTR INITIALIZE_MESSAGE	= "SUBSCRIBE CAMERA ;";
	RTT4ECAccessor g_accessor;
	RTT4ECContext g_rtt4ecContext = {0};
	DataNotifier g_dataNotifier;

	RTTContext g_context = {0};
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

BOOL RTT4ECController::Initialize(LPCSTR szBuffer, char* termination, USHORT uRTTPort, USHORT uNotifyPort)
{
	char tmpCommand[BUFFER_SIZE] = {0};
	char szModKeys[BUFFER_SIZE] = {0};
	double tumbleRate, trackRate, dollyRate;

	// bAlive = trueにすることでプログラムが稼働出来る状態にする
	g_context.bAlive = true;
	sscanf_s(szBuffer, g_initCommandFormat, tmpCommand,	sizeof(tmpCommand), szModKeys, sizeof(szModKeys), &tumbleRate, &trackRate, &dollyRate, termination, sizeof(*termination));
	m_cTermination = *termination;

	// RTT4ECContextが同時に修正されるのを防ぐ
	InitializeCriticalSection(&g_context.lockObject);

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

	// コアへの通知用クラスを初期化
	g_dataNotifier.Initialize("127.0.0.1", uNotifyPort);
	g_context.pNotifier = &g_dataNotifier;

	// CAMERA情報取得のためのメッセージ送信(初回のみこのコマンドを送信する)
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
	WaitForSingleObject(g_context.hCameraMonitorThread, INFINITE);

	if (g_rtt4ecContext.socketHandler != INVALID_SOCKET) {
		closesocket(g_rtt4ecContext.socketHandler);
		g_rtt4ecContext.socketHandler = INVALID_SOCKET;
	}

	if (g_context.hCameraMonitorThread != NULL) {
		CloseHandle(g_context.hCameraMonitorThread);
		g_context.hCameraMonitorThread = NULL;
	}

	g_dataNotifier.UnInitialize();
	DeleteCriticalSection(&g_context.lockObject);
}

void RTT4ECController::Execute(HWND /*hWnd*/, LPCSTR szCommand, double /*deltaX*/, double /*deltaY*/)
{
	OriginalCommandExecute(szCommand);
}

void RTT4ECController::OriginalCommandExecute(LPCSTR command)
{
	char termination;
	static float ox = 0., oy = 0., oz = 0., op = 0., oh = 0., or = 0.;
	float x = 0., y = 0., z = 0., p = 0., h = 0., r = 0.;
	char message[BUFFER_SIZE] = {0};

	if (sscanf_s(command, g_cameraCommandFormat, &x, &y, &z, &p, &h, &r, &termination, sizeof(termination)) != 7) {
		strcpy_s(message, _countof(message), command);
		char* pos = strchr(message, m_cTermination);
		if (pos != NULL) {
			*pos = ' ';
		}
		RemoveWhiteSpaceA(message);
		g_accessor.RTT4ECSend(&g_rtt4ecContext, message);
		return;
	}

	EnterCriticalSection(&g_context.lockObject);

	g_rtt4ecContext.z += z-oz;
	g_rtt4ecContext.p += p-op;
	g_rtt4ecContext.h += h-oh;
	g_rtt4ecContext.r += r-or;

	{
		float sinval = 0., cosval = 0.;	
		float deltaX = x-ox, deltaY = y-oy;
		float move;
		sinval = sin(M_PI * h / 180);
		cosval = cos(M_PI * h / 180);
		// 0除算防止
		move = (fabs(sinval - 0.0) > DBL_EPSILON) ? fabs(deltaX / sinval) : fabs(deltaY / cosval);

		if (deltaX * sinval > 0 || (fabs(g_rtt4ecContext.h - 0.0) > DBL_EPSILON && deltaY * cosval < 0)) {
			//OutputDebugString(_T("forward_x\n"));
			g_rtt4ecContext.x += move * sin(M_PI * g_rtt4ecContext.h / 180);
		} else if (deltaX * sinval < 0 || (fabs(g_rtt4ecContext.h - 0.0) > DBL_EPSILON && deltaY * cosval > 0)) {
			//OutputDebugString(_T("backward_x\n"));
			g_rtt4ecContext.x -= move * sin(M_PI * g_rtt4ecContext.h / 180);
		} else {
			g_rtt4ecContext.x += deltaX;
		}

		if (deltaY * cosval > 0) {
			//OutputDebugString(_T("backward_y\n"));
			g_rtt4ecContext.y += move * cos(M_PI * g_rtt4ecContext.h / 180);
		} else if (deltaY * cosval < 0) {
			//OutputDebugString(_T("forward_y\n"));
			g_rtt4ecContext.y -= move * cos(M_PI * g_rtt4ecContext.h / 180);
		} else {
			g_rtt4ecContext.y += deltaY;
		}
	}

	// 通知するためにデータを詰める
	g_context.pNotifier->Int2Byte((int)g_rtt4ecContext.z, g_context.notifyData.bodyHeight);
	g_dataNotifier.Notify(&g_context.notifyData);

	sprintf_s(message, _countof(message), g_cameraCommandFormat,
		g_rtt4ecContext.x, g_rtt4ecContext.y, g_rtt4ecContext.z,
		g_rtt4ecContext.p, g_rtt4ecContext.h, g_rtt4ecContext.r, ' ');
	RemoveWhiteSpaceA(message);

	g_accessor.RTT4ECSend(&g_rtt4ecContext, message);
	
	LeaveCriticalSection(&g_context.lockObject);

	ox = x;
	oy = y;
	oz = z;
	op = p;
	oh = h;
	or = r;
}
