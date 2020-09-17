#ifndef _AUTHTYPE_H
#define _AUTHTYPE_H

#include "GlobalAuth.h"
#include "ServerId.h"

#define MAX_IP_LEN 15
#define MAX_SERVER_NAME 25

#define FREE_DURATION	  0
#define PERSONAL_DURATION 1
#define PERSONAL_SPECIFIC 2
#define PERSONAL_POINT	  3

enum UserMode {
	UM_PRE_LOGIN,
	UM_LOGIN,
	UM_PLAY_REQUEST,
	UM_PLAY_OK,
	UM_IN_GAME
};

typedef char ServerStatus;

class CSocketServer;

struct WorldServer
{
	ServerId serverid;
	char ip[MAX_IP_LEN+1];
	char inner_ip[MAX_IP_LEN+1];
	char name[MAX_SERVER_NAME+1];
	in_addr outer_addr;
	short int outer_port;
	in_addr inner_addr;
	char  ageLimit;
	unsigned char  pkflag;
	int   UserNum;
	int   maxUsers;
	CSocketServer *s;
	ServerStatus status;
    int region_id;
	int isVIP;
};


class iless {
public:
	bool operator()(const std::string& a, const std::string& b) const {
		return _stricmp(a.c_str(), b.c_str()) < 0;
	}
	bool operator()(const char *a, const std::string& b) const {
		return _stricmp(a, b.c_str()) < 0;
	}
	bool operator()(const std::string& a, const char *b) const {
		return _stricmp(a.c_str(), b) < 0;
	}
};

typedef struct world_user_num {
	
	char serverid;
	int	limituser;
	int	playuser;

} WorldUserNum;

typedef std::map<int, int > SESSIONMAP;
typedef void (*EncPwdType)(char *);

extern EncPwdType EncPwd;

#endif