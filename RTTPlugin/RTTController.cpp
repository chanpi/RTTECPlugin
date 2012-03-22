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

//static const PCTSTR g_szChildWindowTitle		= _T("untitled");
//static const PCTSTR g_szMouseInputWindowTitle	= _T("pGLWidget");
static const PCTSTR g_szChildWindowTitle		= _T("Scene 1 [Camera]");
static const PCTSTR g_szMouseInputWindowTitle	= _T("unnamed");

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
 * Control�I�u�W�F�N�g�̏��������s���܂��B
 * 
 * @returns
 * �������ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * Control�I�u�W�F�N�g�̏��������s���܂��B
 * �ǂ�Control�I�u�W�F�N�g���́A������I4C3DContext�|�C���^�ɓo�^����Ă���Control�h���N���X�ɂ��܂��B
 * 
 * @remarks
 * InitializeModifierKeys()�ŏC���L�[�̐ݒ���s���܂��B
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
 * 3D�\�t�g�Ŏg�p����C���L�[�̎擾�A�ݒ�A����уL�[�����̊Ď��v���O�����ւ̓o�^���s���܂��B
 * 
 * @returns
 * �C���L�[�̎擾�A�ݒ�A�o�^�ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * 3D�\�t�g�Ŏg�p����C���L�[�̎擾�A�ݒ�A����уL�[�����̊Ď��v���O�����ւ̓o�^���s���܂��B
 * 
 * @remarks
 * I4C3DKeysHook.dll��AddHookedKeyCode()�ŃL�[�t�b�N�̓o�^���s���܂��B
 * 
 * @see
 * AddHookedKeyCode()
 */
BOOL RTTController::InitializeModifierKeys(PCSTR szModifierKeys)
{
	if (_strcmpi(szModifierKeys, "NULL") == 0 || szModifierKeys == NULL) {
		LogDebugMessage(Log_Error, _T("�C���L�[���w�肳��Ȃ��������߁AAlt�L�[�ɐݒ肵�܂����B"));
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
	// �L�[���̓E�B���h�E���擾
	m_hKeyInputWnd = NULL;
	EnumChildWindows(m_hTargetTopWnd, EnumChildProcForKeyInput, (LPARAM)&m_hKeyInputWnd);
	if (m_hKeyInputWnd == NULL) {
		LogDebugMessage(Log_Error, _T("�L�[���̓E�B���h�E���擾�ł��܂���B<RTTController::GetTargetChildWnd>"));
		return FALSE;
	}

	// �}�E�X���̓E�B���h�E���擾
	m_hMouseInputWnd = NULL;
	EnumChildWindows(m_hKeyInputWnd, EnumChildProcForMouseInput, (LPARAM)&m_hMouseInputWnd);
	if (m_hMouseInputWnd == NULL) {
		LogDebugMessage(Log_Error, _T("�}�E�X���̓E�B���h�E���擾�ł��܂���B<RTTController::GetTargetChildWnd>"));
		return FALSE;
	}
	return TRUE;
}

// �R�����g�A�E�g 2011.06.10
// GetTargetChildWnd�Ƃœ�d�`�F�b�N�ɂȂ��Ă��܂����߁B
// GetTargetChildWnd��AdjustCursorPos���g�p
//BOOL RTTController::CheckTargetState(void)
//{
//	if (m_hTargetTopWnd == NULL) {
//		//ReportError(_T("�^�[�Q�b�g�E�B���h�E���擾�ł��܂���B<RTTController::CheckTargetState>"));
//		LogDebugMessage(Log_Error, _T("�^�[�Q�b�g�E�B���h�E���擾�ł��܂���B<RTTController::CheckTargetState>"));
//
//	} else if (m_hKeyInputWnd == NULL) {
//		LogDebugMessage(Log_Error, _T("�L�[���̓E�B���h�E���擾�ł��܂���B<RTTController::CheckTargetState>"));
//
//	} else if (m_hMouseInputWnd == NULL) {
//		LogDebugMessage(Log_Error, _T("�}�E�X���̓E�B���h�E���擾�ł��܂���B<RTTController::CheckTargetState>"));
//
//	} else {
//		return TRUE;
//	}
//
//	return FALSE;
//}

void RTTController::AdjustCursorPos(RECT* pWindowRect)
{
	// �^�[�Q�b�g�E�B���h�E�̈ʒu�̃`�F�b�N
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

	// ���ۂɉ��z�L�[�E���z�}�E�X������s���q�E�B���h�E�̎擾
	if (!GetTargetChildWnd()) {
		return;
	}

	//{
	//	TCHAR szBuffer[256];
	//	_stprintf_s(szBuffer, 256, _T("keywindow: %X, mousewindow: %X"), m_hKeyInputWnd, m_hMouseInputWnd);
	//	LogDebugMessage(Log_Debug, szBuffer);
	//}

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
 * Tumble�R�}���h�𑗐M���܂��B�}�E�X�����SendInput���g�p���܂��B
 * 
 * Tumble�R�}���h�𑗐M���܂��B�}�E�X�����SendInput���g�p���܂��B
 * �{�o�[�W�����ł��܂����삵�Ȃ��ꍇ�́ARTTDeltaGen_OLD.cpp���g�p���Ă�������
 * �i�C���L�[��Alt��Ctrl�̂Ƃ��Ƃŏ����𕪊򂵂Ă���܂��j�B
 */
void RTTController::TumbleExecute(int deltaX, int deltaY)
{
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
}

void RTTController::TrackExecute(int deltaX, int deltaY)
{
	////if (CheckTargetState()) {
	//	AdjustCursorPos(NULL);

	//	m_bUsePostMessageToMouseDrag	= TRUE;
	//	m_mouseMessage.bUsePostMessage	= m_bUsePostMessageToMouseDrag;
	//	m_mouseMessage.hTargetWnd		= m_hMouseInputWnd;
	//	m_mouseMessage.dragButton		= MButtonDrag;
	//	m_mouseMessage.dragStartPos		= m_currentPos;
	//	m_currentPos.x					+= deltaX;
	//	m_currentPos.y					+= deltaY;

	//	m_mouseMessage.dragEndPos		= m_currentPos;
	//	m_mouseMessage.uKeyState		= MK_MBUTTON;
	//	if (m_ctrl) {
	//		m_mouseMessage.uKeyState	|= MK_CONTROL;
	//	}
	//	if (m_shift) {
	//		m_mouseMessage.uKeyState	|= MK_SHIFT;
	//	}
	//	VMMouseDrag(&m_mouseMessage);
	////}
	RECT windowRect = {0};
	AdjustCursorPos(&windowRect);

	m_bUsePostMessageToMouseDrag	= FALSE;
	m_mouseMessage.bUsePostMessage	= m_bUsePostMessageToMouseDrag;
	m_mouseMessage.hTargetWnd		= m_hMouseInputWnd;
	m_mouseMessage.dragButton		= MButtonDrag;
	m_mouseMessage.dragStartPos.x	= m_currentPos.x + windowRect.left;
	m_mouseMessage.dragStartPos.y	= m_currentPos.y + windowRect.top;
	m_currentPos.x					+= deltaX;
	m_currentPos.y					+= deltaY;

	m_mouseMessage.dragEndPos.x = m_currentPos.x + windowRect.left;
	m_mouseMessage.dragEndPos.y = m_currentPos.y + windowRect.top;
	VMMouseDrag(&m_mouseMessage);
}

void RTTController::DollyExecute(int deltaX, int deltaY)
{
	////if (CheckTargetState()) {
	//	AdjustCursorPos(NULL);

	//	m_bUsePostMessageToMouseDrag	= TRUE;
	//	m_mouseMessage.bUsePostMessage	= m_bUsePostMessageToMouseDrag;
	//	m_mouseMessage.hTargetWnd		= m_hMouseInputWnd;
	//	m_mouseMessage.dragButton		= RButtonDrag;
	//	m_mouseMessage.dragStartPos		= m_currentPos;
	//	m_currentPos.x					+= deltaX;
	//	m_currentPos.y					+= deltaY;
	//
	//	m_mouseMessage.dragEndPos		= m_currentPos;
	//	m_mouseMessage.uKeyState		= MK_RBUTTON;
	//	if (m_ctrl) {
	//		m_mouseMessage.uKeyState	|= MK_CONTROL;
	//	}
	//	if (m_shift) {
	//		m_mouseMessage.uKeyState	|= MK_SHIFT;
	//	}
	//	VMMouseDrag(&m_mouseMessage);
	////}

	RECT windowRect = {0};
	AdjustCursorPos(&windowRect);

	m_bUsePostMessageToMouseDrag	= FALSE;
	m_mouseMessage.bUsePostMessage	= m_bUsePostMessageToMouseDrag;
	m_mouseMessage.hTargetWnd		= m_hMouseInputWnd;
	m_mouseMessage.dragButton		= RButtonDrag;
	m_mouseMessage.dragStartPos.x	= m_currentPos.x + windowRect.left;
	m_mouseMessage.dragStartPos.y	= m_currentPos.y + windowRect.top;
	m_currentPos.x					+= deltaX;
	m_currentPos.y					+= deltaY;

	m_mouseMessage.dragEndPos.x = m_currentPos.x + windowRect.left;
	m_mouseMessage.dragEndPos.y = m_currentPos.y + windowRect.top;
	VMMouseDrag(&m_mouseMessage);
}

//void RTTController::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand) const
//{
//	I4C3DControl::HotkeyExecute(pContext, m_hTargetTopWnd, szCommand);
//}

/**
 * @brief
 * �o�^�����C���L�[�������ꂽ���m�F���܂��B
 * 
 * @returns
 * �o�^�����C���L�[��������Ă���ꍇ�ɂ�TRUE�A�����łȂ��ꍇ��FALSE��Ԃ��܂��B
 * 
 * �o�^�����C���L�[�������ꂽ���m�F���܂��B
 * ������Ă��Ȃ��ꍇ�́ASleep���܂��B
 * �L�[�t�b�N�𗘗p���ăL�[�������b�Z�[�W�������������ǂ����𒲂ׂĂ��܂��B
 * �Ώۃv���O�����Ń��b�Z�[�W�����������O�̃L�[�����̔��f�ł��B
 * 
 * @remarks
 * I4C3DKeysHook.dll��IsAllKeysDown()�֐��ŃL�[�������m�F���܂��B
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

		// �o�^�����L�[�͉�����Ă���
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
			LogDebugMessage(Log_Error, _T("�����J�[�\���ւ̕ύX�Ɏ��s���Ă��܂��B"));
		}
		m_bSyskeyDown = IsModKeysDown();
		//if (!m_bSyskeyDown) {
		//	TCHAR szError[BUFFER_SIZE];
		//	_stprintf_s(szError, _countof(szError), _T("�C���L�[��������܂���ł���[�^�C���A�E�g]�B") );
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
	static int count = 0;		// TODO Delete

	TCHAR szWindowTitle[BUFFER_SIZE];
	GetWindowText(hWnd, szWindowTitle, _countof(szWindowTitle));
	
	if (!_tcsicmp(g_szChildWindowTitle, szWindowTitle) /*&& !_tcsicmp(_T("QWidget"), szClassTitle)*/) {
		*(HWND*)lParam = hWnd;
		count++;
		
		if (count == 2) {
			count = 0;
			return FALSE;
		}
	}
	return TRUE;
}

BOOL CALLBACK EnumChildProcForMouseInput(HWND hWnd, LPARAM lParam)
{
	// �q�E�B���h�E�̈�ԏ��߁A��ԏ�ʂ̂��̂�I��
	// (GetWindowText��"pGLWidget"���Ƃ肽�����Ƃ�Ȃ�����)

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