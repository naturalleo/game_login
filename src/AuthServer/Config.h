/**
 * Config.h
 *
 * Config class declaration
 *
 * author: woojeong
 *
 * created: 2002-03-11
 *
**/

#if !defined(AFX_CONFIG_H__3512F1FE_5606_4C80_AE8E_840467086630__INCLUDED_)
#define AFX_CONFIG_H__3512F1FE_5606_4C80_AE8E_840467086630__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GlobalAuth.h"

typedef struct File
{
	int size;
	char *fileBuff;
}File;


typedef std::map<std::string, std::string, iless> StringMap;
typedef std::map<int, File> ResouceMap;

// These values are duplicated in Common/auth/auth.h
#define USA_AUTH_PROTOCOL_VERSION			30206	// version of the auth used in us/europe production coh from 2004/4/28 - ???
#define ASIA_AUTH_PROTOCOL_VERSION			30207	// version of the auth used in asia
#define GLOBAL_AUTH_PROTOCOL_VERSION		30810	// version used for worldwide release
#define GR_REACTIVATION_PROTOCOL_VERSION	100903	// First version to support GR reactivation

class Config {
public:

    // configuration items

    // main port for server socket
    int serverPort;
	int serverExPort;
	int serverIntPort;
	int worldPort;
	int numDBConn;
	int numServerThread; 
	int numServerIntThread;

	bool encrypt;
	int PacketSizeType;
	int ProtocolVer;
	bool OneTimeLogOut;
	const char *logDirectory;

	Config();
	~Config();

	bool Load(const char *filename);
	const char *Get(const char *key) const;
	bool GetBool(const char *key, bool def = false) const;
	int GetInt(const char *key, int def = 0) const;
	in_addr GetInetAddr(const char *key) const;
	int gameId;

	bool DesApply;
    bool ReadLocalServerList;
	bool GMCheckMode;
	bool UserData;
	bool DumpPacket;
	bool PCCafeFirst;
	bool UseIPServer;
	const char *DevIP;	

	in_addr IPServer;
	int  IPPort;
	int  IPConnectInterval;
	
	int  IPAccessLimit;

	int  Country;
	
	int  SocketLimit;
	int  LimitIOObject;
	int  SocketTimeOut;
	int  AcceptCallNum;
	int  WaitingUserLimit;


	// 2003-07-15 logd 
	bool UseLogD;
	in_addr LogDIP;             // LOGD
	int  LogDPort;				// LOGD
	int  LogDReconnectInterval; // 


	// 2003-07-27 GM Flag 
	bool RestrictGMIP;
	in_addr GMIP;

	// 2003-08-22 Wanted System 
	bool UseWantedSystem;
	in_addr WantedIP;
	int WantedPort;
	int WantedReconnectInterval;

	// doing reactivation via a config flag
	int Reactivation;
	int ReactivationValue;
	SYSTEMTIME ReactivationStart;
	SYSTEMTIME ReactivationEnd;

	bool FreeServer;
	bool HybridServer;

	// 2003-11-25 forbidden Ip list 
	bool useForbiddenIPList;

	// 2004-01-28 reconnect
	bool supportReconnect; // default = false

	bool gameServerSpecifiesId;
	bool allowUnknownServers;
	bool useQueue;
    bool sendQueueLevel;
	int payStatOverride;
    
    bool enableVerboseLogging;
    bool enableDebugLogging;


    int serverVersion;

private:
	StringMap map;
	ResouceMap mapResource;
	int DateTimestringToSystemTime(SYSTEMTIME *time, const char *str);
	void ClearDateTime(SYSTEMTIME *time);

	int CompareDateTime(const SYSTEMTIME *time1, const SYSTEMTIME *time2);
	int IsValidDateTime(const SYSTEMTIME *time);

	bool InitClientVersionRes();

public:
	int IsReactivationActive();
	const File* GetClientResourceById(int n);
};

extern Config config;
#endif // !defined(AFX_CONFIG_H__3512F1FE_5606_4C80_AE8E_840467086630__INCLUDED_)
