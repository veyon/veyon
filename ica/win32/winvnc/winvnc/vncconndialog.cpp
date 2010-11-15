//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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


// vncConnDialog.cpp: implementation of the vncConnDialog class, used
// to forge outgoing connections to VNC-viewer 

#include "stdhdrs.h"
#include "vncconndialog.h"
#include "winvnc.h"

#include "resource.h"
#include "common/win32_helpers.h"

#include <ctype.h>

#include "Localization.h" // ACT : Add localization on messages

//	[v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;
extern BOOL SPECIAL_SC_PROMPT;

// Constructor

vncConnDialog::vncConnDialog(vncServer *server)
{
	m_server = server;
	m_hicon = NULL;
	m_hfont = NULL;
}

// Destructor

vncConnDialog::~vncConnDialog()
{
}

// Routine called to activate the dialog and, once it's done, delete it

INT_PTR vncConnDialog::DoDialog()
{
	//	[v1.0.2-jp1 fix]
	//DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_OUTGOING_CONN), 	
	//adzm 2009-06-20 - Return the result
	INT_PTR nResult = DialogBoxParam(hInstResDLL, MAKEINTRESOURCE(IDD_OUTGOING_CONN), 
		NULL, (DLGPROC) vncConnDlgProc, (LONG_PTR) this);
	delete this;
	return nResult;
}

// Callback function - handles messages sent to the dialog box

BOOL CALLBACK vncConnDialog::vncConnDlgProc(HWND hwnd,
											UINT uMsg,
											WPARAM wParam,
											LPARAM lParam) {
	// This is a static method, so we don't know which instantiation we're 
	// dealing with. But we can get a pseudo-this from the parameter to 
	// WM_INITDIALOG, which we therafter store with the window and retrieve
	// as follows:
     vncConnDialog *_this = helper::SafeGetWindowUserData<vncConnDialog>(hwnd);

	switch (uMsg) {
	case WM_PAINT:
		{
			//adzm 2009-06-20
			if (GetUpdateRect(hwnd, NULL, TRUE)) {
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);

				{
					RECT rcIcon;
					rcIcon.top = 0;
					rcIcon.left = 0;
					rcIcon.bottom = 48;
					rcIcon.right = 48;
					
					RECT rcClient;
					if (GetClientRect(hwnd, &rcClient)) {
						int nDifference = (rcClient.bottom - rcIcon.bottom) / 2;
						if (nDifference > 0) {
							rcIcon.top += nDifference;
							rcIcon.bottom += nDifference;
						}
					}

					RECT rcLabel;
					HWND hwndLabel = GetDlgItem(hwnd, IDC_CONNECTION_NUMBER_STATIC);
					if (GetWindowRect(hwndLabel, &rcLabel)) {
						LPRECT lprcLabel = &rcLabel;
						ScreenToClient(hwnd, (LPPOINT)lprcLabel);
						ScreenToClient(hwnd, ((LPPOINT)lprcLabel)+1);

						int nAdjustment = (rcLabel.left - rcIcon.right) / 2;

						rcIcon.left += nAdjustment;
						rcIcon.right += nAdjustment;
					}

					RECT rcIntersect;

					if (IntersectRect(&rcIntersect, &rcIcon, &(ps.rcPaint))) {
						if (_this->m_hicon == NULL) {
							_this->m_hicon = (HICON)LoadImage(hInstResDLL, MAKEINTRESOURCE(IDI_WINVNC), IMAGE_ICON,
							   48, 48, 0);
						}
						if (_this->m_hicon) {
							HBRUSH hbr = (HBRUSH)SendMessage(hwnd, WM_CTLCOLORDLG, (WPARAM)hdc, (WPARAM)hwnd);
							DrawIconEx(hdc, rcIcon.left, rcIcon.top, _this->m_hicon, 48, 48, 0, hbr, DI_NORMAL);
						}
					}
				}

				EndPaint(hwnd, &ps);
			}

			return 0;
		}

		// Dialog has just been created
	case WM_INITDIALOG:
		{
			// Save the lParam into our user data so that subsequent calls have
			// access to the parent C++ object
            helper::SafeSetWindowUserData(hwnd, lParam);

            vncConnDialog *_this = (vncConnDialog *) lParam;

			//adzm 2009-06-20
			if (g_szRepeaterHost) {
				SetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, g_szRepeaterHost);

				//adzm 2009-06-20
				if (SPECIAL_SC_PROMPT) {
					HWND hwndChild = GetDlgItem(hwnd, IDC_HOSTNAME_EDIT);
					if (hwndChild) {
						ShowWindow(hwndChild, SW_HIDE);
					}
					hwndChild = GetDlgItem(hwnd, IDC_HOSTNAME_STATIC);
					if (hwndChild) {
						ShowWindow(hwndChild, SW_HIDE);
					}

					HWND hwndEdit = GetDlgItem(hwnd, IDC_IDCODE);
					HWND hwndLabel = GetDlgItem(hwnd, IDC_CONNECTION_NUMBER_STATIC);

					RECT rcEdit;
					RECT rcLabel;
					GetWindowRect(hwndEdit, &rcEdit);
					GetWindowRect(hwndLabel, &rcLabel);

					LPRECT lprcEdit = &rcEdit;
					LPRECT lprcLabel = &rcLabel;

					ScreenToClient(hwnd, (LPPOINT)lprcEdit);
					ScreenToClient(hwnd, ((LPPOINT)lprcEdit)+1);
					
					ScreenToClient(hwnd, (LPPOINT)lprcLabel);
					ScreenToClient(hwnd, ((LPPOINT)lprcLabel)+1);

					RECT rcClient;
					GetClientRect(hwnd, &rcClient);

					long nTotalHeight = rcEdit.bottom - rcLabel.top;

					long nAdjustedTop = (rcClient.bottom - nTotalHeight) / 2;

					long nAdjustment = nAdjustedTop - rcLabel.top;

					MoveWindow(hwndLabel, rcLabel.left, rcLabel.top + nAdjustment, rcLabel.right - rcLabel.left, rcLabel.bottom - rcLabel.top, TRUE);
					MoveWindow(hwndEdit, rcEdit.left, rcEdit.top + nAdjustment, rcEdit.right - rcEdit.left, rcEdit.bottom - rcEdit.top, TRUE);

					HWND hwndCaption = GetDlgItem(hwnd, IDC_CAPTION_STATIC);
					HFONT hFont = (HFONT)SendMessage(hwndCaption, WM_GETFONT, 0, 0);
					if (hFont) {
						LOGFONT lf;
						if (GetObject(hFont, sizeof(LOGFONT), &lf)) {
							lf.lfWidth = 0;
							lf.lfHeight = (lf.lfHeight * 6) / 4;

							_this->m_hfont = CreateFontIndirect(&lf);

							if (_this->m_hfont) {
								SendMessage(hwndCaption, WM_SETFONT, (WPARAM)_this->m_hfont, (LPARAM)TRUE);
							}
						}
					}
					SetWindowText(hwndCaption, "Connect to Technical Support");
					ShowWindow(hwndCaption, SW_SHOWNA);
				}

				SetFocus(GetDlgItem(hwnd, IDC_IDCODE));
			} else {            
				// Make the text entry box active
				SetFocus(GetDlgItem(hwnd, IDC_HOSTNAME_EDIT));
			}
			
			SetForegroundWindow(hwnd);
			
            // Return success!
			return TRUE;
		}

		// Dialog has just received a command
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			// User clicked OK or pressed return
		case IDOK:
		{
			// sf@2002 - host:num & host::num analyse.
			// Compatible with both RealVNC and TightVNC methods
			char hostname[_MAX_PATH];
			char actualhostname[_MAX_PATH];
			char idcode[_MAX_PATH];
			char *portp;
			int port;
			bool id;

			// Get the hostname of the VNCviewer
			GetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, hostname, _MAX_PATH);
			GetDlgItemText(hwnd, IDC_IDCODE, idcode, _MAX_PATH);
			if (strcmp(idcode,"")==0) id=false;
			else id=true;

			strcpy(actualhostname, hostname);
			
			//adzm 2010-02-15 - Multiple repeaters chosen by modulo of ID

			char finalidcode[_MAX_PATH];
			//adzm 2010-08 - this was sending uninitialized data over the wire
			ZeroMemory(finalidcode, sizeof(finalidcode));

			//adzm 2009-06-20
			if (id) {
				size_t i = 0;

				for (i = 0; i < strlen(idcode); i++)
				{
					finalidcode[i] = toupper(idcode[i]);
				} 
				finalidcode[i] = 0;

				if (0 != strncmp("ID:", idcode, 3)) {
					strcpy(finalidcode, "ID:");
					
					for (i = 0; i < strlen(idcode); i++)
					{
						finalidcode[i+3] = toupper(idcode[i]);
					} 
					finalidcode[i+3] = 0;
				}

				
				//adzm 2010-02-15 - At this point, finalidcode is of the form "ID:#####"
				int numericId = atoi(finalidcode + 3);

				int numberOfHosts = 1;
				for (i = 0; i < strlen(hostname); i++) {
					if (hostname[i] == ';') {
						numberOfHosts++;
					}
				}

				if (numberOfHosts <= 1) {
					// then hostname == actualhostname
				} else {
					int modulo = numericId % numberOfHosts;

					char* szToken = strtok(hostname, ";");
					while (szToken) {
						if (modulo == 0) {
							strcpy(actualhostname, szToken);
							break;
						}

						modulo--;
						szToken = strtok(NULL, ";");
					}
				}
			}

			// Calculate the Display and Port offset.
			port = INCOMING_PORT_OFFSET;
			portp = strchr(actualhostname, ':');
			if (portp)
			{
				*portp++ = '\0';
				if (*portp == ':') // Tight127 method
				{
					port = atoi(++portp);		// Port number after "::"
				}
				else // RealVNC method
				{
					if (atoi(portp) < 100)		// If < 100 after ":" -> display number
						port += atoi(portp);
					else
						port = atoi(portp);	    // If > 100 after ":" -> Port number
				}
			}


			
			// Attempt to create a new socket
			VSocket *tmpsock;
			tmpsock = new VSocket;
			if (!tmpsock) {
				return TRUE;
			}
			
			// Connect out to the specified host on the VNCviewer listen port
			// To be really good, we should allow a display number here but
			// for now we'll just assume we're connecting to display zero
			tmpsock->Create();
			if (tmpsock->Connect(actualhostname, port))
			{
				if (id) 
					{							
						

						tmpsock->Send(finalidcode,250);
						tmpsock->SetTimeout(0);
/*						if (strncmp(hostname,"ID",2)!=0)
						{
						while (true)
						{
							char result[1];
							tmpsock->Read(result,1);
							if (strcmp(result,"2")==0)
								break;
							tmpsock->Send("1",1);
						}
						}*/
						
						// adzm 2009-07-05 - repeater IDs
						// Add the new client to this server
						// adzm 2009-08-02
						_this->m_server->AddClient(tmpsock, TRUE, TRUE, 0, NULL, finalidcode, actualhostname, port);
					} else {
						// Add the new client to this server
						// adzm 2009-08-02
						_this->m_server->AddClient(tmpsock, TRUE, TRUE, 0, NULL, NULL, actualhostname, port);				
					}
				// And close the dialog
				EndDialog(hwnd, TRUE);
			}
			else
			{
				// Print up an error message
				MessageBox(NULL, 
					sz_ID_FAILED_CONNECT_LISTING_VIEW,
					sz_ID_OUTGOING_CONNECTION,
					MB_OK | MB_ICONEXCLAMATION );
				delete tmpsock;
			}
			return TRUE;
		}
		
			// Cancel the dialog
		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;
		};

		break;

	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		if (_this->m_hicon != NULL) {
			DestroyIcon(_this->m_hicon);
			_this->m_hicon = NULL;
		}
		if (_this->m_hfont != NULL) {
			DeleteObject(_this->m_hfont);
			_this->m_hfont = NULL;
		}
		return TRUE;
	}
	return 0;
}

