#include "GameTelnet.h"
#include "resource.h"
#include "config.h"
#include "DBConn.h"
#include "util.h"
#include "mbctype.h"
#include "WantedSocket.h"
#include <wchar.h>

extern HINSTANCE g_instance;
GameTelnet *g_gameTelnet;

void GameTelnet::ShowComandHelp()
{
	pWantedSocket->Send("s", "help\n");
	//DialogBoxParam(g_instance, MAKEINTRESOURCE(IDD_DIALOG_TELNET), NULL,
	//(DLGPROC)TelnetProc, (LPARAM)this);
}
BOOL CALLBACK TelnetProc(HWND hDlg, DWORD dwMessage, DWORD wParam, DWORD lParam)
{
	static GameTelnet *pTelnet;

	switch (dwMessage) {
	case WM_INITDIALOG:
	{
		pTelnet = (GameTelnet *)(INT_PTR)lParam;
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