#ifndef __RTT4EC_COMMON__
#define __RTT4EC_COMMON__

#include <map>
#include <string>
using namespace std;
typedef std::basic_string<TCHAR> tstring;

typedef struct RTT4ECContext
{
	SOCKET socketHandler;
	struct sockaddr_in address;

	map<tstring, string> macroMap;	// key:マクロ名、value:マクロの値

	int move;
	int angle;
	int pitch;
	int height;
	float x;
	float y;
	float z;
	float p;
	float h;
	float r;
	float speed;
} RTT4ECContext;

// RTTへの送信コマンドフォーマット
// "POSORIENT CAMERA PHR x y z P H R; ?" [?はこのアプリケーション用]
static const char* g_cameraCommandFormat = "POSORIENT PHR CAMERA %f %f %f %f %f %f ; %c";

#endif /* __RTT4EC_COMMON__ */