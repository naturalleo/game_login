// 2003-07-06 darkangel
// Ã³À½ ·Îµå½Ã¿¡ BuildNumberÃâ·ÂÇÏµµ·Ï Ãß°¡ÇÔ.
#include "config.h"
#include "Thread.h"
#include "GlobalAuth.h"
#include "job.h"
#include "ServerList.h"
#include "accountdb.h"
#include "util.h"
#include "ioserver.h"
#include "IPSessionDB.h"
#include "buildn.h"
#include "dbconn.h"
#include "logsocket.h"
#include "blowfish.h"
#include "WantedSocket.h"
#include "IPList.h"
#include "resource.h"
//#include <windows.h>

#define BUTTON_WIDTH	160

#define RELOAD_BUTTON_ID	1
#define	LOGLEVEL_BUTTON_ID	2

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_arr_)	(sizeof(_arr_)/sizeof(_arr_[0]))
#endif

HWND mainWnd;
HWND logWnd;
HWND reporterWnd;
HWND reloadServerButtonWnd, 
	 verboseLoggingButtonWnd;
HINSTANCE g_instance;
bool globalTeminateEvent=false;

EncPwdType EncPwd;

static void ShowLoggingLevel( void )
{
	bool bVerboseON	= log_1.GetMsgAllowed( LOG_VERBOSE );
	bool bDebugON	= log_1.GetMsgAllowed( LOG_DEBUG );
	
	// Show the message in the text color of the related message type.
	// Helps the user know what the colors represent.
	log_1.SetMsgAllowed( LOG_VERBOSE, true );
	log_1.SetMsgAllowed( LOG_DEBUG, true );
	
	log_1.AddLog( LOG_NORMAL,  "----------------------------------------" );
	log_1.AddLog( LOG_VERBOSE, "Verbose logging... %s", ( bVerboseON ) ? "ON" : "OFF" );
	log_1.AddLog( LOG_DEBUG,   "Debug logging..... %s", ( bDebugON )   ? "ON" : "OFF" );
	log_1.AddLog( LOG_NORMAL,  "----------------------------------------" );

	log_1.SetMsgAllowed( LOG_VERBOSE, bVerboseON );
	log_1.SetMsgAllowed( LOG_DEBUG, bDebugON );
}

static void OnChangeLoggingLevel( void )
{
	static struct	{
		bool bVerboseEnabled;
		bool bDebugEnabled;
	} stateList[] = {
		{ false,	false },
		{ true,		false },
		{ true,		true  }
	};
	static int sCurrState = 0;
	static int sIncr = 1;
	
	sCurrState += sIncr;
	if ( sCurrState == ARRAY_SIZE(stateList) )
	{
		sCurrState = ( ARRAY_SIZE(stateList) - 2 );
		sIncr = -1;
	}
	else if ( sCurrState < 0 )
	{
		sCurrState = 1;
		sIncr = 1;
	}
	log_1.SetMsgAllowed( LOG_VERBOSE, stateList[sCurrState].bVerboseEnabled );
	log_1.SetMsgAllowed( LOG_DEBUG, stateList[sCurrState].bDebugEnabled );
	ShowLoggingLevel();
}

static void ShowConfigFileLoadError( void )
{
	char cwd[_MAX_PATH];
	char msg[_MAX_PATH + 256];
	_getcwd(cwd, _MAX_PATH);
	sprintf_s( msg, ARRAY_SIZE(msg),
		"Could not open config file: \n"
		"    %s\\%s", 
			cwd, 
			CONFIG_FILENAME );
	MessageBox( mainWnd, msg, "Fatal Error", MB_ICONERROR | MB_OK );
}

static void ShowLogDirectoryError( void )
{
	char cwd[_MAX_PATH];
	char msg[_MAX_PATH + 256];
	_getcwd(cwd, _MAX_PATH);
	sprintf_s( msg, ARRAY_SIZE(msg),
		"Can't create log file in folder '%s'.\n"
		"Current directory: '%s'\n\n"
		"Please make sure a '%s' folder exists in that location.\n",
			config.logDirectory,
			cwd,
			config.logDirectory );
	MessageBox( mainWnd, msg, "Fatal Error", MB_ICONERROR | MB_OK );
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
_BEFORE
	int mainWidth;
	int mainHeight;
	
	switch (uMsg) {

	case WM_SIZE:
		if (hwnd == mainWnd) {
			mainWidth = LOWORD(lParam);
			mainHeight = HIWORD(lParam);
			MoveWindow(reporterWnd, BUTTON_WIDTH * 2, 0, mainWidth - BUTTON_WIDTH * 2, 20, TRUE);
			MoveWindow(logWnd, 0, 20, mainWidth, mainHeight - 20, TRUE);
			MoveWindow(reloadServerButtonWnd, 0, 0, BUTTON_WIDTH, 20, TRUE);
			MoveWindow(verboseLoggingButtonWnd, BUTTON_WIDTH, 0, BUTTON_WIDTH, 20, TRUE);
		}
		else if (hwnd == logWnd) {
			log_1.Resize(LOWORD(lParam), HIWORD(lParam));
		}
		else if (hwnd = reporterWnd) {
			reporter.Resize(LOWORD(lParam), HIWORD(lParam));
		}
		break;
	case WM_PAINT:
		if (hwnd == logWnd) {
			log_1.Redraw();
		} else if ( hwnd == reporterWnd ) {
			reporter.Redraw();
		}
		break;
	case WM_CLOSE:
		if (hwnd == mainWnd) {
			DestroyWindow(hwnd);
		}
		break;
	case WM_DESTROY:
		if (hwnd == mainWnd) {
			g_bTerminating = true;
			log_1.Enable( false );
			job.SetTerminate();
			Sleep(2000);
			PostQuitMessage(0);
		}
		break;
	case WM_TIMER:
		if (wParam == 102)
		{
			reporter.m_UserCount = accountdb.GetUserNum();
			InvalidateRect(reporterWnd, NULL, FALSE);

		}
		else if ( wParam == 103 )
		{
			g_ServerList.RequestUserCounts();
		}
		break;

	case WM_COMMAND:
		{
			int notification = HIWORD(wParam);
			int buttonId = LOWORD(wParam);
			switch (notification) {
			case BN_CLICKED:
				if (buttonId == RELOAD_BUTTON_ID) {
					g_ServerList.Load();
				}
				else if (buttonId == LOGLEVEL_BUTTON_ID) {
					OnChangeLoggingLevel();
				}
				break;
			}
		}
		break;
	case WM_KEYDOWN:
		break;
	}
_AFTER_FIN
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
};
unsigned char blowFishKey[] = "[;'.]94-31==-%&@!^+]";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{


#ifdef _DEBUG
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(tmpFlag);
#endif
	//HWND prevHwnd = FindWindow( NULL, "AuthServer" );
	//if ( prevHwnd != NULL ){
	//	//MessageBox( NULL, "ÀÌ¹Ì ÀÎÁõ¼­¹ö°¡ ½ÇÇàµÇ¾î ÀÖ½À´Ï´Ù. ", "°æ°í", MB_ICONERROR | MB_OK );
	//	MessageBox( NULL, "An instance of Authserver is already running.", "Error", MB_ICONERROR | MB_OK );
	//	exit(0);
	//}
	
	InitializeBlowfish(blowFishKey, sizeof(blowFishKey));


	//extern void InitRSAParams();
	//InitRSAParams();

	g_linDB = new DBEnv;
	server = new CServer; //game server
	serverEx = new CIOServerEx; //auth gate
	serverInt = new CServerInt; //inner server for gm tool

	WNDCLASSEX wcx;
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.style = CS_CLASSDC;
	wcx.lpfnWndProc = WindowProc;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hInstance;
	wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON3));;
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground = (HBRUSH) NULL;
	wcx.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN_MENU);
	wcx.lpszClassName = "AuthServer";
	wcx.hIconSm = NULL;
	ATOM windowClass = RegisterClassEx(&wcx);
	g_instance = hInstance;

exception_init();

	WSADATA wsaData;
	int err = WSAStartup(0x0202, &wsaData);

	if (err) {
		log_1.AddLog(LOG_ERROR, "WSAStartup error 0x%x", err);
		return 0;
	}

	mainWnd = CreateWindowEx(0, (const char*)windowClass, "AuthServer", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 860, 440, NULL, NULL, hInstance, NULL);

	logWnd = CreateWindowEx(WS_EX_CLIENTEDGE, (const char*)windowClass, "", WS_CHILD, 0, 30, 640, 720,
		mainWnd, NULL, hInstance, NULL);

	reporterWnd = CreateWindowEx(WS_EX_CLIENTEDGE, (const char*)windowClass, "", WS_CHILD, 0, 0, 640, 30,
		mainWnd, NULL, hInstance, NULL);

	reloadServerButtonWnd = CreateWindowEx(0, "BUTTON", "Reload Server List", WS_CHILD|BS_PUSHBUTTON, 600, 0, 40, 30, mainWnd,
		(HMENU)RELOAD_BUTTON_ID, hInstance, NULL);

	verboseLoggingButtonWnd = CreateWindowEx(0, "BUTTON", "Logging Level", WS_CHILD|BS_PUSHBUTTON, 600, 0, 40, 30, mainWnd,
		(HMENU)LOGLEVEL_BUTTON_ID, hInstance, NULL);
		
	log_1.SetWnd( logWnd );
	reporter.SetWnd( reporterWnd );
	//SetProcessPriorityBoost(GetCurrentProcess(), TRUE);

	ShowWindow(mainWnd, nCmdShow);
	UpdateWindow(mainWnd);

	ShowWindow(logWnd, SW_SHOW);
	UpdateWindow(logWnd);

	ShowWindow(reporterWnd, SW_SHOW);
	UpdateWindow(reporterWnd);

	ShowWindow(reloadServerButtonWnd, SW_SHOW);
	UpdateWindow(reloadServerButtonWnd);

	ShowWindow(verboseLoggingButtonWnd, SW_SHOW);
	UpdateWindow(verboseLoggingButtonWnd);

// Start Init
	DesKeyInit("TEST");
	if ( ! config.Load( CONFIG_FILENAME ) )
	{
		ShowConfigFileLoadError();
		exit(0);
	}

	unsigned listenThreadId;
	HANDLE listenThread=NULL;

	log_1.SetMsgAllowed( LOG_VERBOSE, config.enableVerboseLogging );
	log_1.SetMsgAllowed( LOG_DEBUG, config.enableDebugLogging );
	
	if (strlen(config.logDirectory) <= 0 )
	{
		log_1.SetDirectory( "log" );
		log_1.Enable(true );
		filelog.SetDirectory( config.logDirectory);
		actionlog.SetDirectory( config.logDirectory );
		logdfilelog.SetDirectory( config.logDirectory );
		log_1.AddLog( LOG_ERROR, "Error load config.txt" );
		logdfilelog.SetDirectory( config.logDirectory );
	} else {
		if ( !log_1.SetDirectory( config.logDirectory ) )
		{
			ShowLogDirectoryError();
			exit(0);		
		}
		log_1.Enable( true );
		filelog.SetDirectory( config.logDirectory);
		actionlog.SetDirectory( config.logDirectory );
		errlog.SetDirectory( config.logDirectory );
		logdfilelog.SetDirectory( config.logDirectory );
		//Every 2 seconds, update our UI's list of players
        SetTimer(mainWnd, 102, 2000, NULL);
		//TBROWN - explanation - every minute, ping all of the connected servers and ask how many users are logged on 
        SetTimer(mainWnd, 103, 60000, NULL );
		switch ( config.gameId ) 
		{
		case 4:
			EncPwd = EncPwdShalo;
			break;
		case 8:
		case 16:
		case 32:
			EncPwd = EncPwdL2;
			break;
		default:
			EncPwd = EncPwdDev;
			break;
		}
		// write the major loaded config environment
		log_1.AddLog( LOG_VERBOSE,   "LOADED Config");
		ShowLoggingLevel();
		log_1.AddLog( LOG_DEBUG, "WorldPort:%d",		config.worldPort );
		log_1.AddLog( LOG_DEBUG, "ServerPort:%d",	config.serverPort );
		log_1.AddLog( LOG_DEBUG, "ServerIntPort:%d", config.serverIntPort );
		log_1.AddLog( LOG_DEBUG, "ServerExPort:%d",	config.serverExPort );
		log_1.AddLog( LOG_DEBUG, "Protocol Version:%d", config.ProtocolVer );
		log_1.AddLog( LOG_DEBUG, "Log Directory:%s", config.logDirectory );
		log_1.AddLog( LOG_DEBUG, "DBConnectionNum:%d,GameID:%d", config.numDBConn, config.gameId );
		log_1.AddLog( LOG_DEBUG, "ServerThread:%d", config.numServerThread );

		if ( config.encrypt )
			log_1.AddLog( LOG_DEBUG, "Encrypt:True" );
		else
			log_1.AddLog( LOG_DEBUG, "Encrypt:False" );

		if ( config.DesApply )
			log_1.AddLog( LOG_DEBUG, "DesApply:True" );
		else
			log_1.AddLog( LOG_DEBUG, "DesApply:False" );

		if ( config.OneTimeLogOut )
			log_1.AddLog( LOG_DEBUG, "OneTimeLogOut:True" );
		else
			log_1.AddLog( LOG_DEBUG, "OneTimeLogOut:False" );
		if ( config.RestrictGMIP )
			log_1.AddLog( LOG_DEBUG, "RestrictGMIP:True" );
		else
			log_1.AddLog( LOG_DEBUG, "RestrictGMIP:False" );

		log_1.AddLog( LOG_DEBUG, "GMIP:%d.%d.%d.%d",
								config.GMIP.S_un.S_un_b.s_b1, 
								config.GMIP.S_un.S_un_b.s_b2, 
								config.GMIP.S_un.S_un_b.s_b3, 
								config.GMIP.S_un.S_un_b.s_b4 );
		log_1.AddLog( LOG_DEBUG, "logdPort:%d, logdReconnectInterval:%d", config.LogDPort, config.LogDReconnectInterval );
		log_1.AddLog( LOG_NORMAL, "BuildNumber : %d", buildNumber );
		if ( config.AcceptCallNum == 0 ){
			log_1.AddLog( LOG_ERROR, "AcceptCallNull ÀÌ 0 ÀÌ¸é ¾î¶² Å¬¶óÀÌ¾ðÆ®µµ Á¢¼ÓÀÌ ¾ÈµË´Ï´Ù. 1 ·Î ÀÚµ¿¼¼ÆÃÇÕ´Ï´Ù." );
			config.AcceptCallNum = 1;
		}
		if ( config.SocketTimeOut == 0 ){
			log_1.AddLog( LOG_ERROR, "SocketTimeOutÀÌ 0ÀÌ¸é ConnectionÀÌ ¹Ù·Î ²÷±â°Ô µË´Ï´Ù. 180À¸·Î ÀÚµ¿¼¼ÆÃÇÕ´Ï´Ù. " );
			config.SocketTimeOut = 180;
		}

		if ( config.WaitingUserLimit == 0 ){
			log_1.AddLog( LOG_ERROR, "WaitingUserLimit°¡ 0ÀÌ¸é ConnectionÀÌ ÀÌ·ç¾îÁöÁö ¾Ê½À´Ï´Ù. 100À¸·Î ÀÚµ¿¼¼ÆÃÇÕ´Ï´Ù." );
			config.WaitingUserLimit = 100;
		}

		if ( config.useForbiddenIPList ) {
			if ( config.Country == CC_KOREA ) {
				log_1.AddLog( LOG_NORMAL, "Á¢±Ù ±ÝÁö IP¸ñ·ÏÀ» ÀÐ¾î µéÀÔ´Ï´Ù. " );
			} else {
				log_1.AddLog( LOG_NORMAL, "LOAD FORBIDDEN IP LIST" );
			}
			forbiddenIPList.Load( "etc\\BlockIPs.txt" );
		}

		g_linDB->Init( config.numDBConn );
		g_ServerList.Load();

		CDBConn conn(g_linDB);
		conn.Execute( "update worldstatus set status=0" );		

	// 2003-07-15 // logd paste

		CreateIOThread( );
		if ( config.UseLogD ) {

			// LOGD Server¿¡ °ü·ÃµÈ ³»¿ëÀÌ µé¾î°£´Ù. 
			// 1. SocketÀ» »ý¼ºÇÑ´Ù. 
			SOCKET LOGSock = socket(AF_INET, SOCK_STREAM, 0);
			// 2. ¼ÒÄÏ Connection¿¡ »ç¿ëÇÒ Destination SettingÀ» ÇÑ´Ù.
			sockaddr_in Destination;
			Destination.sin_family = AF_INET;
			Destination.sin_addr   = config.LogDIP;
			Destination.sin_port   = htons( (u_short)config.LogDPort );
			// 3. ConnectionÀ» ¸Î´Â´Ù. 
			int ErrorCode = connect( LOGSock, ( sockaddr *)&Destination, sizeof( sockaddr ));

			
			// 4. ¸Î¾îÁø ConnectionÀ» ÀÌ¿ëÇÏ¿© LOGSocketÀ» »ý¼ºÇÑ´Ù. 
			//    Connection Error°¡ »ý°å´õ¶óµµ °ü°è ¾ø´Ù. 
			//    ±×·¸°Ô µÇ¸é ÀÚµ¿ÀûÀ¸·Î Timer°¡ ÀÛµ¿ÇÏ¿© 10ÃÊ¿¡ ÇÑ¹ø¾¿ ReconnectionÀ» ½ÃµµÇÏ°Ô µÈ´Ù. 
			pLogSocket = CLogSocket::Allocate(LOGSock);
			pLogSocket->SetAddress( config.LogDIP );
			if ( ErrorCode == SOCKET_ERROR ){
				pLogSocket->CloseSocket();
			} else {
				pLogSocket->Initialize( g_hIOCompletionPortInt );
			}
		}

		if ( config.UseIPServer ) {

			// IP Server¿¡ °ü·ÃµÈ ³»¿ëÀÌ µé¾î°£´Ù. 
			// 1. SocketÀ» »ý¼ºÇÑ´Ù. 
			SOCKET IPSock = socket(AF_INET, SOCK_STREAM, 0);
			// 2. ¼ÒÄÏ Connection¿¡ »ç¿ëÇÒ Destination SettingÀ» ÇÑ´Ù.
			sockaddr_in Destination;
			Destination.sin_family = AF_INET;
			Destination.sin_addr   = config.IPServer;
			Destination.sin_port   = htons( (u_short)config.IPPort );
			// 3. ConnectionÀ» ¸Î´Â´Ù. 
			int ErrorCode = connect( IPSock, ( sockaddr *)&Destination, sizeof( sockaddr ));

			
			// 4. ¸Î¾îÁø ConnectionÀ» ÀÌ¿ëÇÏ¿© IPSocketÀ» »ý¼ºÇÑ´Ù. 
			//    Connection Error°¡ »ý°å´õ¶óµµ °ü°è ¾ø´Ù. 
			//    ±×·¸°Ô µÇ¸é ÀÚµ¿ÀûÀ¸·Î Timer°¡ ÀÛµ¿ÇÏ¿© 10ÃÊ¿¡ ÇÑ¹ø¾¿ ReconnectionÀ» ½ÃµµÇÏ°Ô µÈ´Ù. 
			pIPSocket = new CIPSocket(IPSock);
			pIPSocket->SetAddress( config.IPServer );
			if ( ErrorCode == SOCKET_ERROR ){
				pIPSocket->CloseSocket();
			} else
				pIPSocket->Initialize( g_hIOCompletionPort );
		}

		// Wanted ½Ã½ºÅÛ°ú ¿¬µ¿ ºÎºÐ
		if ( config.UseWantedSystem ) {
			SOCKET WantedSocket = socket( AF_INET, SOCK_STREAM, 0 );
			sockaddr_in WantedAddr;
			WantedAddr.sin_family = AF_INET;
			WantedAddr.sin_addr = config.WantedIP;
			WantedAddr.sin_port = htons( (u_short)config.WantedPort );
			
			int ErrorCode = connect( WantedSocket, (sockaddr *)&WantedAddr, sizeof(sockaddr));
			pWantedSocket = new CWantedSocket( WantedSocket );
			pWantedSocket->SetAddress( config.WantedIP );
			if ( ErrorCode == SOCKET_ERROR ) {
				pWantedSocket->CloseSocket();
			} else 
				pWantedSocket->Initialize( g_hIOCompletionPortInt );
		}

		listenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, 0, 0, &listenThreadId);
	}

	// end Init
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if ( listenThread != NULL )
		CloseHandle(listenThread);	
	
	while( !globalTeminateEvent )
		Sleep(1000);
	
	Sleep(2000);

	server->ReleaseRef();
	serverEx->ReleaseRef();
	serverInt->ReleaseRef();
	g_linDB->ReleaseRef();

	WSACleanup();

	return 0;
}


