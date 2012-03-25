#include "StdAfx.h"
#include "RTTController.h"
#include "I4C3DKeysHook.h"
#include "I4C3DCommon.h"
#include "I4C3DCursor.h"
#include "Miscellaneous.h"
#include <math.h>
#include <float.h>

extern const int BUFFER_SIZE = 256;

#define MOUSE_EVENT_X(x)    ((x) * (65535 / ::GetSystemMetrics(SM_CXSCREEN)))
#define MOUSE_EVENT_Y(y)    ((y) * (65535 / ::GetSystemMetrics(SM_CYSCREEN)))

static BOOL CALLBACK EnumChildProcForKeyInput(HWND hWnd, LPARAM lParam);
static BOOL CALLBACK EnumChildProcForMouseInput(HWND hWnd, LPARAM lParam);

static const PCTSTR g_szChildWindowTitle		= _T("untitled");
static const PCTSTR g_szMouseInputWindowTitle	= _T("pGLWidget");

const PCSTR COMMAND_TUMBLE	= "tumble";
const PCSTR COMMAND_TRACK	= "track";
const PCSTR COMMAND_DOLLY	= "dolly";

RTTController::RTTController(void)
{
	m_hTargetTopWnd		= NULL;
	m_hKeyInputWnd		= NULL;
	m_hMouseInputWnd	= NULL;
	m_currentPos.x		= 0;
	m_currentPos.y		= 0;
	m_DisplayWidth		= GetSystemMetrics(SM_CXSCREEN);
	m_DisplayHeight		= GetSystemMetrics(SM_CYSCREEN);
	m_fTumbleRate		= 0;
	m_fTrackRate		= 0;
	m_fDollyRate		= 0;
	m_bUsePostMessageToSendKey		= FALSE;
	m_bUsePostMessageToMouseDrag	= TRUE;
	m_ctrl = m_alt = m_shift = m_bSyskeyDown = FALSE;

	ZeroMemory(&m_mouseMessage, sizeof(m_mouseMessage));
	m_mouseMessage.dragButton = DragNONE;
	
	MakeHook(NULL);
	m_hKeyInputWnd	= NULL;
	m_pCursor		= new I4C3DCursor;
}

RTTController::~RTTController(void)
{
	ModKeyUp();
	UnHook();
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

BOOL RTTController::Initialize(LPCSTR szBuffer, char* termination)
{
	char tmpCommand[BUFFER_SIZE] = {0};
	char szModKeys[BUFFER_SIZE] = {0};

	sscanf_s(szBuffer, g_initCommandFormat, tmpCommand,	sizeof(tmpCommand), szModKeys, sizeof(szModKeys), &m_fTumbleRate, &m_fTrackRate, &m_fDollyRate, termination, sizeof(*termination));
	if (fabs(m_fTumbleRate - 0.0) < DBL_EPSILON) {
		m_fTumbleRate = 1.0;
	}
	if (fabs(m_fTrackRate - 0.0) < DBL_EPSILON) {
		m_fTrackRate = 1.0;
	}
	if (fabs(m_fDollyRate - 0.0) < DBL_EPSILON) {
		m_fDollyRate = 1.0;
	}

	{
		TCHAR szBuf[32];
		_stprintf_s(szBuf, 32, _T("tum:%.2f, tra:%.2f dol:%.2f\n"), m_fTumbleRate, m_fTrackRate, m_fDollyRate);
		OutputDebugString(szBuf);
	}

	return InitializeModifierKeys(szModKeys);
}

/**
 * @brief
 * 3Dソフトで使用する修飾キーの取得、設定、およびキー押下の監視プログラムへの登録を行います。
 * 
 * @returns
 * 修飾キーの取得、設定、登録に成功した場合にはTRUE、失敗した場合にはFALSEを返します。
 * 
 * 3Dソフトで使用する修飾キーの取得、設定、およびキー押下の監視プログラムへの登録を行います。
 * 
 * @remarks
 * I4C3DKeysHook.dllのAddHookedKeyCode()でキーフックの登録を行います。
 * 
 * @see
 * AddHookedKeyCode()
 */
BOOL RTTController::InitializeModifierKeys(PCSTR szModifierKeys)
{
	if (_strcmpi(szModifierKeys, "NULL") == 0 || szModifierKeys == NULL) {
		LogDebugMessage(Log_Error, _T("修飾キーが指定されなかったため、Altキーに設定しました。"));
		m_alt = TRUE;
		AddHookedKeyCode(VK_MENU);
		return TRUE;
	}

	PCSTR pType = NULL;
	do {
		char szKey[BUFFER_SIZE] = {0};
		pType = strchr(szModifierKeys, '+');
		if (pType != NULL) {
			strncpy_s(szKey, _countof(szKey), szModifierKeys, pType-szModifierKeys+1);
			szModifierKeys = pType+1;
		} else {
			strcpy_s(szKey, _countof(szKey), szModifierKeys);
		}
		RemoveWhiteSpaceA(szKey);
		switch (szKey[0]) {
		case _T('C'):
		case _T('c'):
			m_ctrl = TRUE;
			AddHookedKeyCode( VK_CONTROL );
			break;

		case _T('S'):
		case _T('s'):
			m_shift = TRUE;
			AddHookedKeyCode( VK_SHIFT );
			break;

		case _T('A'):
		case _T('a'):
			m_alt = TRUE;
			AddHookedKeyCode( VK_MENU );
			break;
		}
	} while (pType != NULL);

	return TRUE;
}


BOOL RTTController::GetTargetChildWnd(void)
{
	// キー入力ウィンドウを取得
	m_hKeyInputWnd = NULL;
	EnumChildWindows(m_hTargetTopWnd, EnumChildProcForKeyInput, (LPARAM)&m_hKeyInputWnd);
	if (m_hKeyInputWnd == NULL) {
		return FALSE;
	}

	// マウス入力ウィンドウを取得
	m_hMouseInputWnd = NULL;
	EnumChildWindows(m_hKeyInputWnd, EnumChildProcForMouseInput, (LPARAM)&m_hMouseInputWnd);
	if (m_hMouseInputWnd == NULL) {
		return FALSE;
	}
	return TRUE;
}

// コメントアウト 2011.06.10
// GetTargetChildWndとで二重チェックになってしまうため。
// GetTargetChildWndとAdjustCursorPosを使用
//BOOL RTTController::CheckTargetState(void)
//{
//	if (m_hTargetTopWnd == NULL) {
//		//ReportError(_T("ターゲットウィンドウが取得できません。<RTTController::CheckTargetState>"));
//		LogDebugMessage(Log_Error, _T("ターゲットウィンドウが取得できません。<RTTController::CheckTargetState>"));
//
//	} else if (m_hKeyInputWnd == NULL) {
//		LogDebugMessage(Log_Error, _T("キー入力ウィンドウが取得できません。<RTTController::CheckTargetState>"));
//
//	} else if (m_hMouseInputWnd == NULL) {
//		LogDebugMessage(Log_Error, _T("マウス入力ウィンドウが取得できません。<RTTController::CheckTargetState>"));
//
//	} else {
//		return TRUE;
//	}
//
//	return FALSE;
//}

void RTTController::AdjustCursorPos(RECT* pWindowRect)
{
	// ターゲットウィンドウの位置のチェック
	POINT tmpCurrentPos = m_currentPos;
	ClientToScreen(m_hMouseInputWnd, &tmpCurrentPos);

	RECT windowRect;
	GetWindowRect(m_hMouseInputWnd, &windowRect);
	if (WindowFromPoint(tmpCurrentPos) != m_hMouseInputWnd ||
		tmpCurrentPos.x < windowRect.left+500 || windowRect.right-500 < tmpCurrentPos.x ||
		tmpCurrentPos.y < windowRect.top+300 || windowRect.bottom-300 < tmpCurrentPos.y) {
			if (m_mouseMessage.dragButton != DragNONE) {
				VMMouseClick(&m_mouseMessage, TRUE);
				m_mouseMessage.dragButton = DragNONE;
			}

			RECT rect;
			GetClientRect(m_hMouseInputWnd, &rect);
			m_currentPos.x = rect.left + (rect.right - rect.left) / 2;
			m_currentPos.y = rect.top + (rect.bottom - rect.top) / 2;
	}

	if (pWindowRect != NULL) {
		pWindowRect->top = windowRect.top;
		pWindowRect->bottom = windowRect.bottom;
		pWindowRect->left = windowRect.left;
		pWindowRect->right = windowRect.right;
	}
}

void RTTController::Execute(HWND hWnd, LPCSTR szCommand, double deltaX, double deltaY)
{
	m_hTargetTopWnd = hWnd;

	// 実際に仮想キー・仮想マウス操作を行う子ウィンドウの取得
	if (!GetTargetChildWnd()) {
		return;
	}

	if (_strcmpi(szCommand, COMMAND_TUMBLE) == 0) {
		ModKeyDown();
		if (m_bSyskeyDown) {
			TumbleExecute((INT)(deltaX * m_fTumbleRate), (INT)(deltaY * m_fTumbleRate));
		}

	} else if (_strcmpi(szCommand, COMMAND_TRACK) == 0) {
		ModKeyDown();
		if (m_bSyskeyDown) {
			TrackExecute((INT)(deltaX * m_fTrackRate), (INT)(deltaY * m_fTrackRate));
		}

	} else if (_strcmpi(szCommand, COMMAND_DOLLY) == 0) {
		ModKeyDown();
		if (m_bSyskeyDown) {
			DollyExecute((INT)(deltaX * m_fDollyRate), (INT)(deltaY * m_fDollyRate));
		}

	} else {
//#if _UNICODE || UNICODE
//		TCHAR wszCommand[BUFFER_SIZE] = {0};
//		MultiByteToWideChar(CP_ACP, 0, szCommand, -1, wszCommand, _countof(wszCommand));
//		ModKeyUp();
//		HotkeyExecute(pContext, wszCommand);
//#else
//		pContext->pController->HotkeyExecute(lpszCommand);
//#endif
	}
}

/**
 * @brief
 * Tumbleコマンドを送信します。修飾キーがAltの場合にはSendInput、Ctrlの場合にはPostMessageでマウス操作します。
 * 
 * Tumbleコマンドを送信します。修飾キーがAltの場合にはSendInput、Ctrlの場合にはPostMessageでマウス操作します。
 * RTTDeltaGenではPostMessageでのマウス操作に不具合がある可能性があります。
 * その場合はRTTController.cppを使用してください。
 */
void RTTController::TumbleExecute(int deltaX, int deltaY)
{
	if (m_alt) {
		//if (CheckTargetState()) {
			RECT windowRect = {0};
			AdjustCursorPos(&windowRect);

			m_bUsePostMessageToMouseDrag	= FALSE;
			m_mouseMessage.bUsePostMessage	= m_bUsePostMessageToMouseDrag;
			m_mouseMessage.hTargetWnd		= m_hMouseInputWnd;
			m_mouseMessage.dragButton		= LButtonDrag;
			m_mouseMessage.dragStartPos.x	= m_currentPos.x + windowRect.left;
			m_mouseMessage.dragStartPos.y	= m_currentPos.y + windowRect.top;
			m_currentPos.x					+= deltaX;
			m_currentPos.y					+= deltaY;

			m_mouseMessage.dragEndPos.x = m_currentPos.x + windowRect.left;
			m_mouseMessage.dragEndPos.y = m_currentPos.y + windowRect.top;
			VMMouseDrag(&m_mouseMessage, 2);
		//}
	} else {
		//if (CheckTargetState()) {
			AdjustCursorPos(NULL);

			m_bUsePostMessageToMouseDrag	= TRUE;
			m_mouseMessage.bUsePostMessage	= m_bUsePostMessageToMouseDrag;
			m_mouseMessage.hTargetWnd		= m_hMouseInputWnd;
			m_mouseMessage.dragButton		= LButtonDrag;
			m_mouseMessage.dragStartPos		= m_currentPos;
			m_currentPos.x					+= deltaX;
			m_currentPos.y					+= deltaY;

			m_mouseMessage.dragEndPos		= m_currentPos;
			m_mouseMessage.uKeyState		= MK_LBUTTON;
			if (m_ctrl) {
				m_mouseMessage.uKeyState	|= MK_CONTROL;
			}
			if (m_shift) {
				m_mouseMessage.uKeyState	|= MK_SHIFT;
			}
			VMMouseDrag(&m_mouseMessage);
		//}
	}
}

void RTTController::TrackExecute(int deltaX, int deltaY)
{
	//if (CheckTargetState()) {
		AdjustCursorPos(NULL);

		m_bUsePostMessageToMouseDrag	= TRUE;
		m_mouseMessage.bUsePostMessage	= m_bUsePostMessageToMouseDrag;
		m_mouseMessage.hTargetWnd		= m_hMouseInputWnd;
		m_mouseMessage.dragButton		= MButtonDrag;
		m_mouseMessage.dragStartPos		= m_currentPos;
		m_currentPos.x					+= deltaX;
		m_currentPos.y					+= deltaY;

		m_mouseMessage.dragEndPos		= m_currentPos;
		m_mouseMessage.uKeyState		= MK_MBUTTON;
		if (m_ctrl) {
			m_mouseMessage.uKeyState	|= MK_CONTROL;
		}
		if (m_shift) {
			m_mouseMessage.uKeyState	|= MK_SHIFT;
		}
		VMMouseDrag(&m_mouseMessage);
	//}
}

void RTTController::DollyExecute(int deltaX, int deltaY)
{
	//if (CheckTargetState()) {
		AdjustCursorPos(NULL);

		m_bUsePostMessageToMouseDrag	= TRUE;
		m_mouseMessage.bUsePostMessage	= m_bUsePostMessageToMouseDrag;
		m_mouseMessage.hTargetWnd		= m_hMouseInputWnd;
		m_mouseMessage.dragButton		= RButtonDrag;
		m_mouseMessage.dragStartPos		= m_currentPos;
		m_currentPos.x					+= deltaX;
		m_currentPos.y					+= deltaY;
	
		m_mouseMessage.dragEndPos		= m_currentPos;
		m_mouseMessage.uKeyState		= MK_RBUTTON;
		if (m_ctrl) {
			m_mouseMessage.uKeyState	|= MK_CONTROL;
		}
		if (m_shift) {
			m_mouseMessage.uKeyState	|= MK_SHIFT;
		}
		VMMouseDrag(&m_mouseMessage);
	//}
}

//void RTTController::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand) const
//{
//	I4C3DControl::HotkeyExecute(pContext, m_hTargetTopWnd, szCommand);
//}

/**
 * @brief
 * 登録した修飾キーが押されたか確認します。
 * 
 * @returns
 * 登録した修飾キーが押されている場合にはTRUE、そうでない場合はFALSEを返します。
 * 
 * 登録した修飾キーが押されたか確認します。
 * 押されていない場合は、Sleepします。
 * キーフックを利用してキー押下メッセージが発生したかどうかを調べています。
 * 対象プログラムでメッセージが処理される前のキー押下の判断です。
 * 
 * @remarks
 * I4C3DKeysHook.dllのIsAllKeysDown()関数でキー押下を確認します。
 * 
 * @see
 * IsAllKeysDown()
 */
BOOL RTTController::IsModKeysDown(void)
{
	int i = 0;
	for (i = 0; i < waitModkeyDownCount; ++i) {
		Sleep(1);
		if (m_ctrl && !IsKeyDown(VK_CONTROL)) {
			continue;
		}
		if (m_alt && !IsKeyDown(VK_MENU)) {
			continue;
		}
		if (m_shift && !IsKeyDown(VK_SHIFT)) {
			continue;
		}

		// 登録したキーは押されていた
		break;
	}

	if (i < waitModkeyDownCount) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void RTTController::ModKeyDown(void)
{
	if (!m_bSyskeyDown) {
		if (m_ctrl) {
			VMVirtualKeyDown(m_hKeyInputWnd, VK_CONTROL, m_bUsePostMessageToSendKey);
		}
		if (m_alt) {
			VMVirtualKeyDown(m_hKeyInputWnd, VK_MENU, m_bUsePostMessageToSendKey);
		}
		if (m_shift) {
			VMVirtualKeyDown(m_hKeyInputWnd, VK_SHIFT, m_bUsePostMessageToSendKey);
		}

		if  (!m_pCursor->SetTransparentCursor()) {
			LogDebugMessage(Log_Error, _T("透明カーソルへの変更に失敗しています。"));
		}
		m_bSyskeyDown = IsModKeysDown();
		//if (!m_bSyskeyDown) {
		//	TCHAR szError[BUFFER_SIZE];
		//	_stprintf_s(szError, _countof(szError), _T("修飾キーが押されませんでした[タイムアウト]。") );
		//}
	}
}

void RTTController::ModKeyUp(void)
{
	if (m_bSyskeyDown) {
		if (!SetForegroundWindow(m_hTargetTopWnd)) {
			OutputDebugString(_T("SetForegroundWindow -> FALSE\n"));
		}

		if (m_mouseMessage.dragButton != DragNONE) {
			VMMouseClick(&m_mouseMessage, TRUE);
			m_mouseMessage.dragButton = DragNONE;
		}

		if (m_shift) {
			VMVirtualKeyUp(m_hKeyInputWnd, VK_SHIFT);
		}
		if (m_alt) {
			VMVirtualKeyUp(m_hKeyInputWnd, VK_MENU);
		}
		if (m_ctrl) {
			VMVirtualKeyUp(m_hKeyInputWnd, VK_CONTROL);
		}
		m_bSyskeyDown = FALSE;
	}
	m_pCursor->RestoreCursor();
}


BOOL CALLBACK EnumChildProcForKeyInput(HWND hWnd, LPARAM lParam)
{
	TCHAR szWindowTitle[BUFFER_SIZE];
	GetWindowText(hWnd, szWindowTitle, _countof(szWindowTitle));

	if (!_tcsicmp(g_szChildWindowTitle, szWindowTitle) /*&& !_tcsicmp(_T("QWidget"), szClassTitle)*/) {
		*(HWND*)lParam = hWnd;
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK EnumChildProcForMouseInput(HWND hWnd, LPARAM lParam)
{
	// 子ウィンドウの一番初め、一番上位のものを選択
	// (GetWindowTextで"pGLWidget"をとりたいがとれないため)

	//TCHAR szWindowTitle[I4C3D_BUFFER_SIZE];
	//GetWindowText(hWnd, szWindowTitle, _countof(szWindowTitle));
	//{
	//	OutputDebugString(szWindowTitle);
	//	OutputDebugString(L"\n");
	//}

	//if (!_tcsicmp(g_szMouseInputWindowTitle, szWindowTitle)) {
		*(HWND*)lParam = hWnd;
		return FALSE;
	//}
	//return TRUE;
}