#pragma once
#include "notificationhelper.h"
#include "stdafx.h"

class RTTECAccessor;

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
	RTTECAccessor* m_accessor;
};

