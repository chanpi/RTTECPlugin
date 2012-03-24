#include "StdAfx.h"
#include "MacroPlugin.h"
#include "I4C3DCommon.h"
#include "VirtualMotion.h"
#include "I4C3DKeysHook.h"
#include "Miscellaneous.h"
#include <map>
#include <string>

using namespace std;

namespace {
	const int BUFFER_SIZE = 256;

	typedef struct MacroKey {
		UINT uKeyCode;		// このマクロキーが表すキーコード
		struct MacroKey* next;
	} MacroKey;

	map<string, MacroKey*> g_macroTable;
};

static BOOL IsModKeysDown(UINT uKeyCode);

MacroPlugin::MacroPlugin(void)
{
}


MacroPlugin::~MacroPlugin(void)
{
	map<string, MacroKey*>::iterator it = g_macroTable.begin();
	MacroKey *tmp, *next = NULL;
	for (; it != g_macroTable.end(); ++it) {
		tmp = it->second;
		while (tmp != NULL) {
			next = tmp->next;
			delete tmp;
			tmp = next;
		}
	}
	g_macroTable.clear();
}

BOOL MacroPlugin::RegisterMacro(LPCSTR szBuffer, char* termination)
{
	char tmpCommand[BUFFER_SIZE] = {0};
	char macroName[BUFFER_SIZE] = {0};
	char macroValue[BUFFER_SIZE] = {0};

	sscanf_s(szBuffer, g_registerMacroFormat, tmpCommand, sizeof(tmpCommand), macroName, sizeof(macroName), macroValue, sizeof(macroValue), termination, sizeof(*termination));
	if (!AnalyzeMacro(macroName, macroValue)) {
		char szError[BUFFER_SIZE];
		sprintf_s(szError, _countof(szError), "キーマクロは登録されませんでした: [マクロ名: %s] [内容: %s]", macroName, macroValue);
		LogDebugMessageA(Log_Error, szError);
		return FALSE;
	}

	return TRUE;
}

void MacroPlugin::PlayMacro(LPCSTR macroName, HWND hKeyInputWnd, BOOL bUsePostMessageToSendKey)
{
	map<string, MacroKey*>::iterator it = g_macroTable.find(string(macroName));
	if (it != g_macroTable.end()) {
		MacroKey* macro = it->second;

		while (macro != NULL) {
			VMVirtualKeyDown(hKeyInputWnd, macro->uKeyCode, bUsePostMessageToSendKey);
			if (!IsModKeysDown(macro->uKeyCode)) {
				break;
			}
			macro = macro->next;
		}
		//Sleep(10);
		macro = it->second;
		while (macro != NULL) {
			VMVirtualKeyUp(hKeyInputWnd, macro->uKeyCode);
			macro = macro->next;
		}
	}
}

BOOL IsModKeysDown(UINT uKeyCode)
{
	int i = 0;
	for (i = 0; i < waitModkeyDownCount; ++i) {
		Sleep(1);
		if (!IsKeyDown(uKeyCode)) {
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

BOOL MacroPlugin::AnalyzeMacro(LPCSTR macroName, LPCSTR macroValue)
{
	if (macroName == NULL || macroValue == NULL) {
		return FALSE;
	}

#if _UNICODE || UNICODE
	TCHAR szBuffer[BUFFER_SIZE] = {0};
	MultiByteToWideChar(CP_ACP, 0, macroValue, -1, szBuffer, _countof(szBuffer));
#else
	LPCTSTR szBuffer = macroValue;
#endif
	LPCTSTR szMacroValue = szBuffer;
	MacroKey* macro = NULL;
	UINT uKeyCode = 0;

	PCTSTR pType = NULL;
	do {
		TCHAR szKey[BUFFER_SIZE] = {0};
		pType = _tcschr(szMacroValue, _T('+'));
		if (pType != NULL) {
			_tcsncpy_s(szKey, _countof(szKey), szMacroValue, pType-szMacroValue);
			szMacroValue = pType+1;
		} else {
			_tcscpy_s(szKey, _countof(szKey), szMacroValue);
		}
		RemoveWhiteSpace(szKey);

		// キーコードが見つかった
		if ((uKeyCode = VMGetVirtualKey(szKey)) != 0) {
			AddHookedKeyCode(uKeyCode);
		} else {
			uKeyCode = (_T('a') <= szKey[0] && szKey[0] <= _T('z')) ? szKey[0]-32 : szKey[0];
			AddHookedKeyCode(uKeyCode);
		}

		if (macro == NULL) {
			g_macroTable.insert(map<string, MacroKey*>::value_type(macroName, (macro = new MacroKey)));
		} else {
			macro->next = new MacroKey;
			macro = macro->next;
		}
		macro->uKeyCode = uKeyCode;
		macro->next = NULL;
		OutputDebugString(_T(" "));
		OutputDebugString(szKey);
	} while (pType != NULL);

	OutputDebugString(_T("\n"));
	return TRUE;
}
