#pragma once

class MacroPlugin
{
public:
	MacroPlugin(void);
	~MacroPlugin(void);

	virtual BOOL RegisterMacro(LPCSTR szBuffer, char* termination);
	virtual void PlayMacro(LPCSTR macroName, HWND hKeyInputWnd, BOOL bUsePostMessageToSendKey);

private:
	virtual BOOL AnalyzeMacro(LPCSTR macroName, LPCSTR macroValue);
};

