#include "StdAfx.h"
#include "RTTECAccessor.h"
#include "Misc.h"
#include "RTTECCommon.h"
#include "SharedConstants.h"

#if UNICODE || _UNICODE
static LPCTSTR g_FILE = __FILEW__;
#else
static LPCTSTR g_FILE = __FILE__;
#endif

const int BUFFER_SIZE = 256;

RTTECAccessor::RTTECAccessor(void)
{
}


RTTECAccessor::~RTTECAccessor(void)
{
}

SOCKET RTTECAccessor::InitializeTCPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, BOOL bSend, USHORT uPort)
{
	SOCKET socketHandler = INVALID_SOCKET;

	socketHandler = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketHandler == INVALID_SOCKET) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_INVALID), GetLastError(), g_FILE, __LINE__);
		return socketHandler;
	}

	pAddress->sin_family = AF_INET;
	pAddress->sin_port = htons(uPort);
	if (bSend) {
		pAddress->sin_addr.S_un.S_addr = inet_addr(szAddress);
	} else {
		pAddress->sin_addr.S_un.S_addr = INADDR_ANY;
	}
	return socketHandler;
}

SOCKET RTTECAccessor::InitializeUDPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, USHORT uPort)
{
	SOCKET socketHandler = INVALID_SOCKET;

	socketHandler = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketHandler == INVALID_SOCKET) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_INVALID), GetLastError(), g_FILE, __LINE__);
		return socketHandler;
	}
	pAddress->sin_family = AF_INET;
	pAddress->sin_port = htons(uPort);
	pAddress->sin_addr.S_un.S_addr = inet_addr(szAddress);
	return socketHandler;
}

BOOL RTTECAccessor::SetConnectingSocket(const SOCKET& socketHandler, const struct sockaddr_in* pAddress)
{
	BOOL bUse = TRUE;
	setsockopt(socketHandler, SOL_SOCKET, SO_REUSEADDR, (const char*)&bUse, sizeof(bUse));
	int nResult = 0;
	
	nResult = connect(socketHandler, (const sockaddr*)pAddress, sizeof(*pAddress));
	if (nResult == SOCKET_ERROR) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_CONNECT), GetLastError(), g_FILE, __LINE__);
		closesocket(socketHandler);
		return FALSE;
	}
	return TRUE;
}

BOOL RTTECAccessor::RTTECSend(RTTECContext* pContext, LPCSTR lpszCommand) {
	int nResult = 0;

	if (pContext->socketHandler == INVALID_SOCKET ||
		pContext->socketHandler == 0) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_INVALID), GetLastError(), g_FILE, __LINE__);
		return FALSE;
	}
		
	nResult = send(pContext->socketHandler, lpszCommand, strlen(lpszCommand), 0);
	if (nResult == SOCKET_ERROR) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_SEND), GetLastError(), g_FILE, __LINE__);
		return FALSE;
	}
	return TRUE;
}

BOOL RTTECAccessor::RTTECRecv(RTTECContext* pContext, LPSTR lpszCommand, int nLength) {
	int nResult = 0;
	
	if (pContext->socketHandler == INVALID_SOCKET ||
		pContext->socketHandler == 0) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_INVALID), GetLastError(), g_FILE, __LINE__);
		return FALSE;
	}

	nResult = recv(pContext->socketHandler, lpszCommand, nLength, 0);
	if (nResult == SOCKET_ERROR) {
		DWORD error = GetLastError();
		if (error == WSAETIMEDOUT) {
			// 今回タイムアウトをイベントとして扱うため、あえてエラーメッセージは表示させていません。
			return FALSE;
		} else {
			LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_RECV), GetLastError(), g_FILE, __LINE__);
		}
		return FALSE;
	}
	return TRUE;
}
