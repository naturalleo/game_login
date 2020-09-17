// ServerList.cpp: implementation of the CServerList class.
//
//////////////////////////////////////////////////////////////////////
#include "ServerList.h"
#include "DBConn.h"
#include "config.h"
#include "util.h"
#include "IOSocket.h"

CServerList g_ServerList;

static void CALLBACK UpdateDBHelper(PVOID /* param */, BYTE /* TimerOrWaitFired */)
{
	g_ServerList.UpdateDB();
}

CServerList::CServerList() :
	m_serverList(),
	m_timer(),
	mylock()
{
	CreateTimerQueueTimer( &m_timer, NULL, UpdateDBHelper, NULL , 300000, 300000, 0);    
}

CServerList::~CServerList()
{
	DeleteTimerQueueTimer( NULL, m_timer, NULL );
}

void CServerList::Load()
{
}

void CServerList::SetServerStatus(ServerId id, ServerStatus status)
{
	mylock.WriteLock();
	ServerListType::iterator i=m_serverList.find(id);
	if (i!=m_serverList.end())
	{
		i->second.status = status;
	}
	mylock.WriteUnlock();
}

void CServerList::SetServerVIPStatus(ServerId id, int isVIP)
{
	mylock.WriteLock();
	ServerListType::iterator i=m_serverList.find(id);
	if (i!=m_serverList.end())
	{
		i->second.isVIP = isVIP;
	}
	mylock.WriteUnlock();
}

ServerId CServerList::SetServerSocketByAddress(in_addr address, CSocketServer *socket)
{
	ServerId id;

	mylock.WriteLock();
	for (ServerListType::iterator i=m_serverList.begin(); i!= m_serverList.end(); ++i)
	{
		if (i->second.inner_addr.s_addr == address.s_addr)
		{
			if (i->second.s)
			{
				// Two connections from the same server.  Drop the old connection
				log_1.AddLog( LOG_ERROR, "Got second connection from Server %d.  Dropping first connection.", static_cast<int>(id.GetValueChar()));
				CSocketServer *oldSocket = i->second.s;
				i->second.s=NULL;

				mylock.WriteUnlock(); //CloseSocket() calls back into CServerList::RemoveServer, so the lock must be released first
				oldSocket->CloseSocket();
				mylock.WriteLock();
			}
			i->second.s = socket;
			id = i->first;
		}
	}

	if (config.allowUnknownServers && !id.IsValid())
	{
		id = GetFreeServerId();

		WorldServer newServer;
		_snprintf(newServer.name,sizeof(newServer.name),"Server %d",static_cast<int>(id.GetValueChar()));
		newServer.s = socket;
		newServer.inner_addr=address;
		newServer.outer_addr=address;
		newServer.outer_port=config.worldPort;
		newServer.ageLimit = 0;
		newServer.UserNum = 0;
		newServer.maxUsers = 0;
		newServer.pkflag = 0;
		
		m_serverList.insert(std::make_pair(id,newServer));

		log_1.AddLog( LOG_NORMAL, "Added Server %d which was not in the database (allowUnkownServers is enabled)",
				static_cast<int>(id.GetValueChar()));
	}
	mylock.WriteUnlock();

	return id;
}

void CServerList::RemoveSocket(CSocketServer *socket)
{
	mylock.WriteLock();
	for (ServerListType::iterator i=m_serverList.begin(); i!= m_serverList.end(); ++i)
	{
		if (i->second.s == socket)
		{
			i->second.status = 0;
			i->second.s = NULL;
		}
	}
	mylock.WriteUnlock();
}

bool CServerList::SetServerSocketById(ServerId id, CSocketServer * socket, in_addr actualAddress, short int actualPort)
{
	bool found = false;

	mylock.WriteLock();
	ServerListType::iterator i=m_serverList.find(id);
	if (i!=m_serverList.end())
	{
		if (i->second.s)
		{
			// Two connections from the same server.  Drop the old connection
			log_1.AddLog( LOG_ERROR, "Got second connection from Server %d.  Dropping first connection.", static_cast<int>(id.GetValueChar()));
			CSocketServer *oldSocket = i->second.s;
			i->second.s=NULL;

			mylock.WriteUnlock();
			oldSocket->CloseSocket();
			mylock.WriteLock();
		}
		i->second.s = socket;
		i->second.inner_addr=actualAddress;
		i->second.outer_addr=actualAddress;
		i->second.outer_port=actualPort;
		found = true;
	}

	if (config.allowUnknownServers && !found)
	{
		WorldServer newServer;
		_snprintf(newServer.name,sizeof(newServer.name),"Server %d",static_cast<int>(id.GetValueChar()));
		newServer.s = socket;
		newServer.inner_addr=actualAddress;
		newServer.outer_addr=actualAddress;
		newServer.outer_port=actualPort;
		newServer.serverid=id;
		newServer.ageLimit = 0;
		newServer.UserNum = 0;
		newServer.maxUsers = 0;
		newServer.pkflag = 0;
        newServer.region_id = 0;
		
		m_serverList.insert(std::make_pair(id,newServer));

		log_1.AddLog( LOG_NORMAL, "Added Server %d which was not in the database (allowUnkownServers is enabled)",
				static_cast<int>(id.GetValueChar()));

		found = true;
	}

	mylock.WriteUnlock();

	return found;
}

in_addr CServerList::GetInternalAddress(ServerId id) const
{
	in_addr result;
	result.S_un.S_addr=0;

	mylock.ReadLock();
	ServerListType::const_iterator i=m_serverList.find(id);
	if (i!=m_serverList.end())
	{
		result = i->second.inner_addr;
	}
	mylock.ReadUnlock();

	return result;
}

bool CServerList::IsServerUp(ServerId id) const
{
	bool result=false;

	mylock.ReadLock();
	ServerListType::const_iterator i=m_serverList.find(id);
	if (i!=m_serverList.end())
	{
		result = i->second.status != 0; 
	}
	mylock.ReadUnlock();

	return result;
}

bool CServerList::IsServerVIPonly(ServerId id) const
{
	bool result=false;

	mylock.ReadLock();
	ServerListType::const_iterator i=m_serverList.find(id);
	if (i!=m_serverList.end())
	{
		result = !!i->second.isVIP; 
	}
	mylock.ReadUnlock();

	return result;
}
void CServerList::RequestUserCounts() const
{
	mylock.ReadLock();

	for(ServerListType::const_iterator i=m_serverList.begin(); i!=m_serverList.end(); ++i)
	{
		SendSocket( i->second.inner_addr, "c", SQ_SERVER_NUM);
	}

	mylock.ReadUnlock();
}

void CServerList::SetServerUserCount( ServerId id, short userCount, short userLimit)
{
	mylock.WriteLock();

	ServerListType::iterator i=m_serverList.find(id);
	if (i!=m_serverList.end())
	{
		i->second.UserNum=userCount;
		i->second.maxUsers=userLimit;
	}

	mylock.WriteUnlock();
}

static void PushInt(std::vector<char> & buffer, int value)
{
	buffer.push_back(value);
	buffer.push_back(value >> 8);
	buffer.push_back(value >> 16);
	buffer.push_back(value >> 24);
}

static void PushShort(std::vector<char> & buffer, int value)
{
	buffer.push_back(value);
	buffer.push_back(value >> 8);	
}


void CServerList::MakeQueueSizePacket(std::vector<char> & buffer) const
{
	mylock.ReadLock();

	buffer.push_back(0);  // Reserving space for packet size
	buffer.push_back(0);
	buffer.push_back(AC_QUEUE_SIZE);
	buffer.push_back(0);  // Reserving space for queue list size

	char count=0;
		
	for (QueueSizesType::const_iterator i=m_queueSizes.begin(); i!=m_queueSizes.end(); ++i)
	{
        char queueLevel = 0;
		for (std::vector<std::pair<int, int> >::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
		{
			buffer.push_back(i->first.GetValueChar());
			buffer.push_back(queueLevel); // queue level
			PushInt(buffer,j->first); // queue size
			PushInt(buffer,j->second); // wait time

			++count;
            ++queueLevel;
		}
	}

	buffer[3]=count;
		
	mylock.ReadUnlock();
}

/**
 * Record the number of users on each server to the database
 */
void CServerList::UpdateDB() const
{
	mylock.ReadLock();

	CDBConn conn(g_linDB);

	char buffer[256];
	sprintf( buffer, "{CALL dbo.sp_LogUserNumbers (?,?,?,?,?,?) }" );

    // Convert the current time to a DB-compatible format, and use it for all rows recorded
    time_t log_time;
    struct tm log_timeTM;
    TIMESTAMP_STRUCT dblogtime;

    log_time = time(0);
    log_timeTM = *localtime(&log_time);

    dblogtime.year     = log_timeTM.tm_year + 1900;
    dblogtime.month    = log_timeTM.tm_mon + 1;
    dblogtime.day      = log_timeTM.tm_mday;
    dblogtime.hour     = log_timeTM.tm_hour;
    dblogtime.minute   = log_timeTM.tm_min;
    dblogtime.second   = log_timeTM.tm_sec;
    dblogtime.fraction = 0;

	mylock.ReadUnlock();
}

/**
 * Gets an arbitrary server id that is not in use.
 */
ServerId CServerList::GetFreeServerId() const
{
	if (m_serverList.empty())
	{
		return ServerId(1);
	}
	else
	{
		ServerId maxId = m_serverList.rbegin()->first;
		return ServerId(maxId.GetValueChar() + 1);
	}
}

void CServerList::SetServerQueueSize( ServerId id, int queueLevel, int queueSize, int queueTime)
{
	if (queueLevel < 0)
		return;

	std::vector<std::pair<int, int> > & queueSizes = m_queueSizes[id];
	if (queueSizes.size() < static_cast<size_t>(queueLevel+1))
	{
		queueSizes.resize(static_cast<size_t>(queueLevel+1),std::make_pair(0,0));
	}
	queueSizes[static_cast<size_t>(queueLevel)]=std::make_pair(queueSize, queueTime);
}
