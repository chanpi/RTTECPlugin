#pragma once
#include "I4C3DCommon.h"

class NotificationHelper
{
public:
	NotificationHelper(void);
	virtual ~NotificationHelper(void);

	virtual int Notify(NotifyDataFormat* pData) = 0;
};

