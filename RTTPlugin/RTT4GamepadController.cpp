#include "StdAfx.h"
#include "RTT4GamepadController.h"

const PCSTR COMMAND_TUMBLE	= "tumble";
const PCSTR COMMAND_TRACK	= "track";
const PCSTR COMMAND_DOLLY	= "dolly";

const PCSTR COMMAND_POSORIENT	= "POSORIENT";
const PCSTR COMMAND_ANIMATION	= "ANIM";



RTT4GamepadController::RTT4GamepadController(void)
{
}


RTT4GamepadController::~RTT4GamepadController(void)
{
}

void RTT4GamepadController::Execute(HWND hWnd, LPCSTR szCommand, double x, double y, double z, double p, double h, double r, LPCSTR animationTitle)
{
	m_hTargetTopWnd = hWnd;

	// 実際に仮想キー・仮想マウス操作を行う子ウィンドウの取得
	if (!GetTargetChildWnd()) {
		return;
	}

	if (_strcmpi(szCommand, COMMAND_TUMBLE) == 0) {
		ModKeyDown();
		if (m_bSyskeyDown) {
			TumbleExecute((INT)(x * m_fTumbleRate), (INT)(y * m_fTumbleRate));
		}

	} else if (_strcmpi(szCommand, COMMAND_TRACK) == 0) {
		ModKeyDown();
		if (m_bSyskeyDown) {
			TrackExecute((INT)(x * m_fTrackRate), (INT)(y * m_fTrackRate));
		}

	} else if (_strcmpi(szCommand, COMMAND_DOLLY) == 0) {
		ModKeyDown();
		if (m_bSyskeyDown) {
			DollyExecute((INT)(x * m_fDollyRate), (INT)(y * m_fDollyRate));
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