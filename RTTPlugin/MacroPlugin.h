#pragma once

class MacroPlugin
{
public:
	MacroPlugin(void);
	~MacroPlugin(void);

	BOOL RegisterMacro(LPCSTR szBuffer, char* termination);
	void PlayMacro(LPCSTR macroName, HWND hKeyInputWnd);

private:
	BOOL AnalyzeMacro(LPCSTR macroName, LPCSTR macroValue);
};

