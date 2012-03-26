#pragma once
class RTT4ECAccessor
{
public:
	RTT4ECAccessor(void);
	~RTT4ECAccessor(void);

	SOCKET InitializeTCPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, BOOL bSend, USHORT uPort);
	BOOL SetConnectingSocket(const SOCKET& socketHandler, const struct sockaddr_in* pAddress);
};

