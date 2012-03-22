#pragma once
#include "rttcontroller.h"
class RTT4GamepadController :
	public RTTController
{
public:
	RTT4GamepadController(void);
	~RTT4GamepadController(void);
	void Execute(HWND hWnd, LPCSTR szCommand, double x, double y, double z, double p, double h, double r, LPCSTR animationTitle);
};

