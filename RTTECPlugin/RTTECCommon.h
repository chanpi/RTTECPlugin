#ifndef __RTTEC_COMMON__
#define __RTTEC_COMMON__

#include <map>
#include <string>
using namespace std;
typedef std::basic_string<TCHAR> tstring;

typedef struct RTTECContext
{
	SOCKET socketHandler;
	struct sockaddr_in address;

	map<tstring, string> macroMap;	// key:�}�N�����Avalue:�}�N���̒l

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
} RTTECContext;

// RTT�ւ̑��M�R�}���h�t�H�[�}�b�g
// "POSORIENT CAMERA PHR x y z P H R; ?" [?�͂��̃A�v���P�[�V�����p]
static const char* g_cameraCommandFormat = "POSORIENT PHR CAMERA %f %f %f %f %f %f ; %c";

#endif /* __RTTEC_COMMON__ */