#pragma once

#include "CADController.h"
#include "RTT4ECCommon.h"

class I4C3DCursor;
class RTT4ECAccessor;

typedef struct {
	volatile bool bAlive;
	HANDLE hCameraMonitorThread;
	UINT uCameraMonitorThreadID;

	RTT4ECAccessor* pAccessor;
	RTT4ECContext* pRtt4ecContext;
} RTTContext;

class RTT4ECController : public CADController
{
public:
	RTT4ECController(void);
	virtual ~RTT4ECController(void);

	BOOL Initialize(LPCSTR szBuffer, char* termination, USHORT uRTTPort);
	void UnInitialize(void);
	
	virtual void Execute(HWND hWnd, LPCSTR szCommand, double deltaX, double deltaY);

protected:
	virtual void OriginalCommandExecute(LPCSTR command);

	BOOL InitializeModifierKeys(PCSTR szModifierKeys);
	char m_cTermination;
	I4C3DCursor* m_pCursor;
};

