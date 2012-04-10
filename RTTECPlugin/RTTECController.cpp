#include "StdAfx.h"
#include "I4C3DCommon.h"
#include "RTTECController.h"
#include "RTTECAccessor.h"
#include "RTTECCameraMonitor.h"
#include "DataNotifier.h"
#include "Misc.h"
#include "SharedConstants.h"
#include <math.h>
#include <float.h>
#include <process.h>

#if UNICODE || _UNICODE
static LPCTSTR g_FILE = __FILEW__;
#else
static LPCTSTR g_FILE = __FILE__;
#endif

extern int F710ErrorCode;

namespace {
	extern const int BUFFER_SIZE = 256;
	const float M_PI = (float)3.14159;
	const PCSTR INITIALIZE_MESSAGE	= "SUBSCRIBE CAMERA ;";
	RTTECAccessor g_accessor;
	RTTECContext g_rttecContext = {0};
	DataNotifier g_dataNotifier;

	RTTContext g_context = {0};
};

RTTECController::RTTECController(void):
m_cTermination(NULL)
{
}

RTTECController::~RTTECController(void)
{
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

BOOL RTTECController::Initialize(LPCSTR szBuffer, char* termination, USHORT uRTTPort, USHORT uNotifyPort)
{
	char tmpCommand[BUFFER_SIZE] = {0};
	char szModKeys[BUFFER_SIZE] = {0};
	double tumbleRate = 0., trackRate = 0., dollyRate = 0.;

	// bAlive = trueにすることでプログラムが稼働出来る状態にする
	g_context.bAlive = true;
	sscanf_s(szBuffer, g_initCommandFormat, tmpCommand,	sizeof(tmpCommand), szModKeys, sizeof(szModKeys), &tumbleRate, &trackRate, &dollyRate, termination, sizeof(*termination));
	m_cTermination = *termination;

	// RTTECContextが同時に修正されるのを防ぐ
	InitializeCriticalSection(&g_context.lockObject);

	// RTTContextの初期化
	g_context.pAccessor = &g_accessor;
	g_context.pRttecContext = &g_rttecContext;

	// TCPソケットの作成
	g_rttecContext.socketHandler = g_accessor.InitializeTCPSocket(&g_rttecContext.address, "127.0.0.1", TRUE, uRTTPort);
	if (g_rttecContext.socketHandler == INVALID_SOCKET) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_INVALID), GetLastError(), g_FILE, __LINE__);
		UnInitialize();
		F710ErrorCode = EXIT_RTTEC_CONNECT_ERROR;
		return FALSE;
	}
	if (!g_accessor.SetConnectingSocket(g_rttecContext.socketHandler, &g_rttecContext.address)) {
		UnInitialize();
		F710ErrorCode = EXIT_RTTEC_CONNECT_ERROR;
		return FALSE;
	}

	// コアへの通知用クラスを初期化
	g_dataNotifier.Initialize("127.0.0.1", uNotifyPort);
	g_context.pNotifier = &g_dataNotifier;

	// CAMERA情報取得のためのメッセージ送信(初回のみこのコマンドを送信する)
	if (!g_accessor.RTTECSend(&g_rttecContext, INITIALIZE_MESSAGE)) {
		UnInitialize();
		F710ErrorCode = EXIT_RTTEC_CONNECT_ERROR;
		return FALSE;
	}

	// カメラ情報受信用スレッドを作成
	g_context.hCameraMonitorThread = (HANDLE)_beginthreadex(NULL, 0, RTTECCameraMonitorThreadProc, &g_context, CREATE_SUSPENDED, &g_context.uCameraMonitorThreadID);
	if (g_context.hCameraMonitorThread == INVALID_HANDLE_VALUE) {
		UnInitialize();
		F710ErrorCode = EXIT_RTTEC_CONNECT_ERROR;
		return FALSE;
	}
	ResumeThread(g_context.hCameraMonitorThread);

	return TRUE;
}

void RTTECController::UnInitialize(void)
{
	g_context.bAlive = false;
	WaitForSingleObject(g_context.hCameraMonitorThread, INFINITE);

	if (g_rttecContext.socketHandler != INVALID_SOCKET) {
		closesocket(g_rttecContext.socketHandler);
		g_rttecContext.socketHandler = INVALID_SOCKET;
	}

	if (g_context.hCameraMonitorThread != NULL) {
		CloseHandle(g_context.hCameraMonitorThread);
		g_context.hCameraMonitorThread = NULL;
	}

	g_dataNotifier.UnInitialize();
	DeleteCriticalSection(&g_context.lockObject);
}

void RTTECController::Execute(HWND /*hWnd*/, LPCSTR szCommand, double /*deltaX*/, double /*deltaY*/)
{
	OriginalCommandExecute(szCommand);
}

void RTTECController::OriginalCommandExecute(LPCSTR command)
{
	char termination = '?';
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
		g_accessor.RTTECSend(&g_rttecContext, message);
		return;
	}

	EnterCriticalSection(&g_context.lockObject);

	g_rttecContext.z += z-oz;
	g_rttecContext.p += p-op;
	g_rttecContext.h += h-oh;
	g_rttecContext.r += r-or;

	{
		float sinval = 0., cosval = 0.;	
		float deltaX = x-ox, deltaY = y-oy;
		float move = 0.;
		sinval = sin(M_PI * h / 180);
		cosval = cos(M_PI * h / 180);
		// 0除算防止
		move = (fabs(sinval - 0.0) > DBL_EPSILON) ? fabs(deltaX / sinval) : fabs(deltaY / cosval);

		if (deltaX * sinval > 0 || (fabs(g_rttecContext.h - 0.0) > DBL_EPSILON && deltaY * cosval < 0)) {
			//OutputDebugString(_T("forward_x\n"));
			g_rttecContext.x += move * sin(M_PI * g_rttecContext.h / 180);
		} else if (deltaX * sinval < 0 || (fabs(g_rttecContext.h - 0.0) > DBL_EPSILON && deltaY * cosval > 0)) {
			//OutputDebugString(_T("backward_x\n"));
			g_rttecContext.x -= move * sin(M_PI * g_rttecContext.h / 180);
		} else {
			g_rttecContext.x += deltaX;
		}

		if (deltaY * cosval > 0) {
			//OutputDebugString(_T("backward_y\n"));
			g_rttecContext.y += move * cos(M_PI * g_rttecContext.h / 180);
		} else if (deltaY * cosval < 0) {
			//OutputDebugString(_T("forward_y\n"));
			g_rttecContext.y -= move * cos(M_PI * g_rttecContext.h / 180);
		} else {
			g_rttecContext.y += deltaY;
		}
	}

	// 通知するためにデータを詰める
	g_context.pNotifier->Int2Byte((int)g_rttecContext.z, g_context.notifyData.bodyHeight);
	g_dataNotifier.Notify(&g_context.notifyData);

	sprintf_s(message, _countof(message), g_cameraCommandFormat,
		g_rttecContext.x, g_rttecContext.y, g_rttecContext.z,
		g_rttecContext.p, g_rttecContext.h, g_rttecContext.r, ' ');
	RemoveWhiteSpaceA(message);

	g_accessor.RTTECSend(&g_rttecContext, message);
	
	LeaveCriticalSection(&g_context.lockObject);

	ox = x;
	oy = y;
	oz = z;
	op = p;
	oh = h;
	or = r;
}
