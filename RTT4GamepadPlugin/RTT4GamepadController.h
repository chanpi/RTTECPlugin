#pragma once

#include "VirtualMotion.h"
#include "CADController.h"
#include "MacroPlugin.h"
#include "RTT4GamepadCameraMonitor.h"

class I4C3DCursor;

class RTT4GamepadController : public CADController, public MacroPlugin
{
public:
	RTT4GamepadController(void);
	virtual ~RTT4GamepadController(void);

	BOOL Initialize(LPCSTR szBuffer, char* termination, USHORT uRTTPort);
	void UnInitialize(void);
	virtual void Execute(HWND hWnd, LPCSTR szCommand, double deltaX, double deltaY);
	void ModKeyUp(void);

protected:
	RTTContext m_rttContext;

	virtual void TumbleExecute(int deltaX, int deltaY);
	virtual void TrackExecute(int deltaX, int deltaY);
	virtual void DollyExecute(int deltaX, int deltaY);
	virtual void OriginalCommandExecute(LPCSTR command);

	BOOL InitializeModifierKeys(PCSTR szModifierKeys);
	BOOL GetTargetChildWnd(void);
	//BOOL CheckTargetState(void);
	void AdjustCursorPos(RECT* pWindowRect);
	void ModKeyDown(void);
	BOOL IsModKeysDown(void);

	HWND m_hTargetTopWnd;
	HWND m_hMouseInputWnd;
	HWND m_hKeyInputWnd;

	VMMouseMessage m_mouseMessage;
	POINT m_currentPos;
	BOOL m_ctrl;
	BOOL m_alt;
	BOOL m_shift;
	BOOL m_bUsePostMessageToSendKey;
	BOOL m_bUsePostMessageToMouseDrag;
	BOOL m_bSyskeyDown;
	int m_DisplayWidth;
	int m_DisplayHeight;
	double m_fTumbleRate;
	double m_fTrackRate;
	double m_fDollyRate;

	I4C3DCursor* m_pCursor;
};

