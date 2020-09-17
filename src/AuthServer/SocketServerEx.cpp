#include "GlobalAuth.h"
#include "Packet.h"
#include "IOSocket.h"
#include "buildn.h"
#include "protocol.h"
#include "DBConn.h"
#include "util.h"
#include "des.h"
#include "ServerList.h"
#include "IOServer.h"
#include "config.h"
#include "ipsessiondb.h"
#include "iplist.h"
#include <assert.h>

BOOL SendSocket(in_addr , const char *format, ...);


static bool ClientVersion(CSocketServerEx *mysocket, const unsigned char *packet)
{
	in_addr cip = mysocket->getaddr();

	if (config.useForbiddenIPList) {
		if (forbiddenIPList.IpExists(cip)) {
			log_1.AddLog( LOG_DEBUG, "forbiddenIPList: %s", mysocket->IP());
			mysocket->Send("cc", AC_LOGIN_FAIL, S_BLOCKED_IP);
			return false;
		}
	}
	mysocket->m_lastIO = GetTickCount();
	mysocket->Send( "ch", 0, config.serverVersion);
	return false;
}
static bool SelectClientVersion(CSocketServerEx *mysocket, const unsigned char *packet)
{
	unsigned short key = GetShortFromPacket(packet);
	mysocket->m_lastIO = GetTickCount();
	if (key > config.serverVersion)
	{
		mysocket->Send( "cc", 255, S_LIMIT_EXCEED );		
		log_1.AddLog( LOG_ERROR, "select client resource beyond limit" );
		return true;
	}
	const File *file = config.GetClientResourceById(key);
	if (file->fileBuff == NULL)
	{
		mysocket->Send( "cc", 255, S_LOAD_SSN_ERROR );		
		log_1.AddLog( LOG_ERROR, "select no client resource " );
		return true;
	}
	mysocket->SendResource(1, key, file->fileBuff, file->size);
	return false;
}



static ServerPacketFuncEx ServerPacketEx[] = {
	ClientVersion,
	SelectClientVersion,
};

BOOL SendSocket( in_addr ina, const char *format, ...)
{
_BEFORE
	CSocketServer *pSocket = server->FindSocket(ina);
	if (!pSocket)
		return FALSE;
	CIOBuffer *pBuffer = CIOBuffer::Alloc();
	char *buffer = pBuffer->m_buffer;
	va_list ap;
	va_start(ap, format);
	int len = Assemble(buffer + 2, BUFFER_SIZE - 2, format, ap);
	va_end(ap);
	if (len == 0) {
		log_1.AddLog(LOG_ERROR, "%d: assemble too large packet. format %s", pSocket->GetSocket(), format);
	} else {
		len -= 1;
		len = len + config.PacketSizeType;
		buffer[0] = len;
		buffer[1] = len >> 8;
	}
	pBuffer->m_size = len+3-config.PacketSizeType;
	pSocket->Write(pBuffer);
	pSocket->ReleaseRef();
_AFTER_FIN
	return TRUE;
}




VOID CALLBACK SocketExTimerCallback(PVOID lpParam, unsigned char TimerOrWaitFired)
{
	SOCKET s = (SOCKET)lpParam;
	CSocketServerEx *mysocket = serverEx->FindSocket( s );
	if ( mysocket )
		mysocket->OnTimerCallback( );
}

CSocketServerEx::CSocketServerEx( SOCKET aSoc )
: CIOSocket( aSoc )
{
	addr.s_addr = 0;
	host = 0;
	mode = SM_READ_LEN;
	packetTable =ServerPacketEx;
	um_mode = UM_PRE_LOGIN;
	uid = 0;
	m_TimerHandle = NULL;
	int zero = 0;
	

	m_lastIO = GetTickCount();
	InterlockedIncrement( &(reporter.m_SocketCount ));
//	InterlockedIncrement( &(reporter.m_SocketEXObjectCount ));
	int ret = CreateTimerQueueTimer( &m_TimerHandle, g_hSocketTimer,SocketExTimerCallback, (LPVOID)GetSocket(), config.SocketTimeOut, config.SocketTimeOut , 0 );
//	int ret = RegisterTimer( (config.SocketTimeOut), TRUE );
	if ( ret == 0 ) {
		log_1.AddLog(LOG_ERROR, "create socket timer error" );
	}
	if ( config.ProtocolVer == USA_AUTH_PROTOCOL_VERSION )
	{
		EncPacket = Encrypt;
		DecPacket = Decrypt;
	} else if ( config.ProtocolVer >= GLOBAL_AUTH_PROTOCOL_VERSION ) {
		EncPacket = BlowFishEncryptPacket;
		DecPacket = BlowFishDecryptPacket;
	}
}

CSocketServerEx::~CSocketServerEx()
{
//	InterlockedDecrement( &(reporter.m_SocketEXObjectCount ));
}

CSocketServerEx *CSocketServerEx::Allocate( SOCKET s )
{
	return new CSocketServerEx( s );
}

void CSocketServerEx::OnClose(SOCKET closedSocket)
{
	// Must not use the GetSocket() function from within
 	// this function!  Instead, use the socket argument
 	// passed into the function.
	HANDLE timerhandle = NULL;
	#pragma warning( push )
	#pragma warning( disable: 4312 )
	timerhandle = InterlockedExchangePointer( &m_TimerHandle, NULL );
	#pragma warning( pop )
	
	if ( timerhandle )
		DeleteTimerQueueTimer( g_hSocketTimer, timerhandle, NULL );
	m_TimerHandle = NULL;
	mode = SM_CLOSE;
	InterlockedDecrement( &(reporter.m_SocketCount ));
	serverEx->RemoveSocket(closedSocket);
	log_1.AddLog(LOG_DEBUG, "*close connection from %s, %x(%x)", IP(), closedSocket, this);

	IPaccessLimit.DelAccessIP( getaddr() );
}

void CSocketServerEx::OnRead()
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
			if (pi + 2 <= ri) {
				//packetLen = inBuf[pi + 4] + (inBuf[pi + 4 + 1] << 8) ;
				packetLen = *(unsigned short*)(&inBuf[pi]) - 2;
				if (packetLen <= 0 || packetLen > BUFFER_SIZE) {
					log_1.AddLog(LOG_ERROR, "%d: bad packet size %d", m_hSocket, packetLen);
					break;
				} else {
/////////////////////////////
//decode routine need
/////////////////////////////
					log_1.AddLog( LOG_DEBUG, "Received %d bytes ", packetLen);
					pi += 2;
					mode = SM_READ_BODY;
				}
			} else {
				Read(ri - pi);
				return;
			}
		} else if (mode == SM_READ_BODY) {
			if (pi + packetLen <= ri) {
#ifdef _DEBUG
				DumpPacket( inBuf, packetLen+2 );
#endif
				if (inBuf[pi] >= AQ_MAX) {
					log_1.AddLog(LOG_ERROR, "unknown protocol %d", inBuf[pi]);
					break;
				} else {
					CPacketServerEx *pPacket = CPacketServerEx::Alloc();
					pPacket->m_pSocket = this;
					pPacket->m_pBuf = m_pReadBuf;
					pPacket->m_pFunc = (CPacketServerEx::ServerPacketFuncEx) ServerPacketEx[inBuf[pi]];
					CIOSocket::AddRef();
					m_pReadBuf->AddRef();
					InterlockedIncrement(&CPacketServerEx::g_nPendingPacket);
					pPacket->PostObject(pi, g_hIOCompletionPort);
					pi += packetLen;
					mode = SM_READ_LEN;
					m_lastIO = GetTickCount();
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

const char *CSocketServerEx::IP()
{
	return inet_ntoa(addr);
}

void CSocketServerEx::OnCreate()
{
	OnRead();
}

void CSocketServerEx::SetAddress(in_addr inADDR )
{
	addr = inADDR;
}

void CSocketServerEx::Send(const char* format, ...)
{
	if (mode == SM_CLOSE) {
		return;
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

		len = len -1 + config.PacketSizeType;
		
		log_1.AddLog( LOG_DEBUG, "Sent %d bytes", len+3-config.PacketSizeType);
		int tmplen = 	len+1-config.PacketSizeType;
		//EncPacket( (unsigned char *)(buffer+2), EncOneTimeKey, tmplen );
#ifdef _DEBUG
		DumpPacket( (unsigned char*)buffer, len+3-config.PacketSizeType );
#endif
		len = tmplen - 1 + config.PacketSizeType;
		buffer[0] = len;
		buffer[1] = len >> 8;
		
		pBuffer->m_size = tmplen+2;

		Write(pBuffer);
		m_lastIO = GetTickCount();
	}
}

void CSocketServerEx::NonEncSend(const char* format, ...)
{
	if (mode == SM_CLOSE) {
		return;
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
		len = len + config.PacketSizeType;
		buffer[0] = len;
		buffer[1] = len >> 8;

	}
	pBuffer->m_size = len+3-config.PacketSizeType;
	log_1.AddLog( LOG_DEBUG, "Sent %d bytes, Non Encrypt", len+3-config.PacketSizeType);
#ifdef _DEBUG
	DumpPacket( (unsigned char *)buffer, len+3-config.PacketSizeType );
#endif
	Write(pBuffer);
	m_lastIO = GetTickCount();
}

void CSocketServerEx::Send( const char *sendmsg, int msglen )
{
	if (mode == SM_CLOSE) {
		return;
	}

	CIOBuffer *pBuffer = CIOBuffer::Alloc();
	char *buffer = pBuffer->m_buffer;
	memcpy( buffer, sendmsg, msglen);
	log_1.AddLog( LOG_DEBUG, "Sent %d bytes", msglen);
#ifdef _DEBUG
	DumpPacket( (unsigned char *)buffer, msglen );
#endif
	buffer[0] = msglen;
	buffer[1] = msglen >> 8;
	pBuffer->m_size = msglen;
	Write(pBuffer);
}

void CSocketServerEx::SendResource(int cmdId, short key, const char *sendmsg, int msglen )
{
	if (mode == SM_CLOSE) {
		return;
	}

	CIOBuffer *pBuffer = CIOBuffer::Alloc();
	char *buffer = pBuffer->m_buffer;
	memcpy(buffer + 5, sendmsg, msglen);
	log_1.AddLog( LOG_DEBUG, "Sent %d bytes", msglen);
//#ifdef _DEBUG
//	DumpPacket( (unsigned char *)buffer, msglen );
//#endif
	int len = msglen + 1 + 2 + 2;
	buffer[0] = len;
	buffer[1] = len >> 8;
	buffer[2] = cmdId & 0xff;
	buffer[3] = key & 0xff;
	buffer[4] = (key >> 8) & 0xff;	
	pBuffer->m_size = len;
	Write(pBuffer);
}

void CSocketServerEx::OnTimerCallback( void )
{
	if (mode == SM_CLOSE) {
		ReleaseRef();
		return;
	}
	DWORD curTick = GetTickCount();
	if ( curTick - (DWORD)m_lastIO >= (DWORD)config.SocketTimeOut - (DWORD)500 ){
		CIOSocket::CloseSocket();
	} 
	ReleaseRef();
	
	return;
}
