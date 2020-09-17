#include "GameReport.h"
#include "resource.h"
#include "config.h"
#include "DBConn.h"
#include "util.h"
#include "mbctype.h"
#include <wchar.h>


extern HINSTANCE g_instance;
GameReport *g_gameReport;

GameReport::GameReport()
{

}
GameReport::~GameReport()
{

}
void GameReport::GetBossInfoFromDB()
{
	m_boss.clear();
	CDBConn conn(g_linDB);
	CMySQLConenction* sqlConnection = conn.getConnect();
	static char sql[1024] = { 0 };
	memset(sql, 0, sizeof(sql));

	snprintf(sql, ARRAY_LEN(sql), "SELECT id, location, next_spawn_time from spawnlist_boss where next_spawn_time > DATE_SUB(CURDATE(),INTERVAL 1 DAY) order by next_spawn_time desc");
	if (0 != sqlConnection->Query(sql))
	{
		log_1.AddLog(LOG_ERROR, "ExecuteSql: %s 有误！", sql);
		return;
	}
	MYSQL_ROW pRow = sqlConnection->CurrentRow();
	while (pRow)
	{
		int rowIdx = 0;
		BossInfo boss;
		boss.id = Str2Int(pRow[rowIdx++]);
		const char* ptr = pRow[rowIdx];
		if (ptr)
			boss.boss_name.assign(ptr);
		else
			boss.boss_name.assign(" ");

		rowIdx++;
		ptr = pRow[rowIdx];
		if (ptr)
			boss.date_time.assign(ptr);
		else
			boss.date_time.assign("00-00-00 00:00:00");
		rowIdx++;

		m_boss.push_back(boss);
		pRow = sqlConnection->NextRow();
	}
	
}

void GameReport::GetGameBossInfo()
{
	GetBossInfoFromDB();
	DialogBoxParam(g_instance, MAKEINTRESOURCE(IDD_DIALOG_BOSS), NULL,
		(DLGPROC)BossProc, (LPARAM)this);
}
BOOL CALLBACK BossProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam)
{
	static GameReport *pReport;

	switch (dwMessage) {
	case WM_INITDIALOG:
	{
		pReport = (GameReport *)(INT_PTR)lParam;
		char *pDefault;
		char *pTitle;
		pDefault = "AuthDB";
		pTitle = "测试";
		LVCOLUMN LvCol;
		SendDlgItemMessage(hDlg, IDC_FILE, WM_SETTEXT, 0, (LPARAM)pDefault);
		SetWindowText(hDlg, (LPCTSTR)pTitle);
		HWND hList = GetDlgItem(hDlg, IDC_TEST_LIST1);
		memset(&LvCol, 0, sizeof(LvCol));
		LvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		LvCol.fmt = LVCFMT_LEFT;
		LvCol.cx = 50;
		//初始化列表
		LvCol.pszText = "编号";
		SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);
		LvCol.pszText = "时间";
		LvCol.cx = 120;
		SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);
		LvCol.pszText = "名字";
		LvCol.cx = 90;
		SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);
		ListView_SetExtendedListViewStyleEx(GetDlgItem(hDlg, IDC_TEST_LIST1), 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

		LVITEM LvItem;
		CHAR Buff[64] = { 0 };

		SendDlgItemMessage(hDlg, IDC_TEST_LIST1, LVM_DELETEALLITEMS, NULL, NULL);
		int i = 0;
		for (list<GameReport::BossInfo>::iterator itr = pReport->m_boss.begin(); itr != pReport->m_boss.end(); ++itr)
		{
			LvItem.iItem = i++;
			LvItem.mask = LVIF_TEXT;
			LvItem.iSubItem = 0;
			sprintf(Buff, "%d", itr->id);
			LvItem.pszText = Buff;
			SendDlgItemMessage(hDlg, IDC_TEST_LIST1, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&LvItem);
			memset(Buff, 0, sizeof(Buff));
			LvItem.iSubItem = 1;
			sprintf(Buff, "%s", Utf8ToGbk(itr->boss_name).c_str());
			LvItem.pszText = Buff;
			SendDlgItemMessage(hDlg, IDC_TEST_LIST1, LVM_SETITEM, (WPARAM)0, (LPARAM)&LvItem);
			memset(Buff, 0, sizeof(Buff));
			LvItem.iSubItem = 2;
			sprintf(Buff, "%s", Utf8ToGbk(itr->date_time).c_str());
			LvItem.pszText = Buff;
			SendDlgItemMessage(hDlg, IDC_TEST_LIST1, LVM_SETITEM, (WPARAM)0, (LPARAM)&LvItem);
		}
	}
	break;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
		{
			break;
		}
		case IDCANCEL:
		{
			EndDialog(hDlg, 0);
			break;
		}
		}
		break;
	case WM_NOTIFY:
	{
		NMHDR* pNm = (NMHDR*)lParam;
		// 是不是list控件的单击消息
		if (pNm->code == NM_CLICK && pNm->idFrom == IDC_TEST_LIST1)
		{
			CHAR text[64] = {};
			LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
			LVITEM lvi = {};
			lvi.iItem = lpnmitem->iItem;
			lvi.iSubItem = lpnmitem->iSubItem;
			lvi.pszText = text;
			lvi.cchTextMax = 10;
			lvi.mask = LVIF_TEXT;
			SendMessage(pNm->hwndFrom, LVM_GETITEM, 0, (LPARAM)&lvi);
			//SetDlgItemText(hwndDlg, IDC_EDIT1, text);
			MessageBox(hDlg, text, 0, MB_OK);
		}
	}
	break;
	case WM_CLOSE:
		EndDialog(hDlg, NULL);
		DestroyWindow(hDlg);
		break;
	}
	return 0;
}