// WantedSocket.cpp: implementation of the CWantedSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "PreComp.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool WantedServerReconnect = false;
CRWLock gWantedLock;
LONG CWantedPacketServer::g_nPendingPacket;
CWantedSocket *pWantedSocket;
HANDLE g_hWantedServerTimer=NULL;

VOID CALLBACK WantedSocketTimerRoutine(PVOID lpParam, BYTE TimerOrWaitFired)
{
	AS_LOG_DEBUG( "WantedSocketTimerRoutine" );
	if ( g_hWantedServerTimer )
		DeleteTimerQueueTimer( NULL, g_hWantedServerTimer, NULL );
	g_hWantedServerTimer = NULL;

	if ( WantedServerReconnect == true ) {
		SOCKET WantedSock = socket(AF_INET, SOCK_STREAM, 0);

		sockaddr_in Destination;
		Destination.sin_family = AF_INET;
		Destination.sin_addr   = config.WantedIP;
		Destination.sin_port   = htons( (u_short)config.WantedPort );

		int ErrorCode = connect( WantedSock, ( sockaddr *)&Destination, sizeof( sockaddr ));
		
		CWantedSocket *tempWantedSocket = CWantedSocket::Allocate(WantedSock);
		tempWantedSocket->SetAddress( config.WantedIP );
		if ( ErrorCode == SOCKET_ERROR ){
			tempWantedSocket->CloseSocket();
			tempWantedSocket->ReleaseRef();
		} else {
			gWantedLock.WriteLock();
			CWantedSocket *tmpWantedSocket = pWantedSocket;
			pWantedSocket = tempWantedSocket;
			tmpWantedSocket->ReleaseRef();
			WantedServerReconnect = false;
			config.UseWantedSystem = true;
			pWantedSocket->Initialize( g_hIOCompletionPortInt );
			gWantedLock.WriteUnlock();
		}	
	} 
}

class CWantedPacketServerPool
{
public:
	class CSlot
	{
	public:
		CWantedPacketServer*   m_pPacket;
		CLock m_lock;
		CSlot() : m_pPacket(NULL),m_lock(eCustomSpinLock) {}
	};
	static CSlot g_slot[16];
	static long	g_nAlloc;
	static long	g_nFree;
	~CWantedPacketServerPool() { CWantedPacketServer::FreeAll(); }
};

CWantedPacketServerPool::CSlot	CWantedPacketServerPool::g_slot[16];
long	CWantedPacketServerPool::g_nAlloc = -1;
long	CWantedPacketServerPool::g_nFree = 0;
CWantedPacketServerPool theWantedPacketPool;


CWantedPacketServer * CWantedPacketServer::Alloc()
{
	CWantedPacketServer *newPacket;

	CWantedPacketServerPool::CSlot *pSlot =
		&CWantedPacketServerPool::g_slot[InterlockedIncrement(&CWantedPacketServerPool::g_nAlloc) & 15];
	pSlot->m_lock.Enter();
	if ((newPacket = pSlot->m_pPacket) != NULL) {
		pSlot->m_pPacket = reinterpret_cast<CWantedPacketServer *> (newPacket->m_pSocket);
		pSlot->m_lock.Leave();
	}
	else {
		pSlot->m_lock.Leave();
		newPacket = new CWantedPacketServer;
	}

	return newPacket;
}

void CWantedPacketServer::FreeAll()
{
	for (int i = 0 ; i < 16; i++) {
		CWantedPacketServerPool::CSlot *pSlot = &CWantedPacketServerPool::g_slot[i];
		pSlot->m_lock.Enter();
		CWantedPacketServer *pPacket;
		while ((pPacket = pSlot->m_pPacket) != NULL) {
			pSlot->m_pPacket = reinterpret_cast<CWantedPacketServer *> (pPacket->m_pSocket);
			pPacket->ReleaseRef();
		}
		pSlot->m_lock.Leave();
	}
}

void CWantedPacketServer::Free()
{
	CWantedPacketServerPool::CSlot *pSlot =
		&CWantedPacketServerPool::g_slot[InterlockedDecrement(&CWantedPacketServerPool::g_nFree) & 15];
	pSlot->m_lock.Enter();
	m_pSocket = reinterpret_cast<CIOSocket *>(pSlot->m_pPacket);
	pSlot->m_pPacket = this;
	pSlot->m_lock.Leave();
}

void CWantedPacketServer::OnIOCallback(BOOL bSuccess, DWORD dwTransferred, LPOVERLAPPED lpOverlapped)
{
_BEFORE
	unsigned char *packet = (unsigned char *) m_pBuf->m_buffer + dwTransferred;

	if ((*m_pFunc)(m_pSocket, packet)) {
		m_pSocket->CIOSocket::CloseSocket();
	}

	m_pSocket->ReleaseRef();
	m_pBuf->Release();
	InterlockedDecrement(&g_nPendingPacket);
	Free();
_AFTER_FIN
	return;
}

static bool DummyPacket( CWantedSocket *s, const unsigned char *packet )
{
	static char buf[1024] = {0};
	memset(buf, 0, sizeof(buf));
	int size = s->GetPacketLen();
//#ifdef _DEBUG
//	DumpPacket((unsigned char *)packet, size);
//#endif

	strncpy(buf, (const char*)packet, size);

_BEFORE
	log_1.AddLog( LOG_WARN, "Call DummyPacket len: %d What What %s", size, buf);
_AFTER_FIN
	return false;
}

static WantedPacketFunc WantedPacketFuncTable[] = {
	DummyPacket,
};

CWantedSocket *CWantedSocket::Allocate( SOCKET s )
{
	return new CWantedSocket( s );
}

CWantedSocket::CWantedSocket( SOCKET aSoc )
: CIOSocket( aSoc )
{
	addr = config.WantedIP;
	host = 0;
	mode = SM_READ_LEN;
	packetTable =WantedPacketFuncTable;
	WantedServerReconnect = false;
}
CWantedSocket::~CWantedSocket()
{
	log_1.AddLog( LOG_ERROR, "WantedSocket Deleted" );
}

void CWantedSocket::OnClose(SOCKET closedSocket)
{
	// Must not use the GetSocket() function from within
 	// this function!  Instead, use the socket argument
 	// passed into the function.

	mode = SM_CLOSE;
	WantedServerReconnect = true;
	config.UseWantedSystem = false;

	log_1.AddLog(LOG_ERROR, "*close connection WantedSocket from %s, %x(%x)", IP(), closedSocket, this);
	AddRef();
	CreateTimerQueueTimer( &g_hWantedServerTimer, NULL, WantedSocketTimerRoutine, this, config.WantedReconnectInterval, 0, 0 );
}


void CWantedSocket::OnTimerCallback( void )
{
}

const char *CWantedSocket::IP()
{
	return inet_ntoa(addr);
}

void CWantedSocket::OnCreate()
{
	AddRef();
	OnRead();
}

void CWantedSocket::OnRead()
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
			mode = SM_READ_BODY;
			packetLen = ri;
		} else if (mode == SM_READ_BODY) {
			if (pi + packetLen <= ri) {
				CWantedPacketServer *pPacket = CWantedPacketServer::Alloc();
				pPacket->m_pSocket = this;
				pPacket->m_pBuf = m_pReadBuf;
				pPacket->m_pFunc = (CWantedPacketServer::WantedPacketFunc) packetTable[0];
				CIOSocket::AddRef();
				m_pReadBuf->AddRef();
				InterlockedIncrement(&CWantedPacketServer::g_nPendingPacket);
				pPacket->PostObject(pi, g_hIOCompletionPort);
				pi += packetLen;
				mode = SM_READ_LEN;
			}
		}
		else
			break;
	}
	CIOSocket::CloseSocket();
}


bool CWantedSocket::Send(const char* format, ...)
{
	AddRef();
	if (mode == SM_CLOSE || WantedServerReconnect || !config.UseWantedSystem || m_hSocket == INVALID_SOCKET) {
		ReleaseRef();
		return false;
	}

	CIOBuffer *pBuffer = CIOBuffer::Alloc();
	char *buffer = pBuffer->m_buffer;
	va_list ap;
	va_start(ap, format);
	int len = Assemble(buffer, BUFFER_SIZE, format, ap);
	va_end(ap);
	if (len == 0) {
		log_1.AddLog(LOG_ERROR, "%d: assemble too large packet. format %s", m_hSocket, format);
	}
	pBuffer->m_size = len;
	Write(pBuffer);
	ReleaseRef();
	return true;
}