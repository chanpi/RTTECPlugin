#pragma once
#include "RTT4ECCommon.h"

class RTT4ECAccessor
{
public:
	RTT4ECAccessor(void);
	~RTT4ECAccessor(void);

	SOCKET InitializeTCPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, BOOL bSend, USHORT uPort);
	SOCKET InitializeUDPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, USHORT uPort);

	BOOL SetConnectingSocket(const SOCKET& socketHandler, const struct sockaddr_in* pAddress);
	BOOL RTT4ECSend(RTT4ECContext* pContext, LPCSTR lpszCommand);
	BOOL RTT4ECRecv(RTT4ECContext* pContext, LPSTR lpszCommand, int nLength);
};

