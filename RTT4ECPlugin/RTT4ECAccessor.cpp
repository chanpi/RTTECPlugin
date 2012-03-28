#include "StdAfx.h"
#include "RTT4ECAccessor.h"
#include "Miscellaneous.h"
#include "RTT4ECCommon.h"

const int BUFFER_SIZE = 256;

RTT4ECAccessor::RTT4ECAccessor(void)
{
}


RTT4ECAccessor::~RTT4ECAccessor(void)
{
}

SOCKET RTT4ECAccessor::InitializeTCPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, BOOL bSend, USHORT uPort)
{
	SOCKET socketHandler;
	TCHAR szError[BUFFER_SIZE];

	socketHandler = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketHandler == INVALID_SOCKET) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] socket() : %d"), WSAGetLastError());
		ReportError(szError);
		LogDebugMessage(Log_Error, szError);
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

SOCKET RTT4ECAccessor::InitializeUDPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, USHORT uPort)
{
	SOCKET socketHandler = INVALID_SOCKET;
	TCHAR szError[BUFFER_SIZE];

	socketHandler = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketHandler == INVALID_SOCKET) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] socket() : %d"), WSAGetLastError());
		ReportError(szError);
		LogDebugMessage(Log_Error, szError);
		return socketHandler;
	}
	pAddress->sin_family = AF_INET;
	pAddress->sin_port = htons(uPort);
	pAddress->sin_addr.S_un.S_addr = inet_addr(szAddress);
	return socketHandler;
}

BOOL RTT4ECAccessor::SetConnectingSocket(const SOCKET& socketHandler, const struct sockaddr_in* pAddress)
{
	BOOL bUse = TRUE;
	setsockopt(socketHandler, SOL_SOCKET, SO_REUSEADDR, (const char*)&bUse, sizeof(bUse));
	int nResult = 0;

	TCHAR szError[BUFFER_SIZE] = {0};

	nResult = connect(socketHandler, (const sockaddr*)pAddress, sizeof(*pAddress));
	if (nResult == SOCKET_ERROR) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] connect() : %d"), WSAGetLastError());
		ReportError(szError);
		LogDebugMessage(Log_Error, szError);
		closesocket(socketHandler);
		return FALSE;
	}
	return TRUE;
}

BOOL RTT4ECAccessor::RTT4ECSend(RTT4ECContext* pContext, LPCSTR lpszCommand) {
	static unsigned int counter = 0;
	int nResult = 0;

	if (pContext->socketHandler == INVALID_SOCKET ||
		pContext->socketHandler == 0) {
		OutputDebugString(_T("Invalid Socket <RTT4ECAccessor::RTT4ECRecv>\n"));
		return FALSE;
	}
		
	nResult = send(pContext->socketHandler, lpszCommand, strlen(lpszCommand), 0);
	if (nResult == SOCKET_ERROR) {
		TCHAR szError[BUFFER_SIZE] = {0};
		_stprintf_s(szError, _countof(szError), _T("[ERROR] send(). Error No. %d <RTT4ECAccessor::RTT4ECSend>"), GetLastError());
		LogDebugMessage(Log_Error, szError);
		return FALSE;
	}
	return TRUE;
}

BOOL RTT4ECAccessor::RTT4ECRecv(RTT4ECContext* pContext, LPSTR lpszCommand, int nLength) {
	int nResult = 0;
	
	if (pContext->socketHandler == INVALID_SOCKET ||
		pContext->socketHandler == 0) {
		OutputDebugString(_T("Invalid Socket <RTT4ECAccessor::RTT4ECRecv>\n"));
		return FALSE;
	}

	nResult = recv(pContext->socketHandler, lpszCommand, nLength, 0);
	if (nResult == SOCKET_ERROR) {
		DWORD error = GetLastError();
		if (error == WSAETIMEDOUT) {
			// 今回タイムアウトをイベントとして扱うため、あえてエラーメッセージは表示させていません。
			return FALSE;
		} else {
			TCHAR szError[BUFFER_SIZE] = {0};
			_stprintf_s(szError, _countof(szError), _T("[ERROR] recv(). Error No. %d <RTT4ECAccessor::RTT4ECRecv>"), GetLastError());
			LogDebugMessage(Log_Error, szError);
		}
		return FALSE;
	}
	return TRUE;
}
