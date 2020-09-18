#pragma once
#include "GlobalAuth.h"
#include <string>
#include <list>

using namespace std;

class GameTelnet
{
public:
	GameTelnet() {}
	~GameTelnet() {}

	struct Info
	{
		string parameter_1;
		string parameter_2;
		string parameter_3;
		string parameter_4;
		string parameter_5;
	};

	void ShowComandHelp();
	friend BOOL CALLBACK TelnetProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam);

private:
	list<Info> m_info;
};


extern GameTelnet *g_gameTelnet;
