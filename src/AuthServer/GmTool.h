#pragma once
#include "GlobalAuth.h"

using namespace std;

class Tool
{
public:
	Tool();
	~Tool();
	void SendGiftToRole();
	void TestList();
	void ExecuteSqlGift(string name, string gift_name, int count);
	friend BOOL CALLBACK ToolGiftDlgProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam);
	friend BOOL CALLBACK TestListProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam);
private:

};

extern Tool *g_tool;

