#ifndef __RTT4EC_COMMON__
#define __RTT4EC_COMMON__

#include <map>
#include <string>
using namespace std;

typedef struct RTT4ECContext
{
	SOCKET socketHandler;
	struct sockaddr_in address;

#if UNICODE || _UNICODE
	map<wstring, string> macroMap;	// key:�}�N�����Avalue:�}�N���̒l
#else
	map<string, string> macroMap;	// key:�}�N�����Avalue:�}�N���̒l
#endif

	int move;
	int angle;
	int pitch;
	int height;
	int x;
	int y;
	int z;
	int p;
	int h;
	int r;
	int speed;
} RTT4ECContext;

// RTT�ւ̑��M�R�}���h�t�H�[�}�b�g
// "POSORIENT CAMERA PHR x y z P H R; ?" [?�͂��̃A�v���P�[�V�����p]
LPCSTR g_cameraCommandFormat = "POSORIENT CAMERA PHR %f %f %f %f %f %f ; %c";

#endif /* __RTT4EC_COMMON__ */