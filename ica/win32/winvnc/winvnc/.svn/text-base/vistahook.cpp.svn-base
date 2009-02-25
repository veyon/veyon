
#include "vncDesktopThread.h"
#include "Localization.h"
#include "vncTimedMsgBox.h"
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
	vnclog.Print(LL_INTERR, VNCLOG("counter,g_Oldcounter %i %i  \n"),ringbuffer->counter,g_Oldcounter);
	if (ringbuffer->counter==g_Oldcounter) return 0;
	int counter=ringbuffer->counter;
	if (counter<1 || counter>1999) return 0;

	if (g_Oldcounter<counter)
	{
		for (short i =g_Oldcounter+1; i<=counter;i++)
		{
			if (ringbuffer->type[i]==0)
			{
				rfb::Rect rect;
				rect.tl = rfb::Point(ringbuffer->rect1[i].left, ringbuffer->rect1[i].top);
				rect.br = rfb::Point(ringbuffer->rect1[i].right, ringbuffer->rect1[i].bottom);
				//Buffer coordinates
				rect.tl.x-=m_desktop->m_ScreenOffsetx;
				rect.br.x-=m_desktop->m_ScreenOffsetx;
				rect.tl.y-=m_desktop->m_ScreenOffsety;
				rect.br.y-=m_desktop->m_ScreenOffsety;
				//vnclog.Print(LL_INTERR, VNCLOG("REct %i %i %i %i  \n"),rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
				
				rect = rect.intersect(m_desktop->m_Cliprect);
				if (!rect.is_empty())
				{
					rgncache = rgncache.union_(rect);
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
		short i = 0;
		for (i =g_Oldcounter+1;i<2000;i++)
		{
			if (ringbuffer->type[i]==0 )
			{
				rfb::Rect rect;
				rect.tl = rfb::Point(ringbuffer->rect1[i].left, ringbuffer->rect1[i].top);
				rect.br = rfb::Point(ringbuffer->rect1[i].right, ringbuffer->rect1[i].bottom);
				//Buffer coordinates
				rect.tl.x-=m_desktop->m_ScreenOffsetx;
				rect.br.x-=m_desktop->m_ScreenOffsetx;
				rect.tl.y-=m_desktop->m_ScreenOffsety;
				rect.br.y-=m_desktop->m_ScreenOffsety;
				//vnclog.Print(LL_INTERR, VNCLOG("REct %i %i %i %i  \n"),rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
				
				rect = rect.intersect(m_desktop->m_Cliprect);
				if (!rect.is_empty())
				{
					rgncache = rgncache.union_(rect);
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
				rect.tl.x-=m_desktop->m_ScreenOffsetx;
				rect.br.x-=m_desktop->m_ScreenOffsetx;
				rect.tl.y-=m_desktop->m_ScreenOffsety;
				rect.br.y-=m_desktop->m_ScreenOffsety;
				//vnclog.Print(LL_INTERR, VNCLOG("REct %i %i %i %i  \n"),rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);
				
				rect = rect.intersect(m_desktop->m_Cliprect);
				if (!rect.is_empty())
				{
					rgncache = rgncache.union_(rect);
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
	return 1;
}

DWORD WINAPI Cadthread(LPVOID lpParam)
{
	HDESK desktop;
	//vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop \n"));
	//vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 NULL\n"));
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

	if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
	{
		vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation \n"));
	}

	vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, desktop, old_desktop);

	if (!SetThreadDesktop(desktop))
	{
		vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK:!SetThreadDesktop \n"));
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

	int nr=(int)ShellExecute(GetDesktopWindow(), "open", mycommand, "", 0, SW_SHOWNORMAL);
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

	CloseDesktop(desktop);
	return 0;
}