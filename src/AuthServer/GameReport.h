#pragma once
#include "GlobalAuth.h"
#include <string>
#include <list>

using namespace std;

class GameReport
{
public:

	struct BossInfo
	{
		int id;
		string boss_name;
		string date_time;
	};


	GameReport();
	~GameReport();
	void GetGameBossInfo();
	void GetBossInfoFromDB();

	friend BOOL CALLBACK BossProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam);



private:
	list<BossInfo> m_boss;

};

extern GameReport *g_gameReport;