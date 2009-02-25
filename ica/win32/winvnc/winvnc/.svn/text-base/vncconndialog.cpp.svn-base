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
#include "vncConnDialog.h"
#include "WinVNC.h"

#include "resource.h"
#include "common/win32_helpers.h"


#include "localization.h" // ACT : Add localization on messages

//	[v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

// Constructor

vncConnDialog::vncConnDialog(vncServer *server)
{
	m_server = server;
}

// Destructor

vncConnDialog::~vncConnDialog()
{
}

// Routine called to activate the dialog and, once it's done, delete it

void vncConnDialog::DoDialog()
{
	//	[v1.0.2-jp1 fix]
	//DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_OUTGOING_CONN), 
	DialogBoxParam(hInstResDLL, MAKEINTRESOURCE(IDD_OUTGOING_CONN), 
		NULL, (DLGPROC) vncConnDlgProc, (LONG) this);
	delete this;
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

		// Dialog has just been created
	case WM_INITDIALOG:
		{
			// Save the lParam into our user data so that subsequent calls have
			// access to the parent C++ object
            helper::SafeSetWindowUserData(hwnd, lParam);

            vncConnDialog *_this = (vncConnDialog *) lParam;
            
			// Make the text entry box active
			SetFocus(GetDlgItem(hwnd, IDC_HOSTNAME_EDIT));
			
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
			char idcode[_MAX_PATH];
			char *portp;
			int port;
			bool id;

			// Get the hostname of the VNCviewer
			GetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, hostname, _MAX_PATH);
			GetDlgItemText(hwnd, IDC_IDCODE, idcode, _MAX_PATH);
			if (strcmp(idcode,"")==NULL) id=false;
			else id=true;

			// Calculate the Display and Port offset.
			port = INCOMING_PORT_OFFSET;
			portp = strchr(hostname, ':');
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
			if (!tmpsock)
				return TRUE;
			
			// Connect out to the specified host on the VNCviewer listen port
			// To be really good, we should allow a display number here but
			// for now we'll just assume we're connecting to display zero
			tmpsock->Create();
			if (tmpsock->Connect(hostname, port))
			{
				if (id) 
					{	
						for (size_t i = 0; i < strlen(idcode); i++)
						{
							idcode[i] = toupper(idcode[i]);
						} 
						tmpsock->Send(idcode,250);
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
					}
				// Add the new client to this server
				_this->m_server->AddClient(tmpsock, TRUE, TRUE);
				
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
		return TRUE;
	}
	return 0;
}

