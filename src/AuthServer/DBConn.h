// DBConn.h: interface for the CDBConn class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DBCONN_H__BE8285EA_B6EF_4AB1_83B4_8FE6EA59F5BA__INCLUDED_)
#define AFX_DBCONN_H__BE8285EA_B6EF_4AB1_83B4_8FE6EA59F5BA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GlobalAuth.h"
#include "IOObject.h"
#include "lock.h"
#include "MySQLConnection.h"

#define MAX_CONN_STR 256

typedef struct _SQLPool_ 
{
    //TBROWN - adding a constructor to init these values so that it's obvious if they
	//need to be cleaned up or not
	_SQLPool_() : reset(false), connect(NULL), pNext(NULL)
	{
		bool here = true;
	}
	BOOL reset;
	CMySQLConenction *connect;
	struct _SQLPool_ *pNext;
} SQLPool;

class DBEnv : public CIOObject{
protected:
    CLock m_lock;
	CMySQLConenction *m_henv;
	SQLPool *m_pSqlPool;
    SQLPool *m_pFreeSqlPool;
    SQLPool *m_pFreeSqlPoolEnd;
    SQLCHAR m_connStr[MAX_CONN_STR];
    int m_connCount;
    BOOL m_recoveryNeeded;

	static const char* GameIdToRegistryKey(int gameId);

public:
    virtual void OnEventCallback() {}
    virtual void OnTimerCallback();
    virtual void OnIOCallback(BOOL success, DWORD transferred, LPOVERLAPPED pOverlapped) {}

    friend class CDBConn;
    friend BOOL CALLBACK LoginDlgProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam);

    DBEnv();
    ~DBEnv();
	void Init(int connCount);
	void Destroy();
	bool Login(bool reset=false);
	void AllocSQLPool();
    bool LoadConnStrFromReg();
    void SaveConnStrToReg();
    void SetupSQLConnection(CMySQLConenction *lpConnection);
};

class CDBConn {
public:
	CMySQLConenction *m_connect;
    int m_colNum;
    int m_paramNum;

	CDBConn(DBEnv *env);
	~CDBConn();
	CMySQLConenction* getConnect() { return m_connect; }


    friend BOOL CALLBACK LoginDlgProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam);

protected:
	SQLPool *m_pCurSql;
    DBEnv *m_pEnv;

};

extern DBEnv *g_linDB;

#endif // !defined(AFX_DBCONN_H__BE8285EA_B6EF_4AB1_83B4_8FE6EA59F5BA__INCLUDED_)
