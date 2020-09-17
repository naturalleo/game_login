// DBConn.cpp: implementation of the CDBConn class.
//
//////////////////////////////////////////////////////////////////////

#include "DBConn.h"
#include "resource.h"
#include "config.h"

#define RECOVERY_INTERVAL       30000
#define GLOBALAUTH_REG_ENTRY		"Software\\NCSoft\\GLOBALAUTH"

extern HINSTANCE g_instance;

DBEnv *g_linDB;

DBEnv::DBEnv() : m_lock(eSystemSpinLock, 4000)
{
    m_henv = NULL;
    m_pSqlPool = NULL;
    m_pFreeSqlPool = NULL;
    m_pFreeSqlPoolEnd = NULL;
    m_connCount = 0;	
}

DBEnv::~DBEnv()
{
    if(m_henv != NULL) {
        Destroy();
    }
    if(m_pSqlPool != NULL) {
        delete [] m_pSqlPool;
    }
}

const char* DBEnv::GameIdToRegistryKey(int gameId)
{
	return "AuthDB";
}


void DBEnv::Init(int connCount)
{
    m_connCount = connCount;
	m_pSqlPool = new SQLPool[m_connCount];

	if(Login()) {
		AllocSQLPool();
	} else {
		log_1.AddLog(LOG_ERROR, "db login mysql connection failed");
		m_henv = NULL;
		//TBROWN - delete our pool
		delete[] m_pSqlPool;
	}
	return;
}

bool DBEnv::Login(bool reset)
{
	CMySQLConenction lpConnection;
	bool ret = false;
	SetupSQLConnection(&lpConnection);
	if(!lpConnection.Connect())
	{
		log_1.AddLog(LOG_ERROR, "connection mysql failed");
		return false;
	}
	return true;

}

void DBEnv::SetupSQLConnection(CMySQLConenction *lpConnection)
{
	lpConnection->SetServerHost(config.Get("mysql_host"));
	lpConnection->SetServerPort(config.GetInt("mysql_root", 3306));
	lpConnection->SetDataBaseName(config.Get("mysql_database"));
	lpConnection->SetUserName(config.Get("mysql_user"));
	lpConnection->SetPassWord(config.Get("mysql_password"));
	lpConnection->SetConnectionFlags(CLIENT_FOUND_ROWS | CLIENT_MULTI_RESULTS);
}


void DBEnv::AllocSQLPool(void)
{
	for (int i = 0; i < m_connCount; i++) {
        m_pSqlPool[i].pNext = NULL;
		// Allocate connection.
        CMySQLConenction* connect = new CMySQLConenction;
        m_pSqlPool[i].connect = connect;
        SetupSQLConnection(connect);
		if (!connect->Connect())
		{
			log_1.AddLog(LOG_ERROR, "allocate connection mysql failed:%d", i);
		}

	}
// Let's salvage valid statement handles. (Actually all handles should be valid.)
    for (int i = 0; i < m_connCount; i++) {
        if (m_pSqlPool[i].connect) {
            if (m_pFreeSqlPool) {
                m_pSqlPool[i].pNext = m_pFreeSqlPool;
                m_pFreeSqlPool = &m_pSqlPool[i];
            } else {
                m_pFreeSqlPool = &m_pSqlPool[i];
				m_pFreeSqlPoolEnd = &m_pSqlPool[i];
            }
        }
    }
}

void DBEnv::Destroy()
{
	for(int i = 0; i < m_connCount; i++) {
        if(m_pSqlPool[i].connect) {
			delete m_pSqlPool[i].connect;
			m_pSqlPool[i].connect = NULL;
//            m_pSqlPool[i].stmt = 0;
        }
	}
    m_henv = NULL;
}

bool DBEnv::LoadConnStrFromReg()
{
	HKEY hKey;
	unsigned char buffer[MAX_CONN_STR];
	bool strExists = false;
	LONG e;
	DWORD dwType;
	DWORD dwSize;

	const char *keyStr = GameIdToRegistryKey(config.gameId);
	if (keyStr == NULL)
	{
		//TBROWN - returning false here instead of exceptioning below
		log_1.AddLog(LOG_WARN, "Invalid gameId set in config file. Will manually prompt instead.");
		return false;
	}

	e = RegOpenKeyEx(HKEY_LOCAL_MACHINE, GLOBALAUTH_REG_ENTRY, 0, KEY_READ, &hKey);
	if (e == ERROR_SUCCESS) {
		dwSize = MAX_CONN_STR;
		e = RegQueryValueEx(hKey, (LPTSTR)keyStr, NULL, &dwType, buffer, &dwSize);
		if ((e == ERROR_SUCCESS) && (dwType == REG_BINARY)) {
			strExists = true;
		}
		RegCloseKey(hKey);
	}

	if (strExists) {
        DesReadBlock(buffer, MAX_CONN_STR);
		strcpy((char *)m_connStr, (const char *)buffer);
	}

	return strExists;
}

void DBEnv::SaveConnStrToReg()
{
	HKEY hKey;
	DWORD dwResult;
	unsigned char buffer[MAX_CONN_STR];
	//char *keyStr;

	//TBROWN - adding the gameId == 0 here because that's the default, and it wasn't handled before
	const char *keyStr = GameIdToRegistryKey(config.gameId);
	if (keyStr == NULL)
	{
		const char *error_message = "Invalid gameId set in config file. Don't know how to store the connection in the registry.";
		log_1.AddLog(LOG_ERROR, error_message);
		MessageBox( NULL, error_message, "Error", MB_ICONERROR | MB_OK );
		exit(0);
	}


	strcpy((char *)buffer, (const char *)m_connStr);
    DesWriteBlock(buffer, MAX_CONN_STR);
	LONG e = RegCreateKeyEx(HKEY_LOCAL_MACHINE, GLOBALAUTH_REG_ENTRY, 0, "", 
      REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwResult);
	if (e == ERROR_SUCCESS) {
		DWORD dwType = REG_BINARY;
		DWORD dwData = 0;
		RegSetValueEx(hKey, (LPTSTR)keyStr, NULL, REG_BINARY, buffer, MAX_CONN_STR);
		RegCloseKey(hKey);
	}
}

void DBEnv::OnTimerCallback()
{
    if(Login(true)) {
        m_lock.Enter();
        Destroy();
		m_lock.Leave();
    } else {
		log_1.AddLog(LOG_ERROR, "db recovery failed: login impossible");
        RegisterTimer(RECOVERY_INTERVAL);
    }
    ReleaseRef();
	return;
}

CDBConn::CDBConn(DBEnv *env)
{
    m_pEnv = env;
    m_pCurSql = NULL;
	while (1) {
		m_pEnv->m_lock.Enter();
        // Take one off the front of m_pEnv->m_pFreeSqlPool
		if (m_pEnv->m_pFreeSqlPool)
		{
			m_pCurSql = m_pEnv->m_pFreeSqlPool;
			m_pCurSql->reset = FALSE;

			m_pEnv->m_pFreeSqlPool = m_pEnv->m_pFreeSqlPool->pNext;

			if (!m_pEnv->m_pFreeSqlPool) 
			{
				// Must be the last item in the pool
				// assert(m_pEnv->m_pFreeSqlPool == m_pEnv->m_pFreeSqlPoolEnd);
				m_pEnv->m_pFreeSqlPoolEnd = NULL;
			}
		}
		
        m_pEnv->m_lock.Leave();
		if (m_pCurSql) {
			break;
        } else {
		    Sleep(100);            // Wait 100 ms and check again
        }
	}
	m_connect = m_pCurSql->connect;
	m_colNum = 1;
    m_paramNum = 1;
}

CDBConn::~CDBConn()
{
    m_pEnv->m_lock.Enter();
    if(!m_pCurSql->reset) {

		// Add it back to the end of the free pool
		m_pCurSql->pNext = NULL;
		if (m_pEnv->m_pFreeSqlPool)
		{
			m_pEnv->m_pFreeSqlPoolEnd->pNext = m_pCurSql;
		}
		else
		{
			// assert(!m_pEnv->m_pFreeSqlPoolEnd);
			m_pEnv->m_pFreeSqlPool = m_pCurSql;
		}
		m_pEnv->m_pFreeSqlPoolEnd = m_pCurSql;
    }
    m_pEnv->m_lock.Leave();
}

BOOL CALLBACK LoginDlgProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam)
{
    static DBEnv *pEnv;

	switch (dwMessage) {
	case WM_INITDIALOG:
        pEnv = (DBEnv *)(INT_PTR)lParam;
        char *pDefault;
        char *pTitle;
        pDefault = "AuthDB";
        pTitle = "SQL Connection Info";
		SendDlgItemMessage(hDlg, IDC_FILE, WM_SETTEXT, 0, (LPARAM)pDefault);
        SetWindowText(hDlg, (LPCTSTR)pTitle);
		return 0;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
			{
				char buffer[64];
                char *pTempConnStr;
                pTempConnStr = (char *)pEnv->m_connStr;

				SendDlgItemMessage(hDlg, IDC_FILE, WM_GETTEXT, 64, (LPARAM) buffer);
                strcpy(pTempConnStr, "DATABASE=");
				strcat(pTempConnStr, buffer);
				SendDlgItemMessage(hDlg, IDC_USER, WM_GETTEXT, 64, (LPARAM) buffer);
				strcat(pTempConnStr, ";USER="); 
				strcat(pTempConnStr, buffer);
				SendDlgItemMessage(hDlg, IDC_PASS, WM_GETTEXT, 64, (LPARAM) buffer);
				strcat(pTempConnStr, ";PWD=");
				strcat(pTempConnStr, buffer);
				pEnv->SaveConnStrToReg();
				EndDialog(hDlg, 0);
				break;
			}
		}
		break;
	}
	return 0;
}
