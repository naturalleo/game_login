#include "GlobalAuth.h"
#include "Packet.h"
#include "IOSocket.h"
#include "buildn.h"
#include "protocol.h"
#include "DBConn.h"
#include "util.h"
#include "des.h"
#include "ServerList.h"
#include "Thread.h"
#include "IOServer.h"
#include "config.h"
#include "ipsessiondb.h"

BOOL SendSocketEx(SOCKET mys, const char *format, ...);
extern BOOL SendSocket(in_addr , const char *format, ...);
// ¿©±â¼­ IP¼­¹ö¿¡ °ú±Ý ½ÃÀÛÀÌ¶ó°í ¾Ë·ÁÁÖ¾î¾ß ÇÑ´Ù.
static bool ServerPlayOk( CSocketServer *mysocket, const unsigned char *packet)
{
	return false;
}

static bool ServerPlayFail( CSocketServer *mysocket, const unsigned char *packet )
{

	return false;
}

static bool ServerPlayGame( CSocketServer *mysocket, const unsigned char *packet )
{
	return false;
}

// AS_QUIT_GAME 

static bool ServerPlayQuit( CSocketServer *mysocket, const unsigned char *packet )
{
	return false;
}

static bool ServerKickAccount( CSocketServer *mysocket, const unsigned char *packet )
{

	return false;
}


static bool ServerGetUserNum( CSocketServer *mysocket, const unsigned char *packet )
{

	return false;
}

static bool ServerBanUser( CSocketServer *mysocket, const unsigned char *packet )
{

	return false;
}

static bool ServerVersion( CSocketServer *mysocket, const unsigned char *packet )
{
	// version 
	int version = GetIntFromPacket( packet );
	AS_LOG_VERBOSE( "get server version %d", version );
	
	return false;
}

static bool SetServerId( CSocketServer *mysocket, const unsigned char *packet )
{


	return false;
}

static bool ServerPing( CSocketServer *mysocket, const unsigned char *packet )
{
// Currently, pings are only used to give the TCP protocol a bump.  By sending some data, we can test for connection
// problems.  Therefore, no reply is required.
	AS_LOG_VERBOSE( "RCV: SQ_PING");
	return false;
}

static bool ServerWriteUserData( CSocketServer *mysocket, const unsigned char *packet )
{
	return false;
}

static bool ServerReadUserData( CSocketServer *mysocket, const unsigned char *packet )
{
   
	return false;
}

static bool ServerWriteGameData( CSocketServer *mysocket, const unsigned char *packet )
{

	return false;
}

static bool ServerReadGameData( CSocketServer *mysocket, const unsigned char *packet )
{
  
	return false;
}

static bool ServerShardTransfer( CSocketServer *mysocket, const unsigned char *packet )
{

	return false;
}

// ¼­¹öÂÊ¿¡¼­ ÀÌÁ¦ ·Î±×ÀÎÀ» ¹Þ¾Æµµ ÁÁ´Ù°í ÇÏ´Â ÆÐÅ¶ Ã³¸®ÀÌ´Ù. 
static bool ServerSetActive( CSocketServer *mysocket, const unsigned char *packet )
{

	return false;
}

// ÀüÃ¼ ¸î¸íÀÎÁö ¾Ë·ÁÁÖ¾î¾ß ÇÑ´Ù. 
// ÇÑ¹ø¿¡ 100¸íÀ» ³ÑÁö ¾Êµµ·Ï ÇÏÀÚ. L2´Â ¾Æ·¡¸é ÃæºÐÇÏ´Ù.
// account, uid, stat, ip, loginflag, warnflag 
// ÇÏÁö¸¸ ´Ù¸¥°ÔÀÓÀÌ ÇÊ¿äÇÒ½Ã¿¡´Â md5key¸¦ ÁÖ¾î¾ß ÇÑ´Ù.
// ÀÌ FunctionÀº ÇöÀç Áö¿øÁßÀÎ °ÔÀÓ CODE¿¡ µû¶ó¼­ µû·Î µ¿ÀÛÇÒ ÇÊ¿ä°¡ ÀÖ´Ù.
// select ssn from user_info with (nolock) where account='%s'
static bool ServerPlayUserList( CSocketServer *mysocket, const unsigned char *packet )
{

	return false;
}

static bool ServerUserNumByQueueLevel( CSocketServer *mysocket, const unsigned char *packet )
{
	// what to do????
	return false;
}

static bool FinishedQueue( CSocketServer *mysocket, const unsigned char *packet )
{
	return false;
}

static bool SetLoginFrequency( CSocketServer *mysocket, const unsigned char *packet )
{	
	// This message is sent between the game server and the queue server --- the auth server should ignore it
	return false;
}

static bool QueueSizes( CSocketServer *mysocket, const unsigned char *packet )
{
	return false;
}

static ServerPacketFunc ServerPacket[] = {
	ServerPlayOk,
	ServerPlayFail,
	ServerPlayGame,
	ServerPlayQuit,
	ServerKickAccount,
	ServerGetUserNum,
	ServerBanUser,
	ServerVersion,
	ServerPing,
	ServerWriteUserData,
	ServerSetActive,
	ServerPlayUserList,
	SetServerId,
	ServerUserNumByQueueLevel,
	FinishedQueue,
	SetLoginFrequency,
	QueueSizes,
	ServerReadUserData,   // for AS_READ_USERDATA
    ServerWriteGameData,  // for AS_WRITE_GAMEDATA
    ServerReadGameData,   // for AS_READ_GAMEDATA
    ServerShardTransfer,  // for AS_SHARD_TRANSFER
};

BOOL SendSocketEx(SOCKET mys, const char *format, ...)
{
_BEFORE
	CSocketServerEx *pSocket = serverEx->FindSocket(mys);
	if (!pSocket)
		return FALSE;
	CIOBuffer *pBuffer = CIOBuffer::Alloc();
	char *buffer = pBuffer->m_buffer;
	va_list ap;
	va_start(ap, format);
	int len = Assemble(buffer + 2, BUFFER_SIZE - 2, format, ap);
	va_end(ap);
	if (len == 0) {
		log_1.AddLog(LOG_ERROR, "%d: assemble too large packet. format %s", mys, format);
	} else {
		len -= 1;
	}
	if ( buffer[2] == AC_PLAY_OK )
		pSocket->um_mode = UM_PLAY_OK;

	int tmplen = len+1;
	
	pSocket->EncPacket( (unsigned char *)(buffer+2), pSocket->EncOneTimeKey, tmplen);
	int len2 = tmplen - 1 + config.PacketSizeType;
	buffer[0] = len2;
	buffer[1] = len2 >> 8;

	pBuffer->m_size = tmplen+2;
	pSocket->Write(pBuffer);
	pSocket->ReleaseRef();
_AFTER_FIN
	return TRUE;
}
CSocketServer::CSocketServer( SOCKET aSoc )
: CIOSocket( aSoc )
{
	addr.s_addr = 0;
	host = 0;
	mode = SM_READ_LEN;
	packetTable =ServerPacket;
	opFlag = 0;
}

CSocketServer::~CSocketServer()
{
}

CSocketServer *CSocketServer::Allocate( SOCKET s )
{
	return new CSocketServer( s );
}
void CSocketServer::OnClose(SOCKET closedSocket)
{
	// Must not use the GetSocket() function from within
	// this function!  Instead, use the socket argument
	// passed into the function.
}

void CSocketServer::OnRead()
{
	int pi = 0;
	int ri = m_pReadBuf->m_size;
	int errorNum = 0;
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
				packetLen = inBuf[pi] + (inBuf[pi + 1] << 8) + 1 - config.PacketSizeType;
				if (packetLen <= 0 || packetLen > BUFFER_SIZE) {
					errorNum = 1;
					log_1.AddLog(LOG_ERROR, "%d: bad packet size %d", GetSocket(), packetLen);
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

				if (inBuf[pi] >= AS_MAX) {
					log_1.AddLog(LOG_ERROR, "unknown protocol %d", inBuf[pi]);
					errorNum = 2;
					break;
				} else {
					CPacketServer *pPacket = CPacketServer::Alloc();
					pPacket->m_pSocket = this;
					pPacket->m_pBuf = m_pReadBuf;
					pPacket->m_pFunc = (CPacketServer::ServerPacketFunc) packetTable[inBuf[pi]];
					CIOSocket::AddRef();
					m_pReadBuf->AddRef();
					InterlockedIncrement(&CPacketServer::g_nPendingPacket);
					pPacket->PostObject(pi, g_hIOCompletionPort);
					
					pi += packetLen;
					mode = SM_READ_LEN;
				}
			} else {
				Read(ri - pi);
				return;
			}
		}
		else{
			errorNum = 3;
			break;
		}
	}
	
	time_t ActionTime;
	struct tm ActionTm;

	ActionTime = time(0);
	ActionTm = *localtime(&ActionTime);

	errlog.AddLog( LOG_NORMAL, "main server connection close. invalid status and protocol %s, errorNum :%d", IP(), errorNum);
	CIOSocket::CloseSocket();
}

const char *CSocketServer::IP()
{
	return inet_ntoa(addr);
}

void CSocketServer::OnCreate()
{
	OnRead();
	int reactivationActive = config.IsReactivationActive();

	if (config.ProtocolVer >= GR_REACTIVATION_PROTOCOL_VERSION)
	{
		AS_LOG_VERBOSE( "SND: SQ_VERSION buildnumber :%d, protocol version : %d, reactivation active: %d",
			buildNumber, config.ProtocolVer, reactivationActive );
		Send( "cddd", SQ_VERSION, buildNumber, config.ProtocolVer, reactivationActive );
	}
	else
	{
		AS_LOG_VERBOSE( "SND: SQ_VERSION buildnumber :%d, protocol version : %d",
			buildNumber, PROTOCOL_VERSION );
		Send( "cdd", SQ_VERSION, buildNumber, PROTOCOL_VERSION );
	}
}

void CSocketServer::SetAddress(in_addr inADDR )
{
	addr = inADDR;

//  2004-01-29 darkangel
//  reconnect ¸¦ ¼­Æ÷Æ® ÇÏµµ·Ï µÇ¾î ÀÖ´Ù¸é ÀÌ°ÍÀº ConnectÆÐÅ¶ÀÌ ¿ÔÀ»¶§ Ã³¸®ÇÑ´Ù.

	if ( !config.supportReconnect ) {

		bSetActiveServer = true;
	}
}

void CSocketServer::Send(const char* format, ...)
{
_BEFORE
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
		log_1.AddLog(LOG_ERROR, "%d: assemble too large packet. format %s", GetSocket(), format);
	} else {
		len -= 1;
		len = len + config.PacketSizeType;
		buffer[0] = len;
		buffer[1] = len >> 8;

	}
	pBuffer->m_size = len+3 - config.PacketSizeType;
	Write(pBuffer);
_AFTER_FIN
}


bool CServerKickList::AddKickUser( int uid ) 
{
	bool result = false;

	return result;
}
