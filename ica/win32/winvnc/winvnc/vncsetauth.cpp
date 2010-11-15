//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncSetAuth.cpp

// Implementation of the About dialog!

#include "stdhdrs.h"

#include "winvnc.h"
#include "vncsetauth.h"
#include "vncservice.h"
#include "common/win32_helpers.h"

#define MAXSTRING 254

const TCHAR REGISTRY_KEY [] = "Software\\UltraVnc";

// [v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

void
vncSetAuth::OpenRegistry()
{
	if (m_fUseRegistry)
	{
		DWORD dw;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			REGISTRY_KEY,
			0,REG_NONE, REG_OPTION_NON_VOLATILE,
			KEY_READ,
			NULL, &hkLocal, &dw) != ERROR_SUCCESS)
			return;
		if (RegCreateKeyEx(hkLocal,
			"mslogon",
			0, REG_NONE, REG_OPTION_NON_VOLATILE,
			KEY_WRITE | KEY_READ,
			NULL, &hkDefault, &dw) != ERROR_SUCCESS)
			return;
	}
}

void
vncSetAuth::CloseRegistry()
{
	if (m_fUseRegistry)
	{
		if (hkDefault != NULL) RegCloseKey(hkDefault);
		if (hkUser != NULL) RegCloseKey(hkUser);
		if (hkLocal != NULL) RegCloseKey(hkLocal);
	}
}

LONG
vncSetAuth::LoadInt(HKEY key, LPCSTR valname, LONG defval)
{
	if (m_fUseRegistry)
	{
		LONG pref;
		ULONG type = REG_DWORD;
		ULONG prefsize = sizeof(pref);

		if (RegQueryValueEx(key,
			valname,
			NULL,
			&type,
			(LPBYTE) &pref,
			&prefsize) != ERROR_SUCCESS)
			return defval;

		if (type != REG_DWORD)
			return defval;

		if (prefsize != sizeof(pref))
			return defval;

		return pref;
	}
	else
	{
		return myIniFile.ReadInt("admin_auth", (char *)valname, defval);
	}
}

TCHAR *
vncSetAuth::LoadString(HKEY key, LPCSTR keyname)
{
	if (m_fUseRegistry)
	{
		DWORD type = REG_SZ;
		DWORD buflen = 256*sizeof(TCHAR);
		TCHAR *buffer = 0;

		// Get the length of the string
		if (RegQueryValueEx(key,
			keyname,
			NULL,
			&type,
			NULL,
			&buflen) != ERROR_SUCCESS)
			return 0;

		if (type != REG_BINARY)
			return 0;
		buflen = 256*sizeof(TCHAR);
		buffer = new TCHAR[buflen];
		if (buffer == 0)
			return 0;

		// Get the string data
		if (RegQueryValueEx(key,
			keyname,
			NULL,
			&type,
			(BYTE*)buffer,
			&buflen) != ERROR_SUCCESS) {
			delete [] buffer;
			return 0;
		}

		// Verify the type
		if (type != REG_BINARY) {
			delete [] buffer;
			return 0;
		}

		return (TCHAR *)buffer;
	}
	else
	{
		TCHAR *authhosts=new char[150];
		myIniFile.ReadString("admin_auth", (char *)keyname,authhosts,150);
		return (TCHAR *)authhosts;
	}
}

void
vncSetAuth::SaveInt(HKEY key, LPCSTR valname, LONG val)
{
	if (m_fUseRegistry)
	{
		RegSetValueEx(key, valname, 0, REG_DWORD, (LPBYTE) &val, sizeof(val));
	}
	else
	{
		myIniFile.WriteInt("admin_auth", (char *)valname, val);
	}
}

void
vncSetAuth::SaveString(HKEY key,LPCSTR valname, TCHAR *buffer)
{
	if (m_fUseRegistry)
	{
		RegSetValueEx(key, valname, 0, REG_BINARY, (LPBYTE) buffer, MAXSTRING);
	}
	else
	{
		myIniFile.WriteString("admin_auth", (char *)valname,buffer);
	}
}

void
vncSetAuth::savegroup1(TCHAR *value)
{
	if (m_fUseRegistry)
	{
		OpenRegistry();
		if (hkDefault)SaveString(hkDefault, "group1", value);
		CloseRegistry();
	}
	else
	{
		SaveString(hkDefault, "group1", value);
	}
}
TCHAR*
vncSetAuth::Readgroup1()
{
	if (m_fUseRegistry)
	{
		TCHAR *value=NULL;
		OpenRegistry();
		if (hkDefault) value=LoadString (hkDefault, "group1");
		CloseRegistry();
		return value;
	}
	else
	{
		TCHAR *value=NULL;
		value=LoadString (hkDefault, "group1");
		return value;
	}
}

void
vncSetAuth::savegroup2(TCHAR *value)
{
	if (m_fUseRegistry)
	{
		OpenRegistry();
		if (hkDefault)SaveString(hkDefault, "group2", value);
		CloseRegistry();
	}
	else
	{
		SaveString(hkDefault, "group2", value);
	}
}
TCHAR*
vncSetAuth::Readgroup2()
{
	if (m_fUseRegistry)
		{
		TCHAR *value=NULL;
		OpenRegistry();
		if (hkDefault) value=LoadString (hkDefault, "group2");
		CloseRegistry();
		return value;
	}
	else
	{
		TCHAR *value=NULL;
		value=LoadString (hkDefault, "group2");
		return value;
	}
}

void
vncSetAuth::savegroup3(TCHAR *value)
{
	if (m_fUseRegistry)
	{
		OpenRegistry();
		if (hkDefault)SaveString(hkDefault, "group3", value);
		CloseRegistry();
	}
	else
	{
		SaveString(hkDefault, "group3", value);
	}
}
TCHAR*
vncSetAuth::Readgroup3()
{
	if (m_fUseRegistry)
	{
		TCHAR *value=NULL;
		OpenRegistry();
		if (hkDefault) value=LoadString (hkDefault, "group3");
		CloseRegistry();
		return value;
	}
	else
	{
		TCHAR *value=NULL;
		value=LoadString (hkDefault, "group3");
		return value;
	}
}

LONG
vncSetAuth::Readlocdom1(LONG returnvalue)
{
	if (m_fUseRegistry)
	{
		OpenRegistry();
		if (hkDefault) returnvalue=LoadInt(hkDefault, "locdom1",returnvalue);
		CloseRegistry();
		return returnvalue;
	}
	else
	{
		returnvalue=LoadInt(hkDefault, "locdom1",returnvalue);
		return returnvalue;
	}
}

void
vncSetAuth::savelocdom1(LONG value)
{
	if (m_fUseRegistry)
	{
		OpenRegistry();
		if (hkDefault)SaveInt(hkDefault, "locdom1", value);
		CloseRegistry();
	}
	else
	{
		SaveInt(hkDefault, "locdom1", value);
	}

}

LONG
vncSetAuth::Readlocdom2(LONG returnvalue)
{
	if (m_fUseRegistry)
	{
		OpenRegistry();
		if (hkDefault) returnvalue=LoadInt(hkDefault, "locdom2",returnvalue);
		CloseRegistry();
		return returnvalue;
	}
	else
	{
		returnvalue=LoadInt(hkDefault, "locdom2",returnvalue);
		return returnvalue;
	}
}

void
vncSetAuth::savelocdom2(LONG value)
{
	if (m_fUseRegistry)
	{
		OpenRegistry();
		if (hkDefault)SaveInt(hkDefault, "locdom2", value);
		CloseRegistry();
	}
	else
	{
		SaveInt(hkDefault, "locdom2", value);
	}

}

LONG
vncSetAuth::Readlocdom3(LONG returnvalue)
{
	if (m_fUseRegistry)
	{
		OpenRegistry();
		if (hkDefault) returnvalue=LoadInt(hkDefault, "locdom3",returnvalue);
		CloseRegistry();
		return returnvalue;
	}
	else
	{
		returnvalue=LoadInt(hkDefault, "locdom3",returnvalue);
		return returnvalue;
	}
}

void
vncSetAuth::savelocdom3(LONG value)
{
	if (m_fUseRegistry)
	{
		OpenRegistry();
		if (hkDefault)SaveInt(hkDefault, "locdom3", value);
		CloseRegistry();
	}
	else
	{
		SaveInt(hkDefault, "locdom3", value);
	}

}

///////////////////////////////////////////////////////////



// Constructor/destructor
vncSetAuth::vncSetAuth()
{
	m_fUseRegistry = ((myIniFile.ReadInt("admin", "UseRegistry", 0) == 1) ? TRUE : FALSE);
	m_dlgvisible = FALSE;
	hkLocal=NULL;
	hkDefault=NULL;
	hkUser=NULL;
	locdom1=0;
	locdom2=0;
	locdom3=0;
	group1=Readgroup1();
	group2=Readgroup2();
	group3=Readgroup3();
	locdom1=Readlocdom1(locdom1);
	locdom2=Readlocdom2(locdom2);
	locdom3=Readlocdom3(locdom3);
	if (group1){strcpy(pszgroup1,group1);delete [] group1;}
	else strcpy(pszgroup1,"");
	if (group2){strcpy(pszgroup2,group2);delete [] group2;}
	else strcpy(pszgroup2,"");
	if (group3){strcpy(pszgroup3,group3);delete [] group3;}
	else strcpy(pszgroup3,"");
}

vncSetAuth::~vncSetAuth()
{
}

// Initialisation
BOOL
vncSetAuth::Init(vncServer *server)
{
	m_server = server;
	m_dlgvisible = FALSE;
	hkLocal=NULL;
	hkDefault=NULL;
	hkUser=NULL;
	locdom1=0;
	locdom2=0;
	locdom3=0;
	group1=Readgroup1();
	group2=Readgroup2();
	group3=Readgroup3();
	locdom1=Readlocdom1(locdom1);
	locdom2=Readlocdom2(locdom2);
	locdom3=Readlocdom3(locdom3);
	if (group1){strcpy(pszgroup1,group1);delete group1;}
	else strcpy(pszgroup1,"");
	if (group2){strcpy(pszgroup2,group2);delete group2;}
	else strcpy(pszgroup2,"");
	if (group3){strcpy(pszgroup3,group3);delete group3;}
	else strcpy(pszgroup3,"");
	return TRUE;
}

// Dialog box handling functions
void
vncSetAuth::Show(BOOL show)
{
	if (show)
	{
		if (!m_dlgvisible)
		{
			// [v1.0.2-jp1 fix] Load resouce from dll
			//DialogBoxParam(hAppInstance,
			DialogBoxParam(hInstResDLL,
				MAKEINTRESOURCE(IDD_MSLOGON), 
				NULL,
				(DLGPROC) DialogProc,
				(LONG_PTR) this);
		}
	}
}

BOOL CALLBACK
vncSetAuth::DialogProc(HWND hwnd,
					 UINT uMsg,
					 WPARAM wParam,
					 LPARAM lParam )
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
    vncSetAuth *_this = helper::SafeGetWindowUserData<vncSetAuth>(hwnd);

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			// Retrieve the Dialog box parameter and use it as a pointer
			// to the calling vncProperties object
            helper::SafeSetWindowUserData(hwnd, lParam);

			_this = (vncSetAuth *) lParam;
			SetDlgItemText(hwnd, IDC_GROUP1, _this->pszgroup1);
			SetDlgItemText(hwnd, IDC_GROUP2, _this->pszgroup2);
			SetDlgItemText(hwnd, IDC_GROUP3, _this->pszgroup3);
			if (_this->locdom1==1 || _this->locdom1==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1L);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1L);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (_this->locdom1==2 || _this->locdom1==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1D);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG1D);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (_this->locdom2==1 || _this->locdom2==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2L);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2L);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (_this->locdom2==2 || _this->locdom2==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2D);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG2D);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (_this->locdom3==1 || _this->locdom3==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3L);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3L);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			if (_this->locdom3==2 || _this->locdom3==3)
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3D);
				SendMessage(hG1l,BM_SETCHECK,true,0);
			}
			else
			{
				HWND hG1l = GetDlgItem(hwnd, IDC_CHECKG3D);
				SendMessage(hG1l,BM_SETCHECK,false,0);
			}
			//already handled by vncproperties
			//if we get at thgis place
			//IDC_MSLOGON_CHECKD was checked
			/*HWND hMSLogon = GetDlgItem(hwnd, IDC_MSLOGON_CHECKD);
            SendMessage(hMSLogon, BM_SETCHECK, _this->m_server->MSLogonRequired(), 0);
			if (SendMessage(hMSLogon, BM_GETCHECK,0, 0) == BST_CHECKED)
				{
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG1D), true);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG2D), true);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG3D), true);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG1L), true);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG2L), true);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG3L), true);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP1), true);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP2), true);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP3), true);
				}
			else
				{
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG1D), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG2D), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG3D), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG1L), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG2L), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG3L), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP1), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP2), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP3), FALSE);
				}*/



			// Show the dialog
			SetForegroundWindow(hwnd);

			_this->m_dlgvisible = TRUE;

			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDCANCEL:
			EndDialog(hwnd, TRUE);
				_this->m_dlgvisible = FALSE;
				return TRUE;
		case IDOK:
			{
				_this->locdom1=0;
				_this->locdom2=0;
				_this->locdom3=0;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG1L),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom1=_this->locdom1+1;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG1D),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom1=_this->locdom1+2;

				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG2L),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom2=_this->locdom2+1;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG2D),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom2=_this->locdom2+2;

				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG3L),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom3=_this->locdom3+1;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKG3D),BM_GETCHECK,0,0) == BST_CHECKED) _this->locdom3=_this->locdom3+2;

				GetDlgItemText(hwnd, IDC_GROUP1, (LPSTR) _this->pszgroup1, 240);
				GetDlgItemText(hwnd, IDC_GROUP2, (LPSTR) _this->pszgroup2, 240);
				GetDlgItemText(hwnd, IDC_GROUP3, (LPSTR) _this->pszgroup3, 240);

				bool use_uac=false;
				if (!_this->myIniFile.IsWritable() || vncService::RunningAsService() )
				{
					// We can't write to the ini file , Vista in service mode
					if (!Copy_to_Temp(_this->m_Tempfile))
					{
						// temp is not writable, we need to close else
						// ini get overwritten by default
						EndDialog(hwnd, TRUE);
						_this->m_dlgvisible = FALSE;
						return TRUE;
					}
					_this->myIniFile.IniFileSetTemp(_this->m_Tempfile);
					use_uac=true;
				}
	
				_this->savegroup1(_this->pszgroup1);
				_this->savegroup2(_this->pszgroup2);
				_this->savegroup3(_this->pszgroup3);
				_this->savelocdom1(_this->locdom1);
				_this->savelocdom2(_this->locdom2);
				_this->savelocdom3(_this->locdom3);
				if (use_uac==true)
				{
				_this->myIniFile.copy_to_secure();
				_this->myIniFile.IniFileSetSecure();
				}

				EndDialog(hwnd, TRUE);
				_this->m_dlgvisible = FALSE;
				return TRUE;
			}
		// rdv not needed, already handled by vncproperties
		// 
		/*case IDC_MSLOGON_CHECKD:
			{
				HWND hMSLogon = GetDlgItem(hwnd, IDC_MSLOGON_CHECKD);
				_this->m_server->RequireMSLogon(SendMessage(hMSLogon, BM_GETCHECK, 0, 0) == BST_CHECKED);
				if (SendMessage(hMSLogon, BM_GETCHECK,0, 0) == BST_CHECKED)
					{
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG1D), true);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG2D), true);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG3D), true);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG1L), true);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG2L), true);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG3L), true);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP1), true);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP2), true);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP3), true);
						
					}
				else
					{
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG1D), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG2D), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG3D), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG1L), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG2L), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECKG3L), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP1), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP2), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_GROUP3), FALSE);
					}
			}*/
		}

		break;

	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		_this->m_dlgvisible = FALSE;
		return TRUE;
	}
	return 0;
}
