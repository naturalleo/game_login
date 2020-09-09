#ifndef _SQL_H_
#define	_SQL_H_
#include "Platform.h"
//#undef bool //config-win.h�л��bool����ΪBOOL
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
	char				m_sEncodingName[64];//�ַ�����Ĭ��Ϊutf8
	char				m_sQueryBuffer[SQLQueryBufferSize];
	CRITICAL_SECTION	m_QueryLock;			//SQL��ѯ��
	BOOL				m_boConnected;			//�Ƿ����ӵı�־
	BOOL				m_boMultiThread;		//�Ƿ���̵߳ı�־�����ΪTRUE����в�ѯ�ǻ����
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
	//Query��RealQuery����ִ�д����ؽ���Ĳ�ѯ������
	//��ѯ�ɹ��򷵻�0������Ҫ���ⲿ����ResetQuery���ͷŲ�ѯ�������������
	//�����ѯʧ������Զ�������
	int Query(const char* sQueryFormat, ...);
	int RealQuery(const char* sQueryText, const size_t nTextLen);

	//Exec��RealExec����ִ�в������ؽ���Ĳ�ѯ����������delete,update,create,drop,alter��
	//ִ�гɹ��򷵻�0������Ҫ���ⲿ����ResetQuery���ͷŲ�ѯ�������������
	//���ִ��ʧ������Զ�������
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
