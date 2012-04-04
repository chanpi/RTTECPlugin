#include "StdAfx.h"
#include "DataNotifier.h"
#include "RTTECAccessor.h"

DataNotifier::DataNotifier(void)
{
	m_accessor = new RTTECAccessor();
	m_socketHandler = INVALID_SOCKET;
}


DataNotifier::~DataNotifier(void)
{
	delete m_accessor;
}


void DataNotifier::Initialize(LPCSTR ipAddress, USHORT uPort)
{
	m_socketHandler = m_accessor->InitializeUDPSocket(&m_address, ipAddress, uPort);
}

void DataNotifier::UnInitialize(void)
{
	if (m_socketHandler != INVALID_SOCKET) {
		closesocket(m_socketHandler);
		m_socketHandler = INVALID_SOCKET;
	}
}

int DataNotifier::Notify(NotifyDataFormat* pData)
{
	//{
	//	NotifyDataFormat f = (NotifyDataFormat)*pData;
	//	int height = 0;
	//	Bytes2Int(f.bodyHeight, &height);
	//	TCHAR szBuffer[32];
	//	_stprintf_s(szBuffer, 32, _T(">>> %d\n"), height);
	//	OutputDebugString(szBuffer);
	//}

	int sendBytes = 0;
	if (m_socketHandler != INVALID_SOCKET) {
		sendBytes = sendto(m_socketHandler, (const char*)pData, sizeof(NotifyDataFormat), 0, (const sockaddr*)&m_address, sizeof(m_address));
	}
	return sendBytes;
}