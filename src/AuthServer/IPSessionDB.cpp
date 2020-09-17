// IPSessionDB.cpp: implementation of the CIPSessionDB class.
//
//////////////////////////////////////////////////////////////////////

#include "IPSessionDB.h"
#include "util.h"
#include "config.h"
#include "buildn.h"
#include "IOServer.h"

bool IPServerReconnect = false;
bool g_IPServeropFlag = false;
HANDLE g_hIPServerTimer = NULL;
CRWLock gIPLock;

LONG CIPPacketServer::g_nPendingPacket;
CIPSessionDB ipsessionDB;
extern bool g_bTerminating;
extern BOOL SendSocketEx(SOCKET s, const char *format, ...);


VOID CALLBACK IPSocketTimerRoutine(PVOID lpParam, BYTE TimerOrWaitFired)
{
	AS_LOG_VERBOSE( "IPSocketTimerRoutine" );
	if ( g_hIPServerTimer )
		DeleteTimerQueueTimer( NULL, g_hIPServerTimer, NULL );
	
	g_hIPServerTimer = NULL;

	if ( IPServerReconnect == true ) {
		SOCKET LOGSock = socket(AF_INET, SOCK_STREAM, 0);
		// 2. 소켓 Connection에 사용할 Destination Setting을 한다.
		sockaddr_in Destination;
		Destination.sin_family = AF_INET;
		Destination.sin_addr   = config.IPServer;
		Destination.sin_port   = htons( (u_short)config.IPPort );
		// 3. Connection을 맺는다. 
		
		// 4. 맺어진 Connection을 이용하여 LOGSocket을 생성한다. 
		//    Connection Error가 생겼더라도 관계 없다. 
		//    그렇게 되면 자동적으로 Timer가 작동하여 10초에 한번씩 Reconnection을 시도하게 된다. 

		int ErrorCode = connect( LOGSock, ( sockaddr *)&Destination, sizeof( sockaddr ));
		
		CIPSocket *tempIPSocket = CIPSocket::Allocate(LOGSock);
		tempIPSocket->SetAddress( config.IPServer );
		if ( ErrorCode == SOCKET_ERROR ){
			tempIPSocket->CloseSocket();
			tempIPSocket->ReleaseRef();
		} else {
			gIPLock.WriteLock();
			CIPSocket *tmpIPSocket = pIPSocket;
			pIPSocket = tempIPSocket;
			IPServerReconnect = false;
			config.UseIPServer = true;
			pIPSocket->Initialize( g_hIOCompletionPort );
			gIPLock.WriteUnlock();
			tmpIPSocket->ReleaseRef();
		}	
	} 
}


class CIPPacketServerPool
{
public:
	class CSlot
	{
	public:
		CIPPacketServer*   m_pPacket;
		CLock m_lock;
		CSlot() : m_pPacket(NULL),m_lock(eCustomSpinLock) {}
	};
	static CSlot g_slot[16];
	static long	g_nAlloc;
	static long	g_nFree;
	~CIPPacketServerPool() { CIPPacketServer::FreeAll(); }
};

CIPPacketServerPool::CSlot	CIPPacketServerPool::g_slot[16];
long	CIPPacketServerPool::g_nAlloc = -1;
long	CIPPacketServerPool::g_nFree = 0;
CIPPacketServerPool theIPPacketPool;

CIPPacketServer * CIPPacketServer::Alloc()
{
	CIPPacketServer *newPacket;

	CIPPacketServerPool::CSlot *pSlot =
		&CIPPacketServerPool::g_slot[InterlockedIncrement(&CIPPacketServerPool::g_nAlloc) & 15];
	pSlot->m_lock.Enter();
	if ((newPacket = pSlot->m_pPacket) != NULL) {
		pSlot->m_pPacket = reinterpret_cast<CIPPacketServer *> (newPacket->m_pSocket);
		pSlot->m_lock.Leave();
	}
	else {
		pSlot->m_lock.Leave();
		newPacket = new CIPPacketServer;
	}

	return newPacket;
}

void CIPPacketServer::FreeAll()
{
	for (int i = 0 ; i < 16; i++) {
		CIPPacketServerPool::CSlot *pSlot = &CIPPacketServerPool::g_slot[i];
		pSlot->m_lock.Enter();
		CIPPacketServer *pPacket;
		while ((pPacket = pSlot->m_pPacket) != NULL) {
			pSlot->m_pPacket = reinterpret_cast<CIPPacketServer *> (pPacket->m_pSocket);
			delete pPacket;
		}
		pSlot->m_lock.Leave();
	}
}

void CIPPacketServer::Free()
{
	CIPPacketServerPool::CSlot *pSlot =
		&CIPPacketServerPool::g_slot[InterlockedDecrement(&CIPPacketServerPool::g_nFree) & 15];
	pSlot->m_lock.Enter();
	m_pSocket = reinterpret_cast<CIOSocket *>(pSlot->m_pPacket);
	pSlot->m_pPacket = this;
	pSlot->m_lock.Leave();
}

void CIPPacketServer::OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped)
{
_BEFORE
	unsigned char *packet = (unsigned char *) m_pBuf->m_buffer + dwTransferred;

	if ((*m_pFunc)(m_pSocket, packet + 1)) {
		m_pSocket->CIOSocket::CloseSocket();
	}

	m_pSocket->ReleaseRef();
	m_pBuf->Release();
	InterlockedDecrement(&g_nPendingPacket);
	Free();
_AFTER_FIN
	return;
}


CIPSessionDB::CIPSessionDB()
{

}

CIPSessionDB::~CIPSessionDB()
{

}


int CIPSessionDB::FindSessionID( int Uid )
{
	int SessionID = 0;
_BEFORE
	IPSessionLock.Enter();
	SESSIONMAP::iterator it = IPSessionMap.find( Uid );
	if ( it != IPSessionMap.end() )
		SessionID = it->second;
	IPSessionLock.Leave();
_AFTER_FIN
	return SessionID;
}

int CIPSessionDB::DelSessionID( int Uid )
{
	int SessionID = 0;
_BEFORE
	IPSessionLock.Enter();
	SESSIONMAP::iterator it = IPSessionMap.find( Uid );
	if ( it != IPSessionMap.end() ){
		SessionID = it->second;
		IPSessionMap.erase(it);
	}
	IPSessionLock.Leave();
_AFTER_FIN
	return SessionID;
}

bool CIPSessionDB::DellAllWaitingSessionID( void )
{
	return true;
}

// SessionID는 어떠한 경우에도 0이 오면 안된다.
int CIPSessionDB::AddSessionID ( int uid, int sessionid )
{
_BEFORE
	bool result=false;

	if ( sessionid == 0 )
		return sessionid;

	IPSessionLock.Enter();
	std::pair<SESSIONMAP::iterator, bool> r	= IPSessionMap.insert(SESSIONMAP::value_type(uid, sessionid));		
	result = r.second;
	IPSessionLock.Leave();
	if ( result == true )
		return sessionid;
	else
		return 0;
_AFTER_FIN
	return 0;
}

char CIPSessionDB::ReleaseSessionRequest(int IPSession, in_addr IP, int kind)
{
	char ErrorCode = 0;
_BEFORE
	if ( IPSession == 0 )
		return ErrorCode;

	if ( config.UseIPServer && (!IPServerReconnect) &&( pIPSocket != NULL) ){
		gIPLock.ReadLock();
		pIPSocket->Send( "cddddd", AI_IP_RELEASE, IPSession, pIPSocket->ConnectSessionKey, config.gameId, IP.S_un.S_addr, kind);
		gIPLock.ReadUnlock();
	}
_AFTER_FIN
	return ErrorCode;
}


char CIPSessionDB::AcquireSessionSuccess( int Uid, int IPSession, char ErrorCode, int SpecificTime, int Kind )
{
	return ErrorCode;
}
char CIPSessionDB::AcquireSessionFail( int Uid, int IPSession, char ErrorCode )
{
	return ErrorCode;
}

static bool DummyPacket( CIPSocket *s, const unsigned char *packet )
{
	log_1.AddLog( LOG_WARN, "Call DummyPacket What What What" );
	return false;
}
static bool StartIPChargeFail( CIPSocket *s, const unsigned char *packet )
{
	return false;
}

static bool StartIPCharge( CIPSocket *s, const unsigned char *packet )
{

	return false;
}

static bool GetIPAcquireSuccess( CIPSocket *s, const unsigned char *packet )
{
_BEFORE
	int  uid  = GetIntFromPacket( packet );
	char kind = GetCharFromPacket( packet );
	int  SpecTime = GetIntFromPacket( packet );
	int  SessionID = GetIntFromPacket( packet );

#ifdef _DEBUG
	log_1.AddLog( LOG_WARN, "IA_IP_USE_Success,uid:%d,kind:%d,SpecTime:%d,SessionID:%d", uid, kind, SpecTime, SessionID );
#endif
	ipsessionDB.AcquireSessionSuccess( uid, SessionID, 0, SpecTime, (int)kind );
_AFTER_FIN
	return false;
}

static bool GetIPAcquireFail( CIPSocket *s, const unsigned char *packet )
{
_BEFORE
	int  uid;
	char ErrorCode;

	uid = GetIntFromPacket( packet );
	ErrorCode = GetCharFromPacket( packet );
	
	if ( uid > 0 )
		ipsessionDB.AcquireSessionFail( uid, 0, ErrorCode );

#ifdef _DEBUG
	log_1.AddLog( LOG_WARN, "IA_IP_USE_FAIL,FAILCODE:%d,UID:%d",ErrorCode, uid );
#endif
_AFTER_FIN
	return false;
}

static bool GetConnectSessionKey( CIPSocket *s, const unsigned char *packet )
{
_BEFORE
	UINT SessionKey = (UINT)GetIntFromPacket( packet );
	s->SetConnectSessionKey( SessionKey );
	AS_LOG_VERBOSE( "IA_SERVER_VERSION,SessionKey %d", SessionKey );
_AFTER_FIN	
	return false;
}


static bool GetIPKick( CIPSocket *s, const unsigned char *packet )
{

	return false;
}

static bool ReadyIPOK( CIPSocket *s, const unsigned char *packet )
{

	return false;
}
// ReadyIPFail은 Acquire가 된 다음에 사용되는 것이다.  그렇기 때문에 AcquireSessionFail과는 틀리다.
// logout을 불러줄수 있다면 그게 좋다.
static bool ReadyIPFail( CIPSocket *s, const unsigned char *packet )
{
	return false;
}

static bool SetStartTimeFail( CIPSocket *s, const unsigned char *packet )
{
	return false;
}

static IPPacketFunc IPPacketFuncTable[] = {
	GetConnectSessionKey, // 0
	DummyPacket, // 1
	GetIPAcquireSuccess, //2
	StartIPCharge, //3 
	StartIPChargeFail,//4
	GetIPAcquireFail, //5
	DummyPacket, //6
	DummyPacket, //7
	DummyPacket, //8
	GetIPKick, //9
	ReadyIPFail, //10
	ReadyIPOK, // 11
	DummyPacket,//IA_IP_SET_STARTTIME_OK, //12
	SetStartTimeFail,//IA_IP_SET_STARTTIME_FAIL, //13
};

CIPSocket *CIPSocket::Allocate( SOCKET s )
{
	return new CIPSocket( s );
}

CIPSocket::CIPSocket( SOCKET aSoc )
: CIOSocket( aSoc )
{
	addr = config.IPServer;
	host = 0;
	mode = SM_READ_LEN;
	packetTable =IPPacketFuncTable;
	opFlag = 0;
	Destination.sin_family = AF_INET;
	Destination.sin_addr   = config.IPServer;
	Destination.sin_port   = htons( (u_short)config.IPPort );
	ConnectSessionKey = 0;
	IPServerReconnect = false;
}

CIPSocket::~CIPSocket()
{
//	if( reconnect == true )
//		log_1.AddLog( LOG_ERROR, "Reconnected set" );
	log_1.AddLog( LOG_ERROR, "IPSocket Deleted" );
}

void CIPSocket::OnClose(SOCKET closedSocket)
{
	// Must not use the GetSocket() function from within
 	// this function!  Instead, use the socket argument
 	// passed into the function.

	mode = SM_CLOSE;
	IPServerReconnect = true;
	config.UseIPServer = false;

	log_1.AddLog(LOG_ERROR, "*close connection IPServer from %s, %x(%x)", IP(), closedSocket, this);
/*
	if ( !g_bTerminating ) {
		EnterCriticalSection( &m_cs );
		if ( !reconnect ){
			RegisterTimer( 30000, true );
			reconnect = true;
			config.UseIPServer = false;
		}
		LeaveCriticalSection( &m_cs );
	}
*/
	ipsessionDB.DellAllWaitingSessionID();
	AddRef();
	CreateTimerQueueTimer( &g_hIPServerTimer, NULL, IPSocketTimerRoutine, this, config.IPConnectInterval, 0, 0 );
}

void CIPSocket::OnTimerCallback( void )
{
/*
	log_1.AddLog( LOG_WARN, "Timer Callback Reconnect IPServer Timer Called" );
	if ( g_bTerminating )
	{
		ReleaseRef();
		return ;
	}

	EnterCriticalSection( &m_cs );
	if ( !reconnect ){
		LeaveCriticalSection( &m_cs );
		return ;
	}
	if ( m_hSocket != INVALID_SOCKET ){
		closesocket( m_hSocket );
		m_hSocket = INVALID_SOCKET;
	}
	m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_hSocket == INVALID_SOCKET) {
		log_1.AddLog(LOG_ERROR, "socket error %d", WSAGetLastError());
		RegisterTimer( 30000, true );
		LeaveCriticalSection( &m_cs );
		ReleaseRef();
		return ;
	}

	int ErrorCode = connect( m_hSocket, ( sockaddr *)(&Destination), sizeof( sockaddr ));
	
	if ( ErrorCode == SOCKET_ERROR ){
		closesocket( m_hSocket );
		RegisterTimer( 30000, true );
		LeaveCriticalSection( &m_cs );
		ReleaseRef();
		return ;
	} else {
		reconnect = false;
		mode = SM_READ_LEN;
		LeaveCriticalSection( &m_cs );
		Initialize( g_hIOCompletionPort );
		config.UseIPServer = true;
		ReleaseRef();
		return ;
	}
*/
}

const char *CIPSocket::IP()
{
	return inet_ntoa(addr);
}

void CIPSocket::OnCreate()
{
	AddRef();
	OnRead();
	Send( "cdc", AI_SERVER_VERSION, buildNumber, config.gameId );
}

void CIPSocket::OnRead()
{
	int pi = 0;
	int ri = m_pReadBuf->m_size;
	unsigned char *inBuf = (unsigned char *)m_pReadBuf->m_buffer;
	if (mode == SM_CLOSE) {
		CloseSocket();
		return;
	}

	for  ( ; ; ) {
		if (pi >= ri) {
			pi = 0;
			Read(0);
			return;
		}
		if (mode == SM_READ_LEN) {
			if (pi + 3 <= ri) {
				packetLen = inBuf[pi] + (inBuf[pi + 1] << 8) + 1;
				if (packetLen <= 0 || packetLen > BUFFER_SIZE) {
					log_1.AddLog(LOG_ERROR, "%d: bad packet size %d", m_hSocket, packetLen);
					break;
				} else {
					pi += 2;
					mode = SM_READ_BODY;
				}
			} else {
				Read(ri - pi);
				return;
			}
		} else if (mode == SM_READ_BODY) {
			if (pi + packetLen <= ri) {

				if (inBuf[pi] >= IA_MAX) {
					log_1.AddLog(LOG_ERROR, "unknown protocol %d", inBuf[pi]);
					break;
				} else {
					CIPPacketServer *pPacket = CIPPacketServer::Alloc();
					pPacket->m_pSocket = this;
					pPacket->m_pBuf = m_pReadBuf;
					pPacket->m_pFunc = (CIPPacketServer::IPPacketFunc) packetTable[inBuf[pi]];
					CIOSocket::AddRef();
					m_pReadBuf->AddRef();
					InterlockedIncrement(&CIPPacketServer::g_nPendingPacket);
					pPacket->PostObject(pi, g_hIOCompletionPort);
					pi += packetLen;
					mode = SM_READ_LEN;
				}
			} else {
				Read(ri - pi);
				return;
			}
		}
		else
			break;
	}
	CIOSocket::CloseSocket();
}


bool CIPSocket::Send(const char* format, ...)
{
	AddRef();
	if (mode == SM_CLOSE || IPServerReconnect || !config.UseIPServer ) {
		ReleaseRef();
		return false;
	}

	CIOBuffer *pBuffer = CIOBuffer::Alloc();
	char *buffer = pBuffer->m_buffer;
	va_list ap;
	va_start(ap, format);
	int len = Assemble(buffer + 2, BUFFER_SIZE - 2, format, ap);
	va_end(ap);
	if (len == 0) {
		log_1.AddLog(LOG_ERROR, "%d: assemble too large packet. format %s", m_hSocket, format);
	} else {
		len -= 1;
		len = len;
		buffer[0] = len;
		buffer[1] = len >> 8;
	}
	pBuffer->m_size = len+3;
	Write(pBuffer);
	ReleaseRef();
	return true;
}

char CIPSessionDB::StartIPCharge(UINT uid, UINT ip, int kind, ServerId WorldID)
{
//  과금을 위해 IPServer에 보내야 하는 것은 다음과 같다. 
//	UINT SessionID	//	UINT g_SKey		//	int	Uid		//	char WorldID //	int IP	//	int     Kind		

	char ErrorCode=IP_ALL_OK;
_BEFORE
	UINT IPSessionID = FindSessionID( uid );

	if ( pIPSocket == NULL || IPServerReconnect || !config.UseIPServer || (IPSessionID == 0) )
		return IP_SERVER_SOCKET_FAIL;
	
	gIPLock.ReadLock();
	bool result = pIPSocket->Send( "cdddcdd", AI_IP_START_CHARGE, 
											  IPSessionID, 
											  pIPSocket->ConnectSessionKey,
											  uid,
											  WorldID,
											  ip,
											  kind);
	gIPLock.ReadUnlock();
	if ( !result )
		ErrorCode = IP_SERVER_SOCKET_FAIL;
_AFTER_FIN	
	return ErrorCode;
}


char CIPSessionDB::StopIPCharge(UINT uid, UINT ip, int kind, int UseTime, time_t loginTime, ServerId lastworld, const char *account)
{
	char ErrorCode=IP_ALL_OK;
_BEFORE
	int SessionID = FindSessionID( uid );
	bool result = true;
	if ( SessionID == 0 )
		return IP_SERVER_SOCKET_FAIL;

	if ( (config.UseIPServer) && (SessionID) != 0 && (pIPSocket!=NULL) && ( !IPServerReconnect)){
		gIPLock.ReadLock();
		result = pIPSocket->Send( "cddddcddsd", AI_IP_STOP_CHARGE, 
									   pIPSocket->ConnectSessionKey, 
									   SessionID,
									   ip,
									   kind,
									   lastworld,
									   UseTime,
									   loginTime, 
									   account,
									   config.gameId );
		gIPLock.ReadUnlock();
	}
	if (!result)
		ErrorCode = IP_SERVER_SOCKET_FAIL;
_AFTER_FIN
	
	return ErrorCode;
}

char CIPSessionDB::ReadyToIPCharge(UINT uid, UINT ip, int kind, ServerId WorldID)
{
//  과금을 위해 IPServer에 보내야 하는 것은 다음과 같다. 
//	UINT SessionID	//	UINT g_SKey		//	int	Uid		//	char WorldID //	int IP	//	int     Kind		
	char ErrorCode=IP_ALL_OK;
_BEFORE

	UINT IPSessionID = FindSessionID( uid );

	if ( pIPSocket == NULL || IPServerReconnect || !config.UseIPServer || (IPSessionID == 0) )
		return IP_SERVER_SOCKET_FAIL;
	
	gIPLock.ReadLock();
	bool result = pIPSocket->Send( "cdddcdd", AI_IP_READY_GAME, 
											  IPSessionID, 
											  pIPSocket->ConnectSessionKey,
											  uid,
											  WorldID,
											  ip,
											  kind);
	gIPLock.ReadUnlock();

	if ( !result )
		ErrorCode = IP_SERVER_SOCKET_FAIL;
_AFTER_FIN	
	return ErrorCode;
}

char CIPSessionDB::ConfirmIPCharge( UINT uid, UINT ip, int kind, ServerId WorldID )
{
	char ErrorCode = IP_ALL_OK;
_BEFORE
	UINT IPSessionID = FindSessionID(uid);

	if ( pIPSocket == NULL || IPServerReconnect || !config.UseIPServer || (IPSessionID == 0) )
		return IP_SERVER_SOCKET_FAIL;

	gIPLock.ReadLock();
	bool result = pIPSocket->Send( "cdddcdd", AI_IP_SET_START_TIME, 
											  IPSessionID,
											  pIPSocket->ConnectSessionKey, 
											  uid, 
											  WorldID, 
											  ip,
											  kind );
	gIPLock.ReadUnlock();

	if (!result)
		ErrorCode = IP_SERVER_SOCKET_FAIL;
_AFTER_FIN
	return ErrorCode;
}