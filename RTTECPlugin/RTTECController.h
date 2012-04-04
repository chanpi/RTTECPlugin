#pragma once

#include "CADController.h"
#include "RTTECCommon.h"
#include "I4C3DCommon.h"

class I4C3DCursor;
class RTTECAccessor;
class DataNotifier;

typedef struct {
	volatile bool bAlive;
	CRITICAL_SECTION lockObject;
	HANDLE hCameraMonitorThread;
	UINT uCameraMonitorThreadID;

	RTTECAccessor* pAccessor;
	RTTECContext* pRttecContext;
	DataNotifier* pNotifier;
	NotifyDataFormat notifyData;
} RTTContext;

class RTTECController : public CADController
{
public:
	RTTECController(void);
	virtual ~RTTECController(void);

	BOOL Initialize(LPCSTR szBuffer, char* termination, USHORT uRTTPort, USHORT uNotifyPort);
	void UnInitialize(void);
	
	virtual void Execute(HWND hWnd, LPCSTR szCommand, double deltaX, double deltaY);

protected:
	virtual void OriginalCommandExecute(LPCSTR command);

	BOOL InitializeModifierKeys(PCSTR szModifierKeys);
	char m_cTermination;
	I4C3DCursor* m_pCursor;
};

