// LogSocket.h: interface for the CLogSocket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOGSOCKET_H__32C83A11_E718_4C82_B282_F53B5A1E3E1C__INCLUDED_)
#define AFX_LOGSOCKET_H__32C83A11_E718_4C82_B282_F53B5A1E3E1C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AuthType.h"
#include "IOSocket.h"
#include "logprotocol.h"

#define RQ_LOG_SEND_MSG      0
#define RQ_SET_CHECK_STATUS  1
#define RQ_SERVER_STARTED    3
#define AUTH_LOG_TYPE		 6
#define SERVER_TYPE          1

// LOG Socket�� IOCP�� ������ �ʿ䵵 ���� ��Ŷ�� �Ѱ��� ������ �����Ѵ�. �׸��� ������ �ʱ� ������
// ���� ������ ������ WSASend�� ������ ������ ���� Reconnect�� �����ϸ� �ȴ�. 
class CLogSocket;

typedef CLogSocket* (*LOGSocketAllocator)(SOCKET s);
typedef bool (*LOGPacketFunc)(CLogSocket* , const unsigned char* packet);

class CLogSocket  
: public CIOSocket
{
public:
	SOCKET m_socket;

	CLogSocket(SOCKET sock);
	
	virtual ~CLogSocket();
	static CLogSocket* Allocate(SOCKET s);
	virtual void OnCreate();
	virtual void OnRead();
	virtual void OnClose(SOCKET closedSocket);
	virtual void OnTimerCallback( );
	friend class CLogPacketServer;
	
	in_addr getaddr( void ) { return addr; };
	void SetAddress( in_addr in_Addr ){
		addr = in_Addr;
	};

	void Send(const char* format, ...);
	void Send2(const char* format, ...);
	const char * IP();

	char serverid;

protected:
	int packetLen;

	SocketMode mode;
	LOGPacketFunc *packetTable;
	in_addr addr;
	sockaddr_in LogDAddress;
	char* host;
};

class CLogPacketServer : public CIOObject
{
public:
	typedef bool (*LOGPacketFunc)(CIOSocket* , const unsigned char* packet);

	CIOSocket *m_pSocket;
	CIOBuffer   *m_pBuf;
	LOGPacketFunc	m_pFunc;
	static LONG g_nPendingPacket;
	static CLogPacketServer* Alloc();
	static void FreeAll();
	void Free();
	virtual void OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped);
};

extern CLogSocket *pLogSocket;
extern bool LogDReconnect;
extern bool g_opFlag;
extern CRWLock gLogLock;
#endif // !defined(AFX_LOGSOCKET_H__32C83A11_E718_4C82_B282_F53B5A1E3E1C__INCLUDED_)
