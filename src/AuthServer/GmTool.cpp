#include "GmTool.h"
#include "resource.h"
#include "config.h"
#include "DBConn.h"
#include "util.h"
#include "mbctype.h"
#include <wchar.h>

Tool *g_tool;
extern HINSTANCE g_instance;


Tool::Tool()
{
}

Tool::~Tool()
{
}

void Tool::SendGiftToRole()
{
	DialogBoxParam(g_instance, MAKEINTRESOURCE(IDD_DIALOG_GIFT), NULL,
		(DLGPROC)ToolGiftDlgProc, (LPARAM)this);
}
void Tool::TestList()
{
	DialogBoxParam(g_instance, MAKEINTRESOURCE(IDD_DIALOG_LIST), NULL,
		(DLGPROC)TestListProc, (LPARAM)this);
}



void Tool::ExecuteSqlGift(string name, string gift_name, int count)
{
	CDBConn conn(g_linDB);
	CMySQLConenction* sqlConnection = conn.getConnect();
	int item_id;
	static char sql[1024] = { 0 };
	memset(sql, 0, sizeof(sql));

	snprintf(sql, ARRAY_LEN(sql), "SELECT item_id from etcitem where name = '%s'", GBKToUTF8(gift_name).c_str());
	if (0 != sqlConnection->Query(sql))
	{
		log_1.AddLog(LOG_ERROR, "ExecuteSqlGift: %s 物品名称有误！", sql);
		return;
	}
	MYSQL_ROW row = sqlConnection->CurrentRow();
	if (row)
	{
		item_id = Str2Int(row[0]);
	}
	else
	{
		log_1.AddLog(LOG_ERROR, "ExecuteSqlGift: %s 物品不存在！", gift_name.c_str());
		sqlConnection->ResetQuery();
		return;
	}
	
	snprintf(sql, ARRAY_LEN(sql), "INSERT INTO character_elf_warehouse (account_name, item_id, count) VALUES ('%s', %d, %d);", GBKToUTF8(name).c_str(), item_id, count);
	if (0 != sqlConnection->Exec(sql))
	{
		log_1.AddLog(LOG_ERROR, "ExecuteSqlGift: %s 插入玩家仓库语句有误！", sql);
		return;
	}
	sqlConnection->ResetQuery();
	log_1.AddLog(LOG_NORMAL, "发送%s物品到账号名字:%s 成功！", gift_name.c_str(), name.c_str());
}


BOOL CALLBACK ToolGiftDlgProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam)
{
	static Tool *pEnv;

	switch (dwMessage) {
	case WM_INITDIALOG:
		pEnv = (Tool *)(INT_PTR)lParam;
		char *pDefault;
		char *pTitle;
		pDefault = "AuthDB";
		pTitle = "发送物品";
		SendDlgItemMessage(hDlg, IDC_FILE, WM_SETTEXT, 0, (LPARAM)pDefault);
		SetWindowText(hDlg, (LPCTSTR)pTitle);
		return 0;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
		{
			char buffer[64];
			string name;
			string gift_name;
			int count;

			SendDlgItemMessage(hDlg, IDC_DIALOG_EDIT_1, WM_GETTEXT, 64, (LPARAM)buffer);
			if (strlen(buffer) == 0)
			{
				MessageBox(hDlg, "玩家名字不能为空！", "确认", MB_OK);
				break;
			}
			name.assign(buffer);
			SendDlgItemMessage(hDlg, IDC_DIALOG_EDIT_2, WM_GETTEXT, 64, (LPARAM)buffer);
			if (strlen(buffer) == 0)
			{
				MessageBox(hDlg, "物品名字不能为空！", "确认", MB_OK);
				break;
			}
			gift_name.assign(buffer);
			SendDlgItemMessage(hDlg, IDC_DIALOG_EDIT_3, WM_GETTEXT, 64, (LPARAM)buffer);
			if (strlen(buffer) == 0)
			{
				MessageBox(hDlg, "物品数量不能为空！", "确认", MB_OK);
				break;
			}
			count = Str2Int(buffer);
			pEnv->ExecuteSqlGift(name, gift_name, count);
			EndDialog(hDlg, 0);
			break;
		}
		case IDCANCEL:
		{
			EndDialog(hDlg, 0);
			break;
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

BOOL CALLBACK TestListProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam)
{
	static Tool *pEnv;

	switch (dwMessage) {
	case WM_INITDIALOG:
		{
			pEnv = (Tool *)(INT_PTR)lParam;
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
			LvCol.pszText = "端口号";
			SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);
			LvCol.pszText = "放行/禁止";
			LvCol.cx = 70;
			SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);
			ListView_SetExtendedListViewStyleEx(GetDlgItem(hDlg, IDC_TEST_LIST1), 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

			LVITEM LvItem;
			CHAR Buff[32] = { 0 };

			SendDlgItemMessage(hDlg, IDC_TEST_LIST1, LVM_DELETEALLITEMS, NULL, NULL);
			int i = 0;
			for (; i < 10; ++i)
			{
				LvItem.iItem = i;
				LvItem.mask = LVIF_TEXT;
				LvItem.iSubItem = 0;
				sprintf(Buff, "%d", 12);
				LvItem.pszText = Buff;
				SendDlgItemMessage(hDlg, IDC_TEST_LIST1, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&LvItem);

				memset(Buff, 0, sizeof(Buff));
				LvItem.iSubItem = 1;

				strcpy(Buff, "ffffff");

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
			CHAR text[10] = {};
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

	}
	return 0;
}