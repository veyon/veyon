/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2010 Ultr@VNC Team Members. All Rights Reserved.
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
//
////////////////////////////////////////////////////////////////////////////

#include "vncdesktopthread.h"
#include "Localization.h"
#include "vnctimedmsgbox.h"
namespace
{
    int g_Oldcounter=0;
}
bool stop_hookwatch=false;

inline bool
ClipRect(int *x, int *y, int *w, int *h,
	    int cx, int cy, int cw, int ch) {
  if (*x < cx) {
    *w -= (cx-*x);
    *x = cx;
  }
  if (*y < cy) {
    *h -= (cy-*y);
    *y = cy;
  }
  if (*x+*w > cx+cw) {
    *w = (cx+cw)-*x;
  }
  if (*y+*h > cy+ch) {
    *h = (cy+ch)-*y;
  }
  return (*w>0) && (*h>0);
}

DWORD WINAPI hookwatch(LPVOID lpParam)
{
	vncDesktopThread *dt = (vncDesktopThread *)lpParam;

	while (!stop_hookwatch)
	{
		if (dt->g_obIPC.listall()->counter!=g_Oldcounter)
		{
			dt->m_desktop->TriggerUpdate();
			Sleep(15);
		}
		Sleep(5);
	}

	return 0;
}

bool
vncDesktopThread::Handle_Ringbuffer(mystruct *ringbuffer,rfb::Region2D &rgncache)
{
	bool returnvalue=false;
	//vnclog.Print(LL_INTERR, VNCLOG("counter,g_Oldcounter %i %i  \n"),ringbuffer->counter,g_Oldcounter);
	if (ringbuffer->counter==g_Oldcounter) return 0;
	int counter=ringbuffer->counter;
	if (counter<1 || counter>1999) return 0;

	if (g_Oldcounter<counter)
	{
		for (int i =g_Oldcounter+1; i<=counter;i++)
		{
			if (ringbuffer->type[i]==0)
			{
				rfb::Rect rect;
				rect.tl = rfb::Point(ringbuffer->rect1[i].left, ringbuffer->rect1[i].top);
				rect.br = rfb::Point(ringbuffer->rect1[i].right, ringbuffer->rect1[i].bottom);
				//Buffer coordinates
				//rect.tl.x-=(m_desktop->m_SWOffsetx);
				//rect.br.x-=(m_desktop->m_SWOffsetx);
				//rect.tl.y-=(m_desktop->m_SWOffsety);
				//rect.br.y-=(m_desktop->m_SWOffsety);
				vnclog.Print(LL_INTERR, VNCLOG("REct %i %i %i %i  \n"),rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
	#ifdef _DEBUG
					char			szText[256];
					DWORD error=GetLastError();
					sprintf(szText,"REct1 %i %i %i %i  \n",rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
					SetLastError(0);
					OutputDebugString(szText);		
	#endif			
				rect = rect.intersect(m_desktop->m_Cliprect);
				if (!rect.is_empty())
				{
#ifdef _DEBUG
					char			szText[256];
					DWORD error=GetLastError();
					sprintf(szText,"REctXXXXXXXXXXX %i %i %i %i  \n",rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
					SetLastError(0);
					OutputDebugString(szText);		
	#endif
					returnvalue=true;
					rgncache.assign_union(rect);
				}
			}
			if (ringbuffer->type[i]==2 || ringbuffer->type[i]==4)
			{
				/*if (MyGetCursorInfo)
					{
						MyCURSORINFO cinfo;
						cinfo.cbSize=sizeof(MyCURSORINFO);
						MyGetCursorInfo(&cinfo);
						m_desktop->SetCursor(cinfo.hCursor);
						//vnclog.Print(LL_INTERR, VNCLOG("Cursor %i  \n"),cinfo.hCursor);
					}*/
	
				
			}
			
		}

	}
	else
	{
		int  i = 0;
		for (i =g_Oldcounter+1;i<2000;i++)
		{
			if (ringbuffer->type[i]==0 )
			{
				rfb::Rect rect;
				rect.tl = rfb::Point(ringbuffer->rect1[i].left, ringbuffer->rect1[i].top);
				rect.br = rfb::Point(ringbuffer->rect1[i].right, ringbuffer->rect1[i].bottom);
				//Buffer coordinates
				//rect.tl.x-=m_desktop->m_ScreenOffsetx;
				//rect.br.x-=m_desktop->m_ScreenOffsetx;
				//rect.tl.y-=m_desktop->m_ScreenOffsety;
				//rect.br.y-=m_desktop->m_ScreenOffsety;
				//vnclog.Print(LL_INTERR, VNCLOG("REct %i %i %i %i  \n"),rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
#ifdef _DEBUG
					char			szText[256];
					DWORD error=GetLastError();
					sprintf(szText,"REct2 %i %i %i %i  \n",rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
					SetLastError(0);
					OutputDebugString(szText);		
	#endif				
				rect = rect.intersect(m_desktop->m_Cliprect);
				if (!rect.is_empty())
				{
					returnvalue=true;
					rgncache.assign_union(rect);
				}
			}
			if (ringbuffer->type[i]==2 || ringbuffer->type[i]==4)
			{
				/*if (MyGetCursorInfo)
					{
						MyCURSORINFO cinfo;
						cinfo.cbSize=sizeof(MyCURSORINFO);
						MyGetCursorInfo(&cinfo);
						m_desktop->SetCursor(cinfo.hCursor);
						//vnclog.Print(LL_INTERR, VNCLOG("Cursor %i  \n"),cinfo.hCursor);
					}*/
				
			}
			
		}
		for (i=1;i<=counter;i++)
		{
			if (ringbuffer->type[i]==0 )
			{
				rfb::Rect rect;
				rect.tl = rfb::Point(ringbuffer->rect1[i].left, ringbuffer->rect1[i].top);
				rect.br = rfb::Point(ringbuffer->rect1[i].right, ringbuffer->rect1[i].bottom);
				//Buffer coordinates
				//rect.tl.x-=m_desktop->m_ScreenOffsetx;
				//rect.br.x-=m_desktop->m_ScreenOffsetx;
				//rect.tl.y-=m_desktop->m_ScreenOffsety;
				//rect.br.y-=m_desktop->m_ScreenOffsety;
				//vnclog.Print(LL_INTERR, VNCLOG("REct %i %i %i %i  \n"),rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
				
				rect = rect.intersect(m_desktop->m_Cliprect);
				if (!rect.is_empty())
				{
					returnvalue=true;
					rgncache.assign_union(rect);
				}
			}
			if (ringbuffer->type[i]==2 || ringbuffer->type[i]==4)
			{
				/*if (MyGetCursorInfo)
					{
						MyCURSORINFO cinfo;
						cinfo.cbSize=sizeof(MyCURSORINFO);
						MyGetCursorInfo(&cinfo);
						m_desktop->SetCursor(cinfo.hCursor);
						//vnclog.Print(LL_INTERR, VNCLOG("Cursor %i  \n"),cinfo.hCursor);
					}*/
				
			}
			
		}
	}
	g_Oldcounter=counter;
	return returnvalue;
}
void Enable_softwareCAD_elevated();
void delete_softwareCAD_elevated();
DWORD GetExplorerLogonPid();
bool IsSoftwareCadEnabled();
bool ISUACENabled();
extern HANDLE hShutdownEventcad;


DWORD WINAPI Cadthread(LPVOID lpParam)
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);


	HDESK desktop=NULL;
	desktop = OpenInputDesktop(0, FALSE,
								DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
								DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
								DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
								DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
								);


	
	if (desktop == NULL)
		vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop Error \n"));
	else 
		vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop OK\n"));

	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy;

	char new_name[256];
	if (desktop)
	{
	if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
	{
		vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation \n"));
	}

	vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, desktop, old_desktop);

	if (!SetThreadDesktop(desktop))
	{
		vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK:!SetThreadDesktop \n"));
	}
	}

	//////
	if(OSversion.dwMajorVersion>=6 && vncService::RunningAsService())
			{
				/*if (OSversion.dwMinorVersion==0) //Vista
				{
					if (ISUACENabled() && !IsSoftwareCadEnabled())//ok
					{
						
					}
					if (!ISUACENabled() && IsSoftwareCadEnabled())
					{
						
					}
					if (!ISUACENabled() && !IsSoftwareCadEnabled())
					{
						DWORD result=MessageBox(NULL,"UAC is Disable, make registry changes to allow cad","Warning",MB_YESNO);
						if (result==IDYES)
						{
							HANDLE hProcess,hPToken;
							DWORD id=GetExplorerLogonPid();
							if (id!=0) 
								{
									hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,id);
									if(!OpenProcessToken(hProcess,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
													|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
													|TOKEN_READ|TOKEN_WRITE,&hPToken)) return 0;

									char dir[MAX_PATH];
									char exe_file_name[MAX_PATH];
									GetModuleFileName(0, exe_file_name, MAX_PATH);
									strcpy(dir, exe_file_name);
									strcat(dir, " -softwarecadhelper");
		
							
									STARTUPINFO          StartUPInfo;
									PROCESS_INFORMATION  ProcessInfo;
									HANDLE Token=NULL;
									HANDLE process=NULL;
									ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
									ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));
									StartUPInfo.wShowWindow = SW_SHOW;
									StartUPInfo.lpDesktop = "Winsta0\\Default";
									StartUPInfo.cb = sizeof(STARTUPINFO);
				
									CreateProcessAsUser(hPToken,NULL,dir,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&StartUPInfo,&ProcessInfo);
									DWORD errorcode=GetLastError();
									if (process) CloseHandle(process);
									if (Token) CloseHandle(Token);
									if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
									if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
									if (errorcode==1314)
									{
										Enable_softwareCAD_elevated();
									}
								}
						}
					}
					if (ISUACENabled() && IsSoftwareCadEnabled())
					{
						DWORD result=MessageBox(NULL,"UAC is Enablde, make registry changes to allow cad","Warning",MB_YESNO);
						if (result==IDYES)
						{
							HANDLE hProcess,hPToken;
							DWORD id=GetExplorerLogonPid();
							if (id!=0) 
								{
									hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,id);
									if(!OpenProcessToken(hProcess,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
													|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
													|TOKEN_READ|TOKEN_WRITE,&hPToken)) return 0;

									char dir[MAX_PATH];
									char exe_file_name[MAX_PATH];
									GetModuleFileName(0, exe_file_name, MAX_PATH);
									strcpy(dir, exe_file_name);
									strcat(dir, " -delsoftwarecadhelper");
			
							
									STARTUPINFO          StartUPInfo;
									PROCESS_INFORMATION  ProcessInfo;
									HANDLE Token=NULL;
									HANDLE process=NULL;
									ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
									ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));
									StartUPInfo.wShowWindow = SW_SHOW;
									StartUPInfo.lpDesktop = "Winsta0\\Default";
									StartUPInfo.cb = sizeof(STARTUPINFO);
			
									CreateProcessAsUser(hPToken,NULL,dir,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&StartUPInfo,&ProcessInfo);
									DWORD errorcode=GetLastError();
									if (process) CloseHandle(process);
									if (Token) CloseHandle(Token);
									if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
									if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
									if (errorcode==1314)
										{
											delete_softwareCAD_elevated();
										}							
								}
						}
					}

				}
				else*/
					if( vncService::RunningAsService() &&!IsSoftwareCadEnabled())
					{
						DWORD result=MessageBox(NULL,"UAC is Disable, make registry changes to allow cad","Warning",MB_YESNO);
						if (result==IDYES)
							{
								HANDLE hProcess,hPToken;
								DWORD id=GetExplorerLogonPid();
								if (id!=0) 
									{						
									hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,id);
									if(!OpenProcessToken(hProcess,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
													|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
													|TOKEN_READ|TOKEN_WRITE,&hPToken)) return 0;

									char dir[MAX_PATH];
									char exe_file_name[MAX_PATH];
									GetModuleFileName(0, exe_file_name, MAX_PATH);
									strcpy(dir, exe_file_name);
									strcat(dir, " -softwarecadhelper");
		
							
									STARTUPINFO          StartUPInfo;
									PROCESS_INFORMATION  ProcessInfo;
									HANDLE Token=NULL;
									HANDLE process=NULL;
									ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
									ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));
									StartUPInfo.wShowWindow = SW_SHOW;
									StartUPInfo.lpDesktop = "Winsta0\\Default";
									StartUPInfo.cb = sizeof(STARTUPINFO);
			
									CreateProcessAsUser(hPToken,NULL,dir,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&StartUPInfo,&ProcessInfo);
									DWORD errorcode=GetLastError();
									if (process) CloseHandle(process);
									if (Token) CloseHandle(Token);
									if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
									if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
									if (errorcode==1314)
										{
											Enable_softwareCAD_elevated();
										}							
									}
							}
					
					}
			}
       /////////////////////
	if(OSversion.dwMajorVersion==6)//&& OSversion.dwMinorVersion>=1) //win7  // test win7 +Vista
	{

		if (hShutdownEventcad==NULL ) hShutdownEventcad = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\SessionEventUltraCad");
		if (hShutdownEventcad!=NULL ) SetEvent(hShutdownEventcad);
		if (old_desktop) SetThreadDesktop(old_desktop);
		if (desktop) CloseDesktop(desktop);
		return 0;
	}

	 HKEY hKey; 
	 DWORD isLUAon = 0; 
     if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) 
              { 
              DWORD LUAbufSize = 4; 
              RegQueryValueEx(hKey, TEXT("EnableLUA"), NULL, NULL, (LPBYTE)&isLUAon, &LUAbufSize); 
              RegCloseKey(hKey); 
			  }
     if (isLUAon != 1 && OSversion.dwMajorVersion==6) 
	 {
		 if (hShutdownEventcad==NULL ) hShutdownEventcad = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\SessionEventUltraCad");
		 if (hShutdownEventcad!=NULL ) SetEvent(hShutdownEventcad);
		 if (old_desktop) SetThreadDesktop(old_desktop);
		 if (desktop) CloseDesktop(desktop);
		 return 0;
	 }

//Full path needed, sometimes it just default to system32
	char WORKDIR[MAX_PATH];
	char mycommand[MAX_PATH];
	if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
		{
		char* p = strrchr(WORKDIR, '\\');
		if (p == NULL) return 0;
		*p = '\0';
		}
	strcpy(mycommand,"");
	strcat(mycommand,WORKDIR);//set the directory
	strcat(mycommand,"\\");
	strcat(mycommand,"cad.exe");

	int nr=(LONG_PTR)ShellExecute(GetDesktopWindow(), "open", mycommand, "", 0, SW_SHOWNORMAL);
	if (nr<=32)
	{
		//error
		//
		if ( nr==SE_ERR_ACCESSDENIED )
			vncTimedMsgBox::Do(
									sz_ID_CADPERMISSION,
									sz_ID_ULTRAVNC_WARNING,
									MB_ICONINFORMATION | MB_OK
									);

		if ( nr==ERROR_PATH_NOT_FOUND || nr==ERROR_FILE_NOT_FOUND)
			vncTimedMsgBox::Do(
									sz_ID_CADERRORFILE,
									sz_ID_ULTRAVNC_WARNING,
									MB_ICONINFORMATION | MB_OK
									);

	}

	if (old_desktop) SetThreadDesktop(old_desktop);
	if (desktop) CloseDesktop(desktop);
	return 0;
}