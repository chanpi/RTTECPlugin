// RTT4ECPlugin.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "RTT4ECPlugin.h"
#include "RTT4ECController.h"
#include "Miscellaneous.h"
#include "I4C3DCommon.h"
#include "ErrorCodeList.h"
#include <ShellAPI.h>

#include <cstdlib>	// 必要

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define new  ::new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#endif

#define MAX_LOADSTRING	100

int F710ErrorCode = EXIT_SUCCESS;
const int BUFFER_SIZE = 256;
static const PCSTR COMMAND_INIT				= "init";
static const PCSTR COMMAND_EXIT				= "exit";
static const PCSTR COMMAND_POSORIENT		= "POSORIENT";

// グローバル変数:
HINSTANCE hInst;								// 現在のインターフェイス
TCHAR szTitle[MAX_LOADSTRING];					// タイトル バーのテキスト
TCHAR szWindowClass[MAX_LOADSTRING];			// メイン ウィンドウ クラス名
static USHORT g_uPort = 0;
static USHORT g_uRTTPort = 0;
static USHORT g_uNotifyPort = 0;

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

static int AnalyzeMessage(I4C3DUDPPacket* pPacket, HWND* pHWnd, LPSTR szCommand, SIZE_T size, double* pDeltaX, double* pDeltaY, char cTermination);

static SOCKET InitializeController(HWND hWnd, USHORT uPort);
static void UnInitializeController(SOCKET socketHandler);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

#if DEBUG || _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	// TODO: ここにコードを挿入してください。
	MSG msg;
	HACCEL hAccelTable;

	// グローバル文字列を初期化しています。
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_RTT4ECPlugin, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	LOG_LEVEL logLevel = Log_Error;
#if _DEBUG || DEBUG
	logLevel = Log_Debug;
#else
	logLevel = Log_Error;
#endif
	if (!LogFileOpenW("RTT4ECPlugin", logLevel)) {
		//ReportError(_T("RTTのログは出力されません。"));
	}
	LogDebugMessage(Log_Debug, _T("RTT4EC log file opened."));
	
	int argc = 0;
	LPTSTR *argv = NULL;
	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argc < 5) {	// 最後の引数はランチャーからもらう"-run"
		LogDebugMessage(Log_Error, _T("[ERROR] 引数が足りません[例: RTT4ECPlugin.exe 10005 54321 3333]。<RTT4ECPlugin>"));
		LocalFree(argv);
		LogFileCloseW();
		return EXIT_NO_ARGUMENTS;
	}

	if (0 != _tcsicmp(argv[4], _T("-run"))) {
		LogDebugMessage(Log_Error, _T("起動オプションがありません。このアプリケーションはランチャーから起動される必要があります。"));
		LocalFree(argv);
		LogFileCloseW();
		return EXIT_NOT_EXECUTABLE;
	}

	g_uPort = static_cast<USHORT>(_wtoi(argv[1]));
	OutputDebugString(argv[1]);
	g_uNotifyPort = static_cast<USHORT>(_wtoi(argv[2]));
	OutputDebugString(argv[2]);
	g_uRTTPort = static_cast<USHORT>(_wtoi(argv[3]));
	OutputDebugString(argv[3]);
	LocalFree(argv);

	static WSAData wsaData;
	WORD wVersion;
	int nResult;

	wVersion = MAKEWORD(2,2);
	nResult = WSAStartup(wVersion, &wsaData);
	if (nResult != 0) {
		LogDebugMessage(Log_Error, _T("[ERROR] Initialize Winsock."));
		LogFileCloseW();
		return EXIT_SOCKET_ERROR;
	}
	if (wsaData.wVersion != wVersion) {
		LogDebugMessage(Log_Error, _T("[ERROR] Winsock バージョン."));
		WSACleanup();
		LogFileCloseW();
		return EXIT_SOCKET_ERROR;
	}

	// アプリケーションの初期化を実行します:
	if (!InitInstance (hInstance, nCmdShow))
	{
		WSACleanup();
		LogFileCloseW();
		return EXIT_SYSTEM_ERROR;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RTT4ECPlugin));

	// メイン メッセージ ループ:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WSACleanup();
	LogFileCloseW();
	LogDebugMessage(Log_Debug, _T("RTT4EC log file closed."));
	return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
//  コメント:
//
//    この関数および使い方は、'RegisterClassEx' 関数が追加された
//    Windows 95 より前の Win32 システムと互換させる場合にのみ必要です。
//    アプリケーションが、関連付けられた
//    正しい形式の小さいアイコンを取得できるようにするには、
//    この関数を呼び出してください。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RTT4ECPlugin));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_RTT4ECPlugin);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   //ShowWindow(hWnd, nCmdShow);
   //UpdateWindow(hWnd);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:  メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND	- アプリケーション メニューの処理
//  WM_PAINT	- メイン ウィンドウの描画
//  WM_DESTROY	- 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	static RTT4ECController controller;
	static SOCKET socketHandler = INVALID_SOCKET;
	I4C3DUDPPacket packet = {0};
	char szCommand[32] = {0};
	static char cTermination = '?';
	double deltaX, deltaY;

	int nBytes = 0;
	TCHAR szError[BUFFER_SIZE] = {0};

	switch (message)
	{
	case WM_CREATE:
		socketHandler = InitializeController(hWnd, g_uPort);
		if (INVALID_SOCKET == socketHandler) {
			PostQuitMessage(EXIT_SOCKET_ERROR);
			return 0;
		}
		break;

	case MY_WINSOCKSELECT:
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
			nBytes = recv(socketHandler, (char*)&packet, sizeof(packet), 0);
			if (nBytes == SOCKET_ERROR) {
				_stprintf_s(szError, _countof(szError), _T("recv() : %d <RTT4ECPlugin>"), WSAGetLastError());
				LogDebugMessage(Log_Error, szError);
				break;

			}

			HWND hTargetWnd = 0;
			int scanCount = AnalyzeMessage(&packet, &hTargetWnd, szCommand, _countof(szCommand), &deltaX, &deltaY, cTermination);
			if (scanCount == 3) {
				controller.Execute(hTargetWnd, szCommand, deltaX, deltaY);
				Sleep(1);

			} else if (scanCount == 1) {
				if (_strcmpi(szCommand, COMMAND_INIT) == 0) {
					if (!controller.Initialize(packet.szCommand, &cTermination, g_uRTTPort, g_uNotifyPort)) {
						_stprintf_s(szError, _countof(szError), _T("RTT4ECプラグインの初期化に失敗しています。"));
						LogDebugMessage(Log_Error, szError);
						PostQuitMessage(F710ErrorCode);
						return 0;
					}

				} else if (_strcmpi(szCommand, COMMAND_EXIT) == 0) {
					OutputDebugString(_T("exit\n"));
					controller.UnInitialize();
					DestroyWindow(hWnd);

				// RTT4EC用コマンド
				} else {
				//} else if (_strcmpi(szCommand, COMMAND_POSORIENT) == 0) {
					controller.Execute(hTargetWnd, packet.szCommand, deltaX, deltaY);	// POSORIENT ...

				}
			}
		}
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 選択されたメニューの解析:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 描画コードをここに追加してください...
		EndPaint(hWnd, &ps);
		break;

	case MY_I4C3DREBOOT:
		UnInitializeController(socketHandler);
		socketHandler = InitializeController(hWnd, g_uPort);
		if (INVALID_SOCKET == socketHandler) {
			PostQuitMessage(EXIT_SOCKET_ERROR);
		}
		break;

	case MY_I4C3DDESTROY:
	case WM_CLOSE:
	case WM_DESTROY:
		UnInitializeController(socketHandler);
		PostQuitMessage(EXIT_SUCCESS);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

SOCKET InitializeController(HWND hWnd, USHORT uPort)
{
	SOCKET socketHandler;
	SOCKADDR_IN address;
	TCHAR szError[BUFFER_SIZE];
	int nResult = 0;

	socketHandler = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketHandler == INVALID_SOCKET) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] socket() : %d"), WSAGetLastError());
		LogDebugMessage(Log_Error, szError);
		return INVALID_SOCKET;
	}

	address.sin_family = AF_INET;
	address.sin_port = htons(uPort);
	address.sin_addr.S_un.S_addr = INADDR_ANY;

	nResult = bind(socketHandler, (const SOCKADDR*)&address, sizeof(address));
	if (nResult == SOCKET_ERROR) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] bind() : %d"), WSAGetLastError());
		LogDebugMessage(Log_Error, szError);
		closesocket(socketHandler);
		return INVALID_SOCKET;
	}

	if (WSAAsyncSelect(socketHandler, hWnd, MY_WINSOCKSELECT, FD_READ) == SOCKET_ERROR) {
		TCHAR* szError = _T("ソケットイベント通知設定に失敗しました。<RTT4ECPlugin::InitializeController>");
		LogDebugMessage(Log_Error, szError);
		closesocket(socketHandler);
		return INVALID_SOCKET;
	}

	return socketHandler;
}

void UnInitializeController(SOCKET socketHandler)
{
	closesocket(socketHandler);
}

int AnalyzeMessage(I4C3DUDPPacket* pPacket, HWND* pHWnd, LPSTR szCommand, SIZE_T size, double* pDeltaX, double* pDeltaY, char cTermination)
{
	static char szFormat[BUFFER_SIZE] = {0};
	int scanCount = 0;
	double deltaX = 0., deltaY = 0.;

	if (pHWnd != NULL) {
		*pHWnd = (HWND)(pPacket->hwnd[3] << 24 | pPacket->hwnd[2] << 16 | pPacket->hwnd[1] << 8 | pPacket->hwnd[0]);
	}

	if (szFormat[0] == '\0') {
		sprintf_s(szFormat, sizeof(szFormat), "%%s %%lf %%lf%c", cTermination);
	}
	
	scanCount = sscanf_s(pPacket->szCommand, szFormat, szCommand, size, &deltaX, &deltaY);

	if (3 <= scanCount) {
		*pDeltaX = deltaX;
		*pDeltaY = deltaY;
	}
	return scanCount;
}
