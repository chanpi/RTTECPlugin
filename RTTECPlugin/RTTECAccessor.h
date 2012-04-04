#pragma once
#include "RTTECCommon.h"

class RTTECAccessor
{
public:
	RTTECAccessor(void);
	~RTTECAccessor(void);

	SOCKET InitializeTCPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, BOOL bSend, USHORT uPort);
	SOCKET InitializeUDPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, USHORT uPort);

	BOOL SetConnectingSocket(const SOCKET& socketHandler, const struct sockaddr_in* pAddress);
	BOOL RTTECSend(RTTECContext* pContext, LPCSTR lpszCommand);
	BOOL RTTECRecv(RTTECContext* pContext, LPSTR lpszCommand, int nLength);
};

