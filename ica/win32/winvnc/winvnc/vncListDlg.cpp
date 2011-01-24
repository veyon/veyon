/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@Vnc Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/

// vncListDlg.cpp

// Implementation of the vncListDlg dialog!

#include "stdhdrs.h"

#include "winvnc.h"
#include "vncListDlg.h"
#include "common/win32_helpers.h"

// [v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

//
//
//
vncListDlg::vncListDlg()
{
	m_dlgvisible = FALSE;
}

//
//
//
vncListDlg::~vncListDlg()
{
}

//
//
//
BOOL vncListDlg::Init(vncServer* pServer)
{
	m_pServer = pServer;
	return TRUE;
}

//
//
//
void vncListDlg::Display()
{
	if (!m_dlgvisible)
	{
		// [v1.0.2-jp1 fix] Load resouce from dll
		//DialogBoxParam(	hAppInstance,
		DialogBoxParam(	hInstResDLL,
						MAKEINTRESOURCE(IDD_LIST_DLG), 
						NULL,
						(DLGPROC) DialogProc,
						(LONG_PTR) this
						);
	}
}

//
//
//
BOOL CALLBACK vncListDlg::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    vncListDlg *_this = helper::SafeGetWindowUserData<vncListDlg>(hwnd);
	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
            helper::SafeSetWindowUserData(hwnd, lParam);
			_this = (vncListDlg *) lParam;

			vncClientList::iterator i;
			HWND hList = GetDlgItem(hwnd, IDC_VIEWERS_LISTBOX);

			_this->m_pServer->ListAuthClients(hList);
			SendMessage(hList, LB_SETCURSEL, -1, 0);

			// adzm 2009-07-05
			HWND hPendingList = GetDlgItem(hwnd, IDC_PENDING_LISTBOX);
			_this->m_pServer->ListUnauthClients(hPendingList);

			SetForegroundWindow(hwnd);
			_this->m_dlgvisible = TRUE;
			if (!_this->m_pServer->GetAllowEditClients())
			{
				EnableWindow(GetDlgItem(hwnd, IDC_KILL_B), false);
			}
			else EnableWindow(GetDlgItem(hwnd, IDC_KILL_B), true);

			// Allow TextChat if one client only
			/*
			EnableWindow(GetDlgItem(hwnd, IDC_TEXTCHAT_B),
				         _this->m_pServer->AuthClientCount() == 1 ? TRUE : FALSE);
			*/
			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDCANCEL:
		case IDOK:
			EndDialog(hwnd, TRUE);
			_this->m_dlgvisible = FALSE;
			return TRUE;

		case IDC_KILL_B:
			{
			HWND hList = GetDlgItem(hwnd, IDC_VIEWERS_LISTBOX);
			LRESULT nSelected = SendMessage(hList, LB_GETCURSEL, 0, 0);
			if (nSelected != LB_ERR)
			{
				char szClient[128];
				if (SendMessage(hList, LB_GETTEXT, nSelected, (LPARAM)szClient) > 0)
					_this->m_pServer->KillClient(szClient);
			}
			EndDialog(hwnd, TRUE);
			_this->m_dlgvisible = FALSE;
			return TRUE;
			}
			break;

		case IDC_TEXTCHAT_B:
			{
			HWND hList = GetDlgItem(hwnd, IDC_VIEWERS_LISTBOX);
			LRESULT nSelected = SendMessage(hList, LB_GETCURSEL, 0, 0);
			if (nSelected != LB_ERR)
			{
				char szClient[128];
				if (SendMessage(hList, LB_GETTEXT, nSelected, (LPARAM)szClient) > 0)
					_this->m_pServer->TextChatClient(szClient);
			}
			EndDialog(hwnd, TRUE);
			_this->m_dlgvisible = FALSE;
			return TRUE;
			}
			break;

		}
		break;

	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		_this->m_dlgvisible = FALSE;
		return TRUE;
	}
	return 0;
}
