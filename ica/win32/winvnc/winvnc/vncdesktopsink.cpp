#include "winvnc.h"
#include "vncserver.h"
#include "vncdesktop.h"
#include "vncservice.h"
#include <string.h>

#define MSGFLT_ADD		1
typedef BOOL (WINAPI *CHANGEWINDOWMESSAGEFILTER)(UINT message, DWORD dwFlag);
int OSversion();
DWORD WINAPI Driverwatch(LPVOID lpParam);
DWORD WINAPI InitWindowThread(LPVOID lpParam);

void
vncDesktop::ShutdownInitWindowthread()
{
	// we keep the sink window running
	// but ignore info
	can_be_hooked=false;
	vnclog.Print(LL_INTINFO, VNCLOG("ShutdownInitWindowthread \n"));
}

void
vncDesktop::StopInitWindowthread()
{
	//vndesktopthread is closing, all threads need to be stopped
	//else winvnc wil stay running in background on exit
		can_be_hooked=true;
		if (InitWindowThreadh)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("~vncDesktop::Tell initwindowthread to close \n"));
			PostThreadMessage(pumpID, WM_QUIT, 0, 0);
			DWORD status=WaitForSingleObject(InitWindowThreadh,2000);
			if (status==WAIT_TIMEOUT)
			{
				vnclog.Print(LL_INTERR, VNCLOG("~vncDesktop::ERROR:  messageloop blocked \n"));
				// WE need to kill the thread to prevent a winvnc lock
				TerminateThread(InitWindowThreadh,0);
				CloseHandle(InitWindowThreadh);
				m_hwnd=NULL;
				InitWindowThreadh=NULL;
			}
			else
			{
				vnclog.Print(LL_INTERR, VNCLOG("~vncDesktop:: iniwindowthread proper closed \n"));
				CloseHandle(InitWindowThreadh);
				InitWindowThreadh=NULL;
			}
		}
		else
		{
			vnclog.Print(LL_INTINFO, VNCLOG("initwindowthread already closed \n"));
		}
}

void
vncDesktop::StartInitWindowthread()
{
	// Check if the input desktop == Default desktop
	// Hooking the winlogon is not needed, no clipboard
	// see if the threaddesktop== Default
	HDESK desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy;
	char new_name[256];
	can_be_hooked=false;
	vnclog.Print(LL_INTINFO, VNCLOG("StartInitWindowthread \n"));
	if (GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
	{
		if (strcmp(new_name,"Default")==NULL)
		{
			vnclog.Print(LL_INTINFO, VNCLOG("StartInitWindowthread default desk\n"));
			if (InitWindowThreadh==NULL)
			{
				ResetEvent(restart_event);
				InitWindowThreadh=CreateThread(NULL,0,InitWindowThread,this,0,&pumpID);
				DWORD status=WaitForSingleObject(restart_event,10000);
				if (status==WAIT_TIMEOUT)
				{
					vnclog.Print(LL_INTINFO, VNCLOG("ERROR: initwindowthread failed to start \n"));
					if (InitWindowThreadh!=NULL)
					{
						TerminateThread(InitWindowThreadh,0);
						CloseHandle(InitWindowThreadh);
						m_hwnd=NULL;
						InitWindowThreadh=NULL;
					}
					can_be_hooked=false;
				}
				else
				{
					vnclog.Print(LL_INTINFO, VNCLOG("StartInitWindowthread started\n"));
					can_be_hooked=true;
				}
			}
			else
			{
				// initwindowthread is still running
				// make it back active
				vnclog.Print(LL_INTINFO, VNCLOG("StartInitWindowthread reactivate\n"));
				can_be_hooked=true;
			}
		}
		else
		{
			vnclog.Print(LL_INTINFO, VNCLOG("StartInitWindowthread no default desk\n"));
		}
	}
}

DWORD WINAPI
InitWindowThread(LPVOID lpParam)
{
	vncDesktop *mydesk=(vncDesktop*)lpParam;
	mydesk->InitWindow();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Window procedure for the Desktop window
LRESULT CALLBACK
DesktopWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
#ifndef _X64
	vncDesktop *_this = (vncDesktop*)GetWindowLong(hwnd, GWL_USERDATA);
#else
	vncDesktop *_this = (vncDesktop*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#endif
	#ifdef _DEBUG
										char			szText[256];
										sprintf(szText,"Message %i\n",iMsg );
										OutputDebugString(szText);
										//vnclog.Print(LL_INTERR, VNCLOG("%i  \n"),iMsg);
			#endif
	switch (iMsg)
	{
	case WM_CREATE:
		vnclog.Print(LL_INTERR, VNCLOG("wmcreate  \n"));
		break;
	case WM_TIMER:
		if (_this->can_be_hooked)
		{
			if (wParam==100)
			{
					KillTimer(hwnd, 100); 
					if (_this->SetHook)
					{
						_this->SetHook(hwnd);
						vnclog.Print(LL_INTERR, VNCLOG("set SC hooks OK\n"));
						_this->m_hookinited = TRUE;
					}
					else if (_this->SetHooks)
					{
						if (!_this->SetHooks(
							GetCurrentThreadId(),
							RFB_SCREEN_UPDATE,
							RFB_COPYRECT_UPDATE,
							RFB_MOUSE_UPDATE, 0
							))
						{
							vnclog.Print(LL_INTERR, VNCLOG("failed to set system hooks\n"));
							// Switch on full screen polling, so they can see something, at least...
							_this->m_server->PollFullScreen(TRUE);
							_this->m_hookinited = FALSE;
						} 
						else 
						{
							vnclog.Print(LL_INTERR, VNCLOG("set hooks OK\n"));
							_this->m_hookinited = TRUE;
							// Start up the keyboard and mouse filters
							if (_this->SetKeyboardFilterHook) _this->SetKeyboardFilterHook(_this->m_server->LocalInputsDisabled());
							if (_this->SetMouseFilterHook) _this->SetMouseFilterHook(_this->m_server->LocalInputsDisabled());
						}
					}
			}
			else SetEvent(_this->trigger_events[0]);
		}
		break;
	case WM_MOUSESHAPE:
		if (_this->can_be_hooked)
		{
			SetEvent(_this->trigger_events[3]);
		}
		break;
	case WM_HOOKCHANGE:
		if (wParam==1)
			{
				if (_this->m_hookinited==FALSE)
				SetTimer(hwnd,100,4000,NULL);
			}
			else if (_this->m_hookinited)
			{
				_this->m_hookinited=FALSE;
				if (_this->UnSetHook)
				{
					vnclog.Print(LL_INTERR, VNCLOG("unset SC hooks OK\n"));
					_this->UnSetHook(hwnd);
				}
				else if (_this->UnSetHooks)
				{
				if(!_this->UnSetHooks(GetCurrentThreadId()) )
					vnclog.Print(LL_INTERR, VNCLOG("Unsethooks Failed\n"));
				else vnclog.Print(LL_INTERR, VNCLOG("Unsethooks OK\n"));
				}
				
			}
		return true;

	case WM_QUERYENDSESSION:

		/*if (OSversion()==2) 
		{
		if (_this->m_hnextviewer!=NULL) ChangeClipboardChain(hwnd, _this->m_hnextviewer);
		_this->m_hnextviewer=NULL;
		if (_this->m_hookinited)
			{
				_this->m_hookinited=FALSE;
				if (_this->UnSetHook)
				{
					vnclog.Print(LL_INTERR, VNCLOG("unset SC hooks OK\n"));
					_this->UnSetHook(hwnd);
				}
				else if (_this->UnSetHooks)
				{
				if(!_this->UnSetHooks(GetCurrentThreadId()) )
					vnclog.Print(LL_INTERR, VNCLOG("Unsethooks Failed\n"));
				else vnclog.Print(LL_INTERR, VNCLOG("Unsethooks OK\n"));
				}
				
			}
		vnclog.Print(LL_INTERR, VNCLOG("WM_QUERYENDSESSION\n"));
		PostQuitMessage(0);
		SetEvent(_this->trigger_events[5]);
		}*/
		return DefWindowProc(hwnd, iMsg, wParam, lParam);

	case WM_CLOSE:
		if (_this->m_hnextviewer!=NULL) ChangeClipboardChain(hwnd, _this->m_hnextviewer);
		_this->m_hnextviewer=NULL;
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:		
		KillTimer(hwnd, 100); 
		if (_this->m_hnextviewer!=NULL) ChangeClipboardChain(hwnd, _this->m_hnextviewer);
		_this->m_hnextviewer=NULL;
		if (_this->m_hookinited)
			{
				_this->m_hookinited=FALSE;
				if (_this->UnSetHook)
				{
					vnclog.Print(LL_INTERR, VNCLOG("unset SC hooks OK\n"));
					_this->UnSetHook(hwnd);
				}
				else if (_this->UnSetHooks)
				{
				if(!_this->UnSetHooks(GetCurrentThreadId()) )
					vnclog.Print(LL_INTERR, VNCLOG("Unsethooks Failed\n"));
				else vnclog.Print(LL_INTERR, VNCLOG("Unsethooks OK\n"));
				}
				
			}
		vnclog.Print(LL_INTERR, VNCLOG("WM_DESTROY\n"));
		break;
	///ddihook
	case WM_SYSCOMMAND:
		// User has clicked an item on the tray menu
		switch (wParam)
		{
			case SC_MONITORPOWER:
				vnclog.Print(LL_INTINFO, VNCLOG("Monitor22 %i\n"),lParam);
		}
		vnclog.Print(LL_INTINFO, VNCLOG("Monitor3 %i %i\n"),wParam,lParam);
		return DefWindowProc(hwnd, iMsg, wParam, lParam);
	case WM_POWER:
	case WM_POWERBROADCAST:
		// User has clicked an item on the tray menu
		switch (wParam)
		{
			case SC_MONITORPOWER:
				vnclog.Print(LL_INTINFO, VNCLOG("Monitor222 %i\n"),lParam);
		}
		vnclog.Print(LL_INTINFO, VNCLOG("Power3 %i %i\n"),wParam,lParam);
		return DefWindowProc(hwnd, iMsg, wParam, lParam);

	case WM_COPYDATA:
        {
			PCOPYDATASTRUCT pMyCDS = (PCOPYDATASTRUCT) lParam;
			if (pMyCDS->dwData==112233)
			{
					DWORD mysize=pMyCDS->cbData;
					char mytext[1024];
					char *myptr;
					char split[4][6];
					strcpy(mytext,(LPCSTR)pMyCDS->lpData);
					myptr=mytext;
					for (DWORD j =0; j<(mysize/20);j++)
					{
						for (int i=0;i<4;i++)
							{
								strcpy(split[i],"     ");
								strncpy(split[i],myptr,4);
								myptr=myptr+5;
							}
						_this->QueueRect(rfb::Rect(atoi(split[0]), atoi(split[1]), atoi(split[2]), atoi(split[3])));
					}
					
			}
			//vnclog.Print(LL_INTINFO, VNCLOG("copydata\n"));	
        }
			return 0;

	// GENERAL

	case WM_DISPLAYCHANGE:
		// The display resolution is changing
		// We must kick off any clients since their screen size will be wrong
		// WE change the clients screensize, if they support it.
		vnclog.Print(LL_INTERR, VNCLOG("WM_DISPLAYCHANGE\n"));
		// We First check if the Resolution changed is caused by a temp resolution switch
		// For a temp resolution we don't use the driver, to fix the mirror driver
		// to the new change, a resolution switch is needed, preventing screensaver locking.

		if (_this->m_videodriver != NULL) //Video driver active
		{
			if (!_this->m_videodriver->blocked)
			{
				_this->m_displaychanged = TRUE;
				_this->m_hookdriver=true;
				_this->m_videodriver->blocked=true;
				vnclog.Print(LL_INTERR, VNCLOG("Resolution switch detected, driver active\n"));	
			}
			else
			{
				//Remove display change, cause by driver activation
				_this->m_videodriver->blocked=false;
				vnclog.Print(LL_INTERR, VNCLOG("Resolution switch by driver activation removed\n"));
			}
		}
		else 
		{
				_this->m_displaychanged = TRUE;
				_this->m_hookdriver=true;
				vnclog.Print(LL_INTERR, VNCLOG("Resolution switch detected, driver NOT active\n"));
			
		}
		return 0;

	case WM_SYSCOLORCHANGE:
	case WM_PALETTECHANGED:
		if (!_this->m_displaychanged)
		{
		// The palette colours have changed, so tell the server

		// Get the system palette
            // better to use the wrong colors than close the connection
		_this->SetPalette();

		// Update any palette-based clients, too
		_this->m_server->UpdatePalette(true);
		}
		return 0;

		// CLIPBOARD MESSAGES

	case WM_CHANGECBCHAIN:
		// The clipboard chain has changed - check our nextviewer handle
		if ((HWND)wParam == _this->m_hnextviewer)
			_this->m_hnextviewer = (HWND)lParam;
		else
			if (_this->m_hnextviewer != NULL)
				SendMessage(_this->m_hnextviewer,
							WM_CHANGECBCHAIN,
							wParam, lParam);

		return 0;

	case WM_DRAWCLIPBOARD:
		if (_this->can_be_hooked)
		{
			// The clipboard contents have changed
			if((GetClipboardOwner() != _this->Window()) &&
				_this->m_initialClipBoardSeen &&
				_this->m_clipboard_active && !_this->m_server->IsThereFileTransBusy())
			{
				LPSTR cliptext = NULL;

				// Open the clipboard
				if (OpenClipboard(_this->Window()))
				{
					// Get the clipboard data
					HGLOBAL cliphandle = GetClipboardData(CF_TEXT);
					if (cliphandle != NULL)
					{
						LPSTR clipdata = (LPSTR) GlobalLock(cliphandle);

						// Copy it into a new buffer
						if (clipdata == NULL)
							cliptext = NULL;
						else
							cliptext = _strdup(clipdata);

						// Release the buffer and close the clipboard
						GlobalUnlock(cliphandle);
					}

					CloseClipboard();
				}

				if (cliptext != NULL)
				{
					int cliplen = strlen(cliptext);
					LPSTR unixtext = (char *)malloc(cliplen+1);

					// Replace CR-LF with LF - never send CR-LF on the wire,
					// since Unix won't like it
					int unixpos=0;
					for (int x=0; x<cliplen; x++)
					{
						if (cliptext[x] != '\x0d')
						{
							unixtext[unixpos] = cliptext[x];
							unixpos++;
						}
					}
					unixtext[unixpos] = 0;

					// Free the clip text
					free(cliptext);
					cliptext = NULL;

					// Now send the unix text to the server
					omni_mutex_lock l(_this->m_update_lock);
					_this->m_server->UpdateClipText(unixtext);

					free(unixtext);
				}
			}

			_this->m_initialClipBoardSeen = TRUE;

			if (_this->m_hnextviewer != NULL)
			{
				// Pass the message to the next window in clipboard viewer chain.  
				return SendMessage(_this->m_hnextviewer, WM_DRAWCLIPBOARD, 0,0); 
			}
		}

		return 0;

	default:
		return DefWindowProc(hwnd, iMsg, wParam, lParam);
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////

ATOM m_wndClass = 0;

BOOL
vncDesktop::InitWindow()
{
	vnclog.Print(LL_INTERR, VNCLOG("InitWindow called\n"));
	
	HDESK desktop;
	desktop = OpenInputDesktop(0, FALSE,
								DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
								DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
								DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
								DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
								);

	if (desktop == NULL)
		vnclog.Print(LL_INTERR, VNCLOG("InitWindow:OpenInputdesktop Error \n"));
	else 
		vnclog.Print(LL_INTERR, VNCLOG("InitWindow:OpenInputdesktop OK\n"));

	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy;

	char new_name[256];

	if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
	{
		vnclog.Print(LL_INTERR, VNCLOG("InitWindow:!GetUserObjectInformation \n"));
	}

	vnclog.Print(LL_INTERR, VNCLOG("InitWindow:SelectHDESK to %s (%x) from %x\n"), new_name, desktop, old_desktop);

	if (!SetThreadDesktop(desktop))
	{
		vnclog.Print(LL_INTERR, VNCLOG("InitWindow:SelectHDESK:!SetThreadDesktop \n"));
	}

	HMODULE  hUser32 = LoadLibrary("user32.dll");
	CHANGEWINDOWMESSAGEFILTER pfnFilter = NULL;
	pfnFilter =(CHANGEWINDOWMESSAGEFILTER)GetProcAddress(hUser32,"ChangeWindowMessageFilter");
	if (pfnFilter) pfnFilter(RFB_SCREEN_UPDATE, MSGFLT_ADD);
	if (pfnFilter) pfnFilter(RFB_COPYRECT_UPDATE, MSGFLT_ADD);
	if (pfnFilter) pfnFilter(RFB_MOUSE_UPDATE, MSGFLT_ADD);
	if (pfnFilter) pfnFilter(WM_QUIT, MSGFLT_ADD);
	if (pfnFilter) pfnFilter(WM_SHUTDOWN, MSGFLT_ADD);


	
	if (m_wndClass == 0) {
		// Create the window class
		WNDCLASSEX wndclass;

		wndclass.cbSize			= sizeof(wndclass);
		wndclass.style			= 0;
		wndclass.lpfnWndProc	= &DesktopWndProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hAppInstance;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= NULL;
		wndclass.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName	= (const char *) NULL;
		wndclass.lpszClassName	= szDesktopSink;
		wndclass.hIconSm		= NULL;

		// Register it
		m_wndClass = RegisterClassEx(&wndclass);
		if (!m_wndClass) {
			vnclog.Print(LL_INTERR, VNCLOG("failed to register window class\n"));
			SetEvent(restart_event);
			return FALSE;
		}
	}

	// And create a window
	m_hwnd = CreateWindow(szDesktopSink,
				"WinVNC",
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				400, 200,
				NULL,
				NULL,
				hAppInstance,
				NULL);

	if (m_hwnd == NULL) {
		vnclog.Print(LL_INTERR, VNCLOG("failed to create hook window\n"));
		SetEvent(restart_event);
		return FALSE;
	}

	// Set the "this" pointer for the window
    helper::SafeSetWindowUserData(m_hwnd, (long)this);

	// Enable clipboard hooking
	m_hnextviewer = SetClipboardViewer(m_hwnd);
	StopDriverWatches=false;
		DrvWatch mywatch;
		mywatch.stop=&StopDriverWatches;
		mywatch.hwnd=m_hwnd;
	if (VideoBuffer())
	{
		DWORD myword;
		HANDLE T1=NULL;
		T1=CreateThread(NULL,0,Driverwatch,m_hwnd,0,&myword);
		if (T1) CloseHandle(T1);
	}
	vnclog.Print(LL_INTERR, VNCLOG("OOOOOOOOOOOO load hookdll's\n"));
	////////////////////////
		hModule=NULL;
	char szCurrentDir[MAX_PATH];
		if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
		{
			char* p = strrchr(szCurrentDir, '\\');
			if (p == NULL) return 0;
			*p = '\0';
			strcat (szCurrentDir,"\\vnchooks.dll");
		}
	hSCModule=NULL;
	char szCurrentDirSC[MAX_PATH];
		if (GetModuleFileName(NULL, szCurrentDirSC, MAX_PATH))
		{
			char* p = strrchr(szCurrentDirSC, '\\');
			if (p == NULL) return 0;
			*p = '\0';
			strcat (szCurrentDirSC,"\\schook.dll");
		}

	UnSetHooks=NULL;
	SetMouseFilterHook=NULL;
	SetKeyboardFilterHook=NULL;
	SetHooks=NULL;

	UnSetHook=NULL;
	SetHook=NULL;

	hModule = LoadLibrary(szCurrentDir);
	hSCModule = LoadLibrary(szCurrentDirSC);//TOFIX resource leak
	if (hModule)
		{
			UnSetHooks = (UnSetHooksFn) GetProcAddress( hModule, "UnSetHooks" );
			SetMouseFilterHook  = (SetMouseFilterHookFn) GetProcAddress( hModule, "SetMouseFilterHook" );
			SetKeyboardFilterHook  = (SetKeyboardFilterHookFn) GetProcAddress( hModule, "SetKeyboardFilterHook" );
			SetHooks  = (SetHooksFn) GetProcAddress( hModule, "SetHooks" );
		}
	if (hSCModule)
		{
			UnSetHook = (UnSetHookFn) GetProcAddress( hSCModule, "UnSetHook" );
			SetHook  = (SetHookFn) GetProcAddress( hSCModule, "SetHook" );
		}
	///////////////////////////////////////////////
	vnclog.Print(LL_INTERR, VNCLOG("OOOOOOOOOOOO start dispatch\n"));
	MSG msg;
	SetEvent(restart_event);
	while (TRUE)
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
			vnclog.Print(LL_INTERR, VNCLOG("OOOOOOOOOOOO %i %i\n"),msg.message,msg.hwnd);
			if (msg.message==WM_QUIT || fShutdownOrdered)		
				{
					vnclog.Print(LL_INTERR, VNCLOG("OOOOOOOOOOOO called wm_quit\n"));
					DestroyWindow(m_hwnd);
					SetEvent(trigger_events[5]);
					break;
				}
			else if (msg.message==WM_SHUTDOWN)
				{
					vnclog.Print(LL_INTERR, VNCLOG("OOOOOOOOOOOO called wm_user+4\n"));
					DestroyWindow(m_hwnd);
					break;
				}
			else if (msg.message==RFB_SCREEN_UPDATE)
				{
					if (can_be_hooked)
					{
					vnclog.Print(LL_INTERR, VNCLOG("RFB_SCREEN_UPDATE  \n"));
					rfb::Rect rect;
					rect.tl = rfb::Point((SHORT)LOWORD(msg.wParam), (SHORT)HIWORD(msg.wParam));
					rect.br = rfb::Point((SHORT)LOWORD(msg.lParam), (SHORT)HIWORD(msg.lParam));
					//Buffer coordinates
					rect.tl.x-=m_ScreenOffsetx;
					rect.br.x-=m_ScreenOffsetx;
					rect.tl.y-=m_ScreenOffsety;
					rect.br.y-=m_ScreenOffsety;
					vnclog.Print(LL_INTERR, VNCLOG("REct %i %i %i %i  \n"),rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
				
					rect = rect.intersect(m_Cliprect);
					if (!rect.is_empty())
						{
							while (lock_region_add) Sleep(5);
							rgnpump.assign_union(rect);
							SetEvent(trigger_events[1]);
						}
					}
				}
			else if (msg.message==RFB_MOUSE_UPDATE)
				{
					if (can_be_hooked)
					{
					vnclog.Print(LL_INTERR, VNCLOG("RFB_MOUSE_UPDATE  \n"));
					SetCursor((HCURSOR) msg.wParam);
					SetEvent(trigger_events[2]);
					}
				}
			else
				{
					if (msg.message==WM_USER+3 )vnclog.Print(LL_INTERR, VNCLOG("OOOOOOOOOOOO called wm_user+3\n"));
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
		}
		else WaitMessage();
	}

	if (hModule)FreeLibrary(hModule);
	if (hSCModule)FreeLibrary(hSCModule);
	SetThreadDesktop(old_desktop);
    CloseDesktop(desktop);
	///////////////////////
	vnclog.Print(LL_INTERR, VNCLOG("OOOOOOOOOOOO end dispatch\n"));
	m_hwnd = NULL;
	return TRUE;
}