#pragma once
#include "notificationhelper.h"
#include "stdafx.h"

class RTT4ECAccessor;

class DataNotifier :
	public NotificationHelper
{
public:
	DataNotifier(void);
	~DataNotifier(void);

	void Initialize(LPCSTR ipAddress, USHORT uPort);
	void UnInitialize(void);

	int Notify(NotifyDataFormat* pData);

private:
	SOCKET m_socketHandler;
	struct sockaddr_in m_address;
	RTT4ECAccessor* m_accessor;
};

