#pragma once
class RTT4GamepadAccessor
{
public:
	RTT4GamepadAccessor(void);
	~RTT4GamepadAccessor(void);

	SOCKET InitializeTCPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, BOOL bSend, USHORT uPort);
	BOOL SetConnectingSocket(const SOCKET& socketHandler, const struct sockaddr_in* pAddress);
};

