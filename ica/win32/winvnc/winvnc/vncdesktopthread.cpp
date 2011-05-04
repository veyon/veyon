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
#include "vncOSVersion.h"
bool g_DesktopThread_running;
bool g_update_triggered;
DWORD WINAPI hookwatch(LPVOID lpParam);
extern bool stop_hookwatch;
void testBench();


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

////////////////////////////////////////////////////////////////////////////////////
// Modif rdv@2002 - v1.1.x - videodriver
void
vncDesktopThread::copy_bitmaps_to_buffer(ULONG i,rfb::Region2D &rgncache,rfb::UpdateTracker &tracker)
{
	
		rfb::Rect rect;
		int x = m_desktop->pchanges_buf->pointrect[i].rect.left;
		int w = m_desktop->pchanges_buf->pointrect[i].rect.right-m_desktop->pchanges_buf->pointrect[i].rect.left;
		int y = m_desktop->pchanges_buf->pointrect[i].rect.top;
		int h = m_desktop->pchanges_buf->pointrect[i].rect.bottom-m_desktop->pchanges_buf->pointrect[i].rect.top;
		//vnclog.Print(LL_INTINFO, VNCLOG("Driver ************* %i %i %i %i \n"),x,y,w,h);

		if (!ClipRect(&x, &y, &w, &h, m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.tl.y,
			m_desktop->m_bmrect.br.x-m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.br.y-m_desktop->m_bmrect.tl.y)) return;
		//vnclog.Print(LL_INTINFO, VNCLOG("Driver ************* %i %i %i %i \n"),x,y,w,h);
		rect.tl.x = x;
		rect.br.x = x+w;
		rect.tl.y = y;
		rect.br.y = y+h;

		switch(m_desktop->pchanges_buf->pointrect[i].type)
			{
				case SCREEN_SCREEN:
					{
						int dx=m_desktop->pchanges_buf->pointrect[i].point.x;
						int dy=m_desktop->pchanges_buf->pointrect[i].point.y;
						if (!m_screen_moved && (dx==0 || dy==0) )
								{
//// Fix in case !Cliprect
									int xx=x;
                                    int yy=y;
                                    int hh=h;
                                    int ww=w;
                                    if (ClipRect(&xx,&yy,&ww,&hh,m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.tl.y,
                                                m_desktop->m_bmrect.br.x-m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.br.y-m_desktop->m_bmrect.tl.y))
									{
                                    rect.tl.x=xx;
                                    rect.tl.y=yy;
                                    rect.br.x=xx+ww;
                                    rect.br.y=yy+hh;
                                    rgncache.assign_union(rect);
									}

//////////////////////
// Fix Eckerd
									x=x+dx;;
									y=y+dy;;
									if (!ClipRect(&x,&y,&w,&h,m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.tl.y,
												m_desktop->m_bmrect.br.x-m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.br.y-m_desktop->m_bmrect.tl.y)) return;
//////////////////////
// Fix Eckerd
									rect.tl.x=x-dx;
									rect.tl.y=y-dy;
									rect.br.x=x+w-dx;
									rect.br.y=y+h-dy;

									rfb::Point delta = rfb::Point(-dx,-dy);
									rgncache.assign_union(rect);
									tracker.add_copied(rect, delta);
								//	vnclog.Print(LL_INTINFO, VNCLOG("Copyrect \n"));
								}
						else
								{
									rgncache.assign_union(rect);
								}
						break;
					}

				case SOLIDFILL:
				case TEXTOUT:
				case BLEND:
				case TRANS:
				case PLG:
				case BLIT:;
					rgncache.assign_union(rect);
					break;
				default:
					break;
			}
}



// Modif rdv@2002 - v1.1.x - videodriver
BOOL
vncDesktopThread::handle_driver_changes(rfb::Region2D &rgncache,rfb::UpdateTracker &tracker)
{ 

	omni_mutex_lock l(m_desktop->m_videodriver_lock);

	int oldaantal=m_desktop->m_videodriver->oldaantal;
	int counter=m_desktop->pchanges_buf->counter;
//	int nr_updates=m_desktop->pchanges_buf->pointrect[0].type;
//	vnclog.Print(LL_INTERR, VNCLOG("updates, rects %i\n"),oldaantal-counter);
	if (oldaantal==counter) return FALSE;
	if (counter<1 || counter >1999) return FALSE;
//	m_desktop->pchanges_buf->pointrect[0].type=0;
	if (!m_server->SingleWindow()) m_screen_moved=m_desktop->CalcCopyRects(tracker);
	else m_screen_moved=true;

/// HEITE01E
// buffer was overloaded, so we use the bounding rect
/*	if (nr_updates>2000)
	{
		rfb::Rect rect;
		int x = m_desktop->pchanges_buf->pointrect[0].rect.left;
		int w = m_desktop->pchanges_buf->pointrect[0].rect.right-m_desktop->pchanges_buf->pointrect[0].rect.left;
		int y = m_desktop->pchanges_buf->pointrect[0].rect.top;
		int h = m_desktop->pchanges_buf->pointrect[0].rect.bottom-m_desktop->pchanges_buf->pointrect[0].rect.top;
		if (ClipRect(&x, &y, &w, &h, m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.tl.y,
			m_desktop->m_bmrect.br.x-m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.br.y-m_desktop->m_bmrect.tl.y))
		{
		rect.tl.x = x;
		rect.br.x = x+w;
		rect.tl.y = y;
		rect.br.y = y+h;
		rgncache=rgncache.union_(rect);
		m_desktop->m_videodriver->oldaantal=counter;
		}
	    return TRUE;
	}*/
	if (m_server->SingleWindow()) m_screen_moved=true;
	if (oldaantal<counter)
		{
			for (int i =oldaantal+1; i<=counter;i++)
				{
					copy_bitmaps_to_buffer(i,rgncache,tracker);
				}

		}
	else
		{
		    int i = 0;
			for (i =oldaantal+1;i<MAXCHANGES_BUF;i++)
				{
					copy_bitmaps_to_buffer(i,rgncache,tracker);
				}
			for (i=1;i<=counter;i++)
				{
					copy_bitmaps_to_buffer(i,rgncache,tracker);
				}
		}	
//	vnclog.Print(LL_INTINFO, VNCLOG("Nr rects %i \n"),rgncache.Numrects());
	m_desktop->m_videodriver->oldaantal=counter;
// A lot updates left after combining 
// This generates an overflow
// We expand each single update to minimum 32x32
	if (rgncache.Numrects()>150)
	{
		rfb::Region2D rgntemp;
		rfb::RectVector rects;
		rfb::RectVector::iterator i;
		rgncache.get_rects(rects, 1, 1);
		for (i = rects.begin(); i != rects.end(); i++)
			{
				rfb::Rect rect = *i;
				rect.tl.x=rect.tl.x/32*32;
				rect.tl.y=rect.tl.y/32*32;
				rect.br.x=rect.br.x/32*32+32;
				rect.br.y=rect.br.y/32*32+32;
				if (rect.br.x>m_desktop->m_bmrect.br.x) rect.br.x=m_desktop->m_bmrect.br.x;
				if (rect.br.y>m_desktop->m_bmrect.br.y) rect.br.y=m_desktop->m_bmrect.br.y;
				rgntemp.assign_union(rect);
			}
//Still to many little updates
//Use the bounding rectangle for updates
		if (rgntemp.Numrects()>50)
		{
			Rect brect=rgntemp.get_bounding_rect();
			rgncache.clear();
			rgncache.assign_union(brect);
		}
		else
		{
		rgncache.clear();
		rgncache.assign_union(rgntemp);
		}
	}
	return TRUE;
}

BOOL
vncDesktopThread::Init(vncDesktop *desktop, vncServer *server)
{
	// Save the server pointer
	m_server = server;
	m_desktop = desktop;
	m_returnset = FALSE;
	m_returnsig = new omni_condition(&m_returnLock);
	// Start the thread
	start_undetached();
	// Wait for the thread to let us know if it failed to init
	{	omni_mutex_lock l(m_returnLock);

		while (!m_returnset)
		{
			m_returnsig->wait();
		}
	}
	if (m_return!=0) 
    {
        g_DesktopThread_running=false;
    }
	return m_return;
}

void
vncDesktopThread::ReturnVal(DWORD result)
{
	omni_mutex_lock l(m_returnLock);
	m_returnset = TRUE;
	m_return = result;
	m_returnsig->signal();
}

void
vncDesktopThread::PollWindow(rfb::Region2D &rgn, HWND hwnd)
{
	// Are we set to low-load polling?
	if (m_server->PollOnEventOnly())
	{
		// Yes, so only poll if the remote user has done something
		if (!m_server->RemoteEventReceived()) {
			return;
		}
	}

	// Does the client want us to poll only console windows?
	if (m_desktop->m_server->PollConsoleOnly())
	{
		char classname[20];

		// Yes, so check that this is a console window...
		if (GetClassName(hwnd, classname, sizeof(classname))) {
			if ((strcmp(classname, "tty") != 0) &&
				(strcmp(classname, "ConsoleWindowClass") != 0)) {
				return;
			}
		}
	}

	RECT rect;

	// Get the rectangle
	if (GetWindowRect(hwnd, &rect)) {
		//Buffer coordinates
			rect.left-=m_desktop->m_ScreenOffsetx;
			rect.right-=m_desktop->m_ScreenOffsetx;
			rect.top-=m_desktop->m_ScreenOffsety;
			rect.bottom-=m_desktop->m_ScreenOffsety;
		rfb::Rect wrect = rfb::Rect(rect).intersect(m_desktop->m_Cliprect);
		if (!wrect.is_empty()) {
			rgn.assign_union(wrect);
		}
	}
}


bool vncDesktopThread::handle_display_change(HANDLE& threadHandle, rfb::Region2D& rgncache, rfb::SimpleUpdateTracker& clipped_updates, rfb::ClippedUpdateTracker& updates)
{
	BOOL screensize_changed=false;
	if (first_run)
	{
		first_run=false;
		m_server->SetNewSWSizeFR(m_desktop->m_scrinfo.framebufferWidth,m_desktop->m_scrinfo.framebufferHeight,FALSE);//changed no lock ok
		//m_server->SetScreenOffset(m_desktop->m_ScreenOffsetx,m_desktop->m_ScreenOffsety,m_desktop->nr_monitors);// no lock ok
		//m_desktop->m_displaychanged=true;
		//screensize_changed=true;
	}

	if (vncService::InputDesktopSelected()==2)
	{
		m_desktop->m_buffer.WriteMessageOnScreen("UltraVVNC running as application doesn't \nhave permission to acces \nUAC protected windows.\n\nThe is screen is locked until the remote user \nunlock this window");
		rfb::Rect rect;
		rect.tl = rfb::Point(0,0);
		rect.br = rfb::Point(300,120);
		rgncache.assign_union(rect);
	}
		
	if ((m_desktop->m_displaychanged ||									//WM_DISPLAYCHANGE
			vncService::InputDesktopSelected()==0 ||							//handle logon and screensaver desktops
			m_desktop->m_SWtoDesktop ||										//switch from SW to full desktop or visa versa
			m_desktop->m_hookswitch||										//hook change request
			m_desktop->requested_multi_monitor!=m_desktop->m_buffer.IsMultiMonitor()		//monitor change request
			) )
			{
				// We need to wait until viewer has send if he support Size changes
				if (!m_server->All_clients_initialalized())
				{
					Sleep(30);
					vnclog.Print(LL_INTERR, VNCLOG("Wait for viewer init \n"));
				}

				//logging
				if (m_desktop->m_displaychanged)								vnclog.Print(LL_INTERR, VNCLOG("++++Screensize changed \n"));
				if (m_desktop->m_SWtoDesktop)									vnclog.Print(LL_INTERR, VNCLOG("m_SWtoDesktop \n"));
				if (m_desktop->m_hookswitch)									vnclog.Print(LL_INTERR, VNCLOG("m_hookswitch \n"));
				if (m_desktop->requested_multi_monitor!=m_desktop->m_buffer.IsMultiMonitor()) vnclog.Print(LL_INTERR, VNCLOG("desktop switch %i %i \n"),m_desktop->requested_multi_monitor,m_desktop->m_buffer.IsMultiMonitor());
				if (!m_server->IsThereFileTransBusy())
				if (vncService::InputDesktopSelected()==0)						vnclog.Print(LL_INTERR, VNCLOG("++++InputDesktopSelected \n"));
				
				
				//BOOL screensize_changed=false;
				BOOL monitor_changed=false;
				rfbServerInitMsg oldscrinfo;
				//*******************************************************
				// Lock Buffers from here
				//*******************************************************
				{
					if (XRichCursorEnabled) m_server->UpdateCursorShape();
					/// We lock all buffers,,and also back the client thread update mechanism
					omni_mutex_lock l(m_desktop->m_update_lock);
					/*#ifdef _DEBUG
					char			szText[256];
					sprintf(szText," ++++++ Mutex lock display changes\n");
					OutputDebugString(szText);		
			#endif*/
					// We remove all queue updates from the tracker
					m_server->Clear_Update_Tracker();
					// Also clear the current updates
					rgncache.clear();
					// Also clear the copy_rect updates
					clipped_updates.clear();
					// TESTTESTTEST
					// Are all updates cleared....old updates could generate bounding errors
					// any other queues to clear ? Yep cursor positions
					m_desktop->m_cursorpos.tl.x=0;
					m_desktop->m_cursorpos.tl.y=0;
					m_desktop->m_cursorpos.br.x=0;
					m_desktop->m_cursorpos.br.y=0;
					//keep a copy of the old screen size, so we can check for changes later on
					oldscrinfo = m_desktop->m_scrinfo;
						
					if (m_desktop->requested_multi_monitor!=m_desktop->m_buffer.IsMultiMonitor())
						{
							m_desktop->Checkmonitors();
							m_desktop->requested_multi_monitor=m_desktop->m_buffer.IsMultiMonitor();
							int old_monitor=m_desktop->multi_monitor;	
							m_desktop->multi_monitor=true;
							if (m_desktop->requested_multi_monitor && m_desktop->nr_monitors>1) m_desktop->multi_monitor=true;
							else m_desktop->multi_monitor=false;							
							if ( old_monitor!=m_desktop->multi_monitor) monitor_changed=true;
						}

					//*******************************************************
					// Reinitialize buffers,color, etc
					// monitor change, for non driver, use another buffer
					//*******************************************************
					if (!m_server->IsThereFileTransBusy())
					if (m_desktop->m_displaychanged || vncService::InputDesktopSelected()==0 || m_desktop->m_hookswitch || (monitor_changed && !m_desktop->m_videodriver))
					{
								// Attempt to close the old hooks
								// shutdown(true) driver is reinstalled without shutdown,(shutdown need a 640x480x8 switch)
								vnclog.Print(LL_INTERR, VNCLOG("m_desktop->Shutdown"));
								monitor_changed=false;
								if (!m_desktop->Shutdown())
									{
										vnclog.Print(LL_INTERR, VNCLOG("Shutdown KillAuthClients\n"));
										m_server->KillAuthClients();
										return false;
									}					
								bool fHookDriverWanted = (FALSE != m_desktop->m_hookdriver);
                                Sleep(1000);
								vnclog.Print(LL_INTERR, VNCLOG("m_desktop->Startup"));
								if (m_desktop->Startup() != 0)
									{
										vnclog.Print(LL_INTERR, VNCLOG("Startup KillAuthClients\n"));
										m_server->KillAuthClients();
										SetEvent(m_desktop->restart_event);
										return false;
									}

								if (m_desktop->m_videodriver)
									{
										if (!XRichCursorEnabled) m_desktop->m_videodriver->HardwareCursor();
										else m_desktop->m_videodriver->NoHardwareCursor();
									}
								m_server->SetScreenOffset(m_desktop->m_ScreenOffsetx,m_desktop->m_ScreenOffsety,m_desktop->nr_monitors);

								// sf@2003 - After a new Startup(), we check if the required video driver
								// is actually available. If not, we force hookdll
								// No need for m_hookswitch again because the driver is NOT available anyway.
								// All the following cases are now handled:
								// 1. Desktop thread starts with "Video Driver" checked and no video driver available...
								//    -> HookDll forced (handled by the first InitHookSettings() after initial Startup() call
								// 2. Desktop Thread starts without "Video Driver" checked but available driver
								//    then the user checks "Video Driver" -> Video Driver used
								// 3. Desktop thread starts with "Video Driver" and available driver used
								//    Then driver is switched off (-> hookDll) 
								//    Then the driver is switched on again (-> hook driver used again)
								// 4. Desktop thread starts without "Video Driver" checked and no driver available
								//    then the users checks "Video Driver" 
								if (fHookDriverWanted && m_desktop->m_videodriver == NULL)
									{
										vnclog.Print(LL_INTERR, VNCLOG("m_videodriver == NULL \n"));
										m_desktop->SethookMechanism(true, false); 	// InitHookSettings() would work as well;
									}
								stop_hookwatch=true;
								vnclog.Print(LL_INTERR, VNCLOG("threadHandle \n"));
								if (threadHandle)
								{
									WaitForSingleObject( threadHandle, INFINITE );
									CloseHandle(threadHandle);
									stop_hookwatch=false;
									threadHandle=NULL;
								}
								vnclog.Print(LL_INTERR, VNCLOG("threadHandle2 \n"));

					}
					//*******************************************************
					// end reinit
					//*******************************************************

					if ((m_desktop->m_scrinfo.framebufferWidth != oldscrinfo.framebufferWidth) ||
						(m_desktop->m_scrinfo.framebufferHeight != oldscrinfo.framebufferHeight ||
							m_desktop->m_SWtoDesktop==TRUE ))
							{
								screensize_changed=true;	
								vnclog.Print(LL_INTINFO, VNCLOG("SCR: new screen format %dx%dx%d\n"),
								m_desktop->m_scrinfo.framebufferWidth,
								m_desktop->m_scrinfo.framebufferHeight,
								m_desktop->m_scrinfo.format.bitsPerPixel);
							}

					m_desktop->m_displaychanged = FALSE;
					m_desktop->m_hookswitch = FALSE;
					m_desktop->Hookdll_Changed = m_desktop->On_Off_hookdll; // Set the hooks again if necessary !
					m_desktop->m_SWtoDesktop=FALSE;
					//****************************************************************************
					//************* SCREEN SIZE CHANGED 
					//****************************************************************************

					if (screensize_changed)
						{
							vnclog.Print(LL_INTERR, VNCLOG("Size changed\n"));
							POINT CursorPos;
							m_desktop->SWinit();
							m_desktop->GetQuarterSize();
							GetCursorPos(&CursorPos);
							CursorPos.x -= m_desktop->m_ScreenOffsetx;
							CursorPos.y -= m_desktop->m_ScreenOffsety;
							m_desktop->m_cursorpos.tl = CursorPos;
							m_desktop->m_cursorpos.br = rfb::Point(GetSystemMetrics(SM_CXCURSOR),
							GetSystemMetrics(SM_CYCURSOR)).translate(CursorPos);
							m_server->SetSWOffset(m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety);
							// Adjust the UpdateTracker clip region
							updates.set_clip_region(m_desktop->m_Cliprect);
							m_desktop->m_buffer.ClearCache();
						}
					if (monitor_changed)
					{
						// we are using the driver, so a monitor change is a view change, like a special kind of single window
						// m_desktop->current_monitor is the new monitor we want to see
						// monitor size mymonitor[m_desktop->current_monitor-1]
						// m_SWOffset is used by the encoders to send the correct coordinates to the viewer
						// Cliprect, buffer coordinates
						if (m_desktop->multi_monitor)
						{
							m_desktop->m_SWOffsetx=0;
							m_desktop->m_SWOffsety=0;
							m_server->SetSWOffset(m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety);

							m_desktop->m_Cliprect.tl.x=0;
							m_desktop->m_Cliprect.tl.y=0;
							m_desktop->m_Cliprect.br.x=m_desktop->mymonitor[2].offsetx+m_desktop->mymonitor[2].Width-m_desktop->mymonitor[2].offsetx;
							m_desktop->m_Cliprect.br.y=m_desktop->mymonitor[2].offsety+m_desktop->mymonitor[2].Height-m_desktop->mymonitor[2].offsety;
						}
						else
						{
							m_desktop->m_SWOffsetx=m_desktop->mymonitor[0].offsetx-m_desktop->mymonitor[2].offsetx;
							m_desktop->m_SWOffsety=m_desktop->mymonitor[0].offsety-m_desktop->mymonitor[2].offsety;
							m_server->SetSWOffset(m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety);

							m_desktop->m_Cliprect.tl.x=m_desktop->mymonitor[0].offsetx-m_desktop->mymonitor[2].offsetx;
							m_desktop->m_Cliprect.tl.y=m_desktop->mymonitor[0].offsety-m_desktop->mymonitor[2].offsety;
							m_desktop->m_Cliprect.br.x=m_desktop->mymonitor[0].offsetx+m_desktop->mymonitor[0].Width-m_desktop->mymonitor[2].offsetx;
							m_desktop->m_Cliprect.br.y=m_desktop->mymonitor[0].offsety+m_desktop->mymonitor[0].Height-m_desktop->mymonitor[2].offsety;
						}



						vnclog.Print(LL_INTERR, VNCLOG("***********###############************ %i %i %i %i %i %i\n"),m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety
							,m_desktop->m_Cliprect.tl.x,m_desktop->m_Cliprect.tl.y,m_desktop->m_Cliprect.br.x,m_desktop->m_Cliprect.br.y);


						rgncache.assign_union(rfb::Region2D(m_desktop->m_Cliprect));
						updates.set_clip_region(m_desktop->m_Cliprect);				
						m_desktop->m_buffer.ClearCache();
						m_desktop->m_buffer.BlackBack();


					}
					m_desktop->m_buffer.ClearCache();
					m_desktop->m_buffer.BlackBack();
					InvalidateRect(NULL,NULL,TRUE);
					rgncache.assign_union(rfb::Region2D(m_desktop->m_Cliprect));
					
					if (memcmp(&m_desktop->m_scrinfo.format, &oldscrinfo.format, sizeof(rfbPixelFormat)) != 0)
						{
							vnclog.Print(LL_INTERR, VNCLOG("Format changed\n"));
							m_server->UpdatePalette(false); // changed no lock ok
							m_server->UpdateLocalFormat(false); // changed no lock ok
						}

					if (screensize_changed) 
						{
							screensize_changed=false;
							m_server->SetNewSWSize(m_desktop->m_scrinfo.framebufferWidth,m_desktop->m_scrinfo.framebufferHeight,FALSE);//changed no lock ok
							m_server->SetScreenOffset(m_desktop->m_ScreenOffsetx,m_desktop->m_ScreenOffsety,m_desktop->nr_monitors);// no lock ok
						}
					
					if (monitor_changed)
						{
								monitor_changed=false;
								if (m_desktop->multi_monitor)
									m_server->SetNewSWSize(m_desktop->mymonitor[2].Width,m_desktop->mymonitor[2].Height,TRUE); //changed no lock ok
								else
									m_server->SetNewSWSize(m_desktop->mymonitor[0].Width,m_desktop->mymonitor[0].Height,TRUE); //changed no lock ok
						}

		/*#ifdef _DEBUG
					//char			szText[256];
					sprintf(szText," ++++++ Mutex unlock display changes\n");
					OutputDebugString(szText);		
			#endif*/
			}// end lock
	}

	return true;
}

void vncDesktopThread::do_polling(HANDLE& threadHandle, rfb::Region2D& rgncache, int& fullpollcounter, bool cursormoved)
{
	// POLL PROBLEM AREAS
	// We add specific areas of the screen to the region cache,
	// causing them to be fetched for processing.
	// if can_be_hooked==false, hooking is temp disabled, use polling

	if (m_desktop->SetHook && g_obIPC.listall()!=NULL && m_desktop->can_be_hooked) 
	{
		DWORD dwTId(0);
		if (threadHandle==NULL) threadHandle = CreateThread(NULL, 0, hookwatch, this, 0, &dwTId);
		if (Handle_Ringbuffer(g_obIPC.listall(),rgncache)) return;
	}
	DWORD lTime = timeGetTime();

	m_desktop->m_buffer.SetAccuracy(m_desktop->m_server->TurboMode() ? 8 : 4); 

	//if (cursormoved)
	//	m_lLastMouseMoveTime = lTime;
	if (cursormoved) m_desktop->idle_counter=0;
	if ((m_desktop->m_server->PollFullScreen() /*&& !cursormoved*/) || (!m_desktop->can_be_hooked && !cursormoved))
	{
		int timeSinceLastMouseMove = lTime - m_lLastMouseMoveTime;
		if (timeSinceLastMouseMove > 150) // 150 ms pause after a Mouse move 
		{
			m_lLastMouseMoveTime = lTime;
			++fullpollcounter;
			rfb::Rect r = m_desktop->GetSize();
			// THIS FUNCTION IS A PIG. It uses too much CPU on older machines (PIII, P4)
			if (vncService::InputDesktopSelected()!=2)
			{
				if (m_desktop->FastDetectChanges(rgncache, r, 0, true)) capture=false;
			}
			else
			{
				capture=false;
			}

			/*#ifdef _DEBUG
										char			szText[256];
										sprintf(szText," Capture %i\n",capture);
										OutputDebugString(szText);		
			#endif*/

			// force full screen scan every three seconds after the mouse stops moving
			if (fullpollcounter > 200) 
			{
				rgncache.assign_union(m_desktop->m_Cliprect);
				fullpollcounter = 0;
			}
		}
	}
		
    HWND hWndToPoll = 0;
	if (m_desktop->m_server->PollForeground() || !m_desktop->can_be_hooked)
	{
		// Get the window rectangle for the currently selected window
		hWndToPoll = GetForegroundWindow();
		if (hWndToPoll != NULL)
			 PollWindow(rgncache, hWndToPoll);
		
	}
	
	if (m_desktop->m_server->PollUnderCursor() || !m_desktop->can_be_hooked)
	{
		// Find the mouse position
		POINT mousepos;
		if (GetCursorPos(&mousepos))
		{
			// Find the window under the mouse
			HWND hwnd = WindowFromPoint(mousepos);
            // exclude the foreground window (done above) and desktop
			if (hwnd != NULL && hwnd != hWndToPoll && hwnd != GetDesktopWindow())
				 PollWindow(rgncache, hwnd);

		}
	}
}
void *
vncDesktopThread::run_undetached(void *arg)
{
	//*******************************************************
	// INIT
	//*******************************************************
	//testBench();
	capture=true;
	vnclog.Print(LL_INTERR, VNCLOG("Hook changed 1\n"));
	// Save the thread's "home" desktop, under NT (no effect under 9x)
	m_desktop->m_home_desktop = GetThreadDesktop(GetCurrentThreadId());
    vnclog.Print(LL_INTERR, VNCLOG("Hook changed 2\n"));
	// Attempt to initialise and return success or failure
	m_desktop->KillScreenSaver();
	{
		keybd_event(VK_CONTROL, 0, 0, 0);
        keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
		Sleep(500); //Give screen some time to kill screensaver
	}
    DWORD startup_error;
	if ((startup_error = m_desktop->Startup()) != 0)
	{
		//TAG14
		vncService::SelectHDESK(m_desktop->m_home_desktop);
		if (m_desktop->m_input_desktop)
			CloseDesktop(m_desktop->m_input_desktop);
		ReturnVal(startup_error);
		return NULL;
	}
	// Succeeded to initialise ok
	ReturnVal(0);

	//telling running viewers to wait until first update
	m_server->InitialUpdate(false);
	// sf@2003 - Done here to take into account if the driver is actually activated
	m_desktop->InitHookSettings(); 
	initialupdate=false;

	// We set a flag inside the desktop handler here, to indicate it's now safe
	// to handle clipboard messages
	m_desktop->SetClipboardActive(TRUE);

	// All changes in the state of the display are stored in a local
	// UpdateTracker object, and are flushed to the vncServer whenever
	// client updates are about to be triggered
	rfb::SimpleUpdateTracker clipped_updates;
	rfb::ClippedUpdateTracker updates(clipped_updates, m_desktop->m_Cliprect);
	clipped_updates.enable_copyrect(true);
	rfb::Region2D rgncache;


	// Incoming update messages are collated into a single region cache
	// The region cache areas are checked for changes before an update
	// is triggered, and the changed areas are passed to the UpdateTracker
	rgncache = m_desktop->m_Cliprect;
	m_server->SetScreenOffset(m_desktop->m_ScreenOffsetx,m_desktop->m_ScreenOffsety,m_desktop->nr_monitors);

	// The previous cursor position is stored, to allow us to erase the
	// old instance whenever it moves.
	rfb::Point oldcursorpos;

	// The driver gives smaller rectangles to check
	// if Accuracy is 4 you eliminate pointer updates
	if (m_desktop->VideoBuffer() && m_desktop->m_hookdriver)
		m_desktop->m_buffer.SetAccuracy(4);

	//init vars
	m_desktop->m_SWSizeChanged=FALSE;
	m_desktop->m_SWtoDesktop=FALSE;
	m_desktop->m_SWmoved=FALSE;
	m_desktop->Hookdll_Changed = true;
	m_desktop->m_displaychanged=false;
	m_desktop->m_hookswitch=false;
	m_desktop->m_hookinited = FALSE;

	// Set driver cursor state
	XRichCursorEnabled= (FALSE != m_desktop->m_server->IsXRichCursorEnabled());
	if (!XRichCursorEnabled && m_desktop->m_videodriver) m_desktop->m_videodriver->HardwareCursor();
	if (XRichCursorEnabled && m_desktop->m_videodriver) m_desktop->m_videodriver->NoHardwareCursor();
	if (XRichCursorEnabled) m_server->UpdateCursorShape();

	InvalidateRect(NULL,NULL,TRUE);
	oldtick=timeGetTime();
	oldtick2=timeGetTime();
	int fullpollcounter=0;
	//*******************************************************
	// END INIT
	//*******************************************************
	// START PROCESSING DESKTOP MESSAGES
	/////////////////////
	HANDLE threadHandle=NULL;
	stop_hookwatch=false;
	/////////////////////
	// We use a dynmiac value based on cpu usage
    //DWORD MIN_UPDATE_INTERVAL=33;
	/////////////////////
	bool looping=true;
	int waiting_update=0;
	SetEvent(m_desktop->restart_event);
	///
	Sleep(1000);
	rgncache.assign_union(rfb::Region2D(m_desktop->m_Cliprect));
	if (m_desktop->VideoBuffer() && m_desktop->m_hookdriver)
											{
												m_desktop->m_buffer.GrabRegion(rgncache,true,true);
											}
										else
											{
												m_desktop->m_buffer.GrabRegion(rgncache,false,true);
											}
	//telling running viewers to wait until first update, done
	if  (m_server->MaxCpu() <50)
		{
			MIN_UPDATE_INTERVAL_MIN=50;
			MIN_UPDATE_INTERVAL_MAX=1000;
		}
	if (m_server->IsUltraVncViewer()) first_run=true;
	else first_run=false;

	while (looping && !fShutdownOrdered)
	{		
		DWORD result;
		newtick = timeGetTime();
		int waittime;
		waittime=33;
		if (m_desktop->VideoBuffer() && m_desktop->m_hookdriver) 
		{
			int fastcounter=0;
			POINT cursorpos;
			while (m_desktop->m_videodriver->oldaantal==m_desktop->pchanges_buf->counter)
			{
				Sleep(5);
				fastcounter++;
				if (fastcounter>20)
				{
					/*#ifdef _DEBUG
										char			szText[256];
										sprintf(szText,"fastcounter\n");
										OutputDebugString(szText);		
					#endif*/
					break;
				}
				if (GetCursorPos(&cursorpos) && 
										((cursorpos.x != oldcursorpos.x) ||
										(cursorpos.y != oldcursorpos.y))) break;
			}
			waittime=0;
		}
		/*else
		{
			waittime=waittime-(waiting_update*10);
		}
		if (waittime<0) waittime=0;
		if (waittime>100) waittime=100;*/

		result=WaitForMultipleObjects(6,m_desktop->trigger_events,FALSE,waittime);
		{
			//#ifdef _DEBUG
			//							char			szText[256];
			//							sprintf(szText,"WaitForMultipleObjects %i\n",result );
			//							OutputDebugString(szText);		
			//#endif

			// We need to wait until restart is done
			// else wait_timeout goes in to looping while sink window is not ready
			// if no window could be started in 10 seconds something went wrong, close
			// desktop thread.
			DWORD status=WaitForSingleObject(m_desktop->restart_event,10000);
			if (status==WAIT_TIMEOUT) looping=false;
			switch(result)
			{
				case WAIT_TIMEOUT:
				case WAIT_OBJECT_0:
				{
				waiting_update=0;
				ResetEvent(m_desktop->trigger_events[0]);
							{
								//measure current cpu usage of winvnc
								if (fullpollcounter==10 || fullpollcounter==0 || fullpollcounter==5) cpuUsage = usage.GetUsage();
								if (cpuUsage > m_server->MaxCpu()) 
									MIN_UPDATE_INTERVAL+=10;
								else MIN_UPDATE_INTERVAL-=10;
								if (MIN_UPDATE_INTERVAL<MIN_UPDATE_INTERVAL_MIN) MIN_UPDATE_INTERVAL=MIN_UPDATE_INTERVAL_MIN;
								if (MIN_UPDATE_INTERVAL>MIN_UPDATE_INTERVAL_MAX) MIN_UPDATE_INTERVAL=MIN_UPDATE_INTERVAL_MAX;


					//			vnclog.Print(LL_INTERR, VNCLOG("!PeekMessage \n"));
								// MAX 30fps
								newtick = timeGetTime(); // Better resolution than GetTickCount ;)
								if ((newtick-oldtick)<MIN_UPDATE_INTERVAL)
								{
									Sleep(MIN_UPDATE_INTERVAL-(newtick-oldtick));
									//continue;  Verify, this can cause screen lockup
									// We need another PeekMessage, but this is only done
									// by hookdll and viewer asking for new update
									// can cause a very long wait time
								}	
								
								/*#ifdef _DEBUG
										char			szText[256];
										sprintf(szText," cpu2: %d %i %i\n",cpuUsage,MIN_UPDATE_INTERVAL,newtick-oldtick);
										OutputDebugString(szText);		
								#endif*/
								//oldtick=newtick;
								if (m_desktop->VideoBuffer() && m_desktop->m_hookdriver) handle_driver_changes(rgncache,updates);
								m_desktop->m_update_triggered = FALSE;
								g_update_triggered = FALSE;
								//if (m_desktop->m_timerid==NULL) m_desktop->m_timerid = SetTimer(m_desktop->m_hwnd, 1, 100, NULL);

								//*******************************************************
								// HOOKDLL START STOP need to be executed from the thread
								//*******************************************************
								if (m_desktop->Hookdll_Changed && !m_desktop->m_hookswitch)
								{
									vnclog.Print(LL_INTERR, VNCLOG("Hook changed \n"));
									m_desktop->StartStophookdll(m_desktop->On_Off_hookdll);
									if (m_desktop->On_Off_hookdll)
										m_desktop->m_hOldcursor = NULL; // Force mouse cursor grabbing if hookdll On
									// Todo: in case of hookdriver Off - Hoodll On -> hookdriver On - Hoodll Off
									// we must send an empty mouse cursor to the clients so they get rid of their local
									// mouse cursor bitmap
									m_desktop->Hookdll_Changed=false;
								}
								//*******************************************************
								// SCREEN DISPLAY HAS CHANGED, RESTART DRIVER (IF Used)
								//*******************************************************
								if (!m_server->IsThereFileTransBusy())
									if (!handle_display_change(threadHandle, rgncache, clipped_updates, updates))
									{
										//failed we need to quit thread
										looping=false;
										break;
									}
								//*******************************************************
								// END SCREEN DISPLAY HAS CHANGED
								//*******************************************************
					//			m_server->SetSWOffset(m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety);
							
								//*******************************************************************
								// SINGLE WINDOW 
								// size SW changed
								// Position change -->change offsets
								//*******************************************************************
								bool SWSizeChanged=false;
								if (m_server->SingleWindow())
								{
									omni_mutex_lock l(m_desktop->m_update_lock);
									m_desktop->GetQuarterSize();
									m_server->SetSWOffset(m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety);
									//SW size changed
									if (m_desktop->m_SWSizeChanged)
									{
										SWSizeChanged=true;
										m_desktop->m_SWSizeChanged=FALSE;
										m_desktop->GetQuarterSize();
										rgncache.assign_union(rfb::Region2D(m_desktop->m_Cliprect));
					//					vnclog.Print(LL_INTINFO, VNCLOG("4 %i %i %i %i \n"),m_desktop->m_Cliprect.br.x,m_desktop->m_Cliprect.br.y,m_desktop->m_Cliprect.tl.x,m_desktop->m_Cliprect.tl.y);
										updates.set_clip_region(m_desktop->m_Cliprect);
										m_server->SetSWOffset(m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety);				
										m_desktop->m_buffer.ClearCache();
										m_desktop->m_buffer.BlackBack();
									}
									//SW position changed
									if (m_desktop->m_SWmoved)
									{
										m_desktop->m_SWmoved=FALSE;
										updates.set_clip_region(m_desktop->m_Cliprect);
										m_server->SetSWOffset(m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety);				
										rgncache.assign_union(rfb::Region2D(m_desktop->m_Cliprect));
					//					vnclog.Print(LL_INTINFO, VNCLOG("5 %i %i %i %i \n"),m_desktop->m_Cliprect.br.x,m_desktop->m_Cliprect.br.y,m_desktop->m_Cliprect.tl.x,m_desktop->m_Cliprect.tl.y);
										m_desktop->m_buffer.ClearCache();
										m_desktop->m_buffer.BlackBack();
									}

								if (m_server->SingleWindow() && SWSizeChanged)
									{
										m_server->SetNewSWSize(m_desktop->m_SWWidth,m_desktop->m_SWHeight,FALSE);
										m_server->SetScreenOffset(m_desktop->m_ScreenOffsetx,m_desktop->m_ScreenOffsety,m_desktop->nr_monitors);
									}
								}// end update lock
								
								////////////////////////////////////////////////////////////////////////////////
								// END DYNAMIC CHANGES
								////////////////////////////////////////////////////////////////////////////////

								//Beep(1000,10);
								//
								// CALCULATE CHANGES
								m_desktop->m_UltraEncoder_used=m_desktop->m_server->IsThereAUltraEncodingClient();
					//			vnclog.Print(LL_INTERR, VNCLOG("UpdateWanted B\n"));
//#ifdef _DEBUG
////										char			szText[256];
//									sprintf(szText," m_desktop->m_server->UpdateWanted check\n");
//										OutputDebugString(szText);		
//#endif
								omni_mutex_lock l(m_desktop->m_update_lock);
								if (m_desktop->m_server->UpdateWanted())
								{
									oldtick=newtick;
					//				vnclog.Print(LL_INTERR, VNCLOG("UpdateWanted N\n"));
									//TEST4
									// Re-render the mouse's old location if it's moved
									bool cursormoved = false;
									POINT cursorpos;
									if (GetCursorPos(&cursorpos) && 
										((cursorpos.x != oldcursorpos.x) ||
										(cursorpos.y != oldcursorpos.y)))
									{
					//					vnclog.Print(LL_INTERR, VNCLOG("UpdateWanted M %i %i %i %i\n"),cursorpos.x, oldcursorpos.x,cursorpos.y,oldcursorpos.y);
										cursormoved = TRUE;
										oldcursorpos = rfb::Point(cursorpos);
										// nyama/marscha - PointerPos. Inform clients about mouse move.
										m_desktop->m_server->UpdateMouse();
										if (MyGetCursorInfo)
										{
											MyCURSORINFO cinfo;
											cinfo.cbSize=sizeof(MyCURSORINFO);
											MyGetCursorInfo(&cinfo);
											m_desktop->SetCursor(cinfo.hCursor);
										}
									}

									//****************************************************************************
									//************* Check for moved windows
									//****************************************************************************
									// Back removed, to many artifacts
									bool s_moved=false;
									//if ((cpuUsage >= m_server->MaxCpu()/2))
									{
									if (!m_desktop->m_hookdriver && !m_server->SingleWindow()) 
											s_moved=m_desktop->CalcCopyRects(updates);
									}
								
									//****************************************************************************
									//************* Polling ---- no driver
									//****************************************************************************
									if (!m_desktop->m_hookdriver || !m_desktop->can_be_hooked)
									{
										if (!s_moved)
										do_polling(threadHandle, rgncache, fullpollcounter, cursormoved);
									}
									//****************************************************************************
									//************* driver  No polling
									//****************************************************************************
									else 
									{
										// long lTime = timeGetTime();
										if (cursormoved)
										{
											// if (lTime - m_desktop->m_lLastMouseUpdateTime < 200)
											// 	continue;
											m_desktop->m_buffer.SetAccuracy(m_desktop->m_server->TurboMode() ? 2 : 1);
											// m_desktop->m_lLastMouseUpdateTime = lTime;
										}
										else
											// 4 is not that bad...but not perfect (especially with tree branchs display)
											m_desktop->m_buffer.SetAccuracy(m_desktop->m_server->TurboMode() ? 4 : 2); 
									}
									
									
									// PROCESS THE MOUSE POINTER
									// Some of the hard work is done in clients, some here
									// This code fetches the desktop under the old pointer position
									// but the client is responsible for actually encoding and sending
									// it when required.
									// This code also renders the pointer and saves the rendered position
									// Clients include this when rendering updates.
									// The code is complicated in this way because we wish to avoid 
									// rendering parts of the screen the mouse moved through between
									// client updates, since in practice they will probably not have changed.
								
									if (cursormoved && !m_desktop->m_hookdriver)
										{
											if (!m_desktop->m_cursorpos.is_empty())
											{
												// Cursor position seems to be outsite the bounding
												// When you make the screen smaller
												// add extra check
												rfb::Rect rect;
												int x = m_desktop->m_cursorpos.tl.x;
												int w = m_desktop->m_cursorpos.br.x-x;
												int y = m_desktop->m_cursorpos.tl.y;
												int h = m_desktop->m_cursorpos.br.y-y;
												if (ClipRect(&x, &y, &w, &h, m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.tl.y,
													m_desktop->m_bmrect.br.x-m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.br.y-m_desktop->m_bmrect.tl.y))
													{
														rect.tl.x = x;
														rect.br.x = x+w;
														rect.tl.y = y;
														rect.br.y = y+h;
														rgncache.assign_union(rect);
					//									vnclog.Print(LL_INTINFO, VNCLOG("6 %i %i %i %i \n"),m_desktop->m_cursorpos.br.x,m_desktop->m_cursorpos.br.y,m_desktop->m_cursorpos.tl.x,m_desktop->m_cursorpos.tl.y);
					//									vnclog.Print(LL_INTINFO, VNCLOG("6 %i %i %i %i \n"),rect.br.x,rect.br.y,rect.tl.x,rect.tl.y);
													}
											}

										}
									

									{
										// Prevent any clients from accessing the Buffer
										omni_mutex_lock ll(m_desktop->m_update_lock);
										
										// CHECK FOR COPYRECTS
										// This actually just checks where the Foreground window is
										// Back added, no need to stop polling during move
										if ((cpuUsage < m_server->MaxCpu()/2))
										{
										if (!m_desktop->m_hookdriver && !m_server->SingleWindow() && !s_moved) 
											s_moved=m_desktop->CalcCopyRects(updates);
										}
										
										// GRAB THE DISPLAY
										// Fetch data from the display to our display cache.
										// Update the scaled rects when using server side scaling
										// something wrong inithooking again
										// We make sure no updates are in the regions
										// sf@2002 - Added "&& m_desktop->m_hookdriver"
										// Otherwise we're still getting driver updates (from shared memory buffer)
										// after a m_hookdriver switching from on to off 
										// (and m_hookdll from off to on) that causes mouse cursor garbage,
										// or missing mouse cursor.
										/*char tempchar[10];
										if ((newtick-oldtick2) != 0) itoa(1000/((newtick-oldtick2)),tempchar,10);
										oldtick2=newtick;
										m_desktop->m_buffer.WriteMessageOnScreen(tempchar);*/
										if (m_desktop->VideoBuffer() && m_desktop->m_hookdriver)
											{
												m_desktop->m_buffer.GrabRegion(rgncache,true,capture);
											}
										else
											{
												m_desktop->m_buffer.GrabRegion(rgncache,false,capture);
											}
/*#ifdef _DEBUG
										char			szText[256];
										sprintf(szText," capture %i\n",capture);
										OutputDebugString(szText);		
#endif*/
										capture=true;
											
										// sf@2002 - v1.1.x - Mouse handling
										// If one client, send cursor shapes only when the cursor changes.
										// This is Disabled for now.
										if( !XRichCursorEnabled==m_desktop->m_server->IsXRichCursorEnabled())
											{
												XRichCursorEnabled= (FALSE != m_desktop->m_server->IsXRichCursorEnabled());
												if (m_desktop->m_videodriver)
														{
																if (!XRichCursorEnabled) m_desktop->m_videodriver->HardwareCursor();
																else m_desktop->m_videodriver->NoHardwareCursor();
														}

											}
										if (m_desktop->m_server->IsXRichCursorEnabled() && !m_desktop->m_UltraEncoder_used)
											{
												if (m_desktop->m_hcursor != m_desktop->m_hOldcursor || m_desktop->m_buffer.IsShapeCleared())
														{
																m_desktop->m_hOldcursor = m_desktop->m_hcursor;
																m_desktop->m_buffer.SetCursorPending(TRUE);
																if (!m_desktop->m_hookdriver) m_desktop->m_buffer.GrabMouse(); // Grab mouse cursor in all cases
																m_desktop->m_server->UpdateMouse();
																rfb::Rect rect;
																int x = m_desktop->m_cursorpos.tl.x;
																int w = m_desktop->m_cursorpos.br.x-x;
																int y = m_desktop->m_cursorpos.tl.y;
																int h = m_desktop->m_cursorpos.br.y-y;
																if (ClipRect(&x, &y, &w, &h, m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.tl.y,
																	m_desktop->m_bmrect.br.x-m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.br.y-m_desktop->m_bmrect.tl.y))
																		{
																			rect.tl.x = x;
																			rect.br.x = x+w;
																			rect.tl.y = y;
																			rect.br.y = y+h;
																			rgncache.assign_union(rect);
					//														vnclog.Print(LL_INTINFO, VNCLOG("7 %i %i %i %i \n"),m_desktop->m_cursorpos.br.x,m_desktop->m_cursorpos.br.y,m_desktop->m_cursorpos.tl.x,m_desktop->m_cursorpos.tl.y);
					//														vnclog.Print(LL_INTINFO, VNCLOG("6 %i %i %i %i \n"),rect.br.x,rect.br.y,rect.tl.x,rect.tl.y);
																		}
																m_server->UpdateCursorShape();
																}
											}
										else if (!m_desktop->m_hookdriver)// If several clients, send them all the mouse updates
											{												
												// Render the mouse
												//if (!m_desktop->VideoBuffer())
												m_desktop->m_buffer.GrabMouse();
												
												if (cursormoved /*&& !m_desktop->m_buffer.IsCursorUpdatePending()*/) 
															{
																// Inform clients that it has moved
																m_desktop->m_server->UpdateMouse();
																// Get the buffer to fetch the pointer bitmap
																if (!m_desktop->m_cursorpos.is_empty())
																{
																	rfb::Rect rect;
																int x = m_desktop->m_cursorpos.tl.x;
																int w = m_desktop->m_cursorpos.br.x-x;
																int y = m_desktop->m_cursorpos.tl.y;
																int h = m_desktop->m_cursorpos.br.y-y;
																if (ClipRect(&x, &y, &w, &h, m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.tl.y,
																	m_desktop->m_bmrect.br.x-m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.br.y-m_desktop->m_bmrect.tl.y))
																		{
																			rect.tl.x = x;
																			rect.br.x = x+w;
																			rect.tl.y = y;
																			rect.br.y = y+h;
																			rgncache.assign_union(rect);
																		}
																}

															}
												}	
										
											
										// SCAN THE CHANGED REGION FOR ACTUAL CHANGES
										// The hooks return hints as to areas that may have changed.
										// We check the suggested areas, and just send the ones that
										// have actually changed.
										// Note that we deliberately don't check the copyrect destination
										// here, to reduce the overhead & the likelihood of corrupting the
										// backbuffer contents.
										rfb::Region2D checkrgn;
										rfb::Region2D changedrgn;
										rfb::Region2D cachedrgn;

											
										//Update the backbuffer for the copyrect region
										if (!clipped_updates.get_copied_region().is_empty()) 
											{
												rfb::UpdateInfo update_info;
												rfb::RectVector::const_iterator i;
												clipped_updates.get_update(update_info);
												if (!update_info.copied.empty()) 
													{
														for (i=update_info.copied.begin(); i!=update_info.copied.end(); i++) 						
															m_desktop->m_buffer.CopyRect(*i, update_info.copy_delta);
													}
											}
										//Remove the copyrect region from the other updates					
										//checkrgn = rgncache.union_(clipped_updates.get_copied_region());	
										checkrgn = rgncache.subtract(clipped_updates.get_copied_region());	
										//make sure the copyrect is checked next update
										if (!clipped_updates.get_copied_region().is_empty() && (cpuUsage < m_server->MaxCpu()/2))
										{

											rfb::UpdateInfo update_info;
												rfb::RectVector::const_iterator i;
												clipped_updates.get_update(update_info);
												if (!update_info.copied.empty()) 
													{
														for (i=update_info.copied.begin(); i!=update_info.copied.end(); i++) 						
														{
															rfb::Rect rect;
															rect.br.x=i->br.x+4;
															rect.br.y=i->br.y+4;
															rect.tl.x=i->tl.x-4;
															rect.tl.y=i->tl.y-4;
															rect = rect.intersect(m_desktop->m_Cliprect);
															rgncache=rgncache.union_(rect);
															rfb::Rect src = rect.translate(update_info.copy_delta.negate());
															src = src.intersect(m_desktop->m_Cliprect);
															rgncache=rgncache.union_(src);
														}
													}
										}
										else
											rgncache = clipped_updates.get_copied_region();
										
										//Check all regions for changed and cached parts
										//This is very cpu intensive, only check once for all viewers
										if (!checkrgn.is_empty())
											m_desktop->m_buffer.CheckRegion(changedrgn,cachedrgn, checkrgn);
										if (!initialupdate)
											{
												m_server->InitialUpdate(true);
												initialupdate=true;
											}
										updates.add_changed(changedrgn);
										updates.add_cached(cachedrgn);
												
										clipped_updates.get_update(m_server->GetUpdateTracker());
									}  // end mutex lock

									// Clear the update tracker and region cache an solid
									clipped_updates.clear();
									// screen blanking
									if (m_desktop->OldPowerOffTimeout!=0)
										{
										if (!m_server->BlackAlphaBlending() || m_desktop->VideoBuffer())
											{
												if(OSversion()!=2)
												{
												SystemParametersInfo(SPI_SETPOWEROFFACTIVE, 1, NULL, 0);
												SendMessage(m_desktop->m_hwnd,WM_SYSCOMMAND,SC_MONITORPOWER,(LPARAM)2);
												}
					// don't block input here, this is the wrong thread!
											}
										}
					#ifdef AVILOG
									if (m_desktop->AviGen) m_desktop->AviGen->AddFrame((BYTE*)m_desktop->m_DIBbits);
					#endif
								}
								//newtick = timeGetTime(); 
							}
						}
					break;

				case WAIT_OBJECT_0+1:
					ResetEvent(m_desktop->trigger_events[1]);
					m_desktop->lock_region_add=true;
					rgncache.assign_union(m_desktop->rgnpump);
					m_desktop->rgnpump.clear();
					m_desktop->lock_region_add=false;
					waiting_update++;
					break;
				case WAIT_OBJECT_0+2:
					ResetEvent(m_desktop->trigger_events[2]);
					break;
				case WAIT_OBJECT_0+3:
					if (MyGetCursorInfo)
					{
						MyCURSORINFO cinfo;
						cinfo.cbSize=sizeof(MyCURSORINFO);
						MyGetCursorInfo(&cinfo);
						m_desktop->SetCursor(cinfo.hCursor);
					}
					ResetEvent(m_desktop->trigger_events[3]);
					break;
				case WAIT_OBJECT_0+4:
					rgncache.assign_union(m_desktop->m_Cliprect);
					ResetEvent(m_desktop->trigger_events[4]);
					break;
				case WAIT_OBJECT_0+5:
					//break to close
					looping=false;
					ResetEvent(m_desktop->trigger_events[5]);
					break;
			}
		}
		
	}//while

	stop_hookwatch=true;
	if (threadHandle)
	{
		WaitForSingleObject( threadHandle, 5000 );
		CloseHandle(threadHandle);
	}
	
	m_desktop->SetClipboardActive(FALSE);
	vnclog.Print(LL_INTINFO, VNCLOG("quitting desktop server thread\n"));
	
	// Clear all the hooks and close windows, etc.
    m_desktop->SetBlockInputState(false);
	m_server->SingleWindow(false);
	vnclog.Print(LL_INTINFO, VNCLOG("quitting desktop server thread:SetBlockInputState\n"));
	
	// Clear the shift modifier keys, now that there are no remote clients
	vncKeymap::ClearShiftKeys();
	vnclog.Print(LL_INTINFO, VNCLOG("quitting desktop server thread:ClearShiftKeys\n"));
	
	// Switch back into our home desktop, under NT (no effect under 9x)
	//TAG14
	HWND mywin=FindWindow("blackscreen",NULL);
	if (mywin)SendMessage(mywin,WM_CLOSE, 0, 0);
	g_DesktopThread_running=false;
	vnclog.Print(LL_INTINFO, VNCLOG("quitting desktop server thread:g_DesktopThread_running=false\n"));
	m_desktop->Shutdown();
	vnclog.Print(LL_INTINFO, VNCLOG("quitting desktop server thread:m_desktop->Shutdown\n"));
	return NULL;
}
