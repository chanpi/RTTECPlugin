#include "StdAfx.h"
#include "NotificationHelper.h"


NotificationHelper::NotificationHelper(void)
{
}


NotificationHelper::~NotificationHelper(void)
{
}

void NotificationHelper::Int2Byte(int src, unsigned char* dest)
{
	dest[0] =  src        & 0xFF;
	dest[1] = (src >>  8) & 0xFF;
	dest[2] = (src >> 16) & 0xFF;
	dest[3] = (src >> 24) & 0xFF;
}

void NotificationHelper::Bytes2Int(unsigned char* src, int* dest)
{
	*dest = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
}
