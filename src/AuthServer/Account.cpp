// Account.cpp: implementation of the CAccount class.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "md5.h"
#include "cryptLib/sha.h"
//#include "../../../3rdparty/cryptopp/adler32.h"

//using namespace CryptoPP;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// 2003-11-25 darkangel
// �� �������� ��κ� ������ �ʴ´�. ��κ��� ��� Stored Procedure�� ��ü�Ǿ���.

#define SQL_SELECT_PWD "Select password From user_auth with (nolock) Where account = '%s'"
#define SQL_SELECT_ACCOUNT "Select  uid, pay_stat, login_flag, warn_flag, block_flag, block_flag2, subscription_flag  From user_account with (nolock) Where account = '%s'"
#define SQL_SELECT_ETC "Select ssn From user_info with (nolock) Where account = '%s'"
#define SQL_SELECT_BLOCK_INFO " Select reason, msg From block_msg with (nolock) Where uid = %d"
#define SQL_UPDATE_PASSWORD_TYPE "UPDATE user_auth SET password=?,salt=?,hash_type=1 WHERE account=?"

CAccount::CAccount()
{
	quotaTime = 0;
	remainTime = 0;
	block_flag = 0;
	block_flag2 = 0;
	nSSN = 0;
	gender = 0;
	memset( ssn, 0, 13 );
	ssn2=0;
	loyalty = loyaltyLegacy = 0;
	lastworld.SetInvalid();
}

CAccount::~CAccount()
{
}

// 2003-11-25 darkangel
// block_msg ���̺� �ִ� ������ �̿��Ͽ� ��Ŷ���� ����� unicode msg packet�� �����.
int CAccount::MakeBlockInfo(  char *msg )
{
	int size=1;
	int block_code=0;
	int len=0;
	char count=0;
	char db_msg[256];
	char *buffer = msg;

	WCHAR block_msg[128];
	int t = sizeof(block_msg);
	CDBConn conn(g_linDB);
	
	conn.Bind( &block_code );
	conn.Bind( db_msg, 256 );

	bool nodata;
	buffer++;
	if ( conn.Execute (SQL_SELECT_BLOCK_INFO, uid ) ) {
		while( conn.Fetch(&nodata) ){
			if ( nodata )
				break;
			len=0;
			len=swprintf( block_msg, (sizeof(block_msg)/sizeof(block_msg[0])), L"%S", db_msg );
			len = (len+1) * 2;
			if ( (len+4+size ) >= 4096 )
				break;
			memcpy( buffer, &block_code, 4 );
			buffer +=4;
			memcpy( buffer, block_msg, len );
			buffer+=len;
			count++;
			size=size+len+4;
		}
	}
	memcpy( msg, &count, 1 );

	return (int)(buffer-msg);
}

// 2003-07-06 darkangel
// ���������� ���� DB �� user_account Table�� �о���� ���̴�. 
// StoredProcedure �� ȣ���ϱ� ������ �ݵ�� ������ DB ���� SP�� Ȯ���ؾ� �Ѵ�. 
// ap_GStat�� ����Ѵ�.
char CAccount::Load( const char *name )
{

	CDBConn conn(g_linDB);

 	SQLINTEGER cbName=SQL_NTS;
	SQLBindParameter( conn.m_stmt, 1, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_VARCHAR, MAX_ACCOUNT_LEN, 0, (SQLPOINTER)name, (SQLINTEGER)strlen(name), &cbName );

	SQLINTEGER cbUid=0;
	SQLBindParameter( conn.m_stmt, 2, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&uid), 0, &cbUid );
	
	SQLINTEGER cbPayStat=0;
	SQLBindParameter( conn.m_stmt, 3, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&pay_stat), 0, &cbPayStat );

	SQLINTEGER cbLoginFlag=0;
	SQLBindParameter( conn.m_stmt, 4, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&login_flag), 0, &cbLoginFlag );

	SQLINTEGER cbWarnFlag=0;
	SQLBindParameter( conn.m_stmt, 5, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&warn_flag), 0, &cbWarnFlag );

	SQLINTEGER cbblockFlag=0;
	SQLBindParameter( conn.m_stmt, 6, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&block_flag), 0, &cbblockFlag );

	SQLINTEGER cbblockFlag2=0;
	SQLBindParameter( conn.m_stmt, 7, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&block_flag2), 0, &cbblockFlag2 );
	
	SQLINTEGER cbsubscribe=0;
	SQLBindParameter( conn.m_stmt, 8, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&subscription_flag), 0, &cbsubscribe );

	SQLINTEGER cblastworld=0;
	SQLBindParameter( conn.m_stmt, 9, SQL_PARAM_OUTPUT, SQL_C_UTINYINT, SQL_TINYINT, 0, 0, (SQLPOINTER)(&lastworld), 0, &cblastworld );

	block_end_date.year = -1;
	SQLINTEGER cbBlockEndDate = 0;
	SQLBindParameter( conn.m_stmt,10, 
					  SQL_PARAM_OUTPUT, 
					  SQL_C_TYPE_TIMESTAMP, 
					  SQL_TYPE_TIMESTAMP, 
					  23, 
					  3, 
					  (SQLPOINTER)(&block_end_date), 
					  0, 
					  &cbBlockEndDate );

	SQLBindParameter( conn.m_stmt, 11, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&queueLevel), 0, 0);

	SQLINTEGER cbloyalty=0;
	SQLBindParameter( conn.m_stmt, 12, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&loyalty), 0, &cbloyalty );

	SQLINTEGER cbloyaltyLegacy=0;
	SQLBindParameter( conn.m_stmt, 13, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&loyaltyLegacy), 0, &cbloyaltyLegacy );

	char buffer[256];
	sprintf( buffer, "{CALL dbo.ap_GStat (?,?,?,?,?,?,?,?,?,?,?,?,?) }" );
	RETCODE RetCode= SQLExecDirect( conn.m_stmt, (SQLCHAR*)buffer, SQL_NTS );
	bool nodata;
	if ( RetCode == SQL_SUCCESS ) {
		if ( conn.Fetch(&nodata)){
			if (nodata){
				conn.ResetHtmt();
				return S_ACCOUNT_LOAD_FAIL;
			}
		}else{
			conn.ResetHtmt();
			return S_ACCOUNT_LOAD_FAIL;
		}
		conn.ResetHtmt();
	}else{
		conn.Error(SQL_HANDLE_STMT, conn.m_stmt, buffer);
		conn.ResetHtmt();
		return S_DATABASE_FAIL;
	}

// C Thurow - Aug 20, 2007
// Load the list of regions for this account

    int region(0);
    
    cbUid=0;
	SQLBindParameter( conn.m_stmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&uid), 0, &cbUid );

    SQLINTEGER cbRegion=0;
    SQLBindCol( conn.m_stmt, 1, SQL_C_SLONG, &region, 0, &cbRegion);

	sprintf( buffer, "{CALL get_server_groups (?) }" );
    RetCode= SQLExecDirect( conn.m_stmt, (SQLCHAR*)buffer, SQL_NTS );
	if ( RetCode == SQL_SUCCESS )
    {
        nodata=false;
        size_t nextRegion = 0;
        do
        {
		    if ( !conn.Fetch(&nodata))
            {
   			    conn.ResetHtmt();
			    return S_ACCOUNT_LOAD_FAIL;
            }

			if (!nodata)
                if (nextRegion < MAX_REGIONS)
                {   
                    regions[nextRegion++]=region;
		        }
                else
                {
                    log_1.AddLog( LOG_ERROR, "LOGIN FAIL, Account:%s has more than %i regions.  (MAX_REGIONS must be changed to allow this many.)", name, MAX_REGIONS );
                    conn.ResetHtmt();
                    return S_DATABASE_FAIL;
                }                 
        }
        while(!nodata);

        for (;nextRegion < MAX_REGIONS;++nextRegion)
        {
            regions[nextRegion]=-1;
        }

		conn.ResetHtmt();
	}
    else
    {
		conn.Error(SQL_HANDLE_STMT, conn.m_stmt, buffer);
		conn.ResetHtmt();
		return S_DATABASE_FAIL;
	}

	return S_ALL_OK;
}
// ��� ������ ������� ��쿡 ��Ÿ �ʿ��� ������ �о� ���δ�.  �������� �����ð�, ���� ��û��, �ֹε�Ϲ�ȣ���... ������ ��û�� �������� �̺κи� ����
// LoadEtc�� �� ���� �ʿ��� Localizing �� ���ؼ� �����Ѵ�. 
// �׷��� ������ Stored Procedure�� ���� �ʴ´�. �� ���󸶴� ������ �ٸ� Stored Procedure�� �����ϱ⺸�ٴ� SQL�� ��ü�� �� �������̱� �����̴�. 
// 2003-07-06 darkangel
// �ѱ������� �ֹε�Ϲ�ȣ�� ��ϵǾ� ���� ���� ����ڴ� �α��� �Ҽ� ����. 
// �������ѿ� �ɸ��� �����̴�.
char CAccount::LoadEtc()
{
	// �ѱ��� ��쿡 ���� ����� ����. 
	if ( config.Country == CC_KOREA ) {
		CDBConn conn(g_linDB);		
		
		conn.ResetHtmt();
		conn.Bind( ssn, MAX_SSN_LEN+1);
		bool nodata = true;
		if ( conn.Execute( SQL_SELECT_ETC, account ))
		{
			if (conn.Fetch(&nodata)) {
				if (nodata)
					return S_LOAD_SSN_ERROR;
			}
			else
				return S_LOAD_SSN_ERROR;
		} else{
			return S_DATABASE_FAIL;
		}

		time_t currentTime = time(NULL);
		struct tm *today = localtime(&currentTime);
		char   curYear[2];

		curYear[0] = ( today->tm_year/10 ) + '0'; // ':' = 2000
		curYear[1] = ( today->tm_year%10 ) + '0';
		
		// gender�� ������� ������ �����Ѵ�. 
		// log�󿡴� �������� ssn2�� �ֱ� ������ ssn2�� ���� ū ���� �����ϸ� �ȴ�.. ���??
		gender = ssn[6] - '0';
		
		// 2003-11-25 darkangel
		// 5�� 6�� ���� ���� �ܱ��ε��� �ܱ��� ��Ϲ�ȣ�̴�. 
		// �̰�쿡�� 2000�� ���� �»����� �����Ѵ�.

		if ( ssn[6] == '1' || ssn[6] == '2' || ssn[6] == '5' || ssn[6] == '6')   // before 2000
			age = (curYear[0] - ssn[0]) * 10 + (curYear[1] - ssn[1]);
		else									// after 2000
			age = (ssn[0]-'0') * 10 + ( ssn[1] -'0');
			

		int cur_mmdd = (today->tm_mon+1)*100 + today->tm_mday;
		int nSsnmmdd = (ssn[2]-'0') * 1000 + (ssn[3]-'0')*100 + (ssn[4]-'0') * 10 + ssn[5] -'0';

		if ( cur_mmdd < nSsnmmdd )
			--age;
		if (age < 0) { // Born in the future..?
			age = 0;
		}
		
		nSSN = nSsnmmdd + ( (ssn[0]-'0') * 10 + ( ssn[1] -'0') ) * 10000;
		ssn2 = atoi( ssn+6);
	} 
	return S_ALL_OK;
}

// 2003-07-06 darkangel
// Password �� DB���� �о�´�.
char CAccount::LoadPassword( const char *name, char *passwd, unsigned char& hash_type, unsigned int& salt )
{
#define SQL_IGNORED_PARAM 0

	passwd[0] = 0;
	CDBConn conn(g_linDB);
	
	SQLINTEGER cbName=SQL_NTS;
	SQLBindParameter( conn.m_stmt, 1, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_VARCHAR, MAX_ACCOUNT_LEN, SQL_IGNORED_PARAM, (SQLPOINTER)name, (SQLINTEGER)strlen(name), &cbName );
	SQLINTEGER cbPwd=SQL_NTS;
	SQLBindParameter( conn.m_stmt, 2, SQL_PARAM_OUTPUT, SQL_C_BINARY, SQL_BINARY, ENC_PWD_LEN, SQL_IGNORED_PARAM, (SQLPOINTER)passwd, ENC_PWD_LEN, &cbPwd );
	SQLINTEGER cbHashType=sizeof(hash_type);
	SQLBindParameter( conn.m_stmt, 3, SQL_PARAM_OUTPUT, SQL_C_TINYINT, SQL_TINYINT, SQL_IGNORED_PARAM, SQL_IGNORED_PARAM, (SQLPOINTER)(&hash_type), SQL_IGNORED_PARAM, &cbHashType);
	SQLINTEGER cbSalt=sizeof(salt);
	SQLBindParameter( conn.m_stmt, 4, SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, SQL_IGNORED_PARAM, SQL_IGNORED_PARAM, (SQLPOINTER)(&salt), SQL_IGNORED_PARAM, &cbSalt );

	char buffer[256];
	sprintf( buffer, "{CALL dbo.ap_GPwd (?,?,?,?) }" );
	RETCODE RetCode= SQLExecDirect( conn.m_stmt, (SQLCHAR*)buffer, SQL_NTS );

	char result = S_DATABASE_FAIL;

	// This is more betterer
	if ( RetCode == SQL_SUCCESS )
	{
		if (cbPwd==SQL_NULL_DATA)
		{
			// Account was not found in the database
			result = S_INVALID_ACCOUNT;
		}
		else
		{
			// Set member variable "account" to indicate that the account has been found in the database:
			memset( account, 0, MAX_ACCOUNT_LEN + 1 );
			strcpy( account, name );
			result = S_ALL_OK;
		}
	}
	else
	{
		conn.Error(SQL_HANDLE_STMT, conn.m_stmt, buffer);
		result = S_DATABASE_FAIL;
	}

	conn.ResetHtmt();
	return result;
}

char CAccount::CheckPassword( const char *name, char *dbpwdLineage2, char *dbpwdSHA512, int oneTimeKey, bool useMD5 )
{
	unsigned char passwdDB[ENC_PWD_LEN+1];
	unsigned char passwdSHA512[ENC_PWD_LEN+1];
	char newPasswdDB[ENC_PWD_LEN+1];
	char* passwd = dbpwdLineage2;
	char err_msg = S_ALL_OK;
	unsigned int salt = 0;
	unsigned char hash_type = 0;

	memset(passwdDB,0,ENC_PWD_LEN+1);

	err_msg = LoadPassword( name, (char *)passwdDB, hash_type, salt );

	if ( err_msg == S_ALL_OK )
	{
		if (!useMD5 )
		{
			if (hash_type)
			{
				memset(newPasswdDB,0,ENC_PWD_LEN+1);
				memcpy(newPasswdDB,dbpwdLineage2,MAX_PWD_LEN);
				EncPwdSha512( newPasswdDB, salt );
				passwd = newPasswdDB;
			}
			else
				EncPwd( dbpwdLineage2 );

		} else {
			if (hash_type)
			{
				cryptLib::digest512 hash;
				memcpy(hash._, dbpwdSHA512, 64);
				hash.ToString();
				string sHash(hash.ToString());
				// Copy out new hash
				for (unsigned int i = 0; i < ENC_PWD_LEN; ++i)
				{
					passwdSHA512[i] = sHash.at(i);
				}
				passwd = (char*)passwdSHA512;
			}
			else
			{
				MD5 md5;
				char key[32];

				//make string from one way key
				_snprintf(key, 31, "%d", oneTimeKey);

				//make md5 password
				md5.Update(passwdDB, 16);
				md5.Update((unsigned char*)key, (unsigned int)strlen(key));
				md5.Final(passwdDB);
			}
		}

		unsigned int len;
		if (!hash_type)
		{
			len = MAX_PWD_LEN;
		}
		else
		{
			if (memcmp((char*)(passwdDB+128-16), "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0)
			{
				len = 128-16;	//	detecting if we fixing an old field	
			}
			else
			{
				len = ENC_PWD_LEN;
			}
		}

		if(memcmp(passwd, passwdDB, len) != 0) {
			err_msg = S_INCORRECT_PWD;

			if (config.Country == CC_KOREA ){
				log_1.AddLog( LOG_VERBOSE, "���� %s �н����尡 �߸��Ǿ����ϴ�.", name );
			} else {
				log_1.AddLog( LOG_VERBOSE, "LOGIN FAIL, Incorrect password. Account:%s", name );
			}

            return err_msg;
		}else{
			err_msg = Load( name );
			if ( err_msg != S_ALL_OK ){

				log_1.AddLog( LOG_WARN, "LOGIN FAIL, Can't load user_account table from db. Account:%s", name );

				return err_msg;
			}
			if (useMD5 && ((hash_type != 1) || (len != ENC_PWD_LEN)))
			{
  				cryptLib::digest512 hash;
  				memcpy(hash._, dbpwdSHA512, 64);
  				hash.ToString();
  				string sHash(hash.ToString());
  				// Copy out new hash
  				for (unsigned int i = 0; i < ENC_PWD_LEN; ++i)
  				{
  					passwdSHA512[i] = sHash.at(i);
  				}
  
	  			//Adler32 CRC;
				char salt_str[15];
  				size_t salt;
  				size_t i;
				strncpy(salt_str, name, sizeof(salt_str));
  				size_t salt_len = strlen(salt_str);
  				for (i = 0; i < salt_len; ++i)
  				{
  					if (salt_str[i] >= 'A' && salt_str[i] <= 'Z')
  						_tolower(salt_str[i]);
  				}
  
  				//CRC.Update((byte*)salt_str, salt_len);
  				//CRC.Final((byte*)&salt);

  				CDBConn dbconn(g_linDB);
  				SQLINTEGER cbPwd = ENC_PWD_LEN;
  				SQLBindParameter( dbconn.m_stmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY, ENC_PWD_LEN, 0, (SQLPOINTER)passwdSHA512, ENC_PWD_LEN, &cbPwd );
   				SQLINTEGER cbSalt = 0;
  				SQLBindParameter( dbconn.m_stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, (SQLPOINTER)(&salt), 0, &cbSalt );
  				SQLINTEGER cbName=SQL_NTS;
  				SQLBindParameter( dbconn.m_stmt, 3, SQL_PARAM_INPUT, SQL_C_TCHAR, SQL_VARCHAR, MAX_ACCOUNT_LEN, SQL_IGNORED_PARAM, (SQLPOINTER)name, (SQLINTEGER)strlen(name), &cbName );
  
  				dbconn.Execute( SQL_UPDATE_PASSWORD_TYPE);
			}
		}
	} else {

		log_1.AddLog( LOG_WARN, "LOGIN FAIL, Can't load password from db. Account:%s", name );

		return err_msg;
	}

	// 2003-11-25 darkangel
	// GM CheckMode�� ���� ������Ʈ�� ������ ����ڰ� �̸� ������ ���ϰ� �ϰ� GM�̳� ��Ÿ �系 �����鸸
	// �������� �ϴ� ��ƾ�Դϴ�. �� ���� ����ڴ� GMCheckMode�� Ǯ�������� ��� �ü� �����ϴ�.
	if ( config.GMCheckMode ) {
		if ( (login_flag & 16) || ( login_flag & 32) ) {
		} else {
			AS_LOG_VERBOSE( "SND: AC_LOGIN_FAIL, GM ONLY MODE");
			return S_SERVER_CHECK;
		}
	} 
	// 2003-07-29
	// Quiz�� �н����带 �ݵ�� �����ϰ� ���;� �� ��찡 �ִ�. �׷��� ��� ó��.
	if ( login_flag & 3 ){
		AS_LOG_VERBOSE( "SND: AC_LOGIN_FAIL, S_MODIFY_PASSWORD" );
		return S_MODIFY_PASSWORD;
	}
	
	err_msg = LoadEtc();

	if ( err_msg != S_ALL_OK )
		log_1.AddLog( LOG_WARN, "SND: AC_LOGIN_FAIL,fail to read user_account table, load etc fail : %d", err_msg );
	
	return err_msg;
}
