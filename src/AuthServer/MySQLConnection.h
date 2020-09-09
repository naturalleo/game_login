#ifndef _SQL_H_
#define	_SQL_H_
#include "Platform.h"
//#undef bool //config-win.h中会把bool定义为BOOL
extern "C"
{
#include <mysql.h>

extern "C" FILE * __cdecl __iob_func(void);
}

class CMySQLConenction
{
private:
	static const int SQLQueryBufferSize = 4096 * 10;

private:
	MYSQL				m_MySQL;
	MYSQL_RES*			m_pSqlResult;
	my_ulonglong		m_uSqlRows;
	MYSQL_ROW			m_pSqlRows;
	MYSQL_FIELD	*		m_pSqlFields;
	my_ulonglong		m_uSqlFields;
	my_ulonglong		m_uRowsAffected;

private:
	char				m_sServerHost[32];
	INT_PTR				m_nServerPort;
	char				m_sUserName[64];
	char				m_sPassWord[64];
	char				m_sDataBase[64];
	UINT_PTR			m_nConnectFlags;
	char				m_sEncodingName[64];//字符编码默认为utf8
	char				m_sQueryBuffer[SQLQueryBufferSize];
	CRITICAL_SECTION	m_QueryLock;			//SQL查询锁
	BOOL				m_boConnected;			//是否连接的标志
	BOOL				m_boMultiThread;		//是否多线程的标志，如果为TRUE则进行查询是会加锁
private:
	VOID AfterQueryed(int nError);
	VOID AfterExeced(int nError);

public:
	CMySQLConenction();
	~CMySQLConenction();

	inline const char* GetServerHost(){ return m_sServerHost; };
	VOID SetServerHost(const char *sServerHost);
	inline INT_PTR GetServerPort(){ return m_nServerPort; };
	VOID SetServerPort(INT_PTR nServerPort);
	inline const char* GetDataBaseName(){ return m_sDataBase; };
	VOID SetDataBaseName(const char *sDataBaseName);
	inline const char* GetUserName(){ return m_sUserName; };
	VOID SetUserName(const char* sUserName);
	inline const char* GetPassWord(){ return m_sPassWord; };
	VOID SetPassWord(const char* sPassWord);
	inline UINT_PTR GetConnectionFlags(){ return m_nConnectFlags; };
	VOID SetConnectionFlags(const UINT_PTR nFlags);
	inline BOOL GetIsMultiThread(){ return m_boMultiThread; };
	VOID SetMultiThread(const BOOL boMultiThread);
	inline const char* GetEncodingName() const { return m_sEncodingName; }
	VOID SetEncodingName(const char* encoding);

	BOOL Connect();
	inline BOOL Connected(){ return m_boConnected; };
	VOID Disconnect();

	inline MYSQL* GetMySql() { return &m_MySQL;}
	//Query和RealQuery用于执行带返回结果的查询操作，
	//查询成功则返回0并且需要在外部调用ResetQuery来释放查询结果集并解锁，
	//如果查询失败则会自动解锁。
	int Query(const char* sQueryFormat, ...);
	int RealQuery(const char* sQueryText, const size_t nTextLen);

	//Exec和RealExec用于执行不带返回结果的查询操作，例如delete,update,create,drop,alter等
	//执行成功则返回0并且需要在外部调用ResetQuery来释放查询结果集并解锁，
	//如果执行失败则会自动解锁。
	int Exec(const char* sQueryFormat, ...);
	int RealExec(const char* sExecText, const size_t nTextLen);

	VOID ResetQuery();


	inline int GetFieldCount()
	{
		return (int)m_uSqlFields;
	}
	inline int GetRowCount()
	{
		return (int)m_uSqlRows;
	}
	inline int GetRowsAffected()
	{
		return (int)m_uRowsAffected;
	}
	inline MYSQL_ROW CurrentRow()
	{
		return m_pSqlRows;
	}
	inline MYSQL_ROW NextRow()
	{
		if ( m_pSqlResult )
		{
			m_pSqlRows = mysql_fetch_row( m_pSqlResult );
			return m_pSqlRows;
		}
		return NULL;
	}
	inline unsigned long* GetFieldsLength()
	{
		return mysql_fetch_lengths(m_pSqlResult);
	}
};

#endif
