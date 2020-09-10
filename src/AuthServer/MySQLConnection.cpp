#include <stdio.h>
//#include "config-win.h"
//#include <Tick.h>
//#include <wyString.h>
//#include "../../utils/ShareUtil.h"
#include "Log.h"
#include "MySQLConnection.h"
#ifdef WINDOWS
#include <tchar.h>
#include <SQL.h>
#include "windows.h"
#endif

extern "C" FILE * __cdecl __iob_func(void) { 
	static FILE __inn_fix_iob[] = { *stdin, *stdout, *stderr };
	return __inn_fix_iob; 
}

static int MysqlLibRefer = 0;

CMySQLConenction::CMySQLConenction()
{
	ZeroMemory( &m_MySQL, sizeof(m_MySQL) );

	m_pSqlResult = NULL;
	m_uSqlRows = 0;
	m_pSqlRows = NULL;
	m_pSqlFields = NULL;
	m_uSqlFields = NULL;

	m_sServerHost[0] = 0;
	m_nServerPort = 3306;
	m_sUserName[0] = 0;
	m_sPassWord[0] = 0;
	m_sDataBase[0] = 0;
	m_nConnectFlags = CLIENT_FOUND_ROWS | CLIENT_MULTI_RESULTS;

	strcpy(m_sEncodingName, "utf8");

	m_sQueryBuffer[0] = 0;
	InitializeCriticalSection( &m_QueryLock );

	m_boConnected = FALSE;
	m_boMultiThread = FALSE;

	MysqlLibRefer++;
	if (MysqlLibRefer == 1)
		mysql_library_init(0, NULL, NULL);
}

CMySQLConenction::~CMySQLConenction()
{
	Disconnect();
	DeleteCriticalSection( &m_QueryLock );
	MysqlLibRefer--;
	if (MysqlLibRefer == 0)
		mysql_library_end();
}

VOID CMySQLConenction::SetServerHost(const char *sServerHost)
{
	ZeroMemory( m_sServerHost, sizeof(m_sServerHost) );
	strncpy( m_sServerHost, sServerHost, sizeof(m_sServerHost) - 1 );
}

VOID CMySQLConenction::SetServerPort(INT_PTR nServerPort)
{
	m_nServerPort = nServerPort;
}

VOID CMySQLConenction::SetDataBaseName(const char *sDataBaseName)
{
	ZeroMemory( m_sDataBase, sizeof(m_sDataBase) );
	strncpy( m_sDataBase, sDataBaseName, sizeof(m_sDataBase) - 1 );
}

VOID CMySQLConenction::SetUserName(const char* sUserName)
{
	ZeroMemory( m_sUserName, sizeof(m_sUserName) );
	strncpy( m_sUserName, sUserName, sizeof(m_sUserName) - 1 );
}

VOID CMySQLConenction::SetPassWord(const char* sPassWord)
{
	ZeroMemory( m_sPassWord, sizeof(m_sPassWord) );
	strncpy( m_sPassWord, sPassWord, sizeof(m_sPassWord) - 1 );
}

VOID CMySQLConenction::SetConnectionFlags(const UINT_PTR nFlags)
{
	m_nConnectFlags = nFlags;
}

VOID CMySQLConenction::SetMultiThread(const BOOL boMultiThread)
{
	m_boMultiThread = boMultiThread;
}

VOID CMySQLConenction::SetEncodingName(const char* encoding)
{
	if (strcmp(m_sEncodingName, encoding) != 0)
	{
		strcpy(m_sEncodingName, encoding);
		if (m_boConnected)
		{
			if (0 == Exec("set names %s;", m_sEncodingName))
				ResetQuery();
		}
	}
}


BOOL CMySQLConenction::Connect()
{
	BOOL Result = TRUE;

	if ( !m_boConnected )
	{
		if ( mysql_init(&m_MySQL) )
		{
			m_MySQL.reconnect = TRUE;
			if (mysql_real_connect(&m_MySQL, m_sServerHost, m_sUserName, m_sPassWord, m_sDataBase, (UINT)m_nServerPort,NULL, (UINT)m_nConnectFlags) )
			{
				m_boConnected = TRUE;
				log_1.AddLog(LOG_DEBUG, "mysql character set: %s", mysql_character_set_name(&m_MySQL));
				if (m_sEncodingName[0])
				{
					// 新增检测设置字符编码是否成功
					if (!mysql_set_character_set(&m_MySQL, m_sEncodingName))
					{
						log_1.AddLog(LOG_DEBUG, "成功设置数据库的字符编码, 当前mysql character set: %s", mysql_character_set_name(&m_MySQL));
					}
					else
					{
						m_boConnected = FALSE;
						log_1.AddLog(LOG_ERROR, _T("设置数据库(%s)字符编码出错：%s"), m_sDataBase, mysql_error(&m_MySQL));
						mysql_close(&m_MySQL);
					}
					//if (0 == Exec("set names %s;", m_sEncodingName))
					//	ResetQuery();
				}
			}
			else
			{
				Result = FALSE;
				log_1.AddLog(LOG_ERROR, _T("无法连接到数据库：%s"), mysql_error(&m_MySQL) );
				mysql_close(&m_MySQL);
			}
		}
		else
		{
			log_1.AddLog(LOG_ERROR, _T("无法初始化数据库连接程序") );
			Result	= FALSE;
		}
	}

	return	Result;
}

VOID CMySQLConenction::Disconnect()
{
	if (m_boConnected)
	{
		m_boConnected = FALSE;
		ResetQuery();
		mysql_close(&m_MySQL);
	}
}

VOID CMySQLConenction::AfterQueryed(int nError)
{
	if (!nError)
	{
		m_pSqlResult = mysql_store_result(&m_MySQL);
		if (m_pSqlResult)
		{
			mysql_data_seek( m_pSqlResult, 0 );
			mysql_field_seek( m_pSqlResult, 0 );
			m_uSqlRows	= mysql_num_rows(m_pSqlResult);
			m_uSqlFields	= mysql_num_fields(m_pSqlResult);
			m_pSqlRows    = mysql_fetch_row(m_pSqlResult);
		}
		else
		{
			m_uSqlFields = 0;
			m_pSqlFields = NULL;
			m_uSqlRows = 0;
			m_pSqlRows = NULL;
			m_sQueryBuffer[128] = 0;
			const char* err = mysql_error(&m_MySQL);
			if (strlen(err))
				log_1.AddLog(LOG_DEBUG, _T("%s(%s)"), mysql_error(&m_MySQL), m_sQueryBuffer );
		}
	}
	else
	{
		m_uSqlFields = 0;
		m_pSqlFields = NULL;
		m_uSqlRows = 0;
		m_pSqlRows = NULL;
		m_pSqlResult = NULL;
#ifdef UNICODE
		wylib::string::CAnsiString as(mysql_error(&m_MySQL));
		wylib::string::CWideString *ws = as.toWStr();
		OutputMsg(rmError, *ws);
		delete ws;
#else
		log_1.AddLog(LOG_ERROR, mysql_error(&m_MySQL));
#endif
		if ( m_boMultiThread ) LeaveCriticalSection( &m_QueryLock );
	}
}

VOID CMySQLConenction::AfterExeced(int nError)
{
	if (!nError)
	{
		m_pSqlResult = NULL;
		m_uRowsAffected = mysql_affected_rows( &m_MySQL );
	}
	else
	{
		m_uSqlFields = 0;
		m_pSqlFields = NULL;
		m_uSqlRows = 0;
		m_pSqlRows = NULL;
		m_pSqlResult = NULL;
		m_uRowsAffected = 0;
#ifdef UNICODE
		wylib::string::CAnsiString as(mysql_error(&m_MySQL));
		wylib::string::CWideString *ws = as.toWStr();
		OutputMsg(rmError, *ws);
		delete ws;
#else
		log_1.AddLog(LOG_ERROR, mysql_error(&m_MySQL));
#endif
		if ( m_boMultiThread ) LeaveCriticalSection( &m_QueryLock );
	}
}

int CMySQLConenction::Query(const char* sQueryFormat, ...)
{
	int	Result;
	va_list	args;

	if ( m_boMultiThread ) EnterCriticalSection( &m_QueryLock );

	va_start(args, sQueryFormat);
	Result = vsprintf(m_sQueryBuffer, sQueryFormat, args);
	va_end(args);

	Result = mysql_real_query(&m_MySQL, m_sQueryBuffer, Result);
	AfterQueryed( Result );

	return	Result;
}

int CMySQLConenction::RealQuery(const char* sQueryText, const size_t nTextLen)
{
	int	Result;

	if ( m_boMultiThread ) EnterCriticalSection( &m_QueryLock );

	Result = mysql_real_query(&m_MySQL, sQueryText, (UINT)nTextLen);
	AfterQueryed( Result );

	return	Result;
}

int CMySQLConenction::Exec(const char* sQueryFormat, ...)
{
	int	Result;
	va_list	args;

	if ( m_boMultiThread ) EnterCriticalSection( &m_QueryLock );

	va_start(args, sQueryFormat);
	Result = vsprintf(m_sQueryBuffer, sQueryFormat, args);
	va_end(args);

	Result = mysql_real_query(&m_MySQL, m_sQueryBuffer, Result);
	AfterExeced( Result );

	return	Result;
}

int CMySQLConenction::RealExec(const char* sExecText, const size_t nTextLen)
{
	int	Result;

	if ( m_boMultiThread ) EnterCriticalSection( &m_QueryLock );

	Result = mysql_real_query(&m_MySQL, sExecText, (UINT)nTextLen);
	AfterExeced( Result );

	return	Result;
}

VOID CMySQLConenction::ResetQuery()
{
	if (m_pSqlResult)
	{
		mysql_free_result(m_pSqlResult);
		//Commands out of sync; you can't run this command now
		//get back all results
		while (!mysql_next_result(&m_MySQL));
	}

	m_uSqlFields = 0;
	m_pSqlFields = NULL;
	m_uSqlRows = 0;
	m_pSqlRows = NULL;
	m_pSqlResult = NULL;

	if ( m_boMultiThread ) LeaveCriticalSection( &m_QueryLock );
}
